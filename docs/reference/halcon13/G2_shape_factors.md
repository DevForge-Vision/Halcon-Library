[🏠 홈](../../../README.md) › [📚 문서](../../README.md) › [📦 원문 아카이브](README.md) › **G2 형상 계수**

---

# HALCON 13 Region Features — G2 형상(Shape) 오퍼레이터 8종 원문 정의

<details>
<summary><b>목차</b></summary>

- [공통 사항](#공통-사항)
- [1. circularity — 원형도](#1-circularity--원형도)
- [2. compactness — 조밀도](#2-compactness--조밀도)
- [3. convexity — 볼록도](#3-convexity--볼록도)
- [4. rectangularity — 사각형도](#4-rectangularity--사각형도)
- [5. roundness — 진원도 (4개 출력)](#5-roundness--진원도-4개-출력)
- [6. eccentricity — 이심률 계열 3개 출력](#6-eccentricity--이심률-계열-3개-출력)
- [7. elliptic_axis — 등가 타원 축](#7-elliptic_axis--등가-타원-축)
- [8. orientation_region — 리전 방향](#8-orientation_region--리전-방향)
- [부록 — 한눈에 보는 공식표](#부록--한눈에-보는-공식표)

</details>

출처: https://www.mvtec.com/doc/halcon/13/en/&lt;operator&gt;.html (2026-07-20 수집)

> 수집 방법 주의: MVTec 문서의 수식은 이미지(inline SVG, MetaPost 글리프)로 렌더링되어 있어 일반 텍스트 추출로는 얻을 수 없다.
> 본 문서의 수식은 각 페이지 HTML의 SVG 글리프 ID(cmr10/cmmi10/cmsy10/cmex10/pcrr8r 폰트 코드)를 디코딩하여 복원한 것이며, 위치(x/y) 기준으로 분자·분모·첨자를 재구성했다.

## 공통 사항

- 모든 오퍼레이터는 `Regions (input_object) region(-array) → object` 하나만 입력받고, 제어 파라미터(입력)는 없다. 시그니처 형태는 `operator(Regions : : : Out1[, Out2...])`.
- 출력은 모두 `real(-array) → (real)` 이며, 다중 리전 입력 시 튜플로 반환되고 **튜플 내 위치가 입력 리전의 위치와 일치**한다.
- 빈 리전(empty region) 처리: `set_system('empty_region_result',<Result>)` 로 변경 가능. 기본값은 각 오퍼레이터 항목 참조.
- 입력 객체 자체가 없을 때(no input regions): `set_system('no_object_result',<Result>)`.
- 모든 오퍼레이터는 입력이 비어있지 않으면 반환값 `2 (H_MSG_TRUE)`.
- 좌표계 관례: HALCON 리전 좌표는 (row, column) = (y, x). 각도(Phi)는 **x축(= column 축) 기준 라디안**이며, 문서에는 "Angle between main radius and x-axis in radians"로만 명시. (row 축이 아래로 향하는 이미지 좌표계에서 각도가 수학적 반시계 방향으로 보이도록 정의된다는 별도 설명은 이 8개 페이지에는 **문서에 명시 없음**.)

---

## 1. circularity — 원형도

### 시그니처
```
circularity(Regions : : : Circularity)
```
| 파라미터 | 종류 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. |
| Circularity | output_control | real(-array) → (real) | Circularity of the input region(s). |

**Assertion**: `0 <= Circularity && Circularity <= 1.0`

### Description (원문)
> The operator circularity calculates the similarity of the input region with a circle.
>
> **Calculation:**
> If F is the area of the region and max is the maximum distance from the center to all contour pixels, the shape factor C is defined as:

수식 (SVG 디코딩 결과):

```
        F
C' = ---------
     (max^2 * pi)

C = min(1, C')
```

> The shape factor C of a circle is 1. If the region is long or has holes, C is smaller than 1.
> The operator circularity especially responds to large bulges, holes and unconnected regions. The value of C is clipped to 1.0, because the pixel area of a region can only be an approximation of a real circle's area. This approximation error is bigger for small regions than for large regions.

### 핵심 정리
- **F** = 리전의 면적(area), **max** = 중심(center)에서 모든 **contour 픽셀**까지의 최대 거리.
- 분모는 `(max)^2 * π` — 즉 최대반경을 반지름으로 하는 외접원의 면적.
- 값 범위 0 ~ 1.0. 원 = 1. 길쭉하거나 구멍이 있으면 < 1.
- 1.0 클리핑됨(min 연산). 작은 리전일수록 근사 오차 큼.

### 예외
- 빈 리전 → **0** 반환 (기본).

---

## 2. compactness — 조밀도

### 시그니처
```
compactness(Regions : : : Compactness)
```
| 파라미터 | 종류 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. |
| Compactness | output_control | real(-array) → (real) | Compactness of the input region(s). |

**Assertion**: `Compactness >= 1.0 || Compactness == 0`

### Description (원문)
> The operator compactness calculates the compactness of the input regions.
>
> **Calculation:**
> If L is the length of the contour (see contlength) and F the area of the region the shape factor C is defined as:

수식 (SVG 디코딩 결과):

```
        L^2
C' = ---------
     (4 * F * pi)

C = max(1, C')
```

> The shape factor C of a circle is 1. If the region is long or has holes C is larger than 1.
> The operator compactness responds to the course of the contour (roughness) and to holes. The value of C is clipped to 1.0, because the pixel area of a region can only be an approximation of a real circle's area. This approximation error is bigger for small regions than for large regions.

### 핵심 정리
- 질문의 `F = L^2 / (4*A*pi)` 형태가 **맞다** (HALCON 표기로는 `C' = L² / (4·F·π)`, L = contour length, F = area).
- **C = max(1, C')** — 아래로 1.0에서 잘린다(circularity의 min과 반대 방향).
- 값 범위: 1.0 이상(또는 빈 리전일 때 0). 원 = 1.
- 참고: 본문의 "clipped to 1.0" 문구는 circularity 페이지와 동일 문장을 재사용한 것으로 보이나, 실제 연산은 `max(1, C')` 이다(원문 그대로 기재).

### 예외
- 빈 리전 → **0** 반환 (기본).

---

## 3. convexity — 볼록도

### 시그니처
```
convexity(Regions : : : Convexity)
```
| 파라미터 | 종류 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. |
| Convexity | output_control | real(-array) → (real) | Convexity of the input region(s). |

**Assertion**: `Convexity <= 1`

### Description (원문)
> The operator convexity calculates the convexity of each input region of Regions.
>
> **Calculation:**
> If F_c is the area of the convex hull and F_o the original area of the region the shape factor C is defined as:

수식 (SVG 디코딩 결과):

```
     F_o      (원본 리전의 면적)
C = -----  = -------------------
     F_c      (convex hull 의 면적)
```

> The shape factor C is 1 if the region is convex (e.g., rectangle, circle etc.). If there are indentations or holes C is smaller than 1.

### 핵심 정리
- **분자 = 원본 면적 F_o, 분모 = convex hull 면적 F_c**. (분자/분모 방향 주의 — 자동 요약 도구가 자주 뒤집는 지점)
- 값 범위: ≤ 1. 볼록 도형(사각형, 원 등) = 1. 오목부/구멍 있으면 < 1.

### 예외
- 빈 리전 → **0** 반환 (기본).

---

## 4. rectangularity — 사각형도

### 시그니처
```
rectangularity(Regions : : : Rectangularity)
```
| 파라미터 | 종류 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. |
| Rectangularity | output_control | real(-array) → (real) | Rectangularity of the input region(s). |

**Assertion**: `0 <= Rectangularity && Rectangularity <= 1.0`

### Description (원문)
> The operator rectangularity calculates the rectangularity of the input regions.
>
> To determine the rectangularity, first a rectangle is computed that has the same first and second order moments as the input region. The computation of the rectangularity measure is finally based on the area of the difference between the computed rectangle and the input region normalized with respect to the area of the rectangle.
>
> For rectangles rectangularity returns the value 1. The more the input region deviates from a perfect rectangle, the less the returned value for Rectangularity will be.

### Attention (원문)
> For input regions which orientation cannot be computed by using second order moments (as it is the case for square regions, for example), the returned Rectangularity is underestimated by up to 10% depending on the orientation of the input region.

### 핵심 정리
- **명시적인 닫힌 수식은 이 페이지에 수식 이미지조차 없음(문서에 명시 없음)**. 서술로만 정의:
  1. 입력 리전과 **1차·2차 모멘트가 동일한 기준 사각형**을 계산.
  2. 그 사각형과 입력 리전의 **차집합 면적(area of the difference)** 을 구함.
  3. **기준 사각형의 면적으로 정규화**.
  - 즉 개념적으로 `Rectangularity = 1 - |Rect Δ Region| / Area(Rect)` 형태이나, 이 "1 -" 표현 자체는 문서에 없음(추정 금지 대상이므로 주의).
- 값 범위: 0 ~ 1.0. 완전 사각형 = 1.
- 정사각형처럼 2차 모멘트로 방향을 정할 수 없는 리전은 **최대 10% 과소평가**.

### 예외
- 빈 리전 → **0** 반환 (기본).

---

## 5. roundness — 진원도 (4개 출력)

### 시그니처
```
roundness(Regions : : : Distance, Sigma, Roundness, Sides)
```
| 파라미터 | 종류 | 타입 | 의미 | Assertion |
|---|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. | - |
| Distance | output_control | real(-array) → (real) | Mean distance from the center. | `Distance >= 0.0` |
| Sigma | output_control | real(-array) → (real) | Standard deviation of Distance. | `Sigma >= 0.0` |
| Roundness | output_control | real(-array) → (real) | Shape factor for roundness. | `Roundness <= 1.0` |
| Sides | output_control | real(-array) → (real) | Number of polygon sides. | `Sides >= 0` |

### Description (원문)
> The operator roundness examines the distance between the contour and the center of the area. In particular the mean distance (Distance), the deviation from the mean distance (Sigma) and two shape features derived therefrom are determined.
> Roundness is the relation between mean value and standard deviation, and Sides indicates the number of polygon pieces if a regular polygon is concerned.
> The contour for calculating the features is determined depending on the global neighborhood (see set_system).
>
> **Calculation:**
> If p is the center of the area, p_i the pixels and F the area of the contour.

수식 4개 (SVG 디코딩 결과):

```
                1
Distance   =  ---  *  SUM || p - p_i ||
                F

              2       1
Sigma^2    =  ---  *  SUM ( || p - p_i || - Distance )^2
                F

                       Sigma
Roundness  =  1  -  -----------
                      Distance

                                        0.4724
                          Distance
Sides      =  1.4111 * [ ---------- ]
                           Sigma
```

- `Distance = (1/F) · Σ ‖p − p_i‖`
- `Sigma² = (1/F) · Σ ( ‖p − p_i‖ − Distance )²`  (즉 Sigma = 그 제곱근, contour 거리의 표준편차)
- `Roundness = 1 − Sigma / Distance`
- `Sides = 1.4111 · (Distance / Sigma)^0.4724`

### 핵심 정리
- **p** = 면적의 중심(center of the area), **p_i** = (contour) 픽셀들, **F** = "the area of the contour" (원문 표현 그대로 — contour 픽셀 개수를 의미).
- Roundness 범위: ≤ 1.0. 완전한 원이면 Sigma → 0 이므로 Roundness → 1.
- Sides: 정다각형일 때 변의 수를 근사하는 경험식(상수 1.4111, 지수 0.4724). 원에 가까울수록 큰 값.
- contour 정의는 global neighborhood 설정(`set_system`)에 의존한다.

### 예외
- 빈 리전 → **모든 파라미터 0.0** (기본).

### 복잡도
> If F is the area of a region the mean runtime complexity is O(sqrt(F)).

---

## 6. eccentricity — 이심률 계열 3개 출력

### 시그니처
```
eccentricity(Regions : : : Anisometry, Bulkiness, StructureFactor)
```
| 파라미터 | 종류 | 타입 | 의미 | Assertion |
|---|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. | - |
| Anisometry | output_control | real(-array) → (real) | Shape feature (in case of a circle = 1.0). | `Anisometry >= 1.0` |
| Bulkiness | output_control | real(-array) → (real) | Calculated shape feature. | 없음 |
| StructureFactor | output_control | real(-array) → (real) | Calculated shape feature. | 없음 |

### Description (원문)
> The operator eccentricity calculates the three shape features Anisometry, Bulkiness, and StructureFactor for the given Regions:
> ... where Ra and Rb denote the ellipse radii (see elliptic_axis) and A denotes the area of the region (see area_center).

수식 (SVG 디코딩 결과, 3개 정렬식):

```
                      Ra
Anisometry      =  --------
                      Rb

                   pi * Ra * Rb
Bulkiness       =  ------------
                        A

StructureFactor =  Anisometry * Bulkiness  -  1
```

### Attention (원문)
> It should be noted that, like for all region-moments-based operators, the region's pixels are regarded as mathematical, infinitely small points that are represented by the center of the pixels (see the documentation of elliptic_axis). This can lead to non-empty regions that have Rb = 0. In these cases, the output features that require a division by Rb are set to 0. In particular, regions that contain a single point or regions whose points lie exactly on a straight line (e.g., one pixel high horizontal regions or one pixel wide vertical regions) have an anisometry of 0.

### 핵심 정리
- Ra/Rb는 `elliptic_axis` 의 주/부 반지름, A는 `area_center` 의 면적.
- Anisometry ≥ 1.0, 원 = 1.0.
- Bulkiness = 등가타원 면적 / 실제 면적 (원/타원이면 ≈ 1).
- StructureFactor = Anisometry·Bulkiness − 1 (원이면 ≈ 0).
- **예외적으로 Rb = 0 인 경우(단일 점, 1픽셀 폭 직선 리전 등) Anisometry는 0** → Assertion(≥1.0)과 어긋나는 실제 동작이므로 코드에서 반드시 방어할 것.

### 예외
- 빈 리전 → **모든 파라미터 0.0** (기본).

---

## 7. elliptic_axis — 등가 타원 축

### 시그니처
```
elliptic_axis(Regions : : : Ra, Rb, Phi)
```
| 파라미터 | 종류 | 타입 | 의미 | Assertion |
|---|---|---|---|---|
| Regions | input_object | region(-array) → object | Input regions. | - |
| Ra | output_control | real(-array) → (real) | Main radius (normalized to the area). | `Ra >= 0.0` |
| Rb | output_control | real(-array) → (real) | Secondary radius (normalized to the area). | `Rb >= 0.0 && Rb <= Ra` |
| Phi | output_control | real(-array) → (real) | Angle between main radius and x-axis in radians. | `-pi/2 < Phi && Phi <= pi/2` |

### Description (원문)
> The operator elliptic_axis calculates the radii Ra and Rb and the orientation Phi of the ellipse having the same orientation and the same aspect ratio as the input region in Regions. Several input regions can be passed as tuples.
> Ra represents the main radius of the ellipse whereas the radius Rb represents the secondary radius of the ellipse. The orientation of the main axis with regard to the x-axis is returned in Phi which is represented in radians.
> The principle axis of the ellipse is equivalent to the principle axis of the inertia moment of the input region.
>
> **Calculation:**
> If the moments M20, M02 and M11 are **normalized and passed to the area** (see moments_region_2nd), the radii Ra and Rb are calculated as:

수식 (SVG 디코딩 결과):

```
        sqrt( 8 * ( M20 + M02 + sqrt( (M20 - M02)^2 + 4 * M11^2 ) ) )
Ra  =  --------------------------------------------------------------
                                  2

        sqrt( 8 * ( M20 + M02 - sqrt( (M20 - M02)^2 + 4 * M11^2 ) ) )
Rb  =  --------------------------------------------------------------
                                  2

Phi =  -0.5 * atan2( 2 * M11 , M02 - M20 )
```

### Attention (원문)
> It should be noted that, like for all region-moments-based operators, the region's pixels are regarded as mathematical, infinitely small points that are represented by the center of the pixels. This means that Ra and Rb can assume the value 0. In particular, for an empty region and a region containing a single point Ra = Rb = 0 is returned. Furthermore, for regions whose points lie exactly on a straight line (e.g., one pixel high horizontal regions or one pixel wide vertical regions), Rb = 0 is returned.

### 핵심 정리
- **모멘트는 면적으로 정규화된 값**(normalized and passed to the area, 즉 `moments_region_2nd` 의 M20/M02/M11 — 각각 2차 중심모멘트를 면적 A로 나눈 값)을 사용한다.
- Ra ≥ Rb ≥ 0, "normalized to the area" 라고 명시.
- Phi 범위: `-π/2 < Phi ≤ π/2` (부호 있는 반개구간). 부호는 `atan2` 앞의 **−0.5 계수**에서 나온다 — 즉 row 축이 아래로 향하는 이미지 좌표에서 화면상 반시계 방향이 양(+)이 되도록 뒤집혀 있다.
- atan2 인자 순서 주의: `atan2(2*M11, M02 - M20)` (y인자 = 2·M11, x인자 = M02 − M20).

### 예외
- 빈 리전 → 모든 파라미터 0.0 (기본).
- 단일 점 리전 → Ra = Rb = 0. 직선상 리전 → Rb = 0.

---

## 8. orientation_region — 리전 방향

### 시그니처
```
orientation_region(Regions : : : Phi)
```
| 파라미터 | 종류 | 타입 | 의미 |
|---|---|---|---|
| Regions | input_object | region(-array) → object | Region(s) to be examined. |
| Phi | output_control | real(-array) → (real) | Orientation of region (arc measure). |

**Assertion**: `-pi <= Phi && Phi < pi`

### Description (원문 전문)
> The operator orientation_region calculates the orientation of the region. The operator is based on elliptic_axis. In addition the point on the contour with maximal distance to the center of gravity is calculated. If, in the rotated coordinate system, the column coordinate of this point is less than the column coordinate of the center of gravity, the value of π is added to the angle.

### 핵심 정리 — 부호/방향 결정 규칙
1. 먼저 `elliptic_axis` 로 Phi 를 구한다 (범위 −π/2 < Phi ≤ π/2).
2. **무게중심(center of gravity)에서 거리가 최대인 contour 점**을 찾는다.
3. **회전된 좌표계에서** 그 점의 **column 좌표가 무게중심의 column 좌표보다 작으면 각도에 π를 더한다.**
4. 결과 범위: `-π ≤ Phi < π` (elliptic_axis 의 π 주기 모호성을 제거하여 "긴 쪽이 어느 방향인지"까지 구분).

### 예외
- 빈 리전 → 모든 파라미터 0.0 (기본).

### 복잡도
> O(sqrt(F)) — F는 리전의 면적.

---

## 부록 — 한눈에 보는 공식표

| 오퍼레이터 | 출력 | 공식 | 범위 | 원(circle) 값 |
|---|---|---|---|---|
| circularity | Circularity | `C = min(1, F / (max² · π))`, max = 중심→contour 최대거리 | 0 ~ 1.0 | 1 |
| compactness | Compactness | `C = max(1, L² / (4 · F · π))`, L = contour length | ≥ 1.0 (빈 리전 0) | 1 |
| convexity | Convexity | `C = F_o / F_c` (원본면적 / convex hull 면적) | ≤ 1 | 1 (볼록 전부 1) |
| rectangularity | Rectangularity | 동일 1·2차 모멘트 사각형과의 차집합 면적을 사각형 면적으로 정규화 (닫힌 수식 문서에 명시 없음) | 0 ~ 1.0 | 사각형 = 1 |
| roundness | Distance | `(1/F) Σ ‖p − p_i‖` | ≥ 0 | 반지름 |
| roundness | Sigma | `sqrt( (1/F) Σ (‖p − p_i‖ − Distance)² )` | ≥ 0 | 0 |
| roundness | Roundness | `1 − Sigma / Distance` | ≤ 1.0 | 1 |
| roundness | Sides | `1.4111 · (Distance / Sigma)^0.4724` | ≥ 0 | 큰 값 |
| eccentricity | Anisometry | `Ra / Rb` | ≥ 1.0 (Rb=0이면 0) | 1.0 |
| eccentricity | Bulkiness | `π · Ra · Rb / A` | 문서에 명시 없음 | ≈ 1 |
| eccentricity | StructureFactor | `Anisometry · Bulkiness − 1` | 문서에 명시 없음 | ≈ 0 |
| elliptic_axis | Ra | `sqrt(8·(M20+M02+sqrt((M20−M02)²+4·M11²)))/2` | ≥ 0 | 반지름 |
| elliptic_axis | Rb | `sqrt(8·(M20+M02−sqrt((M20−M02)²+4·M11²)))/2` | 0 ≤ Rb ≤ Ra | 반지름 |
| elliptic_axis | Phi | `−0.5 · atan2(2·M11, M02 − M20)` | −π/2 < Phi ≤ π/2 | 불안정 |
| orientation_region | Phi | elliptic_axis 결과 + (회전좌표계에서 최원점 column < 중심 column 이면 +π) | −π ≤ Phi < π | 불안정 |

### 구현 시 주의 (문서 근거)
- circularity 는 `min`, compactness 는 `max` 로 1.0에서 클리핑 방향이 반대다.
- convexity 분자/분모 = 원본/convex hull (반대로 쓰면 항상 ≥ 1 이 되어 버림).
- elliptic_axis / eccentricity 는 픽셀을 "픽셀 중심의 무한소 점"으로 취급 → 1픽셀 두께 리전은 Rb = 0 → 0 나눗셈 방어 필요.
- rectangularity 는 정사각형 등 방향이 결정되지 않는 리전에서 최대 10% 과소평가.

---

← 이전: [G1 기본 / 크기](G1_basic_size.md)  |  [🏠 홈](../../../README.md)  |  다음: [G3 외접 / 내접 / 런렝스 →](G3_geometry_runlength.md)
