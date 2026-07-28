[🏠 홈](../README.md) › [📚 문서](README.md) › **특징값 정의서**

---

# 02. 특징값 정의서

<details>
<summary><b>목차</b></summary>

- [0. 공통 규약](#0-공통-규약)
  - [0.1 좌표계](#01-좌표계)
  - [0.2 기호](#02-기호)
  - [0.3 픽셀 모델 (중요)](#03-픽셀-모델-중요)
  - [0.4 예외 처리 기본값](#04-예외-처리-기본값)
- [1. `area_center` — 면적과 무게중심](#1-area_center--면적과-무게중심)
  - [1.1 시그니처](#11-시그니처) · [1.2 원문 정의](#12-원문-정의) · [1.3 수식](#13-수식) · [1.4 구현 지침](#14-구현-지침) · [1.5 미결 쟁점](#15-미결-쟁점)
- [2. `contlength` — 윤곽 길이](#2-contlength--윤곽-길이)
  - [2.1 시그니처](#21-시그니처) · [2.2 원문 정의](#22-원문-정의) · [2.3 수식](#23-수식) · [2.4 구현 지침](#24-구현-지침) · [2.5 쟁점 해소](#25-쟁점-해소)
- [3. `circularity` — 원형도](#3-circularity--원형도)
  - [3.1 시그니처](#31-시그니처) · [3.2 원문 정의](#32-원문-정의) · [3.3 수식](#33-수식) · [3.4 구현 지침](#34-구현-지침) · [3.5 쟁점 해소](#35-쟁점-해소)
- [4. `compactness` — 조밀도](#4-compactness--조밀도)
  - [4.1 시그니처](#41-시그니처) · [4.2 원문 정의](#42-원문-정의) · [4.3 수식](#43-수식) · [4.4 구현 지침](#44-구현-지침) · [4.5 미결 쟁점](#45-미결-쟁점)
- [5. `convexity` — 볼록도](#5-convexity--볼록도)
  - [5.1 시그니처](#51-시그니처) · [5.2 원문 정의](#52-원문-정의) · [5.3 수식](#53-수식) · [5.4 구현 지침](#54-구현-지침) · [5.5 쟁점 해소](#55-쟁점-해소)
- [2차 배치 — 2차 모멘트 계열 5종](#2차-배치--2차-모멘트-계열-5종)
- [6. `moments_region_2nd` — 2차 모멘트](#6-moments_region_2nd--2차-모멘트)
  - [6.1 시그니처](#61-시그니처) · [6.2 원문 정의](#62-원문-정의) · [6.3 수식](#63-수식) · [6.4 구현 지침](#64-구현-지침) · [6.5 쟁점 해소](#65-쟁점-해소)
- [7. `moments_region_2nd_invar` — F² 정규화 2차 모멘트](#7-moments_region_2nd_invar--f²-정규화-2차-모멘트)
  - [7.1 시그니처](#71-시그니처) · [7.2 원문 정의](#72-원문-정의) · [7.3 수식](#73-수식) · [7.4 구현 지침](#74-구현-지침) · [7.5 미결 쟁점](#75-미결-쟁점)
- [8. `elliptic_axis` — 등가 타원 축](#8-elliptic_axis--등가-타원-축)
  - [8.1 시그니처](#81-시그니처) · [8.2 원문 정의](#82-원문-정의) · [8.3 수식](#83-수식) · [8.4 EA-1 확정 근거](#84-ea-1-확정-근거-실측-불필요) · [8.5 구현 지침](#85-구현-지침) · [8.6 미결 쟁점](#86-미결-쟁점)
- [9. `eccentricity` — 이심률 3종](#9-eccentricity--이심률-3종)
  - [9.1 시그니처](#91-시그니처) · [9.2 원문 정의](#92-원문-정의) · [9.3 수식](#93-수식) · [9.4 구현 지침](#94-구현-지침) · [9.5 미결 쟁점](#95-미결-쟁점)
- [10. `orientation_region` — 리전 방향](#10-orientation_region--리전-방향)
  - [10.1 시그니처](#101-시그니처) · [10.2 원문 정의](#102-원문-정의) · [10.3 수식](#103-수식) · [10.4 구현 지침](#104-구현-지침) · [10.5 미결 쟁점](#105-미결-쟁점)
- [11. 정의 확정 이력](#11-정의-확정-이력)
- [12. 구현 의존성](#12-구현-의존성)

</details>

> **이 문서가 구현의 유일한 근거다.**
> 코드는 이 문서를 따르고, 이 문서는 `docs/reference/halcon13/` 원문을 따른다.
> 원문에 없는 것은 **미결 쟁점**으로 남기고, 임의로 채우지 않는다.

## 0. 공통 규약

### 0.1 좌표계

HALCON 은 **(Row, Column) = (y, x)** 순서다. OpenCV 의 `cv::Point(x, y)` 와 **반대**다.

```
HALCON  (row, col) = (y, x)      Row = point.y,  Column = point.x
OpenCV  cv::Point(x, y)          x = col,        y = row
```

- 이미지 좌상단이 (0, 0).
- 본 라이브러리의 **공개 API 는 HALCON 관례(row, col)를 따른다.** 내부에서 OpenCV 를 쓰더라도 경계에서 변환한다.
- 각도 `Phi` 는 **x축(= column 축) 기준 라디안**이다.

### 0.2 기호

| 기호 | 의미 |
|---|---|
| `F`, `A` | 리전의 면적 = 리전에 속한 **픽셀 개수** |
| `L` | 윤곽 길이 (`contlength`) |
| `p` | 면적의 중심(무게중심) |
| `p_i` | 개별 픽셀 |
| `max` | 중심에서 contour 픽셀까지의 **최대** 거리 |
| `F_o` / `F_c` | 원본 면적 / convex hull 면적 |

### 0.3 픽셀 모델 (중요)

원문 `elliptic_axis` / `eccentricity` 의 Attention:

> "the region's pixels are regarded as mathematical, **infinitely small points** that are represented by the **center of the pixels**"

즉 픽셀은 **면적 1의 정사각형이 아니라 픽셀 중심의 점**으로 취급된다.
이 때문에 1픽셀 두께 리전에서 `Rb = 0` 이 나온다. 면적 기반 값과 좌표 기반 값이
미묘하게 어긋나는 근본 원인이므로, 모멘트 계열 구현 시 반드시 이 모델을 따른다.

### 0.4 예외 처리 기본값

| 상황 | HALCON 기본 동작 | 본 라이브러리 |
|---|---|---|
| 빈 리전(empty region) | 모든 출력 `0` / `0.0` (`set_system('empty_region_result')` 로 변경 가능) | 동일하게 `0` 반환. 설정 스위치는 후순위 |
| 입력 객체 없음 | `set_system('no_object_result')` | 빈 결과 반환 |
| 0 나눗셈 (예: `Rb = 0`) | 해당 출력 `0` (원문 명시) | 동일하게 `0`, Assertion 위반을 그대로 재현 |

> `eccentricity` 의 Anisometry 는 문서상 Assertion 이 `>= 1.0` 인데
> Rb = 0 일 때 **실제로는 0** 을 반환한다. 문서 자체가 모순이며, **실제 동작(0)을 따른다.**

---

## 1. `area_center` — 면적과 무게중심

### 1.1 시그니처

```
area_center(Regions : : : Area, Row, Column)
```

| 출력 | 타입 | 의미 |
|---|---|---|
| `Area` | integer | 리전의 픽셀 개수 |
| `Row` | real | 중심의 행 좌표 |
| `Column` | real | 중심의 열 좌표 |

### 1.2 원문 정의

> "The area is defined as the number of pixels of a region.
> The center is calculated as the mean value of the line or column coordinates, respectively, of all pixels."

### 1.3 수식

```
Area = F                     (리전에 속한 픽셀 개수)
Row  = (1/F) * SUM(r_i)      (모든 픽셀의 행 좌표 평균)
Col  = (1/F) * SUM(c_i)      (모든 픽셀의 열 좌표 평균)
```

- `Row`, `Column` 은 **실수**다. 정수로 반올림하지 않는다.
- 원문에 명시적 LaTeX 수식은 없고 서술로만 기술되어 있다. 위 수식은 서술의 직역이다.

### 1.4 구현 지침

리전을 **런렝스(run-length)** 로 보관하면 O(런 수) 로 계산된다.
런 = 한 행에서 연속된 전경 픽셀 구간 `(row, colStart, colEnd)`.

```
F   += (colEnd - colStart + 1)
sumR += row * (colEnd - colStart + 1)
sumC += (colStart + colEnd) * (colEnd - colStart + 1) / 2      ← 등차수열 합
```

`sumC` 는 `SUM(colStart..colEnd)` 이므로 `(첫항+끝항)*개수/2` 로 상수시간 처리한다.
픽셀을 하나씩 순회하면 안 된다(대면적에서 성능 붕괴).

### 1.5 미결 쟁점

없음. 정의가 명확하다.

---

## 2. `contlength` — 윤곽 길이

### 2.1 시그니처

```
contlength(Regions : : : ContLength)      Assertion: ContLength >= 0
```

### 2.2 원문 정의

- 윤곽(contour)의 **전체 길이**. 리전이 여러 연결성분이면 **모든 성분의 합**.
- 거리 가중치: **축평행 인접 = 1**, **대각 인접 = √2**.
- **Attention: "The contour of holes is not calculated."**

### 2.3 수식

```
ContLength = (축평행 스텝 수 * 1) + (대각 스텝 수 * sqrt(2))
```

### 2.4 구현 지침

- **구멍(hole)의 윤곽은 절대 더하지 않는다.** 외곽 윤곽만 계산한다.
  - OpenCV 로 구현 시 `cv::findContours` 에 `RETR_EXTERNAL` 을 써야 하며,
    `RETR_LIST` / `RETR_CCOMP` 를 쓰면 구멍 윤곽이 섞여 값이 커진다.
- `cv::arcLength` 는 근사 폴리곤 길이라 **체인코드 정의와 일치하지 않는다.**
  윤곽 픽셀 체인을 직접 순회하며 축평행/대각을 세는 방식으로 구현한다.
- 연결성분이 여러 개면 각 성분의 외곽 윤곽 길이를 **합산**한다.

### 2.5 쟁점 해소

| # | 쟁점 | 상태 |
|---|---|---|
| **C-1** | 윤곽 체인이 **픽셀 중심 경로**인가 **픽셀 경계 둘레**인가 | ✅ **확정: 픽셀 중심 체인** (근거 ↓) |
| C-2 | 윤곽 픽셀 정의가 `set_system('neighborhood', 4/8)` 에 의존하는가 (`roundness` 페이지에는 의존한다는 언급이 있음) | 원문 미명시. **8-이웃 고정**으로 구현하고 스위치를 남긴다 |

#### C-1 확정 근거 (실측 불필요)

원문이 **"대각(diagonal) 인접 = √2"** 라고 명시한다는 사실 자체가 결정적 증거다.

- **픽셀 경계 둘레 모델**(crack following)은 픽셀의 변을 따라가므로 **모든 스텝이 축평행 단위길이**다.
  이 모델에는 **√2 항이 원리적으로 존재할 수 없다.**
- **대각 스텝은 픽셀 중심을 잇는 체인코드(Freeman chain)에서만 발생**한다.

따라서 원문의 √2 가중치는 **픽셀 중심 체인 모델을 전제**한다. → **가설 A 확정.**

귀결:
```
단일 픽셀 리전   → 스텝이 없음 → ContLength = 0.0
100x100 정사각형 → 99+99+99+99 = 396 스텝(전부 축평행) → 396.0
```

> 잔여 리스크: 체인의 **닫힘 처리**(마지막 점 → 첫 점 복귀 스텝 포함 여부).
> 위 396 은 복귀 스텝을 **포함**한 값이다(닫힌 경로). 포함하지 않으면 395 가 된다.
> HALCON 실측 시 이 1 스텝 차이를 우선 확인한다.

---

## 3. `circularity` — 원형도

### 3.1 시그니처

```
circularity(Regions : : : Circularity)    Assertion: 0 <= Circularity <= 1.0
```

### 3.2 원문 정의

> "If F is the area of the region and **max is the maximum distance from the center to all contour pixels**, the shape factor C is defined as:"

### 3.3 수식

```
              F
C' = -------------------
      max^2  *  pi

C  = min(1, C')          ← 위로 1.0 에서 클리핑
```

- `max` = **중심에서 모든 contour 픽셀까지의 최대 거리** (bounding circle 반지름이 아니다)
- 분모는 반지름 `max` 인 원의 면적
- 원 = 1. 길쭉하거나 구멍이 있으면 < 1.
- 원문: "The value of C is clipped to 1.0, because the pixel area of a region can only be an approximation of a real circle's area."

### 3.4 구현 지침

```
1. area_center 로 중심 (rBar, cBar) 를 구한다        ← 무게중심, 실수
2. 외곽 contour 픽셀 집합을 구한다                    ← contlength 와 동일한 정의
3. max = MAX over contour pixels of
         sqrt((r_i - rBar)^2 + (c_i - cBar)^2)
4. C = min(1.0, F / (max*max * PI))
```

- `max` 계산에 **contour 픽셀만** 쓴다. 리전 전체 픽셀을 쓰면 결과는 같지만 O(F) 로 느려진다.
- `max == 0` (단일 픽셀) 이면 0 나눗셈 → **0 을 반환**한다.

### 3.5 쟁점 해소

| # | 쟁점 | 상태 |
|---|---|---|
| CI-1 | "contour pixels" 에 **구멍의 윤곽 픽셀**이 포함되는가 | 원문 미명시. **외곽만 사용**으로 고정(`contlength` 와 일관). 구멍 윤곽은 중심에 가까워 `max` 를 바꾸지 못하므로 실무 영향 없음 |
| CI-2 | 중심이 **무게중심**인가 bounding box 중심인가 | ✅ **확정: 무게중심.** `roundness` 원문이 동일 개념을 "center of the area" 로, `orientation_region` 이 "center of gravity" 로 명시한다. `circularity` 의 "the center" 도 같은 것을 가리킨다 |

---

## 4. `compactness` — 조밀도

### 4.1 시그니처

```
compactness(Regions : : : Compactness)    Assertion: Compactness >= 1.0 || Compactness == 0
```

### 4.2 원문 정의

> "If L is the length of the contour (see contlength) and F the area of the region the shape factor C is defined as:"

### 4.3 수식

```
             L^2
C' = -------------------
      4  *  F  *  pi

C  = max(1, C')          ← 아래로 1.0 에서 클리핑 (circularity 와 방향 반대!)
```

- `L` = `contlength` 의 결과를 그대로 쓴다. **정의 연쇄가 있으므로 contlength 가 틀리면 이 값도 틀린다.**
- 원 = 1. 길쭉하거나 구멍이 있으면 > 1. **상한 없음.**
- 빈 리전은 0.

> **원문의 모순 주의**: Description 본문에는 "The value of C is clipped to 1.0" 라는
> `circularity` 페이지와 동일한 문장이 그대로 복붙되어 있으나,
> 실제 수식은 `max(1, C')` 이고 Assertion 도 `>= 1.0` 이다. **수식과 Assertion 을 따른다.**

### 4.4 구현 지침

```
L = contlength(region)
C = max(1.0, (L*L) / (4.0 * F * PI))
if (F == 0) return 0.0;
```

### 4.5 미결 쟁점

`contlength` 의 쟁점(C-1, C-2)을 그대로 상속한다. 독자적 쟁점은 없다.

---

## 5. `convexity` — 볼록도

### 5.1 시그니처

```
convexity(Regions : : : Convexity)        Assertion: Convexity <= 1
```

### 5.2 원문 정의

> "If **F_c is the area of the convex hull** and **F_o the original area** of the region the shape factor C is defined as:"

### 5.3 수식

```
      F_o        (원본 리전의 면적)
C = -------  =  --------------------
      F_c        (convex hull 의 면적)
```

- **분자 = 원본, 분모 = hull.** 뒤집으면 항상 ≥ 1 이 되어 Assertion(≤ 1)을 위반한다.
- 볼록 도형(사각형, 원 등) = 1. 오목부(indentation)나 구멍이 있으면 < 1.
- 찢김(Tear) / 갈라짐 판별의 핵심 지표.

### 5.4 구현 지침

```
1. 리전의 외곽 contour 점들을 모은다
2. convex hull 을 구한다
3. F_c = hull 의 면적
4. C = F_o / F_c
```

### 5.5 쟁점 해소

| # | 쟁점 | 상태 |
|---|---|---|
| **CV-1** | `F_c` 가 "hull 폴리곤의 기하 면적" 인가 "hull 리전의 픽셀 개수" 인가 | ✅ **확정: 픽셀 개수** (근거 ↓) |
| CV-2 | hull 의 입력이 contour 픽셀인가 전체 픽셀인가 | 결과 동일(hull 은 극점만 사용). **contour 픽셀 사용**(성능) |

#### CV-1 확정 근거 (실측 불필요)

**HALCON 자신의 Assertion 이 폴리곤 면적 해석을 배제한다.**

- 문서상 Assertion 은 `Convexity <= 1` 이다.
- 폴리곤 면적(픽셀 중심 기준)을 쓰면 100×100 정사각형에서
  `F_c = 99 × 99 = 9801` → `C = 10000 / 9801 = 1.0203` → **Assertion 위반**.
- `F_o` 는 명백히 **픽셀 개수**다. 분자·분모의 단위를 맞추려면 `F_c` 도 픽셀 개수여야 한다.
- HALCON 은 볼록껍질을 **리전으로 반환하는** 연산(`shape_trans(..., 'convex')`)을 갖고 있고,
  리전의 면적은 곧 픽셀 개수다.

→ **`F_c` = convex hull 을 래스터화한 리전의 픽셀 개수.** 정사각형에서 `C = 1.0` 이 된다.

> **실무 영향**: `cv::contourArea` 를 그대로 쓰면 **모든 볼록 도형에서 1을 넘어**
> 형상계수 기반 스코어가 통째로 오염된다. 이 항목은 구현에서 가장 흔한 사고 지점이다.

#### 래스터화 규칙 (구현 고정)

hull 폴리곤 내부 픽셀 판정은 **경계 포함(inclusive)** 으로 한다.
- 픽셀 중심 `(r, c)` 가 hull 폴리곤 **내부이거나 변 위에 있으면** 포함.
- 이렇게 해야 정사각형에서 hull 픽셀 = 원본 픽셀 = 10000 이 되어 `C = 1.0` 이 성립한다.
- 구현은 hull 꼭짓점에 대한 **스캔라인 채우기**로 하되, 각 행에서
  `ceil(xLeft - eps) .. floor(xRight + eps)` 를 채운다 (`eps = 1e-9`).

---

## 2차 배치 — 2차 모멘트 계열 5종

§6~§10 은 **하나의 배치**다. 다섯 종이 전부 **2차 중심 모멘트 코어 하나**에서 나오며,
새로 필요한 인프라가 없다. 선정 근거는 [`00_OVERVIEW.md` §5](00_OVERVIEW.md#5-2차-구현-대상-5종) 에 있다.

```
                          ┌─ moments_region_2nd        (§6)  정규화 없음
   Region ── 무게중심 ──┬─┤
   (런렝스)   (§1)      │ └─ moments_region_2nd_invar  (§7)  F² 로 정규화   ← §6 의 대조군
                        │
                        └─ M20/M02/M11 을 F 로 정규화
                                 ├─ elliptic_axis      (§8)  Ra, Rb, Phi
                                 ├─ eccentricity       (§9)  Ra/Rb 에 의존
                                 └─ orientation_region (§10) Phi + 최원 윤곽점
```

**정규화 인자가 세 가지라는 점이 이 배치의 핵심 위험이다.**

| 쓰는 곳 | 정규화 | 근거 |
|---|---|---|
| `moments_region_2nd` 의 출력 | **없음** (순수 합) | G4 §1 수식에 나눗셈이 없다 |
| `moments_region_2nd_invar` 의 출력 | **`F²`** | G4 §2 수식 `(1/F^2)·SUM …` |
| `elliptic_axis` 가 내부에서 쓰는 M20/M02/M11 | **`F`** | G2 §7 "normalized and passed to the area" (§8.4 에서 확정) |

하나라도 섞으면 다섯 종이 통째로 틀린다. §7 을 §6 과 나란히 구현하는 이유가 이것이다 —
**정규화를 잘못 넣어도 §6 만 있으면 아무 테스트도 깨지지 않는다.**

---

## 6. `moments_region_2nd` — 2차 모멘트

### 6.1 시그니처

```
moments_region_2nd(Regions : : : M11, M20, M02, Ia, Ib)
```

| 출력 | 타입 | 원문 설명 |
|---|---|---|
| `M11` | real | "Product of inertia of the axes through the center parallel to the coordinate axes." |
| `M20` | real | "Moment of 2nd order (row-dependent)." |
| `M02` | real | "Moment of 2nd order (column-dependent)." |
| `Ia` | real | "Length of the major axis of the input region." |
| `Ib` | real | "Length of the minor axis of the input region." |

> **Assertion 절이 원문에 없다.** 모멘트 7종 페이지 전부 Value Range 항목 자체가 존재하지 않는다
> (`docs/reference/halcon13/G4_moments.md` 공통 사항). 값 범위로 회귀를 잡을 수 없으므로
> 검증은 전적으로 기준 도형 대조에 의존한다.

### 6.2 원문 정의

> "r0 and c0 are the coordinates of the **center of gravity** of a region R.
> Then the moments M_ij are defined by:"

```
M_ij = SUM_{(r,c) in R} (r0 - r)^i * (c0 - c)^j
```

> "wherein r and c run over all pixels of the region R."

공분산 행렬:

```
[ M20  M11 ]
[ M11  M02 ]
```

> "Furthermore the length of the major and minor axes are defined by:"

```
Ia = h + sqrt( h^2 - M20 * M02 + M11^2 )
Ib = h - sqrt( h^2 - M20 * M02 + M11^2 )
h  = (M20 + M02) / 2
```

근거: `docs/reference/halcon13/G4_moments.md` §1

### 6.3 수식

```
r0 = (1/F)·SUM(r)      c0 = (1/F)·SUM(c)      ← area_center(§1) 와 같은 무게중심

M20 = SUM (r0 - r)^2
M02 = SUM (c0 - c)^2
M11 = SUM (r0 - r)(c0 - c)

h   = (M20 + M02) / 2
Ia  = h + sqrt(h^2 - M20·M02 + M11^2)
Ib  = h - sqrt(h^2 - M20·M02 + M11^2)
```

#### ⚠ 정규화가 없다 (함정 #4 / #6)

**면적으로 나누지 않는다.** 순수 합이다. `F` 로 나누는 순간 이 값을 입력으로 쓰는
`moments_region_2nd_invar`(F² 로 나눠야 함) · `elliptic_axis`(F 로 나눠야 함) 가 전부 오염된다.

#### ⚠ 부호 규약 (함정 #5) — 2차에서는 값에 영향이 **없다**

원문은 `(중심 − 픽셀)` 이고 3차 페이지는 `(픽셀 − 중심)` 이다. 그러나 2차 세 항은

```
(r0 - r)^2      = (r - r0)^2                 짝수 지수라 부호 소거
(r0 - r)(c0 - c) = (r - r0)(c - c0)           음수 두 개가 서로 상쇄
```

이므로 **세 항 전부 통상의 중심 모멘트와 값이 완전히 같다.**
부호 함정이 실제로 값을 바꾸는 것은 **3차 홀수항뿐**이다.
이 사실을 못 박아 두지 않으면 3차를 구현할 때 2차까지 뒤집는 **과잉 수정**을 하게 된다.

#### ⚠ 픽셀 모델 (함정 #7)

`§0.3` 대로 픽셀은 **면적 1의 사각형이 아니라 픽셀 중심의 점**이다. 따라서

```
1픽셀 높이 가로 리전  → M20 = 0  →  Ib = 0  →  Rb = 0
단일 픽셀             → M20 = M02 = M11 = 0  →  Ia = Ib = 0
```

이것은 결함이 아니라 정의다. "두께 1을 더해" 보정하면 안 된다.

### 6.4 구현 지침

#### (1) 런렝스 O(런 수) 누적

런 `(row, colBegin..colEnd)`, `n = colEnd - colBegin + 1`, `a = colBegin` 일 때

```
SUM c        = n·a + n(n-1)/2                          (등차수열 합)
SUM c^2      = n·a^2 + a·n(n-1) + (n-1)n(2n-1)/6       (제곱합, Faulhaber)
SUM r        = row·n
SUM r^2      = row^2·n
SUM r·c      = row · (SUM c)
```

픽셀을 하나씩 순회하지 않는다. `§1.4` 와 같은 원칙이다.

#### (2) 정수 원점 시프트 — 자리수 손실 방어

원시 모멘트 `SUM c^2` 를 그대로 모아 `mu02 = SUM c^2 - (SUM c)^2/F` 로 빼면
**대참사급 자리수 손실**이 난다. 예: 53580 열 이미지의 작은 blob 은
`SUM c^2 ≈ 3e11` 인데 `mu02` 는 1e3 규모다.

그러므로 **무게중심에 가장 가까운 격자점 `(rs, cs)` 로 좌표를 먼저 옮긴다.**

```
rs = floor(r0 + 0.5)      fr = r0 - rs        (|fr| <= 0.5)
cs = floor(c0 + 0.5)      fc = c0 - cs

dr = r - rs               dc = c - cs         (작은 정수)

M20 = SUM dr^2      - 2·fr·SUM dr - ... 아래 일반식
M02 = SUM dc^2      - 2·fc·SUM dc + fc^2·F
M11 = SUM dr·dc     - fr·SUM dc - fc·SUM dr + fr·fc·F
```

`(r0, c0)` 가 정확한 무게중심이면 `SUM dr = fr·F` 이므로 위 식은
`M20 = SUM dr^2 - fr^2·F` 로 축약된다. **그래도 축약형을 쓰지 않고 일반식으로 구현한다** —
축약형은 "넘겨받은 중심이 진짜 무게중심" 이라는 전제에 의존하고, 그 전제가 깨지면
조용히 틀린 값을 낸다. 일반식은 어떤 중심을 넣어도 그 중심 기준의 올바른 값을 낸다.
비용 차이는 누적 3개뿐이다.

#### (3) 판별식은 항상 ≥ 0 이다 — 원문 미언급 사항의 해소

원문 형태 `sqrt(h² − M20·M02 + M11²)` 를 그대로 쓰면 안 된다. 다음 항등식이 성립한다.

```
h^2 - M20·M02 + M11^2  =  ((M20 - M02)/2)^2 + M11^2
```

| | 원문 형태 | 항등 변형 |
|---|---|---|
| 값 | 동일 (대수적 항등) | 동일 |
| `M20 ≈ M02` 일 때 | `h²` 와 `M20·M02` 가 **거의 같은 큰 수**라 자리수가 통째로 날아간다 | 손실 없음 |
| 대면적 (`M ≈ 3e17`) | `h² ≈ 1e35` — 유효자리 전멸 | `M` 규모 그대로 |
| 음수 가능성 | 부동소수 오차로 **음수가 나와 `sqrt` 가 NaN** | 두 제곱의 합이라 **원리적으로 ≥ 0** |

→ **항등 변형으로 구현한다.** 원문 아카이브가 "sqrt 내부가 음수가 될 경우 언급 없음"
(`G4_moments.md` 값 범위/예외 조건) 이라고 남긴 항목은 이 항등식으로 해소된다.
공분산 행렬은 반정치(positive semi-definite)이므로 **`Ib >= 0` 도 수학적으로 보장**된다
(`Ib < 0` 이 나오면 그것은 부동소수 오차이므로 0 으로 클램프한다).

### 6.5 쟁점 해소

| # | 쟁점 | 상태 |
|---|---|---|
| **M2-1** | `Ia`/`Ib` 를 원문 파라미터 설명은 "Length of the major/minor axis" 라고 부르는데, 수식은 **공분산 고유값**(차원이 길이²)이다 | ✅ **수식을 따른다.** 원문 자체 불일치이며, 파라미터 한 줄 설명이 아니라 **Calculation 절이 정본**이다. G4 도 "문서는 이 둘의 단위 차이를 별도로 언급하지 않음" 이라고 기록했다. 실제 "길이" 는 `elliptic_axis` 의 `Ra`/`Rb` 이고 그 관계는 `Ra = 2·sqrt(Ia/F)` 다(§8.5) |
| M2-2 | 빈 리전 | `§0.4` 규약대로 5개 출력 전부 `0.0` |
| M2-3 | 다중 연결성분 | 원문에 분리 규정이 없다. **리전 전체를 하나로 보고 합산**한다 (`area_center` 와 동일) |

---

## 7. `moments_region_2nd_invar` — F² 정규화 2차 모멘트

### 7.1 시그니처

```
moments_region_2nd_invar(Regions : : : M11, M20, M02)
```

출력 **3개**다 (`Ia`, `Ib` 가 없다). 이름과 의미 설명은 §6 과 동일하되
`M20` 만 "row-dependent" 가 아니라 **"line-dependent"** 로 적혀 있다 (같은 뜻).

### 7.2 원문 정의

> "The operator moments_region_2nd_invar calculates the **scaled moments** (M20, M02) and the
> procut [sic] of inertia of the axes through the center parallel to the coordinate axes (M11).
>
> Calculation:
> Z0 and S0 are the coordinates of the **center** of a region R with the area F.
> Then the moments M_ij are defined by:"

```
M_ij = (1 / F^2) * SUM_{(Z,S) in R} (Z0 - Z)^i * (S0 - S)^j
```

> "wherein Z and S run through all pixels of the region R."

근거: `docs/reference/halcon13/G4_moments.md` §2
(`Z` = Zeile(행), `S` = Spalte(열) 로 보이나 **문서에 그 설명은 없다.**)

### 7.3 수식

```
M20_invar = M20 / F^2
M02_invar = M02 / F^2
M11_invar = M11 / F^2          ← M** 는 §6 의 비정규화 값
```

- **지수는 고정 2다.** `i`, `j` 와 무관하다. `F^1` 도 `F^(1+(i+j)/2)` 도 아니다.
- 즉 이 오퍼레이터는 정확히 **`moments_region_2nd` / `F²`** 다.

#### 왜 F² 인가 (스케일 불변성)

도형을 `s` 배로 확대하면

```
F   -> s^2 · F          (픽셀 개수)
M20 -> s^4 · M20        (개수 s^2 배 × 거리제곱 s^2 배)
```

이므로 `M20/F²` 가 불변이 된다. **이 유도 설명은 원문에 없다**(G4 가 "문서에 명시 없음" 으로 기록).
그러나 오퍼레이터 이름(`_invar`)과 원문 표현("scaled moments")에 부합하고,
아래 닫힌 해가 그것을 확인해 준다.

```
n×n 정사각형 :  M20_invar = M02_invar = (n^2 - 1) / (12·n^2)   →  n→∞ 에서 1/12
                M11_invar = 0
```

`n = 100` 이면 `9999/120000 = 0.083325`, `n = 50` 이면 `2499/30000 = 0.0833`.
**두 값이 거의 같다는 것이 스케일 불변성의 직접 증거**이며,
같은 두 도형에서 `moments_region_2nd` 의 `M20` 은 `8332500` 대 `520625` 로 16배 차이가 난다.
→ §6 과 §7 을 **같은 도형 쌍에 나란히 걸면 정규화 오류가 반드시 드러난다.**

### 7.4 구현 지침

- **§6 의 결과를 `F²` 로 나눈다.** 모멘트 코어를 두 번 돌지 않는다.
  두 오퍼레이터가 각자 합을 다시 모으면 언젠가 값이 어긋난다.
- `F == 0`(빈 리전) 이면 0 나눗셈 → 세 출력 전부 `0.0` (`§0.4`).
- `F²` 는 `double` 로 계산한다. `long` 으로 제곱하면 `F ≥ 46341` 에서 32비트가 넘친다
  (풀프레임 리전은 `F ≈ 3e8`, `F² ≈ 1e17` — `double` 로도 정수 정확도를 넘지만
  상대오차 1e-16 이라 무해하다).

### 7.5 미결 쟁점

| # | 쟁점 | 상태 |
|---|---|---|
| **MI-1** | 원점이 **무게중심**인가. §6 은 "center of **gravity**" 인데 이 페이지는 그냥 **"the center"** 다 | 🟡 **잠정: 무게중심.** 근거 ↓. 판정 도형은 **S5 L자**(무게중심 `(57.833, 41.167)` vs 바운딩박스 중심 `(49.5, 49.5)`) — 판별력이 매우 크다 |
| MI-2 | 값 범위 | 원문에 Assertion 절 없음 |

#### MI-1 잠정 근거

1. 두 오퍼레이터가 서로를 `Alternatives` 로 지목하고, 출력 **이름·개수·의미 설명이 동일**하다.
2. 원문이 `moments_region_2nd_invar` 를 "**scaled** moments" 라고 부른다. "scaled" 는
   원점이 아니라 **크기 정규화**만을 가리키는 표현이다. 원점이 달랐다면 "scaled" 로
   설명될 수 없다(평행이동 불변성이 깨진다).
3. 바운딩박스 중심으로 해석하면 **평행이동 불변조차 성립하지 않는** 값이 되어
   `_invar` 라는 이름과 정면으로 모순된다.

→ 잠정 결론은 무게중심이지만 **원문에 "gravity" 라는 단어가 없는 것은 사실**이므로
미결로 남기고, 코드에 스위치(`GLIM_HALCON_INVAR_ORIGIN_BBOX`, 기본 0)를 둔다.

---

## 8. `elliptic_axis` — 등가 타원 축

### 8.1 시그니처

```
elliptic_axis(Regions : : : Ra, Rb, Phi)
```

| 출력 | 의미 | Assertion |
|---|---|---|
| `Ra` | "Main radius (normalized to the area)." | `Ra >= 0.0` |
| `Rb` | "Secondary radius (normalized to the area)." | `Rb >= 0.0 && Rb <= Ra` |
| `Phi` | "Angle between main radius and x-axis in radians." | `-pi/2 < Phi && Phi <= pi/2` |

### 8.2 원문 정의

> "If the moments M20, M02 and M11 are **normalized and passed to the area**
> (see moments_region_2nd), the radii Ra and Rb are calculated as:"

```
        sqrt( 8 * ( M20 + M02 + sqrt( (M20 - M02)^2 + 4 * M11^2 ) ) )
Ra  =  --------------------------------------------------------------
                                  2

        sqrt( 8 * ( M20 + M02 - sqrt( (M20 - M02)^2 + 4 * M11^2 ) ) )
Rb  =  --------------------------------------------------------------
                                  2

Phi =  -0.5 * atan2( 2 * M11 , M02 - M20 )
```

> **Attention**: "the region's pixels are regarded as mathematical, infinitely small points …
> This means that Ra and Rb can assume the value 0. In particular, for an empty region and a
> region containing a single point Ra = Rb = 0 is returned. Furthermore, for regions whose points
> lie exactly on a straight line …, Rb = 0 is returned."

근거: `docs/reference/halcon13/G2_shape_factors.md` §7

### 8.3 수식

```
M20n = M20 / F        M02n = M02 / F        M11n = M11 / F      ← §6 값을 F 로 나눈다

D    = sqrt( (M20n - M02n)^2 + 4·M11n^2 )
Ra   = sqrt( 8·(M20n + M02n + D) ) / 2
Rb   = sqrt( 8·(M20n + M02n - D) ) / 2
Phi  = -0.5 · atan2( 2·M11n , M02n - M20n )
```

#### ⚠ Phi 의 부호 (함정 #11)

통념은 `+0.5·atan2(2·M11, M20 − M02)` 다. **원문은 계수도 인자 순서도 다르다.**

```
원문   :  Phi = -0.5 · atan2( 2·M11 , M02 - M20 )
통념   :  Phi = +0.5 · atan2( 2·M11 , M20 - M02 )
```

두 식은 `M11 ≠ 0` 인 도형에서 **부호가 정확히 반대**로 갈린다
(S5 L자: 원문 `−π/4`, 통념 `+π/4`). 판정 도형은 S5 다.

`Phi` 가 이렇게 정의되면 주축 방향 단위벡터는 이미지 좌표 `(col, row)` 기준으로

```
u = ( cos(Phi), -sin(Phi) )
```

가 된다. 즉 **row 축이 아래로 향하는 이미지에서 화면상 반시계 방향이 양(+)** 이다.
이 대응은 `orientation_region`(§10) 의 `+π` 판정에서 결정적으로 쓰인다.

### 8.4 EA-1 확정 근거 (실측 불필요)

"normalized and passed to the area" 만으로는 **`F` 인지 `F²` 인지 정해지지 않는다.**
바로 옆 오퍼레이터 `moments_region_2nd_invar` 가 `F²` 라서(§7) 실제로 혼동 위험이 크다.
**연속 원판 하나로 결정된다.**

반지름 `R` 인 연속 원판: `F = πR²`, `M20 = M02 = F·R²/4`, `M11 = 0`.

| 해석 | `M20n` | `Ra` | 판정 |
|---|---|---|---|
| **`F` (1제곱)** | `R²/4` | `sqrt(8·(R²/2))/2 = sqrt(4R²)/2 = ` **`R`** | ✅ "Main radius" 와 일치 |
| `F²` | `1/(4π)` | `sqrt(8/(2π))/2 = 0.5642` — **R 과 무관한 상수** | ❌ 반지름이 될 수 없다 |

보강 근거: `eccentricity` 원문이 `Bulkiness = π·Ra·Rb/A` 를 두고
"in case of a circle = 1.0" 이라고 명시한다. `F` 정규화라야 `πR²/πR² = 1` 이 성립한다.
`F²` 해석에서는 `π·0.5642²/(πR²)` 로 도형 크기에 따라 제멋대로 변한다.

→ **`F`(1제곱) 확정. 미결 아님.**

### 8.5 구현 지침

```
1. §6 의 M20, M02, M11 과 면적 F 를 가져온다 (캐시 공유. 다시 계산하지 않는다)
2. F <= 0  → Ra = Rb = Phi = 0 (빈 리전 규약)
3. 정규화 후 D 계산
4. sqrt 인자를 0 으로 클램프한다
     8·(M20n + M02n - D) 는 수학적으로 >= 0 이지만
     축퇴 도형(직선/단일점)에서 -1e-13 같은 값이 나올 수 있다. 그대로 sqrt 하면 NaN 이다
5. Rb > Ra 면 Rb = Ra 로 클램프한다 (Assertion 재현. 역시 부동소수 방어)
```

**`Ia`/`Ib` 와의 관계 (정의 연쇄 검증 지표)**

```
Ra = 2·sqrt(Ia / F)        Rb = 2·sqrt(Ib / F)
```

`M20n + M02n ± D = (Ia ± ... )/F` 를 정리하면 나온다. **이 항등식은 §6 과 §8 이
같은 코어를 쓰는지 검사하는 회귀 지표로 쓴다** (`docs/03_VERIFICATION.md` T-11).
한쪽 정규화를 잘못 넣으면 이 등식이 즉시 깨진다.

**`Phi` 는 정규화 인자에 둔감하다.** 분자 `2·M11n` 과 분모 `M02n − M20n` 이 같은 배율로
스케일되므로 `F` 로 나누든 안 나누든 `Phi` 값은 같다.
→ **`Phi` 로는 정규화 오류를 잡을 수 없다.** 반드시 `Ra`/`Rb` 로 잡아야 한다.

### 8.6 미결 쟁점

| # | 쟁점 | 상태 |
|---|---|---|
| **EA-2** | `Phi` 가 정확히 `−π/2` 로 떨어지는 경우 | 🟡 잠정 해소 ↓ |

#### EA-2 상세

세로 1픽셀 리전(예: `col = 50`, `row ∈ [0,99]`)에서

```
M20n > 0,  M02n = 0,  M11n = 0
atan2(2·M11n, M02n - M20n) = atan2(+0, 음수) = +pi
Phi = -0.5 · pi = -pi/2
```

그런데 Assertion 은 **`-pi/2 < Phi`** 로 **`−π/2` 를 제외**한다. 원문 수식과 원문 Assertion이
서로 모순이다(`compactness` 의 "clipped to 1.0" 문구와 같은 유형의 원문 결함).

- **잠정 결론: `+π/2` 로 접는다.** `−π/2` 와 `+π/2` 는 **같은 축**을 가리키며,
  Assertion 이 지정한 반개구간의 대표값은 `+π/2` 다.
- 코드 스위치: `GLIM_HALCON_PHI_FOLD_HALF_PI` (기본 `1` = 접는다).
- 판정 시험: **S4T(세로선)** 의 `Phi`. HALCON 이 `+1.5708` 이면 기본값이 맞고,
  `−1.5708` 이면 스위치를 `0` 으로 내린다. **한 번의 실측으로 확정된다.**

---

## 9. `eccentricity` — 이심률 3종

### 9.1 시그니처

```
eccentricity(Regions : : : Anisometry, Bulkiness, StructureFactor)
```

출력 **3개**다.

| 출력 | 의미 | Assertion |
|---|---|---|
| `Anisometry` | "Shape feature (in case of a circle = 1.0)." | `Anisometry >= 1.0` |
| `Bulkiness` | "Calculated shape feature." | 없음 |
| `StructureFactor` | "Calculated shape feature." | 없음 |

### 9.2 원문 정의

> "… where Ra and Rb denote the ellipse radii (see elliptic_axis) and A denotes the area of the
> region (see area_center)."

```
Anisometry      = Ra / Rb
Bulkiness       = pi * Ra * Rb / A
StructureFactor = Anisometry * Bulkiness - 1
```

> **Attention**: "… This can lead to non-empty regions that have Rb = 0. In these cases,
> **the output features that require a division by Rb are set to 0.** In particular, regions that
> contain a single point or regions whose points lie exactly on a straight line (e.g., one pixel
> high horizontal regions or one pixel wide vertical regions) **have an anisometry of 0.**"

근거: `docs/reference/halcon13/G2_shape_factors.md` §6

### 9.3 수식

```
Ra, Rb : elliptic_axis (§8)          A : area_center 의 Area (§1, 픽셀 개수)

Anisometry      = (Rb > 0) ? Ra / Rb : 0.0        ← 함정 #12. Assertion(>=1)을 어기는 것이 정답
Bulkiness       = pi · Ra · Rb / A
StructureFactor = Anisometry · Bulkiness - 1
```

#### ⚠ 함정 #12 — Assertion 을 어기는 동작을 **그대로 재현**한다

문서상 Assertion 은 `Anisometry >= 1.0` 인데, `Rb = 0` 일 때 **실제 동작은 `0`** 이라고
같은 문서의 Attention 이 명시한다. **문서가 자기모순이며, 명시된 실제 동작(0)을 따른다.**
여기에 "Assertion 을 지키려고" `1.0` 을 넣으면 1픽셀 두께 스크래치가
"완전한 원(Anisometry=1)" 으로 둔갑한다. 검사기에서 가장 위험한 오판이다.

편의 항등식(둘 다 `Rb > 0` 일 때만 유효):

```
StructureFactor = pi · Ra^2 / A  -  1
```

### 9.4 구현 지침

```
1. F <= 0 (빈 리전) → 세 출력 전부 0.0 하고 즉시 반환      ← StructureFactor 도 0. -1 이 아니다
2. Ra, Rb 를 §8 에서 가져온다 (캐시 공유)
3. Anisometry = (Rb > 0) ? Ra/Rb : 0.0
4. Bulkiness  = pi·Ra·Rb/A        (Rb = 0 이면 자연히 0. 별도 분기 불필요)
5. StructureFactor = Anisometry·Bulkiness - 1
```

**빈 리전과 축퇴 리전을 구분해야 한다.**

| 도형 | Anisometry | Bulkiness | StructureFactor |
|---|---|---|---|
| 빈 리전 (`F = 0`) | 0 | 0 | **0** (§0.4 빈 리전 규약이 우선) |
| 단일 픽셀 / 1픽셀 두께 직선 (`F > 0`, `Rb = 0`) | 0 | 0 | **−1** (기본 해석. EC-1 참조) |

### 9.5 미결 쟁점

| # | 쟁점 | 상태 |
|---|---|---|
| **EC-1** | `Rb = 0` 일 때 `StructureFactor` 가 `−1` 인가 `0` 인가 | 🟡 잠정: **`−1`** (수식 그대로). 스위치 `GLIM_HALCON_STRUCTURE_FACTOR_ZERO_ON_RB0` (기본 `0`) |

#### EC-1 상세

원문 Attention 은 "the output **features**(복수) that require a division by Rb are set to 0" 라고 쓰고,
바로 다음 문장에서 "**in particular** … have an **anisometry** of 0" 로 `Anisometry` 하나만 예시한다.

| 해석 | `StructureFactor` | 논거 |
|---|---|---|
| **(a) 수식 그대로** | `0 · 0 − 1 = ` **`−1`** | 인쇄된 수식에 `Rb` 나눗셈이 직접 등장하지 않는다. `Anisometry` 를 0 으로 대체한 뒤 식을 그대로 평가한 결과 |
| (b) 함께 0 | `0` | "features" 가 복수다. `Anisometry` 를 경유하므로 간접적으로 `Rb` 나눗셈에 의존한다 |

**(a) 를 기본값으로 택한 이유** — 이 저장소가 이미 채택한 원칙이다.
`Convexity` 에 1.0 클램프를 일부러 넣지 않은 것과 같은 판단이다
(`04_IMPLEMENTATION_NOTES.md` §2.3: 덮어쓰면 회귀를 영원히 감지할 수 없다).

- (a) 를 기본으로 두고 HALCON 이 `0` 이면 → 출력에 `−1` 이라는 **눈에 띄는 이상값**이 보여 즉시 발견된다.
- (b) 를 기본으로 두고 HALCON 이 `−1` 이면 → `0` 은 빈 리전 규약과 구분되지 않아 **영원히 안 보인다.**

판정 시험: **S4 가로선** 또는 **S6 단일픽셀**의 `StructureFactor`. `−1` 이면 (a), `0` 이면 (b).

---

## 10. `orientation_region` — 리전 방향

### 10.1 시그니처

```
orientation_region(Regions : : : Phi)        Assertion: -pi <= Phi && Phi < pi
```

### 10.2 원문 정의 (전문)

> "The operator orientation_region calculates the orientation of the region. The operator is
> **based on elliptic_axis**. In addition **the point on the contour with maximal distance to the
> center of gravity** is calculated. If, **in the rotated coordinate system**, the column coordinate
> of this point is less than the column coordinate of the center of gravity, **the value of π is
> added to the angle**."

근거: `docs/reference/halcon13/G2_shape_factors.md` §8

### 10.3 수식

```
1. Phi0 = elliptic_axis 의 Phi                     (범위 -pi/2 < Phi0 <= pi/2)
2. (r0, c0) = 무게중심,  (rm, cm) = 무게중심에서 가장 먼 **외곽 윤곽 픽셀**
3. dr = rm - r0 ,  dc = cm - c0
4. proj = dc·cos(Phi0) - dr·sin(Phi0)              ← 회전 좌표계에서의 column 좌표
5. Phi = (proj < 0) ? Phi0 + pi : Phi0
6. Phi 를 [-pi, pi) 로 접는다
```

#### 4번 식의 유도 (OR-1)

§8.3 에서 `Phi` 에 대응하는 주축 단위벡터는 이미지 좌표 `(col, row)` 로 `u = (cos Phi, −sin Phi)` 다.
"회전 좌표계의 column 좌표" 는 **주축 방향 성분**, 즉 `(dc, dr)` 와 `u` 의 내적이다.

```
proj = (dc, dr) · (cos Phi, -sin Phi) = dc·cos(Phi) - dr·sin(Phi)
```

무게중심 자신의 회전 column 좌표는 0 이므로 "이 점의 column < 중심의 column" 은
곧 **`proj < 0`** 이다.

의미: **`Phi` 가 최원점 쪽을 가리키도록 방향을 확정한다.** `elliptic_axis` 의 `Phi` 는
축(직선)만 정하고 두 방향 중 어느 쪽인지는 정하지 못한다. 그 π 주기 모호성을 없애는 것이
이 오퍼레이터의 존재 이유다.

### 10.4 구현 지침

- 최원 윤곽점은 **`circularity` 의 `max` 와 같은 집합**(외곽 윤곽 픽셀)에서 찾는다.
  같은 순회에서 거리와 점을 함께 구해 **두 값이 어긋날 여지를 없앤다.**
- 윤곽이 비어 있으면(이론상 발생하지 않으나) `Phi0` 를 그대로 돌려준다.
- 단일 픽셀: 최원점이 자기 자신이라 `proj = 0`. `0 < 0` 이 거짓이므로 `+π` 가 붙지 않는다 → `Phi = 0`.
- `Phi + π` 는 `(π/2, 3π/2]` 가 되어 Assertion 범위를 벗어날 수 있다. **접는다**(OR-2).

### 10.5 미결 쟁점

| # | 쟁점 | 상태 |
|---|---|---|
| **OR-1** | "회전 좌표계에서의 column 좌표" 의 부호 규약 | 🟡 잠정: `proj = dc·cos Φ − dr·sin Φ`. 스위치 `GLIM_HALCON_ORIENTATION_PROJ_SIGN` (기본 `+1`) |
| **OR-2** | `+π` 후 `[-π, π)` 를 벗어나는 값의 처리 | 🟡 잠정: **2π 를 빼서 접는다.** 원문에 명시 없음. Assertion `-pi <= Phi < pi` 를 지키려면 접는 수밖에 없다. 스위치 `GLIM_HALCON_ORIENTATION_WRAP` (기본 `1`) |
| **OR-3** | 최원 윤곽점이 **동률**일 때 (대칭 도형) | ⚠ **원문 미명시이며 원리적으로 결정 불가.** 구현 정의: **윤곽 순회 순서상 먼저 나온 점**(엄격 부등호 `>` 갱신). 기준 도형 S1~S7 이 **전부 대칭**이라 이 문제에 걸린다 → 판정용 비대칭 도형 **S8** 을 추가했다 |

#### OR-1 잠정 근거

대안은 `proj = dc·cos Φ + dr·sin Φ` 다. 이것은 주축이 아니라 **주축을 row 축에 대해 반사한
엉뚱한 축**에 투영하는 식이며, 그 축은 리전의 방향과 아무 관계가 없다.
S8(비대칭 아령, 좌상 작은 사각 + 우하 큰 사각)에서 두 해석이 명확히 갈린다.

| 해석 | `proj` | 결과 `Phi` | 의미 |
|---|---|---|---|
| **`−` (기본)** | `−45.96` | `3π/4 = 2.3562` | 최원점(좌상단)을 가리킨다 ✅ |
| `+` | `0` | `−π/4 = −0.7854` | 최원점 반대쪽(우하단)을 가리킨다 ❌ |

#### OR-3 — 대칭 도형에서는 판정하지 않는다

`Phi0 = 0` 인 정사각형에서 네 모서리가 동률이므로 `Phi` 는 `0` 또는 `−π` 가 되며,
**어느 쪽이든 옳다**(같은 축, 대칭이라 방향 구분이 무의미). HALCON 역시
`rectangularity` 원문에서 "정사각형은 2차 모멘트로 방향을 정할 수 없다" 고 인정한다.

→ 검증에서는 **대칭 도형에 정확값을 요구하지 않고 `{Phi0, wrap(Phi0+π)}` 집합 소속만 검사**한다
(`docs/03_VERIFICATION.md` T-10). 정확값 판정은 **S8 하나가 담당**한다.

---

## 11. 정의 확정 이력

| 날짜 | 변경 | 근거 |
|---|---|---|
| 2026-07-20 | 초판. 5종 정의 작성, 미결 쟁점 6건 등록 | `docs/reference/halcon13/` G1·G2 원문 |
| 2026-07-20 | **5종 정의 확정.** 쟁점 6건 중 4건 원문 논리로 해소, 2건 구현 고정 | ↓ |
| 2026-07-28 | **2차 배치 5종 정의 작성**(§6~§10). 확정 3건(M2-1 / EA-1 / OR-1 유도) · 미결 4건(MI-1 / EA-2 / EC-1 / OR-2) · 판정불가 1건(OR-3) 등록. 판별 도형 S1b · S4T · S8 추가 | G2 §6·§7·§8 · G4 §1·§2 |

확정 내역 — 1차 배치 (§1~§5):

| 쟁점 | 결론 | 해소 방법 |
|---|---|---|
| C-1 `contlength` 체인 모델 | 픽셀 중심 체인 (단일픽셀 = 0) | 원문의 **√2 대각 가중치**가 체인코드를 전제 — 실측 불필요 |
| C-2 이웃 정의 4/8 | 8-이웃 고정 + 스위치 | 원문 미명시. 기본값 선택 |
| CI-1 구멍 윤곽 포함 | 외곽만 | `contlength` 와 일관, 실무 영향 없음 |
| CI-2 중심의 정의 | 무게중심 | `roundness` / `orientation_region` 원문 표현과 대조 |
| CV-1 hull 면적 단위 | 픽셀 개수 (래스터화) | **Assertion `Convexity <= 1`** 이 폴리곤 해석을 배제 — 실측 불필요 |
| CV-2 hull 입력 | contour 픽셀 | 결과 동일, 성능 선택 |

> **5종 모두 구현 가능 상태다.** 남은 잔여 리스크는 `contlength` 체인의 닫힘 처리
> (396 vs 395) 1건뿐이며, HALCON 실측이 가능해지면 그때 1스텝 보정 여부만 확인하면 된다.

확정 내역 — 2차 배치 (§6~§10):

| 쟁점 | 결론 | 해소 방법 |
|---|---|---|
| M2-1 `Ia`/`Ib` 의 차원 | **공분산 고유값**(길이²). 파라미터 설명("length of axis")은 부정확 | Calculation 절이 정본. `Ra = 2·sqrt(Ia/F)` 로 "길이" 와 연결된다 — 실측 불필요 |
| EA-1 `elliptic_axis` 의 정규화 인자 | **`F`(1제곱)** | 연속 원판에서 `Ra = R` 이 성립하는 유일한 값. `F²` 면 반지름이 상수가 된다 — 실측 불필요 |
| OR-1 회전 좌표계 투영 부호 | `proj = dc·cos Φ − dr·sin Φ` | Φ 의 정의로부터 주축 단위벡터가 `(cos Φ, −sin Φ)` 로 **유도**된다 — 실측 불필요(스위치는 남김) |
| MI-1 `_invar` 의 원점 | 🟡 잠정 **무게중심** | "scaled" 라는 표현·`_invar` 라는 이름과의 정합. **S5 로 실측 판정 가능** |
| EA-2 `Phi = −π/2` 처리 | 🟡 잠정 **`+π/2` 로 접음** | Assertion `-pi/2 < Phi`. **S4T 로 실측 판정 가능** |
| EC-1 `Rb=0` 의 `StructureFactor` | 🟡 잠정 **`−1`** | 수식 그대로. 틀렸을 때 눈에 띄는 쪽을 기본값으로. **S4/S6 으로 실측 판정 가능** |
| OR-2 `+π` 후 범위 접기 | 🟡 잠정 **접는다** | Assertion `-pi <= Phi < pi` 를 지키는 유일한 방법 |
| OR-3 최원점 동률 | ⚠ **판정 불가**(원문 미명시). 구현 정의 = 순회 순서상 첫 점 | 대칭 도형에는 정확값을 요구하지 않는다. 정확값 판정은 **S8** |

---

## 12. 구현 의존성

**1차·2차 배치 열 종 모두 OpenCV 없이 순수 C++ 만으로 구현한다.**

| 필요 알고리즘 | 쓰는 곳 | 구현 방식 | OpenCV 대체 이유 |
|---|---|---|---|
| 리전 표현 | 전부 | 런렝스 (자체) | `cv::Mat` 은 공개 헤더에 못 나온다 |
| 외곽 윤곽 추출 | 1차 4종 · `orientation_region` | Moore 이웃 경계추적 (자체) | `cv::findContours` 는 체인 정의가 다르고, 반환 좌표가 근사 폴리곤 |
| 윤곽 길이 | `contlength` · `compactness` | 체인 스텝 직접 합산 | `cv::arcLength` 는 근사 폴리곤 길이라 **√2 체인 정의와 불일치** |
| convex hull | `convexity` | Andrew monotone chain (자체) | 좌표만 필요, 의존성 추가 이유 없음 |
| hull 면적 | `convexity` | 스캔라인 래스터화 후 픽셀 카운트 | `cv::contourArea` 는 폴리곤 면적이라 **CV-1 결론과 불일치** |
| **2차 중심 모멘트** | **2차 배치 5종 전부** | 런렝스 등차·제곱합 + 정수 원점 시프트 (자체) | `cv::moments` 는 **다각형/이미지 모멘트**라 리전 픽셀 합과 정의가 다르다. 정규화 규약도 HALCON 과 다르다 |

> **2차 배치는 새 인프라를 요구하지 않는다.** 필요한 것은 위 마지막 행 하나뿐이며,
> 그것도 이미 있는 런렝스 표현 위에서 O(런 수) 로 끝난다. 이 점이 다섯 종을
> 한 배치로 묶은 이유이기도 하다 (`00_OVERVIEW.md` §5).
>
> 결과적으로 이 라이브러리는 여전히 **외부 의존성이 0** 이다. 어떤 컴파일러에서도
> 빌드·검증할 수 있고, 사용처의 OpenCV 버전과 충돌할 여지가 없다.

---

← 이전: [아키텍처](01_ARCHITECTURE.md)  |  [🏠 홈](../README.md)  |  다음: [정의 검증 계획 →](03_VERIFICATION.md)
