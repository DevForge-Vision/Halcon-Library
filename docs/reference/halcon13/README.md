[🏠 홈](../../../README.md) › [📚 문서](../../README.md) › **📦 원문 아카이브**

---

# 📦 HALCON 13 원문 수집 아카이브

MVTec HALCON 13 Operator Reference 의 **Regions ▸ Features 오퍼레이터 40종** 원문을
5개 파일(G1~G5, 총 2,094줄)로 나눠 수집한 아카이브입니다. **이 저장소에서 유일하게 인정되는 근거**입니다.

> ⚠ **다시 웹에서 긁지 마세요.**
> MVTec 문서의 수식은 텍스트가 아니라 **MetaPost 인라인 SVG 글리프**로 렌더링되어 있어,
> 일반적인 크롤링·요약으로는 **수식이 통째로 누락되거나 분자·분모가 뒤집힙니다.**
> 이 아카이브는 HTML 글리프 ID(`cmr10`/`cmmi10`/`cmsy10`/`cmex10`/`pcrr8r`)를 TeX 폰트 인코딩으로
> 역매핑하고 SVG transform 좌표로 분수선·첨자를 재구성해 복원한 것입니다 ([`CLAUDE.md`](../../../CLAUDE.md) §2.4).

<details>
<summary><b>목차</b></summary>

- [1. 파일 구성](#1-파일-구성)
- [2. 오퍼레이터 40종 전체 목록과 상태](#2-오퍼레이터-40종-전체-목록과-상태)
  - [2.1 G1 — 기본 / 크기 (8종)](#21-g1--기본--크기-8종)
  - [2.2 G2 — 형상 계수 (8종)](#22-g2--형상-계수-8종)
  - [2.3 G3 — 외접 / 내접 / 런렝스 (7종)](#23-g3--외접--내접--런렝스-7종)
  - [2.4 G4 — 모멘트 (7종)](#24-g4--모멘트-7종)
  - [2.5 G5 — 선택 / 관계 (10종)](#25-g5--선택--관계-10종)
- [3. 아카이브 사용 규칙](#3-아카이브-사용-규칙)

</details>

---

## 1. 파일 구성

| 파일 | 그룹 | 종수 | 줄 수 | 수집일 |
|---|---|:--:|--:|---|
| [`G1_basic_size.md`](G1_basic_size.md) | 기본 / 크기 | 8 | 483 | 2026-07-20 |
| [`G2_shape_factors.md`](G2_shape_factors.md) | 형상 계수 | 8 | 392 | 2026-07-20 |
| [`G3_geometry_runlength.md`](G3_geometry_runlength.md) | 외접 / 내접 / 런렝스 | 7 | 303 | 2026-07-20 |
| [`G4_moments.md`](G4_moments.md) | 모멘트 | 7 | 448 | 2026-07-20 |
| [`G5_select_relation.md`](G5_select_relation.md) | 선택 / 관계 | 10 | 468 | 2026-07-20 |
| | **합계** | **40** | **2,094** | |

---

## 2. 오퍼레이터 40종 전체 목록과 상태

범례 — **정의** = [`02_DEFINITIONS.md`](../../02_DEFINITIONS.md) 에 확정 · **검증** = [`03_VERIFICATION.md`](../../03_VERIFICATION.md) 기댓값 통과 · **구현** = 코드 완료
상태 원본은 [`00_OVERVIEW.md` §6](../../00_OVERVIEW.md#6-40종-구현-현황) 입니다.

### 2.1 G1 — 기본 / 크기 (8종)

| # | 오퍼레이터 | 원문 | 정의 | 검증 | 구현 | 비고 |
|:--:|---|---|:--:|:--:|:--:|---|
| 1 | `area_center` | [G1 §1](G1_basic_size.md#1-area_center) | ✅ | ✅ | ✅ | Phase 1 |
| 2 | `area_holes` | [G1 §2](G1_basic_size.md#2-area_holes) | ⬜ | ⬜ | ⬜ | 8-connectivity 기본 |
| 3 | `contlength` | [G1 §3](G1_basic_size.md#3-contlength) | ✅ | ✅ | ✅ | Phase 1 / 구멍 제외 |
| 4 | `diameter_region` | [G1 §4](G1_basic_size.md#4-diameter_region) | ⬜ | ⬜ | ⬜ | |
| 5 | `connect_and_holes` | [G1 §5](G1_basic_size.md#5-connect_and_holes) | ⬜ | ⬜ | ⬜ | connectivity 원문 미명시 |
| 6 | `euler_number` | [G1 §6](G1_basic_size.md#6-euler_number) | ⬜ | ⬜ | ⬜ | connectivity 원문 미명시 |
| 7 | `get_region_thickness` | [G1 §7](G1_basic_size.md#7-get_region_thickness) | ⬜ | ⬜ | ⬜ | 단일 리전만, 첫 성분만 |
| 8 | `region_features` | [G1 §8](G1_basic_size.md#8-region_features) | ⬜ | ⬜ | ⬜ | 63 feature 파사드 |

부속 절: [공통 사항](G1_basic_size.md#공통-사항-8개-오퍼레이터-모두-해당) · [부록: 문서에 명시 없음 항목](G1_basic_size.md#부록-문서에-명시-없음-항목-정리)

### 2.2 G2 — 형상 계수 (8종)

| # | 오퍼레이터 | 원문 | 정의 | 검증 | 구현 | 비고 |
|:--:|---|---|:--:|:--:|:--:|---|
| 1 | `circularity` | [G2 §1](G2_shape_factors.md#1-circularity--원형도) | ✅ | ✅ | ✅ | Phase 1 / `min` 클리핑 |
| 2 | `compactness` | [G2 §2](G2_shape_factors.md#2-compactness--조밀도) | ✅ | ✅ | ✅ | Phase 1 / `max` 클리핑 (방향 반대) |
| 3 | `convexity` | [G2 §3](G2_shape_factors.md#3-convexity--볼록도) | ✅ | ✅ | ✅ | Phase 1 / `F_o / F_c` |
| 4 | `rectangularity` | [G2 §4](G2_shape_factors.md#4-rectangularity--사각형도) | ⬜ | ⬜ | ⬜ | **닫힌 수식 없음 — 최난도** |
| 5 | `roundness` | [G2 §5](G2_shape_factors.md#5-roundness--진원도-4개-출력) | ⬜ | ⬜ | ⬜ | 4출력 |
| 6 | `eccentricity` | [G2 §6](G2_shape_factors.md#6-eccentricity--이심률-계열-3개-출력) | ⬜ | ⬜ | ⬜ | `Rb=0` 방어 필수 |
| 7 | `elliptic_axis` | [G2 §7](G2_shape_factors.md#7-elliptic_axis--등가-타원-축) | ⬜ | ⬜ | ⬜ | 모든 모멘트 계열의 관문 |
| 8 | `orientation_region` | [G2 §8](G2_shape_factors.md#8-orientation_region--리전-방향) | ⬜ | ⬜ | ⬜ | |

부속 절: [공통 사항](G2_shape_factors.md#공통-사항) · [부록 — 한눈에 보는 공식표](G2_shape_factors.md#부록--한눈에-보는-공식표)

### 2.3 G3 — 외접 / 내접 / 런렝스 (7종)

| # | 오퍼레이터 | 원문 | 정의 | 검증 | 구현 | 비고 |
|:--:|---|---|:--:|:--:|:--:|---|
| 1 | `inner_circle` | [G3 §1](G3_geometry_runlength.md#1-inner_circle) | ⬜ | ⬜ | ⬜ | radius 보정 없음 |
| 2 | `inner_rectangle1` | [G3 §2](G3_geometry_runlength.md#2-inner_rectangle1) | ⬜ | ⬜ | ⬜ | "largest" 기준 원문 미명시 |
| 3 | `smallest_circle` | [G3 §3](G3_geometry_runlength.md#3-smallest_circle) | ⬜ | ⬜ | ⬜ | radius **+0.5 보정** |
| 4 | `smallest_rectangle1` | [G3 §4](G3_geometry_runlength.md#4-smallest_rectangle1) | ⬜ | ⬜ | ⬜ | |
| 5 | `smallest_rectangle2` | [G3 §5](G3_geometry_runlength.md#5-smallest_rectangle2) | ⬜ | ⬜ | ⬜ | Length = **반변** |
| 6 | `runlength_distribution` | [G3 §6](G3_geometry_runlength.md#6-runlength_distribution) | ⬜ | ⬜ | ⬜ | index 0 은 항상 0 |
| 7 | `runlength_features` | [G3 §7](G3_geometry_runlength.md#7-runlength_features) | ⬜ | ⬜ | ⬜ | LFactor 정의 모호 |

부속 절: [구현 시 핵심 주의사항 요약](G3_geometry_runlength.md#구현-시-핵심-주의사항-요약)

### 2.4 G4 — 모멘트 (7종)

| # | 오퍼레이터 | 원문 | 정의 | 검증 | 구현 | 비고 |
|:--:|---|---|:--:|:--:|:--:|---|
| 1 | `moments_region_2nd` | [G4 §1](G4_moments.md#1-moments_region_2nd) | ⬜ | ⬜ | ⬜ | **정규화 없음(순수 합)** |
| 2 | `moments_region_2nd_invar` | [G4 §2](G4_moments.md#2-moments_region_2nd_invar) | ⬜ | ⬜ | ⬜ | `F²` 로 정규화 |
| 3 | `moments_region_2nd_rel_invar` | [G4 §3](G4_moments.md#3-moments_region_2nd_rel_invar) | ⬜ | ⬜ | ⬜ | PHI1, PHI2 |
| 4 | `moments_region_3rd` | [G4 §4](G4_moments.md#4-moments_region_3rd) | ⬜ | ⬜ | ⬜ | 부호 규약 반대 |
| 5 | `moments_region_3rd_invar` | [G4 §5](G4_moments.md#5-moments_region_3rd_invar) | ⬜ | ⬜ | ⬜ | `mu_00^3` 정규화 |
| 6 | `moments_region_central` | [G4 §6](G4_moments.md#6-moments_region_central) | ⬜ | ⬜ | ⬜ | I1~I4 |
| 7 | `moments_region_central_invar` | [G4 §7](G4_moments.md#7-moments_region_central_invar) | ⬜ | ⬜ | ⬜ | PSI1~PSI4, 아핀 불변 |

부속 절: [공통 사항](G4_moments.md#공통-사항-7개-전부-동일) · [정규화 방식 비교표](G4_moments.md#정규화-방식-비교표-핵심)

### 2.5 G5 — 선택 / 관계 (10종)

| # | 오퍼레이터 | 원문 | 정의 | 검증 | 구현 | 비고 |
|:--:|---|---|:--:|:--:|:--:|---|
| 1 | `select_shape` | [G5 §1](G5_select_relation.md#1-select_shape) | ⬜ | ⬜ | ⬜ | **63 feature 이름 체계의 근간** |
| 2 | `select_shape_proto` | [G5 §2](G5_select_relation.md#2-select_shape_proto) | ⬜ | ⬜ | ⬜ | Operation 파라미터 없음 |
| 3 | `select_shape_std` | [G5 §3](G5_select_relation.md#3-select_shape_std) | ⬜ | ⬜ | ⬜ | |
| 4 | `select_region_spatial` | [G5 §4](G5_select_relation.md#4-select_region_spatial) | ⬜ | ⬜ | ⬜ | |
| 5 | `spatial_relation` | [G5 §5](G5_select_relation.md#5-spatial_relation) | ⬜ | ⬜ | ⬜ | |
| 6 | `hamming_distance` | [G5 §6](G5_select_relation.md#6-hamming_distance) | ⬜ | ⬜ | ⬜ | Similarity 수식 미확보 |
| 7 | `hamming_distance_norm` | [G5 §7](G5_select_relation.md#7-hamming_distance_norm) | ⬜ | ⬜ | ⬜ | 출력명은 `Distance` |
| 8 | `find_neighbors` | [G5 §8](G5_select_relation.md#8-find_neighbors) | ⬜ | ⬜ | ⬜ | MaxDistance 범위 1~255 |
| 9 | `get_region_index` | [G5 §9](G5_select_relation.md#9-get_region_index) | ⬜ | ⬜ | ⬜ | |
| 10 | `select_region_point` | [G5 §10](G5_select_relation.md#10-select_region_point) | ⬜ | ⬜ | ⬜ | |

부속 절: [부록: 문서에 명시가 없어 확정할 수 없는 항목](G5_select_relation.md#부록-문서에-명시가-없어-확정할-수-없는-항목-정리)

---

## 3. 아카이브 사용 규칙

1. **원문이 유일한 기준입니다.** 컴퓨터비전 통념(Hu 모멘트, OpenCV 관례)으로 대체하지 않습니다.
2. **원문에 없으면 "문서에 명시 없음"** 으로 남기고 미결 쟁점으로 등록합니다. 추측으로 채우면 검증이 불가능해집니다.
3. 원문과 통념이 어긋나는 지점 **14건**이 확인되어 있습니다 → [`00_OVERVIEW.md` §5](../../00_OVERVIEW.md#5-원문-수집에서-드러난-함정-구현-전-필독)
4. 새 오퍼레이터를 다룰 때는 이 아카이브 → [`02_DEFINITIONS.md`](../../02_DEFINITIONS.md) → [`03_VERIFICATION.md`](../../03_VERIFICATION.md) → 구현 순서를 지킵니다.

---

← 이전: [구현 노트](../../04_IMPLEMENTATION_NOTES.md)  |  [🏠 홈](../../../README.md)  |  다음: [G1 기본 / 크기 →](G1_basic_size.md)
