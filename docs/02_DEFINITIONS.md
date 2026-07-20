# 02. 특징값 정의서

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

### 2.5 미결 쟁점 ⚠

| # | 쟁점 | 상태 |
|---|---|---|
| C-1 | 윤곽 체인이 **닫힌 경로**인가? 100×100 정사각형에서 396(열린 396스텝) 인가 396+? 인가 | **검증 필요** — `docs/03_VERIFICATION.md` T-2 |
| C-2 | 윤곽 픽셀 정의가 `set_system('neighborhood', 4/8)` 에 의존한다는 언급이 `roundness` 페이지에 있다. `contlength` 도 동일한가? | 원문 미명시 |

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

### 3.5 미결 쟁점 ⚠

| # | 쟁점 | 상태 |
|---|---|---|
| CI-1 | "contour pixels" 에 **구멍의 윤곽 픽셀**이 포함되는가? `contlength` 는 명시적으로 제외하지만 `circularity` 페이지에는 언급이 없다. 구멍 윤곽은 중심에 가까워 `max` 에 영향이 거의 없으므로 실무 영향은 작다 | 원문 미명시 / 영향 경미 |
| CI-2 | 중심이 **무게중심**인가 bounding box 중심인가? 원문은 "the center" 로만 기술 | `area_center` 의 무게중심으로 해석 (다른 페이지와의 일관성). **검증 필요** |

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

### 5.5 미결 쟁점 ⚠ (최우선 해결 대상)

| # | 쟁점 | 왜 중요한가 |
|---|---|---|
| **CV-1** | **`F_c` 가 "hull 폴리곤의 기하 면적" 인가, "hull 리전에 포함되는 픽셀 개수" 인가?** | 두 값이 다르다. HALCON 은 리전 연산 체계(`shape_trans(..., 'convex')`)를 가지므로 **픽셀 개수일 가능성이 높다.** `F_o` 는 명백히 픽셀 개수이므로, 분자·분모의 단위를 맞추려면 `F_c` 도 픽셀 개수여야 자연스럽다. 폴리곤 면적(`cv::contourArea`)을 쓰면 정사각형에서 `100*100=10000` 대신 `99*99=9801` 이 나와 **Convexity 가 1.02 로 Assertion 을 위반**한다 |
| CV-2 | hull 의 입력이 contour 픽셀인가 전체 픽셀인가 | 결과는 동일(hull 은 극점만 사용). 성능 문제일 뿐 |

> **CV-1 은 반드시 검증 후 구현한다.** 잠정 결론: **픽셀 개수(hull 리전 래스터화 후 카운트)**.
> 근거는 (a) 단위 일관성 (b) Assertion `<= 1` 위반 회피 (c) HALCON 의 리전 중심 설계.
> 검증 항목은 `docs/03_VERIFICATION.md` T-5.

---

## 6. 정의 확정 이력

| 날짜 | 변경 | 근거 |
|---|---|---|
| 2026-07-20 | 초판. 5종 정의 작성, 미결 쟁점 6건 등록 | `docs/reference/halcon13/` G1·G2 원문 |
