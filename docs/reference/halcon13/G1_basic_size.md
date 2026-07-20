# Halcon 13 공식 문서 수집 — G1 기본 형상 특징 8종

수집일: 2026-07-20
출처: https://www.mvtec.com/doc/halcon/13/en/&lt;operator&gt;.html (MVTec HALCON 13 Reference Manual, Operator Reference)
원칙: 문서에 명시되지 않은 내용은 "문서에 명시 없음"으로 표기. 추측 없음.

---

## 공통 사항 (8개 오퍼레이터 모두 해당)

- **좌표계 관례**: HALCON은 항상 (Row, Column) = (행, 열) 순서. 즉 (y, x) 순서이며 OpenCV의 (x, y)와 반대.
  - `Row` 파라미터 타입은 `point.y`, `Column` 파라미터 타입은 `point.x`로 문서에 명시됨 (area_center 기준).
  - 이미지 좌상단이 (0, 0).
- **모듈**: 8개 전부 `Foundation` 모듈.
- **반환값**: 입력이 비어있지 않으면 `2 (H_MSG_TRUE)` 반환.
- **빈 입력(no object) 처리**: `set_system('no_object_result', <Result>)` 로 동작 제어. 필요 시 예외(exception) 발생.
- **빈 영역(empty region) 처리**: `set_system('empty_region_result', <Result>)` 로 동작 제어.
- **시그니처 표기법**: `operator(입력객체 : 출력객체 : 입력제어 : 출력제어)`
  - 예) `region_features(Regions : : Features : Value)` → 입력객체 Regions, 입력제어 Features, 출력제어 Value

---

## 1. area_center

### 시그니처
```
area_center(Regions : : : Area, Row, Column)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region(s) to be examined" |
| Area | output_control | integer(-array) | "Area of the region" |
| Row | output_control | point.y(-array) / real | "Line index of the center" |
| Column | output_control | point.x(-array) / real | "Column index of the center" |

### Description (원문)
> "The operator area_center calculates the area and the center of the input regions. The area is defined as the number of pixels of a region. The center is calculated as the mean value of the line or column coordinates, respectively, of all pixels."

### 계산 공식 (문서 서술을 수식화)
- Area = 영역을 구성하는 픽셀의 개수 (number of pixels)
- Row(중심) = 모든 픽셀의 행 좌표의 평균값
- Column(중심) = 모든 픽셀의 열 좌표의 평균값

즉 F = Area 라 할 때
```
Area = F
Row  = (1/F) * Σ r_i
Col  = (1/F) * Σ c_i
```
(문서 본문에는 위와 같은 LaTeX 수식이 별도로 제시되지 않고 "mean value of the line or column coordinates"라는 서술로만 기술됨.)

### 값의 범위 / 기본값 / 예외
- Area: 정수, 픽셀 개수
- Row, Column: 실수(real). 정수가 아닌 실수값으로 반환됨 (무게중심이므로)
- 기본값: 입력 제어 파라미터가 없으므로 기본값 없음
- 빈 영역: 모든 출력값이 0.0 (`set_system('empty_region_result', <Result>)`로 변경 가능)
- 빈 입력: `set_system('no_object_result', <Result>)`
- **Attention 섹션 없음** (문서에 Attention 항목 자체가 존재하지 않음)

### Complexity (원문)
> "If F is the area of a region the mean runtime complexity is O(sqrt(F))"

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- See also: `select_shape`

### 실행 정보
- Multithreading type: reentrant
- Multithreading scope: global
- Automatically parallelized on tuple level

---

## 2. area_holes

### 시그니처
```
area_holes(Regions : : : Area)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region(s) to be examined" |
| Area | output_control | integer(-array) | "Area(s) of holes of the region(s)" |

### Description
- 입력 영역 내부에 존재하는 **구멍(hole)들의 총 면적**을 픽셀 개수 단위로 계산.
- 영역에 구멍이 여러 개면 **모든 구멍 면적의 합**이 반환됨.
- 이웃 관계(연결성) 기본값은 **8-connectivity**이며 `set_system('neighborhood', <4/8>)` 로 4/8 변경 가능.

### 값의 범위 / 예외
- Area: 정수(픽셀 수), 구멍이 없으면 0
- 빈 영역: Area = 0 반환
- 빈 입력: `set_system('no_object_result', <Result>)`

### 좌표계
- 좌표를 반환하지 않으므로 해당 없음.

### Complexity
- 문서에 명시 없음.

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- See also: `area_center`, `fill_up`

### 실행 정보
- Multithreading type: reentrant
- Multithreading scope: global
- Processed without parallelization (병렬화되지 않음)

---

## 3. contlength

### 시그니처
```
contlength(Regions : : : ContLength)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region(s) to be examined" |
| ContLength | output_control | real(-array) | 윤곽선 길이. Assertion: `ContLength >= 0` |

### Description
- 영역 윤곽(contour)의 **전체 길이**를 계산. 영역이 여러 연결성분(connected components)을 가지면 **모든 성분의 합**.
- 거리 가중치:
  - **축 평행(axis-parallel, 상하좌우) 인접 점: 1로 계산**
  - **대각(diagonal) 인접 점: √2 (약 1.414)로 계산**

```
ContLength = Σ (축평행 스텝 수 × 1) + (대각 스텝 수 × √2)
```

### Attention (원문)
> "The contour of holes is not calculated."
→ **구멍의 윤곽은 길이에 포함되지 않는다.** (외곽 윤곽만 계산)

### 값의 범위 / 예외
- ContLength >= 0 (실수)
- 빈 영역: 0 반환
- 빈 입력: `set_system('no_object_result', <Result>)`

### 좌표계
- 좌표를 반환하지 않으므로 해당 없음. (윤곽 좌표가 필요하면 `get_region_contour` 사용, Row/Col 순서)

### Complexity
- 문서에 명시 없음.

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- Possible Successors: `get_region_contour`
- Alternatives: `compactness`
- See also: `area_center`, `get_region_contour`

### 실행 정보
- Multithreading type: reentrant
- Multithreading scope: global
- Automatically parallelized on tuple level

---

## 4. diameter_region

### 시그니처
```
diameter_region(Regions : : : Row1, Column1, Row2, Column2, Diameter)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined" |
| Row1 | output_control | integer(-array) | 첫 번째 극점의 행 좌표 |
| Column1 | output_control | integer(-array) | 첫 번째 극점의 열 좌표 |
| Row2 | output_control | integer(-array) | 두 번째 극점의 행 좌표 |
| Column2 | output_control | integer(-array) | 두 번째 극점의 열 좌표 |
| Diameter | output_control | real(-array) | "Distance of the two extreme points" |

### Description (원문)
> "calculates the maximal distance between two boundary points of a region. The coordinates of these two extremes and the distance between them will be returned."

즉 영역의 **경계점(boundary point)** 쌍 중 유클리드 거리가 최대가 되는 쌍을 찾아 그 두 점의 좌표와 거리를 반환.

```
Diameter = max over (p, q) ∈ boundary(Region) of  sqrt((r_p - r_q)^2 + (c_p - c_q)^2)
(Row1, Column1) = p*,  (Row2, Column2) = q*   (최대값을 만드는 쌍)
```
(문서에는 "maximal distance between two boundary points"라는 서술로만 기술되며 명시적 수식은 없음.)

### 값의 범위 / 예외
- Row1/Column1/Row2/Column2: 정수 좌표
- Diameter: 실수
- **빈 영역 주의 (원문)**: "If the region is empty, the results of Row1, Column1, Row2 and Column2 (all of them = 0) may lead to confusion."
  → 빈 영역이면 네 좌표가 모두 0이 되어 혼동을 줄 수 있음. `set_system('empty_region_result', <Result>)` 로 제어.

### 좌표계
- (Row, Column) 순서. 극점 2개를 각각 (Row1, Column1), (Row2, Column2)로 반환.

### Complexity (원문)
> "If F is the area of a region, the runtime complexity amounts to O(sqrt(F)) on average."

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`, `runlength_features`
- Possible Successors: `disp_line`
- Alternatives: `smallest_rectangle2`

### 실행 정보
- Module: Foundation. (그 외 세부 병렬화 항목은 페이지 추출본에 명시되지 않음)

---

## 5. connect_and_holes

### 시그니처
```
connect_and_holes(Regions : : : NumConnected, NumHoles)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region(s) to be examined" |
| NumConnected | output_control | integer(-array) | "Number of connection components of a region" |
| NumHoles | output_control | integer(-array) | "Number of holes of a region" |

### Description (원문)
> "calculates the number of connection components and the number of holes of each region"

여러 영역 입력 시 출력은 입력 인덱스에 대응하는 튜플로 반환.

### 연결성(connectivity) 처리
- 이 페이지 추출본에는 `set_system('neighborhood', ...)` 관련 기술이 **명시 없음**.
  (참고: 동일 계열의 `area_holes` 문서에는 기본 8-connectivity, `set_system('neighborhood',<4/8>)` 명시가 있음.)

### 값의 범위 / 예외
- NumConnected, NumHoles: 정수
- 빈 입력: `set_system('no_object_result', <Result>)`
- 빈 영역: `set_system('empty_region_result', <Result>)`

### 좌표계
- 좌표 반환 없음. 해당 없음.

### Complexity
- 문서에 명시 없음.

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- Alternatives: `euler_number`
- See also: `connection`, `fill_up`, `fill_up_shape`, `union1`

### 실행 정보
- Multithreading type: reentrant
- Automatically parallelized on tuple level

---

## 6. euler_number

### 시그니처
```
euler_number(Regions : : : EulerNumber)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region(s) to be examined" |
| EulerNumber | output_control | integer(-array) | "Calculated Euler number" |

### Description / 공식 (원문)
> Euler number is "the difference between the number of connection components and the number of holes."

```
EulerNumber = NumConnected - NumHoles
```
(= `connect_and_holes`의 두 출력의 차)

문서는 Regions/Features 챕터의 예시 그림(오일러 수가 다른 영역들)을 참조하도록 안내함.

### 연결성 처리
- 4-connectivity / 8-connectivity 중 어느 모델을 쓰는지는 **문서에 명시 없음**.

### 값의 범위 / 예외
- EulerNumber: 정수 (구멍이 연결성분보다 많으면 음수 가능)
- 빈 입력: `set_system('no_object_result', <Result>)`
- 빈 영역: `set_system('empty_region_result', <Result>)`

### 좌표계
- 좌표 반환 없음. 해당 없음.

### Complexity
- 문서에 명시 없음.

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- Alternatives: `connect_and_holes`

### 실행 정보
- Multithreading type: reentrant
- Multithreading scope: global
- Automatically parallelized on tuple level

---

## 7. get_region_thickness

### 시그니처
```
get_region_thickness(Region : : : Thickness, Histogramm)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Region | input_object | region | "Region to be analysed" (단일 영역) |
| Thickness | output_control | integer-array | 주축(main axis)을 따라 측정한 두께 값 배열 |
| Histogramm | output_control | integer-array | 두께 분포 히스토그램 |

### Description (원문 기반)
> operator calculates "the thickness of the regions along the main axis" by measuring "the distance between the intersections of the contour with the plumb on the main axis" at each point, selecting the pair furthest apart.
> "the length of the histogram corresponds to the largest occurring thickness."

정리:
- 주축(main axis, 등가 타원의 장축 방향) 위의 각 점에서, 그 점을 지나는 **주축에 수직인 직선(plumb)** 과 윤곽선의 교점들을 구한다.
- 교점 중 **가장 멀리 떨어진 쌍의 거리**를 그 위치의 두께로 삼는다.
- Histogramm의 길이 = 발생한 최대 두께 값.

```
Thickness[i] = max distance between intersections of (perpendicular line at position i on main axis) and contour
len(Histogramm) = max(Thickness)
```

### 중요 제약 (원문)
> "Only one region may be passed. If the region has several connection components, only the first one is investigated."
→ **영역은 하나만 전달 가능**. 연결성분이 여러 개면 **첫 번째 것만** 조사됨.

### 값의 범위 / 예외
- Thickness, Histogramm: 정수 배열
- 정확히 1개 영역 입력 시 2 (H_MSG_TRUE) 반환
- 빈 입력: `set_system('no_object_result', <Result>)`
- 빈 영역 전용 서술: 문서에 명시 없음

### 좌표계
- 좌표 반환 없음(길이/개수만 반환). 주축 방향은 `elliptic_axis`의 phi 개념을 따름.

### Complexity
- 문서에 명시 없음.

### 관련 오퍼레이터
- Possible Predecessors: `sobel_amp`, `threshold`, `connection`, `select_shape`, `select_obj`
- See also: `elliptic_axis`, `copy_obj`

### 실행 정보
- Multithreading type: reentrant
- Processed without parallelization (병렬화되지 않음)

---

## 8. region_features

### 시그니처
```
region_features(Regions : : Features : Value)
```

| 파라미터 | 방향 | 타입 | 의미 (원문) |
|---|---|---|---|
| Regions | input_object | region-array | "Regions to be examined" |
| Features | input_control | string(-array) | "Shape features to be calculated" — **Default value: 'area'** |
| Value | output_control | real(-array) | "The calculated features" |

### Description (원문)
> "calculates for each input region from Regions the indicated features (Features)."

여러 feature를 지정하면 지정한 순서대로 순차 계산되어 Value 튜플에 담김.
(영역 N개 × feature M개 → Value는 지정 순서에 따른 튜플)

### 지원 feature 전체 목록 — 63종 (문서 'List of values' 알파벳순 원문 그대로)

| # | Feature | 의미 |
|---|---|---|
| 1 | `anisometry` | Anisometry (이방성) |
| 2 | `area` | Area of the object (면적) |
| 3 | `area_holes` | Area of the holes of the object (구멍 면적) |
| 4 | `bulkiness` | Bulkiness |
| 5 | `circularity` | Circularity (원형도) |
| 6 | `column` | Column index of the center (중심 열 좌표) |
| 7 | `column1` | Column index of upper left corner |
| 8 | `column2` | Column index of lower right corner |
| 9 | `compactness` | Compactness (조밀도) |
| 10 | `connect_num` | Number of connection components |
| 11 | `contlength` | Total length of contour |
| 12 | `convexity` | Convexity (볼록도) |
| 13 | `dist_deviation` | Deviation of the distance from the region border to the center |
| 14 | `dist_mean` | Mean distance from the region border to the center |
| 15 | `euler_number` | Euler number |
| 16 | `height` | Height of the region (축 평행) |
| 17 | `holes_num` | Number of holes |
| 18 | `inner_height` | Height of the largest axis-parallel rectangle that fits into the region |
| 19 | `inner_radius` | Radius of largest inner circle |
| 20 | `inner_width` | Width of the largest axis-parallel rectangle that fits into the region |
| 21 | `max_diameter` | Maximum diameter of the region |
| 22 | `moments_i1` | Central geometric moments |
| 23 | `moments_i2` | Central geometric moments |
| 24 | `moments_i3` | Central geometric moments |
| 25 | `moments_i4` | Central geometric moments |
| 26 | `moments_ia` | Geometric moments (2nd order) |
| 27 | `moments_ib` | Geometric moments (2nd order) |
| 28 | `moments_m02` | Geometric moments (2nd order) |
| 29 | `moments_m02_invar` | Invariant geometric moments (2nd order) |
| 30 | `moments_m03` | Geometric moments (3rd order) |
| 31 | `moments_m03_invar` | Invariant geometric moments (3rd order) |
| 32 | `moments_m11` | Geometric moments (2nd order) |
| 33 | `moments_m11_invar` | Invariant geometric moments (2nd order) |
| 34 | `moments_m12` | Geometric moments (3rd order) |
| 35 | `moments_m12_invar` | Invariant geometric moments (3rd order) |
| 36 | `moments_m20` | Geometric moments (2nd order) |
| 37 | `moments_m20_invar` | Invariant geometric moments (2nd order) |
| 38 | `moments_m21` | Geometric moments (3rd order) |
| 39 | `moments_m21_invar` | Invariant geometric moments (3rd order) |
| 40 | `moments_m30` | Geometric moments (3rd order) |
| 41 | `moments_m30_invar` | Invariant geometric moments (3rd order) |
| 42 | `moments_phi1` | Relative invariant geometric moments (2nd order) |
| 43 | `moments_phi2` | Relative invariant geometric moments (2nd order) |
| 44 | `moments_psi1` | Invariant central geometric moments |
| 45 | `moments_psi2` | Invariant central geometric moments |
| 46 | `moments_psi3` | Invariant central geometric moments |
| 47 | `moments_psi4` | Invariant central geometric moments |
| 48 | `num_sides` | Number of polygon sides |
| 49 | `orientation` | Orientation of the region |
| 50 | `outer_radius` | Radius of smallest surrounding circle |
| 51 | `phi` | Orientation of the equivalent ellipse |
| 52 | `ra` | Main radius of the equivalent ellipse |
| 53 | `rb` | Secondary radius of the equivalent ellipse |
| 54 | `rect2_len1` | Half the length of the smallest surrounding rectangle |
| 55 | `rect2_len2` | Half the width of the smallest surrounding rectangle |
| 56 | `rect2_phi` | Orientation of the smallest surrounding rectangle |
| 57 | `rectangularity` | Rectangularity (직사각형도) |
| 58 | `roundness` | Roundness |
| 59 | `row` | Row index of the center (중심 행 좌표) |
| 60 | `row1` | Row index of upper left corner |
| 61 | `row2` | Row index of lower right corner |
| 62 | `struct_factor` | Structure Factor |
| 63 | `width` | Width of the region (축 평행) |

> 참고: 위 63개가 'List of values'에 나열된 전체 목록이며, 표의 의미 컬럼은 Description 표의 원문 설명을 대응시킨 것이다.

### 값의 범위 / 기본값 / 예외
- Features 기본값: `'area'`
- Value: 실수(real) 튜플. 개별 feature별 범위는 해당 개별 오퍼레이터 문서 참조 (region_features 페이지에는 개별 범위 명시 없음)
- 빈 입력: 반환값 2 (H_MSG_TRUE) if input is not empty; 필요 시 예외 발생
- 빈 영역 전용 서술: 이 페이지에는 명시 없음 (개별 오퍼레이터의 `set_system('empty_region_result',...)` 규칙을 따름)

### 좌표계
- `row`/`row1`/`row2` = 행(y), `column`/`column1`/`column2` = 열(x). (Row, Column) 순서.
- `row1, column1` = 좌상단(upper left), `row2, column2` = 우하단(lower right) — 즉 bounding box.

### 관련 오퍼레이터
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`, `runlength_features`
- Possible Successors: `select_shape`, `select_gray`, `shape_trans`, `reduce_domain`, `count_obj`
- See also: `area_center`, `circularity`, `compactness`, `contlength`, `convexity`, `rectangularity`, `elliptic_axis`, `eccentricity`, `inner_circle`, `smallest_circle`, `smallest_rectangle1`, `smallest_rectangle2`, `inner_rectangle1`, `roundness`, `connect_and_holes`, `area_holes`, `diameter_region`, `orientation_region`, `moments_region_2nd`, `moments_region_2nd_invar`, `moments_region_2nd_rel_invar`, `moments_region_3rd`, `moments_region_3rd_invar`, `moments_region_central`, `moments_region_central_invar`, `select_obj`, `select_shape`

### 실행 정보
- Multithreading type: reentrant
- Multithreading scope: global
- Automatically parallelized on tuple level

---

## 부록: 문서에 명시 없음 항목 정리

| 항목 | 상태 |
|---|---|
| area_center — Attention 섹션 | 문서에 해당 섹션 없음 |
| area_center — 명시적 LaTeX 수식 | 문서에 명시 없음 (서술로만 기술) |
| area_holes — Complexity | 문서에 명시 없음 |
| contlength — Complexity | 문서에 명시 없음 |
| diameter_region — 병렬화 상세 | 추출본에 명시 없음 |
| connect_and_holes — connectivity(4/8) 설정 | 문서에 명시 없음 |
| connect_and_holes — Complexity | 문서에 명시 없음 |
| euler_number — connectivity(4/8) 모델 | 문서에 명시 없음 |
| euler_number — Complexity | 문서에 명시 없음 |
| get_region_thickness — Complexity | 문서에 명시 없음 |
| get_region_thickness — 빈 영역 전용 동작 | 문서에 명시 없음 |
| region_features — feature별 값 범위 | 문서에 명시 없음 (개별 오퍼레이터 참조) |
