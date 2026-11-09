[🏠 홈](../README.md) › **📚 문서**

---

# 📚 문서 허브

이 폴더는 **HALCON 13 `Regions ▸ Features` 오퍼레이터 40종을 SDK 없이 재현**하기 위한 문서 체계입니다.
`원문 아카이브 → 정의서 → 검증 → 구현 노트` 순으로 근거가 이어지며, **코드는 언제나 이 문서들의 결론을 따릅니다.**
정의가 틀린 채로 짠 코드는 전부 폐기이므로, 구현 전에 아래 순서대로 읽는 것을 권합니다.

---

## 1. 읽는 순서

### 1.1 신규 참여자 — 처음부터 끝까지

| 순서 | 문서 | 무엇을 얻는가 |
|:--:|---|---|
| 0 | [`CLAUDE.md`](../CLAUDE.md) | **작업 시작 전 필수.** 절대 원칙 · 함정 8건 · 코딩 규칙 |
| 1 | [`00_OVERVIEW.md`](00_OVERVIEW.md) | 프로젝트 범위 · 절대 원칙 5건 · 원문 함정 14건 · 40종 현황표 |
| 2 | [`01_ARCHITECTURE.md`](01_ARCHITECTURE.md) | 왜 PIMPL·런렝스·캐시인가. 코드를 짜기 직전에 읽는다 |
| 3 | [`02_DEFINITIONS.md`](02_DEFINITIONS.md) | **구현의 유일한 근거.** 5종의 수식·예외·쟁점 해소 |
| 4 | [`03_VERIFICATION.md`](03_VERIFICATION.md) | 기준 도형 S1~S7 과 손계산 기댓값. 테스트의 근거 |
| 5 | [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) | Phase 1 실측 결과 · 문서 보정 4건 · 남은 리스크 |
| 6 | [`reference/halcon13/`](reference/halcon13/README.md) | HALCON 13 원문 아카이브. 새 오퍼레이터를 추가할 때 |

### Blob/그룹 확장값

[region_features 1~20 사용법](15_REGION_FEATURES20.md): 추가 API, 반환 순서,
MFC 표시, 잠정 정의 및 검증 상태. 현재 미빌드·미배포 소스다.

### 1.2 목적별 진입 경로

| 지금 하려는 일 | 이렇게 들어가세요 |
|---|---|
| **특정 오퍼레이터의 값이 궁금하다** | 아래 [§3 오퍼레이터 역인덱스](#3-오퍼레이터--문서-위치-역인덱스) 에서 해당 행의 링크 3개를 순서대로 |
| **값이 HALCON 과 다르다** | [`03_VERIFICATION.md`](03_VERIFICATION.md) 기댓값 대조 → [`04_IMPLEMENTATION_NOTES.md` §3](04_IMPLEMENTATION_NOTES.md#3--가장-중요한-교훈--곡선-도형의-기댓값을-연속-기하로-잡으면-안-된다) (곡선 도형 편향) → [`02_DEFINITIONS.md`](02_DEFINITIONS.md) 정의 재확인 |
| **새 오퍼레이터를 추가한다** | [`reference/halcon13/README.md`](reference/halcon13/README.md) 에서 원문 위치 확인 → [`02_DEFINITIONS.md`](02_DEFINITIONS.md) 절 추가 → [`03_VERIFICATION.md`](03_VERIFICATION.md) 기댓값 손계산 → 구현 → [`00_OVERVIEW.md` §7](00_OVERVIEW.md#7-40종-구현-현황) 현황표 갱신 |
| **코드 구조를 파악한다** | [`01_ARCHITECTURE.md` §2](01_ARCHITECTURE.md#2-계층) 계층도 → [`04_IMPLEMENTATION_NOTES.md` §2](04_IMPLEMENTATION_NOTES.md#2-구현-구조) 실제 파일 구성 |
| **왜 OpenCV 를 안 쓰는지 알고 싶다** | [`02_DEFINITIONS.md` §16](02_DEFINITIONS.md#16-구현-의존성) → [`04_IMPLEMENTATION_NOTES.md` §2.1](04_IMPLEMENTATION_NOTES.md#21-왜-opencv-를-쓰지-않았나) |
| **테스트를 짠다 / 빌드한다** | [`03_VERIFICATION.md` §2~§3](03_VERIFICATION.md#2-기준-도형-정의) → [`04_IMPLEMENTATION_NOTES.md` §7](04_IMPLEMENTATION_NOTES.md#7-재현-방법) → [`CONTRIBUTING.md`](../CONTRIBUTING.md) |
| **미결 쟁점을 만났다** | [`02_DEFINITIONS.md` §15](02_DEFINITIONS.md#15-정의-확정-이력) 확정 이력 → [`03_VERIFICATION.md` §5](03_VERIFICATION.md#5-결정적-판정-요약-이-3개가-5종-구현의-전부를-좌우한다) 결정적 판정 → [`CLAUDE.md`](../CLAUDE.md) §5.3 절차 |

---

## 2. 문서 카탈로그

| 문서 | 한 줄 설명 | 줄 수 | 언제 읽는가 |
|---|---|--:|---|
| [`00_OVERVIEW.md`](00_OVERVIEW.md) | 목적 · 절대 원칙 5건 · 원문 함정 14건 · 40종 구현 현황표 | 158 | 프로젝트를 처음 볼 때, 전체 진척을 확인할 때 |
| [`01_ARCHITECTURE.md`](01_ARCHITECTURE.md) | 설계 제약 5건과 그로부터 나온 패턴(PIMPL·Facade·런렝스·캐시) | 157 | 코드를 짜기 직전 |
| [`02_DEFINITIONS.md`](02_DEFINITIONS.md) | **특징값 정의서.** 시그니처 · 원문 인용 · 수식 · 구현 지침 · 쟁점 | 360 | 수식을 코드로 옮길 때 (구현의 유일한 근거) |
| [`03_VERIFICATION.md`](03_VERIFICATION.md) | 기준 도형 S1~S7 · 손계산 기댓값 · HDevelop 실측 스크립트 | 310 | 테스트를 짤 때, 값이 안 맞을 때 |
| [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) | Phase 1 실측 결과(PASS 70/FAIL 0) · 곡선 도형 교훈 · 남은 리스크 | 204 | 구현 후 결과를 대조할 때, 곡선 도형 값이 이상할 때 |
| [`reference/halcon13/`](reference/halcon13/README.md) | HALCON 13 원문 수집 아카이브 (G1~G5, 40종 전부) | 2,094 | 정의서에 없는 오퍼레이터를 새로 다룰 때 |

> 저장소 루트에는 [`README.md`](../README.md)(프로젝트 표지), [`CLAUDE.md`](../CLAUDE.md)(작업 지침),
> [`CONTRIBUTING.md`](../CONTRIBUTING.md)(빌드·기여 절차)가 있습니다.

---

## 3. 오퍼레이터 → 문서 위치 역인덱스

Phase 1 에서 **정의 · 검증 · 구현이 모두 끝난 5종**입니다. 한 행이 곧 그 오퍼레이터의 전체 근거 사슬입니다.

| 오퍼레이터 | 값 | 정의서 | 검증 기댓값 | HALCON 원문 | 구현 노트 |
|---|---|---|---|---|---|
| `area_center` | 면적 · 무게중심 | [§1](02_DEFINITIONS.md#1-area_center--면적과-무게중심) | [T-1](03_VERIFICATION.md#t-1-area_center) | [G1 §1](reference/halcon13/G1_basic_size.md#1-area_center) | [§1](04_IMPLEMENTATION_NOTES.md#1-결과-요약) |
| `contlength` | 윤곽 길이 (구멍 제외) | [§2](02_DEFINITIONS.md#2-contlength--윤곽-길이) | [T-2](03_VERIFICATION.md#t-2-contlength--최대-쟁점) | [G1 §3](reference/halcon13/G1_basic_size.md#3-contlength) | [§3](04_IMPLEMENTATION_NOTES.md#3--가장-중요한-교훈--곡선-도형의-기댓값을-연속-기하로-잡으면-안-된다) |
| `circularity` | 원형도 `min(1, F/(max²π))` | [§3](02_DEFINITIONS.md#3-circularity--원형도) | [T-3](03_VERIFICATION.md#t-3-circularity) | [G2 §1](reference/halcon13/G2_shape_factors.md#1-circularity--원형도) | [§4](04_IMPLEMENTATION_NOTES.md#4-문서-보정-4건) |
| `compactness` | 조밀도 `max(1, L²/(4Fπ))` | [§4](02_DEFINITIONS.md#4-compactness--조밀도) | [T-4](03_VERIFICATION.md#t-4-compactness) | [G2 §2](reference/halcon13/G2_shape_factors.md#2-compactness--조밀도) | [§5](04_IMPLEMENTATION_NOTES.md#5-추가로-확정한-사항) |
| `convexity` | 볼록도 `F_o / F_c` | [§5](02_DEFINITIONS.md#5-convexity--볼록도) | [T-5](03_VERIFICATION.md#t-5-convexity--최대-쟁점-cv-1) | [G2 §3](reference/halcon13/G2_shape_factors.md#3-convexity--볼록도) | [§2.3](04_IMPLEMENTATION_NOTES.md#23-방어-설계) |

2차 배치(2차 모멘트 계열 5종) — 정의 · 검증 완료, 구현 **미빌드**

| 오퍼레이터 | 값 | 정의서 | 검증 기댓값 | HALCON 원문 |
|---|---|---|---|---|
| `moments_region_2nd` | M11 · M20 · M02 · Ia · Ib (**정규화 없음**) | [§6](02_DEFINITIONS.md#6-moments_region_2nd--2차-모멘트) | [T-7](03_VERIFICATION.md#t-7-moments_region_2nd) | [G4 §1](reference/halcon13/G4_moments.md) |
| `moments_region_2nd_invar` | 위 값 / `F²` | [§7](02_DEFINITIONS.md#7-moments_region_2nd_invar--f²-정규화-2차-모멘트) | [T-8](03_VERIFICATION.md#t-8-moments_region_2nd_invar--정규화-대조군) | [G4 §2](reference/halcon13/G4_moments.md) |
| `elliptic_axis` | Ra · Rb · Phi (내부 `F` 정규화) | [§8](02_DEFINITIONS.md#8-elliptic_axis--등가-타원-축) | [T-9](03_VERIFICATION.md#t-9-elliptic_axis) | [G2 §7](reference/halcon13/G2_shape_factors.md) |
| `eccentricity` | Anisometry · Bulkiness · StructureFactor | [§9](02_DEFINITIONS.md#9-eccentricity--이심률-3종) | [T-10](03_VERIFICATION.md#t-10-eccentricity--rb--0) | [G2 §6](reference/halcon13/G2_shape_factors.md) |
| `orientation_region` | 방향 (Phi + 최원 윤곽점) | [§10](02_DEFINITIONS.md#10-orientation_region--리전-방향) | [T-11](03_VERIFICATION.md#t-11-orientation_region) | [G2 §8](reference/halcon13/G2_shape_factors.md) |

3차 배치(구멍 검출 계열 4종) — 정의 · 검증 완료, 구현 **미빌드**

| 오퍼레이터 | 값 | 정의서 | 검증 기댓값 | HALCON 원문 | 구현 노트 |
|---|---|---|---|---|---|
| `area_holes` | 구멍 픽셀 총합 | [§11](02_DEFINITIONS.md#11-area_holes--구멍의-총-면적) | [T-14](03_VERIFICATION.md#t-14-area_holes) | [G1 §2](reference/halcon13/G1_basic_size.md#2-area_holes) | [§10.2](04_IMPLEMENTATION_NOTES.md#102-배경을-런렝스로-표현한-것이-이-배치의-핵심-설계) |
| `connect_and_holes` | 성분 수 · 구멍 수 | [§12](02_DEFINITIONS.md#12-connect_and_holes--연결성분-수와-구멍-수) | [T-15](03_VERIFICATION.md#t-15-connect_and_holes--배경-연결성) | [G1 §5](reference/halcon13/G1_basic_size.md#5-connect_and_holes) | [§10.1](04_IMPLEMENTATION_NOTES.md#101-이-배치의-위험은-정규화가-아니라-연결성이었다) |
| `euler_number` | 성분 수 − 구멍 수 | [§13](02_DEFINITIONS.md#13-euler_number--오일러-수) | [T-16](03_VERIFICATION.md#t-16-euler_number) | [G1 §6](reference/halcon13/G1_basic_size.md#6-euler_number) | [§10.3](04_IMPLEMENTATION_NOTES.md#103-euler_number-는-값이-아니라-정합성을-검증하는-오퍼레이터다) |
| `diameter_region` | 최대 지름과 두 극점 | [§14](02_DEFINITIONS.md#14-diameter_region--최대-지름) | [T-17](03_VERIFICATION.md#t-17-diameter_region) | [G1 §4](reference/halcon13/G1_basic_size.md#4-diameter_region) | [§10.4](04_IMPLEMENTATION_NOTES.md#104-diameter_region--새-인프라는-회전-캘리퍼스-하나뿐) |

### 3.1 판정 · 리스크 바로가기

| ID | 내용 | 위치 |
|---|---|---|
| **P-1** | `contlength` 체인 모델 (픽셀 중심 체인 확정) | [정의서 C-1](02_DEFINITIONS.md#25-쟁점-해소) · [검증 §5](03_VERIFICATION.md#5-결정적-판정-요약-이-3개가-5종-구현의-전부를-좌우한다) |
| **P-2** | `convexity` 의 `F_c` 단위 (hull 픽셀 개수 확정) | [정의서 CV-1](02_DEFINITIONS.md#55-쟁점-해소) · [검증 §5](03_VERIFICATION.md#5-결정적-판정-요약-이-3개가-5종-구현의-전부를-좌우한다) |
| **P-3** | `contlength` 구멍 제외 (외곽만 확정) | [정의서 §2.2](02_DEFINITIONS.md#22-원문-정의) · [원문 G1 §3](reference/halcon13/G1_basic_size.md#3-contlength) |
| **R-1** | 체인 닫힘 1스텝 (396 vs 395) | [구현 노트 §6](04_IMPLEMENTATION_NOTES.md#6-남은-리스크) |
| **R-2** | 4/8 이웃 설정 (8-이웃 고정) | [구현 노트 §6](04_IMPLEMENTATION_NOTES.md#6-남은-리스크) |
| **HC-1** ⭐ | **배경 연결성 4 vs 8** — 뒤집히면 3종이 동시에 바뀐다 | [정의서 §11.5](02_DEFINITIONS.md#115-미결-쟁점) · [검증 §5C](03_VERIFICATION.md#5c-3차-배치-판정-요약) · [구현 노트 §10.1](04_IMPLEMENTATION_NOTES.md#101-이-배치의-위험은-정규화가-아니라-연결성이었다) |
| **HC-3** | 구멍 면적을 리전 전체로 세는가 (S12: 40 vs 49) | [정의서 §11.5](02_DEFINITIONS.md#115-미결-쟁점) · [검증 §5C](03_VERIFICATION.md#5c-3차-배치-판정-요약) |
| **DR-2** | 최대거리 쌍 동률 (사전순 최소로 고정) | [정의서 §14.6](02_DEFINITIONS.md#146-미결-쟁점) · [검증 T-17](03_VERIFICATION.md#t-17-diameter_region) |

---

## 4. 원문 아카이브 구성 (G1~G5 · 40종)

전체 목록과 단계별 상태는 [`reference/halcon13/README.md`](reference/halcon13/README.md) 에 있습니다.

| 파일 | 그룹 | 담긴 오퍼레이터 | 종수 | 줄 수 |
|---|---|---|:--:|--:|
| [`G1_basic_size.md`](reference/halcon13/G1_basic_size.md) | 기본 / 크기 | `area_center` `area_holes` `contlength` `diameter_region` `connect_and_holes` `euler_number` `get_region_thickness` `region_features` | 8 | 483 |
| [`G2_shape_factors.md`](reference/halcon13/G2_shape_factors.md) | 형상 계수 | `circularity` `compactness` `convexity` `rectangularity` `roundness` `eccentricity` `elliptic_axis` `orientation_region` | 8 | 392 |
| [`G3_geometry_runlength.md`](reference/halcon13/G3_geometry_runlength.md) | 외접 / 내접 / 런렝스 | `inner_circle` `inner_rectangle1` `smallest_circle` `smallest_rectangle1` `smallest_rectangle2` `runlength_distribution` `runlength_features` | 7 | 303 |
| [`G4_moments.md`](reference/halcon13/G4_moments.md) | 모멘트 | `moments_region_2nd` `moments_region_2nd_invar` `moments_region_2nd_rel_invar` `moments_region_3rd` `moments_region_3rd_invar` `moments_region_central` `moments_region_central_invar` | 7 | 448 |
| [`G5_select_relation.md`](reference/halcon13/G5_select_relation.md) | 선택 / 관계 | `select_shape` `select_shape_proto` `select_shape_std` `select_region_spatial` `spatial_relation` `hamming_distance` `hamming_distance_norm` `find_neighbors` `get_region_index` `select_region_point` | 10 | 468 |

> ⚠ 아카이브의 수식은 MVTec 문서의 **MetaPost 인라인 SVG 글리프를 디코딩해 복원**한 것입니다.
> 다시 웹에서 긁으면 수식이 누락되거나 분자·분모가 뒤집힙니다. **이 아카이브를 신뢰하고 쓰세요**
> ([`CLAUDE.md`](../CLAUDE.md) §2.4).

---

[🏠 홈](../README.md)  |  다음: [개요 →](00_OVERVIEW.md)
