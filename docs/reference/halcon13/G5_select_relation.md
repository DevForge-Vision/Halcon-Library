# Halcon 13 오퍼레이터 정의 — G5 (Select / Relation 그룹)

수집 출처: https://www.mvtec.com/doc/halcon/13/en/&lt;operator&gt;.html (MVTec 공식 Halcon 13 Reference)
수집일: 2026-07-20

> 주의: Halcon 공식 문서의 수식(Similarity 등)은 HTML 본문이 아니라 **이미지(gif)** 로 삽입되어 있어
> 텍스트 추출로는 얻을 수 없다. 해당 항목은 "문서에 명시 없음(수식 이미지)"으로 표기했다.
> 추측으로 채우지 않았다.

---

## 1. select_shape

### 시그니처
```
select_shape(Regions : SelectedRegions : Features, Operation, Min, Max : )
```
C:
```c
Herror select_shape(const Hobject Regions, Hobject* SelectedRegions,
                    const char* Features, const char* Operation,
                    double Min, double Max)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) | 검사할 입력 영역 |
| SelectedRegions | output_object | region(-array) | 조건을 만족한 영역 |
| Features | input_control | string(-array) | 검사할 형상 특징값 (튜플 가능) |
| Operation | input_control | string | 개별 특징 조건들의 결합 방식 |
| Min | input_control | number(-array) | 하한 |
| Max | input_control | number(-array) | 상한 |

### 판정 규칙 (Description)
- 입력 영역마다 지정된 형상 특징값을 계산한다.
- `Operation = 'and'` : 계산된 **모든** 특징값이 각각의 [Min, Max] 범위 안에 있어야 선택된다.
- `Operation = 'or'` : **하나 이상**의 특징값이 범위 조건을 만족하면 선택된다.
- 조건을 만족한 영역이 출력으로 복사된다.
- Min/Max 에는 숫자 대신 문자열 `'min'`, `'max'` 를 줄 수 있으며, 이 경우 해당 방향의 한계를 열어둔다(무한대 취급).
- Features 가 튜플이면 Min/Max 도 같은 길이의 튜플로 준다.

### Features 전체 목록 (총 63개)
```
'anisometry', 'area', 'area_holes', 'bulkiness', 'circularity',
'column', 'column1', 'column2', 'compactness', 'connect_num',
'contlength', 'convexity', 'dist_deviation', 'dist_mean', 'euler_number',
'height', 'holes_num', 'inner_height', 'inner_radius', 'inner_width',
'max_diameter',
'moments_i1', 'moments_i2', 'moments_i3', 'moments_i4',
'moments_ia', 'moments_ib',
'moments_m02', 'moments_m02_invar',
'moments_m03', 'moments_m03_invar',
'moments_m11', 'moments_m11_invar',
'moments_m12', 'moments_m12_invar',
'moments_m20', 'moments_m20_invar',
'moments_m21', 'moments_m21_invar',
'moments_m30', 'moments_m30_invar',
'moments_phi1', 'moments_phi2',
'moments_psi1', 'moments_psi2', 'moments_psi3', 'moments_psi4',
'num_sides', 'orientation', 'outer_radius', 'phi', 'ra', 'rb',
'rect2_len1', 'rect2_len2', 'rect2_phi', 'rectangularity', 'roundness',
'row', 'row1', 'row2', 'struct_factor', 'width'
```

번호 매김:
1 anisometry / 2 area / 3 area_holes / 4 bulkiness / 5 circularity /
6 column / 7 column1 / 8 column2 / 9 compactness / 10 connect_num /
11 contlength / 12 convexity / 13 dist_deviation / 14 dist_mean / 15 euler_number /
16 height / 17 holes_num / 18 inner_height / 19 inner_radius / 20 inner_width /
21 max_diameter / 22 moments_i1 / 23 moments_i2 / 24 moments_i3 / 25 moments_i4 /
26 moments_ia / 27 moments_ib / 28 moments_m02 / 29 moments_m02_invar / 30 moments_m03 /
31 moments_m03_invar / 32 moments_m11 / 33 moments_m11_invar / 34 moments_m12 / 35 moments_m12_invar /
36 moments_m20 / 37 moments_m20_invar / 38 moments_m21 / 39 moments_m21_invar / 40 moments_m30 /
41 moments_m30_invar / 42 moments_phi1 / 43 moments_phi2 / 44 moments_psi1 / 45 moments_psi2 /
46 moments_psi3 / 47 moments_psi4 / 48 num_sides / 49 orientation / 50 outer_radius /
51 phi / 52 ra / 53 rb / 54 rect2_len1 / 55 rect2_len2 /
56 rect2_phi / 57 rectangularity / 58 roundness / 59 row / 60 row1 /
61 row2 / 62 struct_factor / 63 width

### Operation 값
- `'and'` (기본값)
- `'or'`

### 기본값 / 범위
- Features 기본값: `'area'`
- Operation 기본값: `'and'`
- Min 기본값: `150.0`, Max 기본값: `99999.0`
- Min/Max 값 범위: `0.0 ≤ 값 ≤ 99999.0`, 최소 증분 0.001
- 제약: `Max >= Min`

### 예외 조건
- 입력이 비어있지 않으면 2 (H_MSG_TRUE) 반환.
- 빈 입력: `set_system('no_object_result', <Result>)` 로 동작 결정.
- 빈 영역: `set_system('empty_region_result', <Result>)` 로 동작 결정.
- 필요 시 예외(exception) 발생.

---

## 2. select_shape_proto

### 시그니처
```
select_shape_proto(Regions, Pattern : SelectedRegions : Feature, Min, Max : )
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) | 검사할 영역 |
| Pattern | input_object | region(-array) | 비교 기준(프로토타입) 영역 |
| SelectedRegions | output_object | region(-array) | 조건을 만족한 영역 |
| Feature | input_control | string(-array) | 비교 특징값 |
| Min | input_control | number | 하한 (기본값 50.0) |
| Max | input_control | number | 상한 (기본값 100.0) |

### 판정 규칙
- Regions 의 각 영역을 Pattern(기준 영역)과 비교하여 지정한 Feature 값을 구하고,
  그 값이 [Min, Max] 범위 안이면 선택한다.

### Feature 전체 목록 (총 7개)
| 값 | 의미 |
|---|---|
| `'distance_dilate'` | Pattern 과 Region 경계 사이의 **최대 노름(maximum norm, 체스보드)** 거리 |
| `'distance_contour'` | Pattern 과 Region 경계 사이의 **유클리드** 거리 |
| `'distance_center'` | 두 영역 **중심(무게중심)** 간 유클리드 거리 |
| `'covers'` | Pattern 을 최적 위치에 놓았을 때 겹치는 비율(%) |
| `'fits'` | Pattern 을 평행이동하여 Region 안에 들어갈 수 있는지 여부 (Min/Max 무시) |
| `'overlaps_abs'` | 교집합 면적(절대 픽셀 수) |
| `'overlaps_rel'` | 교집합 면적을 Region 면적 대비 비율(%)로 표현 |

### Operation
- **문서에 명시 없음** — select_shape_proto 에는 Operation 파라미터가 존재하지 않는다
  (Feature 1개 기준 판정, Feature 튜플 시 Min/Max 튜플 대응).

### 예외 조건
- 입력이 비어있지 않으면 2 (H_MSG_TRUE) 반환.
- 빈 입력/빈 영역 동작은 `set_system('no_object_result', ...)`, `set_system('empty_region_result', ...)` 로 설정.

---

## 3. select_shape_std

### 시그니처
```
select_shape_std(Regions : SelectedRegions : Shape, Percent : )
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) | 선별할 영역 |
| SelectedRegions | output_object | region(-array) | 원하는 형상에 부합하는 영역 |
| Shape | input_control | string | 사용할 표준 형상 |
| Percent | input_control | real | 유사도 임계값(%) |

### 판정 규칙 (Description 원문)
> "The operator select_shape_std compares the shape of the given regions with default shapes."

입력 영역의 형상을 미리 정의된 표준 형상(default shape)과 비교하여, 유사도 조건을 만족하는 영역만 반환한다.

### Shape 전체 목록 (총 3개)
| 값 | 의미 |
|---|---|
| `'max_area'` | 면적이 가장 큰 영역을 선택 |
| `'rectangle1'` | 축 정렬(平行) 최소 외접 사각형과 비교 |
| `'rectangle2'` | 회전된(임의 방향) 최소 외접 사각형과 비교 |

### Percent 의미
- 표준 형상 대비 **면적 차이 기반 유사도 임계값(%)**.
- 범위 `0.0 ≤ Percent ≤ 100.0` (linear), 기본값 `70.0`.
- `'max_area'` 사용 시에는 Percent 가 판정에 사용되지 않는다는 별도 명시는 **문서에 명시 없음**.

### 예외 조건
- 문서에 별도의 Exception 항목이 명시되어 있지 않음(Result 절 기준 일반 규칙 적용).

---

## 4. select_region_spatial

### 시그니처
```
select_region_spatial(Regions1, Regions2 : : Direction : RegionIndex1, RegionIndex2)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions1 | input_object | region(-array) | 기준 영역 |
| Regions2 | input_object | region(-array) | 비교 대상 영역 |
| Direction | input_control | string | 공간 관계 방향 |
| RegionIndex1 | output_control | integer-array | 조건을 만족한 Regions1 인덱스 |
| RegionIndex2 | output_control | integer-array | 조건을 만족한 Regions2 인덱스 |

### 판정 규칙
- 각 영역의 **중심(무게중심)** 을 구하고, 두 중심을 잇는 방향이 해당 좌표축 기준
  **±45도** 범위 안에 들어오면 그 방향 관계가 성립한 것으로 본다.
- 성립한 쌍의 인덱스가 RegionIndex1 / RegionIndex2 의 같은 위치에 출력된다.

### Direction 전체 목록 (총 4개)
| 값 | 의미 |
|---|---|
| `'left'` | Regions2 가 Regions1 의 왼쪽 |
| `'right'` | Regions2 가 Regions1 의 오른쪽 |
| `'above'` | Regions2 가 Regions1 의 위쪽 |
| `'below'` | Regions2 가 Regions1 의 아래쪽 |

기본값: `'left'`

### 예외 조건
- Regions2 가 비어있지 않으면 2 (H_MSG_TRUE) 반환.
- 빈 입력: `set_system('no_object_result', <Result>)`.
- 빈 영역: `set_system('empty_region_result', <Result>)`.

---

## 5. spatial_relation

### 시그니처
```
spatial_relation(Regions1, Regions2 : : Percent : RegionIndex1, RegionIndex2, Relation1, Relation2)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions1 | input_object | region(-array) | 기준 영역 |
| Regions2 | input_object | region(-array) | 비교 대상 영역 |
| Percent | input_control | number | 관계 성립 최소 비율(%) |
| RegionIndex1 | output_control | integer-array | 관계를 만족한 Regions1 인덱스 |
| RegionIndex2 | output_control | integer-array | 관계를 만족한 Regions2 인덱스 |
| Relation1 | output_control | string-array | 수평 방향 관계 |
| Relation2 | output_control | string-array | 수직 방향 관계 |

### 판정 규칙 (Description)
> "selects regions located by Percent percent 'left', 'right', 'above' or 'below' other regions."

Regions1 의 형태에 따른 비교 방식:
- **빈 튜플**: Regions2 내 모든 영역을 순열(permutative)로 상호 비교
- **단일 영역**: Regions2 의 모든 영역과 비교
- **개수 동일**: n번째끼리 1:1 비교

### Percent 의미 (원문)
> "the area of the second region has to be located really left/right or above/below the region margins of the first region by at least [Percent] percent."

즉, 두 번째 영역의 면적 중 **최소 Percent %** 가 첫 번째 영역의 **경계(margin) 바깥**의
해당 방향에 실제로 위치해야 그 관계가 성립한다.
- 기본값: `50`
- 범위: `0 ≤ Percent ≤ 100`

### 반환되는 Relation 문자열 전체 목록
**Relation1 (수평)**
- `'left'`
- `'right'`
- `""` (빈 문자열 = 수평 관계 없음)

**Relation2 (수직)**
- `'above'`
- `'below'`
- `""` (빈 문자열 = 수직 관계 없음)

### 예외 조건
- Regions2 가 비어있지 않고 Percent 가 0~100 범위이면 2 (H_MSG_TRUE) 반환.
- 그 외 빈 입력/빈 영역은 set_system 설정에 따름.

---

## 6. hamming_distance

### 시그니처
```
hamming_distance(Regions1, Regions2 : : : Distance, Similarity)
```

| 파라미터 | 방향 | 타입 | 의미 / Assertion |
|---|---|---|---|
| Regions1 | input_object | region(-array) | 검사할 영역 |
| Regions2 | input_object | region(-array) | 비교 영역 |
| Distance | output_control | integer(-array) | "Hamming distance of two regions." Assertion: `Distance >= 0` |
| Similarity | output_control | real(-array) | "Similarity of two regions." Assertion: `0 <= Similarity && Similarity <= 1` |

### 정의 (Description 원문)
> "the hamming distance between two regions, i.e., the number of pixels of the regions which are different
> ... the number of pixels contained in one region but not in the other."

즉 **Distance = 대칭차집합(symmetric difference)의 픽셀 수** = |R1 \ R2| + |R2 \ R1|.

### Similarity 공식
- Similarity 는 위 Distance 로부터 유도된다고만 서술되어 있고,
  **정확한 수식은 문서 본문에 이미지로만 제공되어 텍스트로 명시되지 않음** (정규화 인자 포함).
- 명시된 사실:
  - 값 범위 `0 ≤ Similarity ≤ 1`
  - **두 영역이 모두 비어 있으면 Similarity 는 0 으로 설정된다.**

### 기타
- Regions1 과 Regions2 의 **영역 개수가 같아야** 하며, 같은 인덱스끼리 비교한다.
- 평균 런타임 복잡도: O(√F) (F = 영역 면적)

### 예외 조건
> "returns the value 2 (H_MSG_TRUE) if the number of objects in both parameters is the same and is not 0."
- 빈 입력: `set_system('no_object_result', <Result>)`
- 빈 영역: `set_system('empty_region_result', <Result>)`
- 필요 시 예외 발생.

---

## 7. hamming_distance_norm

### 시그니처
```
hamming_distance_norm(Regions1, Regions2 : : Norm : Distance, Similarity)
```

> 출력 파라미터 이름은 `DistanceNorm` 이 아니라 **`Distance`** 이다(Halcon 13 문서 확인).

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions1 | input_object | region(-array) | 검사할 영역 |
| Regions2 | input_object | region(-array) | 비교 영역 |
| Norm | input_control | string | 정규화 방식 |
| Distance | output_control | integer(-array) | "Hamming distance of two regions." |
| Similarity | output_control | real(-array) | "Similarity of two regions." |

### 정의 (Description 원문)
> "Before calculating the difference the region in Regions1 is normalized onto the regions in Regions2.
> The result is the number of pixels contained in one region but not in the other."
> "The parameter Similarity describes the similarity between the two regions based on the hamming distance Distance"

즉 hamming_distance 와 달리 **먼저 Regions1 을 Regions2 에 정규화(위치 정렬)한 뒤** 대칭차집합 픽셀 수를 센다.

### Norm 값 전체 목록
| 값 | 의미 |
|---|---|
| `'center'` (기본값이자 유일한 값) | 두 영역의 무게중심(center of gravity)을 일치시켜 정렬 |

### Similarity 공식 / 정규화 인자
- **문서에 텍스트로 명시 없음** (수식이 이미지로 제공).
- 명시된 사실: 값 범위 `0 ≤ Similarity ≤ 1`, 두 영역이 모두 비어 있으면 Similarity = 0.

### 기타
- 평균 런타임 복잡도: O(√F)
- 두 입력의 영역 개수가 동일해야 함.

### 예외 조건 (원문)
> "hamming_distance_norm returns the value 2 (H_MSG_TRUE) if the number of objects in both parameters is
> the same and is not 0. The behavior in case of empty input (no input objects available) is set via the
> operator set_system('no_object_result',&lt;Result&gt;). The behavior in case of empty region (the region is
> the empty set) is set via set_system('empty_region_result',&lt;Result&gt;). If necessary an exception is raised."

---

## 8. find_neighbors

### 시그니처
```
find_neighbors(Regions1, Regions2 : : MaxDistance : RegionIndex1, RegionIndex2)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions1 | input_object | region(-array) | 검사할 영역 |
| Regions2 | input_object | region(-array) | 비교 영역 |
| MaxDistance | input_control | integer | "Maximal distance of regions." |
| RegionIndex1 | output_control | integer-array | 이웃 쌍의 Regions1 인덱스 |
| RegionIndex2 | output_control | integer-array | 이웃 쌍의 Regions2 인덱스 |

### 판정 규칙 (Description)
> "The operator find_neighbors determines neighboring regions with Regions1 and Regions2 containing the
> regions to be examined."

Regions1 의 세 가지 경우:
- **비어 있음**: Regions2 내 모든 영역을 순열로 상호 검사
- **단일 영역**: Regions2 의 모든 영역과 비교
- **개수 동일**: n번째끼리 비교

거리 척도: **체스보드 거리(chessboard distance)** 를 사용한다.

### MaxDistance 의미
- 문서 서술: "Maximal distance of regions." — 두 영역 사이의 체스보드 거리 최댓값.
- **경계(boundary) 거리인지 중심(center) 거리인지 문서에 명시 없음.**
  (문서에는 "chessboard distance between neighboring regions" 라고만 되어 있음)
- **MaxDistance = 0 일 때의 동작은 문서에 명시 없음.**
  값 범위 자체가 `1 ≤ MaxDistance ≤ 255` 로 0 은 허용 범위 밖이다.
- 기본값: `1`
- 제안값: 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20, 50
- 최소 증분 / 권장 증분: 1

### 중요 제한
> "Covered regions are not found!" (완전히 덮인/포함된 영역은 이웃으로 검출되지 않는다)

### 예외 조건 (원문)
> "The operator find_neighbors returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in
> case of empty input is set via the operator set_system('no_object_result',&lt;Result&gt;). The behavior in case
> of empty region is set via set_system('empty_region_result',&lt;Result&gt;). If necessary an exception is raised."

---

## 9. get_region_index

### 시그니처
```
get_region_index(Regions : : Row, Column : Index)
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region-array | 검사할 영역들 |
| Row | input_control | integer | 테스트 픽셀의 행 index (기본값 100) |
| Column | input_control | integer | 테스트 픽셀의 열 index (기본값 100) |
| Index | output_control | integer(-array) | 해당 픽셀을 포함하는 영역들의 인덱스 |

### 판정 규칙 (Description 원문)
> "the index of all regions in Regions (range of values: 1 to n) containing the test pixel (Row, Column)."

- 인덱스는 **1부터 n** 까지.
- 영역이 겹치는 경우: "If the regions overlap more than one region might contain the pixel.
  In this case all these regions are returned." → 해당하는 모든 인덱스를 반환.
- 포함하는 영역이 없으면: "If no region contains the indicated pixel the empty tuple (= no region) is returned."
  → 빈 튜플 반환.

### 예외 조건
- 파라미터가 올바르면 2 (H_MSG_TRUE) 반환.
- 빈 입력: `set_system('no_object_result', <Result>)` 설정에 따름. 필요 시 예외 발생.

### 관련
- 대체 오퍼레이터: `select_region_point`
- 선행 오퍼레이터: `threshold`, `regiongrowing`, `connection`

---

## 10. select_region_point

### 시그니처
```
select_region_point(Regions : DestRegions : Row, Column : )
```

| 파라미터 | 방향 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region-array | 검사할 영역들 |
| DestRegions | output_object | region-array | 테스트 픽셀을 포함하는 모든 영역 |
| Row | input_control | integer (Hlong) | 테스트 픽셀의 행 index (기본값 100) |
| Column | input_control | integer (Hlong) | 테스트 픽셀의 열 index (기본값 100) |

### 판정 규칙 (Description 원문)
> "selects all regions from Regions containing the test pixel (Row,Column)."

- 해당 픽셀을 포함하는 모든 영역을 반환.
- 영역이 겹치면 여러 개가 반환될 수 있다.
- 포함하는 영역이 없으면 빈 결과(empty tuple)를 반환.
- 평균 런타임 복잡도: O(ln(√F) * N) (F = 영역 면적, N = 영역 개수)

### 예외 조건
- 파라미터가 올바르면 2 (H_MSG_TRUE) 반환.
- 빈 입력 처리: `set_system('no_object_result', <Result>)`.

### 관련
- 유사: `test_region_point`
- 선행: `threshold`, `regiongrowing`, `connection`

---

## 부록: 문서에 명시가 없어 확정할 수 없는 항목 정리

| 항목 | 상태 |
|---|---|
| hamming_distance 의 Similarity 정확한 수식/정규화 인자 | 문서 본문 수식이 이미지 → 텍스트로 **명시 없음** |
| hamming_distance_norm 의 Similarity 정확한 수식/정규화 인자 | 동일하게 **명시 없음** |
| hamming_distance_norm 의 출력명 `DistanceNorm` | 존재하지 않음. 실제 출력명은 `Distance` |
| find_neighbors MaxDistance 가 경계 거리인지 중심 거리인지 | **명시 없음** (chessboard distance 라고만 기술) |
| find_neighbors MaxDistance = 0 동작 | **명시 없음** (허용 범위는 1~255) |
| select_shape_proto 의 Operation 파라미터 | 해당 오퍼레이터에 존재하지 않음 |
| select_shape_std 의 Exception 상세 | 문서에 별도 명시 없음 |
