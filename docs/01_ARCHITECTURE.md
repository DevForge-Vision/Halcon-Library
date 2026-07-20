# 01. 아키텍처 설계

## 1. 설계가 풀어야 할 제약

이 라이브러리는 **여러 검사 프로그램이 참조해 쓰는 static lib** 이다. 그래서 다음이 전부 제약이다.

| # | 제약 | 결과 |
|---|---|---|
| 1 | 사용처마다 **OpenCV 버전이 다르다** (vc14 / vc16 혼재) | 공개 헤더에 `cv::Mat` 이 있으면 링크가 깨진다 → **PIMPL 필수** |
| 2 | 사용처마다 **툴셋이 다르다** (v140 / v143) | 표준 레이아웃 · POD 위주 인터페이스 |
| 3 | 기존 코드가 **MFC 멀티바이트** | 공개 API 에 `std::wstring` · 예외 전파 금지 |
| 4 | **실시간 검사 경로** (8스레드, 프레임당 수백 blob) | 가상함수 남발 금지, 중간산물 재계산 금지, 재할당 최소화 |
| 5 | HALCON 스크립트를 **1:1 로 이식**해야 함 | 오퍼레이터 이름·인자 순서를 원문 그대로 노출 |

## 2. 계층

```
┌──────────────────────────────────────────────────────────┐
│  Facade    namespace glim::halcon                        │  ← 사용처가 보는 전부
│            area_center() circularity() select_shape() …  │     HALCON 이름 그대로
│            (공개 헤더 1개, OpenCV 타입 노출 없음)          │
├──────────────────────────────────────────────────────────┤
│  Feature   FeatureContext  — 리전당 중간산물 캐시(지연계산) │  ← 성능의 핵심
│            FeatureRegistry — "이름 → 계산함수" 레지스트리   │     select_shape 의 근간
├──────────────────────────────────────────────────────────┤
│  Core      Region (런렝스, 불변, shared_ptr)              │  ← 자료구조
│            Contour / ConvexHull / Moments                │
├──────────────────────────────────────────────────────────┤
│  Backend   OpenCV · 자체 알고리즘  (헤더 밖으로 안 나감)    │
└──────────────────────────────────────────────────────────┘
```

## 3. 적용 패턴과 그 이유

| 패턴 | 적용 지점 | 왜 이걸 쓰는가 |
|---|---|---|
| **PIMPL** | `Region`, `FeatureContext` | 제약 1·2 해결. 공개 헤더에서 OpenCV 를 완전히 숨긴다 |
| **Facade** | `namespace glim::halcon` | 제약 5 해결. HALCON 문서/스크립트를 그대로 옮길 수 있다 |
| **지연계산 + 메모이제이션** | `FeatureContext` | 제약 4 해결. contour · hull · 모멘트는 여러 특징값이 공유한다. 리전당 1회만 계산 |
| **Registry (Strategy)** | `FeatureRegistry` | `select_shape` 가 **문자열로 특징값을 고르는** HALCON 시맨틱을 자연스럽게 구현. 63개 feature 이름 체계의 단일 진실원 |
| **불변 값객체 + shared_ptr** | `Region` | 8스레드 환경에서 락 없이 공유. 복사가 O(1) |
| **정적 디스패치** | 계산 함수 전반 | 가상함수 오버헤드 제거. 인터페이스 추상화는 **경계(입출력)에만** |

> **의도적으로 쓰지 않는 것**: 깊은 상속 계층, 계산기마다의 `IFeature` 인터페이스,
> DI 컨테이너. 오퍼레이터는 **무상태 순수 함수**이므로 객체지향 계층을 씌우면 손해만 난다.

## 4. 핵심 자료구조 — `Region`

HALCON 과 동일하게 **런렝스(run-length) 인코딩**을 1급 표현으로 삼는다.

```cpp
// 런 = 한 행에서 연속된 전경 픽셀 구간 (끝 포함)
struct Run { int row; int colBegin; int colEnd; };
```

런렝스를 고르는 이유:

- `area_center` · `runlength_features` · `moments_*` 를 **O(런 수)** 로 계산할 수 있다.
  픽셀 순회(O(면적))보다 두 자릿수 빠르다.
- 원시 모멘트 `m_pq = Σ row^p · col^q` 는 런 하나를 **등차수열 부분합(Faulhaber)** 으로
  상수시간 처리할 수 있다.
- HALCON 의 `runlength_*` 오퍼레이터가 이 표현을 전제로 정의되어 있다.

불변식:
- 런은 `row` 오름차순, 같은 행 안에서 `colBegin` 오름차순으로 정렬
- 같은 행의 런끼리 겹치거나 인접(`colEnd + 1 == 다음 colBegin`)하지 않는다 (정규형)
- 생성 후 변경하지 않는다 (불변). 변형 연산은 새 `Region` 을 만든다

## 5. 성능 설계 — `FeatureContext`

같은 리전에서 여러 특징값을 뽑을 때, 중간산물을 반복 계산하면 안 된다.

```
              ┌─ area, center ──────── area_center, circularity, compactness …
   Region ────┼─ contour ───────────── contlength, circularity, roundness …
   (런렝스)    ├─ convexHull ────────── convexity, rectangularity
              └─ moments (m00..m30) ── elliptic_axis, eccentricity, orientation, moments_* 7종
```

- 각 중간산물은 **처음 요청될 때 1회 계산**하고 이후 재사용한다.
- `region_features(Regions, ['area','circularity','convexity'], Value)` 처럼
  여러 값을 한 번에 뽑는 호출에서 효과가 가장 크다.
- 캐시는 `FeatureContext` 인스턴스 수명에 묶인다. **전역 캐시를 두지 않는다** (스레드 안전).

## 6. 공개 API 형태

```cpp
// include/GlimHalcon.h  — 사용처가 include 하는 유일한 헤더
#pragma once
#include <vector>

namespace glim { namespace halcon {

class Region;                       // PIMPL. OpenCV 타입 노출 없음

// HALCON 오퍼레이터를 이름·인자 순서 그대로
void AreaCenter   (const Region& region, long& area, double& row, double& column);
void ContLength   (const Region& region, double& contLength);
void Circularity  (const Region& region, double& circularity);
void Compactness  (const Region& region, double& compactness);
void Convexity    (const Region& region, double& convexity);

}} // namespace glim::halcon
```

규칙:

- **출력은 참조 인자**로 받는다 (HALCON 시그니처와 1:1 대응, 다중 출력 오퍼레이터가 많음).
- 예외를 **공개 경계 밖으로 던지지 않는다.** 내부 `try-catch` 로 잡고 HALCON 의
  빈 리전 규약(`0` 반환)으로 축약한다. MFC 호출부가 예외를 못 받는 경우가 있다.
- 이름은 프로젝트 코딩 규칙(PascalCase)을 따르되, HALCON 원명 대응을 주석으로 남긴다.

## 7. 디렉터리 구조

```
Halcon-Library/
├── CLAUDE.md                    AI 에이전트 / 신규 참여자 작업 지침
├── README.md                    프로젝트 표지
├── CMakeLists.txt               (예정)
├── docs/
│   ├── 00_OVERVIEW.md           범위 · 원칙 · 40종 현황
│   ├── 01_ARCHITECTURE.md       이 문서
│   ├── 02_DEFINITIONS.md        특징값 정의서 ★ 구현의 근거
│   ├── 03_VERIFICATION.md       정의 검증 계획과 기댓값
│   └── reference/halcon13/      HALCON 13 원문 수집 아카이브 (G1~G5)
├── include/
│   └── GlimHalcon.h             (예정) 공개 헤더 — OpenCV 노출 금지
├── src/
│   ├── Core/                    (예정) Region, Contour, ConvexHull, Moments
│   ├── Feature/                 (예정) FeatureContext, FeatureRegistry
│   └── Facade/                  (예정) 오퍼레이터 구현
└── test/
    ├── Shapes.h                 (예정) 기준 도형 S1~S7 생성기
    └── test_*.cpp               (예정) 03_VERIFICATION.md 기댓값 대조
```

## 8. 빌드

| 항목 | 값 |
|---|---|
| 표준 | C++14 (C++17 기능은 사용처 툴셋 확인 후) |
| 플랫폼 | x86 / x64 양쪽 |
| 문자셋 | **멀티바이트** (사용처가 MFC 멀티바이트) |
| 산출물 | static lib |
| 의존성 | OpenCV (내부 전용, 헤더 밖으로 안 나감) |
| 빌드 시스템 | CMake (VS 2015 / 2022 생성기) |

> **소스 파일 인코딩: UTF-8 BOM 필수.**
> BOM 없는 UTF-8 한글 주석은 VS 가 CP949 로 오독해 **다음 줄을 삼킨다.**
> 컴파일은 성공하는데 코드가 사라지는 형태로 나타나 추적이 매우 어렵다.

## 9. 설계 이력

| 날짜 | 결정 | 근거 |
|---|---|---|
| 2026-07-20 | PIMPL + Facade + 런렝스 Region + FeatureContext 캐시 채택 | §1 제약 5건 |
| 2026-07-20 | 오퍼레이터별 인터페이스 추상화 **배제** | 무상태 순수 함수라 계층이 비용만 된다 |
