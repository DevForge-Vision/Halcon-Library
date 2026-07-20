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

## 6. 정의 확정 이력

| 날짜 | 변경 | 근거 |
|---|---|---|
| 2026-07-20 | 초판. 5종 정의 작성, 미결 쟁점 6건 등록 | `docs/reference/halcon13/` G1·G2 원문 |
| 2026-07-20 | **5종 정의 확정.** 쟁점 6건 중 4건 원문 논리로 해소, 2건 구현 고정 | ↓ |

확정 내역:

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

---

## 7. 구현 의존성 (Phase 1)

**Phase 1 다섯 종은 OpenCV 없이 순수 C++ 만으로 구현한다.**

| 필요 알고리즘 | 구현 방식 | OpenCV 대체 이유 |
|---|---|---|
| 리전 표현 | 런렝스 (자체) | `cv::Mat` 은 공개 헤더에 못 나온다 |
| 외곽 윤곽 추출 | Moore 이웃 경계추적 (자체) | `cv::findContours` 는 체인 정의가 다르고, 반환 좌표가 근사 폴리곤 |
| 윤곽 길이 | 체인 스텝 직접 합산 | `cv::arcLength` 는 근사 폴리곤 길이라 **√2 체인 정의와 불일치** |
| convex hull | Andrew monotone chain (자체) | 좌표만 필요, 의존성 추가 이유 없음 |
| hull 면적 | 스캔라인 래스터화 후 픽셀 카운트 | `cv::contourArea` 는 폴리곤 면적이라 **CV-1 결론과 불일치** |

> 결과적으로 Phase 1 은 **외부 의존성이 0** 이다. 어떤 컴파일러에서도 빌드·검증할 수 있고,
> 사용처의 OpenCV 버전과 충돌할 여지가 없다. OpenCV 는 이후 단계에서
> 정말 필요한 지점(예: 이미지 → 리전 변환 어댑터)에서만 도입한다.
