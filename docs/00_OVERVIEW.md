# 00. 프로젝트 개요 및 범위

## 1. 목적

MVTec HALCON 13 의 **Regions ▸ Features** 오퍼레이터 40종을, HALCON SDK 없이
**C++ 자체 엔진**으로 재현하는 라이브러리를 만든다.

- HALCON 런타임 라이선스 없이 동일한 특징값을 얻는 것이 목표다.
- 최종 사용처는 코터 / 슬리터 / 롤프레스 검사 프로그램이며, 이 라이브러리를 **참조(static lib)** 형태로 링크해 쓴다.
- 따라서 **정확도가 개수보다 우선**한다. 5개만 있어도 HALCON 값과 일치해야 의미가 있다.

## 2. 절대 원칙

| # | 원칙 | 이유 |
|---|---|---|
| 1 | **정확도 > 개수** | 값이 틀린 40종보다 값이 맞는 5종이 낫다 |
| 2 | **정의 → 검증 → 구현** 순서 | 정의가 틀리면 구현은 전부 폐기다 |
| 3 | **HALCON 원문이 유일한 기준** | 추측/일반 통념(예: Hu 모멘트)으로 대체하지 않는다 |
| 4 | 원문에 없으면 **"문서에 명시 없음"** 으로 남긴다 | 모르는 것을 아는 척하면 검증이 불가능해진다 |
| 5 | 모든 특징값은 **해석적으로 계산 가능한 도형**으로 단위 검증한다 | 눈으로 맞아 보이는 값은 검증이 아니다 |

## 3. 진행 방식

```
[1단계] 문서 고도화        ✅ 완료
        - 원문 수집 (완료: docs/reference/halcon13/)
        - 특징값 정의서 작성 (docs/02_DEFINITIONS.md)
        - 정의 검증 계획 수립 (docs/03_VERIFICATION.md)

[2단계] 정의 검증
        - 기준 도형의 기댓값을 손으로 계산
        - HALCON 실측값과 대조 (가능한 환경에서)
        - 불일치 시 정의서 수정 → 재검증

[3단계] 구현 (우선 5종)   ✅ 완료 — PASS 70/FAIL 0, MSVC 14.44 검증
        - 검증된 정의만 코드로 옮긴다
        - 단위 테스트가 기댓값과 일치해야 커밋

[4단계] 확장              ← 지금 여기
        - 5종이 통과하면 나머지 35종을 같은 절차로 반복
```

## 4. 1차 구현 대상 (5종)

기반 인프라(Region 자료구조 · contour 추출 · convex hull)를 모두 요구하면서
해석적 검증이 명확한 조합으로 선정했다.

| 순서 | 오퍼레이터 | 선정 이유 | 요구 인프라 |
|---|---|---|---|
| 1 | `area_center` | 모든 특징값의 기초(면적·무게중심). 이것이 틀리면 전부 틀린다 | Region(런렝스) |
| 2 | `contlength` | 윤곽 길이. compactness 의 입력이며 체인코드 정의 검증이 필요 | contour 추출 |
| 3 | `circularity` | 중심→contour 최대거리. 대표 형상계수 | contour + center |
| 4 | `compactness` | `contlength` 의존. 정의 연쇄가 맞는지 확인하는 지표 | contlength + area |
| 5 | `convexity` | convex hull 면적. Tear/찢김 판별의 핵심 | convex hull |

> 이 5종이 통과하면 `elliptic_axis` → `moments_*` → `eccentricity` 순으로 확장한다.
> 모멘트 계열은 원문 수식이 통념과 다른 지점이 여러 곳이라(§5) 별도 검증 라운드를 둔다.

## 5. 원문 수집에서 드러난 함정 (구현 전 필독)

원문 수식은 텍스트가 아니라 **MetaPost 인라인 SVG 글리프**로 렌더링되어 있다.
일반적인 페이지 요약/크롤링으로는 **수식이 통째로 누락되거나 분자·분모가 뒤집힌다.**
아래는 HTML 글리프를 직접 디코딩해 확정한, **통념과 어긋나는** 지점들이다.

| # | 함정 | 통념 | HALCON 13 원문 |
|---|---|---|---|
| 1 | `circularity` 클리핑 | 클리핑 없음 | `C = min(1, F/(max²·π))` — 위로 자름 |
| 2 | `compactness` 클리핑 | 클리핑 없음 | `C = max(1, L²/(4·F·π))` — **아래로** 자름 (방향 반대) |
| 3 | `convexity` 분자/분모 | 자동 요약 도구가 자주 뒤집음 | `C = F_o / F_c` (원본면적 / 볼록껍질면적) |
| 4 | `contlength` 구멍 | 구멍 윤곽 포함 | **"The contour of holes is not calculated"** — 외곽만 |
| 5 | `rectangularity` | `A / minAreaRect` 로 근사 가능 | **닫힌 수식이 문서에 없음.** 1·2차 모멘트가 같은 사각형과의 차집합 면적 기반. 정사각형은 최대 10% 과소평가 |
| 6 | `moments_region_2nd` 정규화 | 면적으로 나눔 | **정규화 없음(순수 합)**. 면적 정규화는 `_invar` 쪽이며 그것도 `F²` 로 나눔 |
| 7 | 모멘트 부호 규약 | 일관됨 | 2nd 계열은 `(중심 − 픽셀)`, 3rd 는 `(픽셀 − 중심)` — **3차 홀수항에서 부호가 뒤집힌다** |
| 8 | `moments_region_3rd_invar` 정규화 지수 | Hu 관례의 2.5 | 원문은 **`mu_00^3`** |
| 9 | `smallest_rectangle2` 의 Length1/2 | 전체 변 길이 | **반변(half length)** — OpenCV `minAreaRect` 대비 2배 차이 |
| 10 | `smallest_circle` 반지름 | 순수 최소외접원 | 반환 radius 에 **+0.5 보정**이 들어감 (`inner_circle` 에는 없음) |
| 11 | `elliptic_axis` 의 Phi 부호 | `+0.5·atan2(2M11, M20−M02)` | **`−0.5·atan2(2·M11, M02 − M20)`** |
| 12 | 1픽셀 두께 리전 | 정상 처리 | 픽셀을 무한소 점으로 취급 → **Rb = 0** → `anisometry` 가 Assertion(≥1)을 어기고 **0** 반환 |
| 13 | `runlength_features` 의 LFactor | "런 수 / 높이" | 원문은 **"mean number of runs for each line index occurring in the region"** — 명시적 분수 수식 없음 |
| 14 | `region_features` feature 개수 | — | **63종** (오퍼레이터 40종과 별개의 이름 체계) |

## 6. 40종 구현 현황

범례: `정의` = 정의서 확정 · `검증` = 기댓값 검증 통과 · `구현` = 코드 완료

### 6.1 기본 / 크기 (8)

| 오퍼레이터 | 정의 | 검증 | 구현 | 비고 |
|---|:--:|:--:|:--:|---|
| area_center | ✅ | ✅ | ✅ | 1차 대상 |
| area_holes | ⬜ | ⬜ | ⬜ | 8-connectivity 기본 |
| contlength | ✅ | ✅ | ✅ | 1차 대상 / 구멍 제외 |
| diameter_region | ⬜ | ⬜ | ⬜ | |
| connect_and_holes | ⬜ | ⬜ | ⬜ | connectivity 원문 미명시 |
| euler_number | ⬜ | ⬜ | ⬜ | connectivity 원문 미명시 |
| get_region_thickness | ⬜ | ⬜ | ⬜ | 단일 리전만, 첫 성분만 |
| region_features | ⬜ | ⬜ | ⬜ | 63 feature 파사드 |

### 6.2 형상 계수 (8)

| 오퍼레이터 | 정의 | 검증 | 구현 | 비고 |
|---|:--:|:--:|:--:|---|
| circularity | ✅ | ✅ | ✅ | 1차 대상 |
| compactness | ✅ | ✅ | ✅ | 1차 대상 |
| convexity | ✅ | ✅ | ✅ | 1차 대상 |
| rectangularity | ⬜ | ⬜ | ⬜ | **닫힌 수식 없음 — 최난도** |
| roundness | ⬜ | ⬜ | ⬜ | 4출력 |
| eccentricity | ⬜ | ⬜ | ⬜ | Rb=0 방어 필수 |
| elliptic_axis | ⬜ | ⬜ | ⬜ | 모든 모멘트 계열의 관문 |
| orientation_region | ⬜ | ⬜ | ⬜ | |

### 6.3 외접 / 내접 / 런렝스 (7)

| 오퍼레이터 | 정의 | 검증 | 구현 | 비고 |
|---|:--:|:--:|:--:|---|
| inner_circle | ⬜ | ⬜ | ⬜ | |
| inner_rectangle1 | ⬜ | ⬜ | ⬜ | "largest" 기준 원문 미명시 |
| smallest_circle | ⬜ | ⬜ | ⬜ | radius +0.5 보정 |
| smallest_rectangle1 | ⬜ | ⬜ | ⬜ | |
| smallest_rectangle2 | ⬜ | ⬜ | ⬜ | Length = 반변 |
| runlength_distribution | ⬜ | ⬜ | ⬜ | index 0 은 항상 0 |
| runlength_features | ⬜ | ⬜ | ⬜ | LFactor 정의 모호 |

### 6.4 모멘트 (7)

| 오퍼레이터 | 정의 | 검증 | 구현 | 비고 |
|---|:--:|:--:|:--:|---|
| moments_region_2nd | ⬜ | ⬜ | ⬜ | 정규화 없음 |
| moments_region_2nd_invar | ⬜ | ⬜ | ⬜ | `F²` 로 정규화 |
| moments_region_2nd_rel_invar | ⬜ | ⬜ | ⬜ | PHI1, PHI2 |
| moments_region_3rd | ⬜ | ⬜ | ⬜ | 부호 규약 반대 |
| moments_region_3rd_invar | ⬜ | ⬜ | ⬜ | `mu_00^3` |
| moments_region_central | ⬜ | ⬜ | ⬜ | I1~I4 |
| moments_region_central_invar | ⬜ | ⬜ | ⬜ | PSI1~PSI4, 아핀 불변 |

### 6.5 선택 / 관계 (10)

| 오퍼레이터 | 정의 | 검증 | 구현 | 비고 |
|---|:--:|:--:|:--:|---|
| find_neighbors | ⬜ | ⬜ | ⬜ | MaxDistance 범위 1~255 |
| get_region_index | ⬜ | ⬜ | ⬜ | |
| hamming_distance | ⬜ | ⬜ | ⬜ | Similarity 수식 미확보 |
| hamming_distance_norm | ⬜ | ⬜ | ⬜ | 출력명은 `Distance` |
| select_region_point | ⬜ | ⬜ | ⬜ | |
| select_region_spatial | ⬜ | ⬜ | ⬜ | |
| select_shape | ⬜ | ⬜ | ⬜ | **63 feature 이름 체계의 근간** |
| select_shape_proto | ⬜ | ⬜ | ⬜ | Operation 파라미터 없음 |
| select_shape_std | ⬜ | ⬜ | ⬜ | |
| spatial_relation | ⬜ | ⬜ | ⬜ | |

## 7. 참고 문서

- `docs/01_ARCHITECTURE.md` — 라이브러리 설계 (경계·패턴·성능)
- `docs/02_DEFINITIONS.md` — 특징값 정의서 (구현의 유일한 근거)
- `docs/03_VERIFICATION.md` — 정의 검증 계획과 기댓값
- `docs/reference/halcon13/` — HALCON 13 원문 수집 아카이브 (G1~G5, 2094줄)
- `docs/04_IMPLEMENTATION_NOTES.md` — Phase 1 구현 노트 · 검증 실측 결과 · 문서 보정 이력
- `CLAUDE.md` — AI 에이전트/신규 참여자용 작업 지침
