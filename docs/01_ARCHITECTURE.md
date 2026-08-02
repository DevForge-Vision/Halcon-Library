[🏠 홈](../README.md) › [📚 문서](README.md) › **아키텍처**

---

# 01. 아키텍처 설계

<details>
<summary><b>목차</b></summary>

- [1. 설계가 풀어야 할 제약](#1-설계가-풀어야-할-제약)
- [2. 계층](#2-계층)
- [3. 적용 패턴과 그 이유](#3-적용-패턴과-그-이유)
- [4. 핵심 자료구조 — `Region`](#4-핵심-자료구조--region)
- [5. 성능 설계 — `FeatureContext`](#5-성능-설계--featurecontext)
- [6. 공개 API 형태](#6-공개-api-형태)
- [7. 디렉터리 구조](#7-디렉터리-구조)
- [8. 빌드](#8-빌드)
- [9. 설계 이력](#9-설계-이력)

</details>

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
│            Contour / ConvexHull / Moments / Holes        │
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
   (런렝스)    ├─ convexHull ────────── convexity, rectangularity, diameter_region
              ├─ moments (m00..m30) ── elliptic_axis, eccentricity, orientation, moments_* 7종
              └─ holeCounts ────────── area_holes, connect_and_holes, euler_number
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

// 배치 계산 (HALCON 대응 없음. 중간산물 중복 계산 제거용)
struct RegionFeatures                // POD. 필드는 끝에만 추가한다 (ABI)
{
    long   area;
    double row;
    double column;
    double contLength;
    double circularity;
    double compactness;
    double convexity;
};
void ComputeFeatures(const Region& region, RegionFeatures& features);

// 2차 배치 — 2차 모멘트 계열 5종 (2026-07-28)
void MomentsRegion2nd     (const Region&, double& m11, double& m20, double& m02,
                           double& ia, double& ib);
void MomentsRegion2ndInvar(const Region&, double& m11, double& m20, double& m02);
void EllipticAxis         (const Region&, double& ra, double& rb, double& phi);
void Eccentricity         (const Region&, double& anisometry, double& bulkiness,
                           double& structureFactor);
void OrientationRegion    (const Region&, double& phi);

struct RegionMoments                 // POD. RegionFeatures 와 **분리**한다 (ABI, §9 이력 참조)
{
    double m11, m20, m02, ia, ib;              // MomentsRegion2nd      (정규화 없음)
    double m11Invar, m20Invar, m02Invar;       // MomentsRegion2ndInvar (F^2 정규화)
    double ra, rb, phi;                        // EllipticAxis          (내부 F 정규화)
    double anisometry, bulkiness, structureFactor;
    double orientation;
};
void ComputeMoments(const Region& region, RegionMoments& moments);
void ComputeFeaturesAndMoments(const Region& region,          // 컨텍스트 하나로 7값 + 15값
                               RegionFeatures& features,
                               RegionMoments& moments);

// 3차 배치 — 구멍 검출 계열 4종 (2026-07-29)
//   정수 출력은 전부 long. 공개 경계에 정수 타입을 하나만 둔다
void AreaHoles       (const Region&, long& area);
void ConnectAndHoles (const Region&, long& numConnected, long& numHoles);
void EulerNumber     (const Region&, long& eulerNumber);
void DiameterRegion  (const Region&, long& row1, long& column1,
                      long& row2, long& column2, double& diameter);

struct RegionTopology                // POD. RegionFeatures/RegionMoments 와 또 **분리**한다
{
    long   areaHoles;
    long   numConnected, numHoles, eulerNumber;
    long   diameterRow1, diameterColumn1, diameterRow2, diameterColumn2;
    double diameter;
};
void ComputeTopology(const Region& region, RegionTopology& topology);
void ComputeFeaturesMomentsAndTopology(const Region&,          // 컨텍스트 하나로 7 + 15 + 9
                                       RegionFeatures&, RegionMoments&, RegionTopology&);

// 검사기 연동 편의 API (HALCON 원명 대응 없음)
Region SelectLargestBlob(const Region& region);               // 최대 8-연결 성분 하나만
bool   ComputeLargestBlobFeatures(const unsigned char* data,  // 마스크 → 최대 blob → 7종
                                  int width, int height, int stride,
                                  RegionFeatures& features);   // 이 함수만 bool (입력 오류/전경 없음 구분)

}} // namespace glim::halcon
```

규칙:

- **출력은 참조 인자**로 받는다 (HALCON 시그니처와 1:1 대응, 다중 출력 오퍼레이터가 많음).
- **HALCON 에 없는 함수는 원칙적으로 추가하지 않는다.** 예외가 셋 있다.
  - `ComputeFeatures` — 개별 오퍼레이터가 호출마다 `FeatureContext` 를 새로 만들어 윤곽 추적이
    4회 중복되는 **실측 가능한 성능 문제**를 해결한다. 값은 개별 호출과 완전히 동일하다
    (양쪽이 내부의 같은 계산 헬퍼를 호출한다).
  - `SelectLargestBlob` / `ComputeLargestBlobFeatures` — 검사기는 이진화 crop 한 장을 던지고
    "그 안의 불량 하나"의 값을 원한다. 그러나 이 라이브러리에는 HALCON `connection` 이 없어
    `FromMask` 가 마스크 전체를 한 리전으로 만든다. 그래서 **최대 연결성분 하나만** 고르는
    단계가 필요하다. 라벨링은 윤곽 추적이 쓰는 `core::LabelConnectedRuns` 를 그대로 재사용해
    "윤곽이 보는 성분" 과 "최대 blob 으로 뽑히는 성분" 이 어긋날 여지를 만들지 않았다.
    `ComputeLargestBlobFeatures` 만 `bool` 을 반환한다 — 원시 포인터/치수를 직접 받아
    **입력 오류와 "전경 없음"** 을 구분해야 하기 때문이다(그 둘이 같아 보이면 검사기가 조용히
    오판한다). 나머지는 기존 "실패도 0" 규약을 그대로 따른다.
- **공개 구조체는 POD 로 유지한다.** 툴셋(v140/v143)이 달라도 레이아웃이 흔들리지 않아야 한다.
  필드 추가는 **끝에만** 가능하고, 그때는 라이브러리와 사용처를 함께 재컴파일해야 한다.
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
│   ├── Core/                    (예정) Region, Contour, ConvexHull, Moments, Holes
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
| 2026-07-21 | 배치 API `ComputeFeatures` 추가 (기존 5종은 그대로 유지) | `FeatureContext` 캐시가 오퍼레이터 호출 경계를 넘지 못해 윤곽 추적이 4회 중복됐다. 기존 시그니처를 깨지 않기 위해 **변경이 아니라 추가**로 해결 |
| 2026-07-21 | 윤곽 추적의 dense 라벨 이미지 → 런 이진탐색 인덱스 | 메모리가 바운딩박스 크기에 비례해 풀프레임 리전에서 1.3 GB 로 폭발했다. 판정이 O(1)→O(log n) 이 되는 대가를 지불했다 |
| 2026-07-21 | `FromMask` 의 `0 < stride < width` 를 빈 리전으로 거부 | 버퍼 밖 읽기를 유발한다. `width` 로 보정해도 읽는 총량이 버퍼를 넘으므로 보정으로는 안전해지지 않는다 |
| 2026-07-21 | 성분 라벨링을 `core::LabelConnectedRuns` 로 승격(추출) | `SelectLargestBlob` 이 같은 라벨링을 필요로 했다. `TraceOuterContours` 안에 복붙하면 "윤곽 성분" 과 "최대 blob 성분" 이 어긋날 수 있어, **양쪽이 함수 하나만 호출**하게 했다. 기존 윤곽 추적 동작은 불변 |
| 2026-07-21 | 검사기 원샷 `ComputeLargestBlobFeatures` 추가, 이 함수만 `bool` 반환 | 원시 포인터/치수를 직접 받아 **입력 오류 ≠ 전경 없음** 을 구분해야 한다. 기존 API 는 검증된 `Region` 만 받아 "실패도 0" 으로 충분했다. 기존 시그니처는 전부 불변, **추가**로만 해결 |
| 2026-07-28 | 2차 배치 결과를 `RegionFeatures` 에 붙이지 않고 **`RegionMoments` 새 구조체**로 분리 | `RegionFeatures` 는 이미 배포된 공개 경계다. 끝에 15개를 붙이면 `sizeof` 가 커져 **옛 헤더로 컴파일된 호출부가 링크는 되면서 스택을 넘겨 쓴다.** 이 라이브러리는 검사기 여러 대에 **소스로** 배포되므로 재컴파일 누락이 실제로 일어난다. 구조체를 나누면 그 사고가 원천적으로 불가능하다 |
| 2026-07-28 | 대신 `ComputeFeaturesAndMoments` 추가 | 구조체를 나눈 대가(컨텍스트 2회 생성 → 윤곽 추적 2회)를 없앤다. **나눠도 성능 손해가 없게** 만든 뒤에 나눴다 |
| 2026-07-28 | 미결 쟁점 스위치를 **내부 헤더의 매크로**로 (`src/Core/Moments.h`) | 런타임 `const bool` 분기는 MSVC `/W4` 에서 `C4127`(조건식이 상수)을 낸다. `#if` 는 경고가 없고, 기본 분기가 항상 컴파일된다. 공개 헤더가 아니므로 사용처를 오염시키지 않는다 |
| 2026-07-28 | `GetMaxContourDistance` 와 최원 윤곽점을 **한 순회**로 통합 | `circularity` 의 `max` 와 `orientation_region` 의 기준점은 같은 점이어야 한다. 순회를 두 벌 두면 언젠가 어긋난다. 값은 불변(초기값만 `0.0`→`-1.0`, 최댓값은 동일) |
| 2026-07-29 | 3차 배치 결과를 **`RegionTopology` 새 구조체**로 또 분리 | `RegionFeatures`·`RegionMoments` 와 같은 이유다. 이미 배포된 구조체에 필드를 붙이면 `sizeof` 가 커져 옛 헤더로 컴파일된 호출부가 스택을 넘겨 쓴다. 대가(컨텍스트 중복)는 `ComputeFeaturesMomentsAndTopology` 로 없앤 뒤에 나눴다 |
| 2026-07-29 | 구멍 검출에서 **배경도 런렝스로** 표현 (`src/Core/Holes.cpp`) | dense 배경 이미지를 만들면 2026-07-21 에 제거한 1.3 GB 사고가 그대로 재발한다. 배경 런 수 ≤ 2 × 전경 런 수이므로 **O(런 수)** 다. 좌우 여백 열만 만들고 위/아래 여백 행은 "이웃 행에 전경이 없으면 바깥" 규칙으로 대신해, **빈 행을 자료구조로 만들지 않는다** |
| 2026-07-29 | `area_holes` · `connect_and_holes` · `euler_number` 를 **코어 하나**(`ComputeHoleCounts`)로 | 원문이 오일러 수를 "연결성분 수 − 구멍 수" 로 정의한다. 구현이 두 벌이면 원문 정의 자체를 어기게 된다. `LabelConnectedRuns` 를 하나로 묶은 것(2026-07-21)과 같은 판단이며, 전경 성분 개수도 그 함수를 **그대로** 호출해 `contlength` 와 어긋날 여지를 없앴다 |
| 2026-07-29 | `FeatureContext` 가 hull **정점**까지 캐시 | `convexity`(hull 면적)와 `diameter_region`(hull 위 최대거리 쌍)이 **같은 hull** 을 봐야 한다. 각자 계산하게 두면 언젠가 갈린다. 계산 입력이 동일하므로 `convexity` 값은 변하지 않는다 |
| 2026-07-29 | 미결 쟁점 스위치를 `src/Core/Holes.h` 상단 매크로로 (`GLIM_HALCON_BACKGROUND_CONNECTIVITY_8`) | `src/Core/Moments.h` 와 같은 방식. 내부 헤더이므로 사용처를 오염시키지 않고, `#if` 는 `/W4` 경고가 없다 |
| 2026-07-29 | ⚠ **N개 묶음 배치 함수는 여기서 멈춘다** | `ComputeFeatures` → `ComputeFeaturesAndMoments` → `ComputeFeaturesMomentsAndTopology` 로 배치마다 이름이 길어지고 조합이 늘고 있다. 4차 배치에서는 이 방식을 더 늘리지 말고 **재사용 가능한 공개 컨텍스트 핸들**을 도입해야 한다 (설계안: `05_USER_GUIDE.md` §6.5). 지금 도입하지 않은 이유는, 그것이 새 공개 타입 하나를 영구히 추가하는 결정이라 배치 작업 중에 끼워 넣을 사안이 아니기 때문이다 |

---

← 이전: [개요](00_OVERVIEW.md)  |  [🏠 홈](../README.md)  |  다음: [특징값 정의서 →](02_DEFINITIONS.md)
