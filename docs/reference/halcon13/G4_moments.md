# Halcon 13 모멘트 오퍼레이터 7종 정확 정의 (G4)

- 출처: HALCON Operator Reference 13.0.4 (https://www.mvtec.com/doc/halcon/13/en/<operator>.html)
- 수집일: 2026-07-20
- 수식은 원문 페이지의 SVG(TeX Computer Modern 글리프)를 좌표/폰트 단위로 디코딩해 ASCII로 재구성한 것이다.
  원문에는 텍스트 수식이 존재하지 않고 전부 벡터 렌더링이므로, 아래 ASCII 수식은 **글리프 단위 1:1 복원**이다.
- 문서에 없는 내용은 전부 "문서에 명시 없음"으로 표기했다. 추측은 넣지 않았다.

## 공통 사항 (7개 전부 동일)

- 입력: `Regions` (input_object) region(-array) - "Regions to be examined." (2nd만 "Input regions.")
- 출력: 전부 real(-array) → HTuple
- 다중 리전 입력 시 결과는 튜플로 반환되며, 튜플 인덱스는 입력 리전 인덱스에 대응한다.
- 빈 리전(empty region)이 입력되면 다른 동작이 설정되지 않은 한 모든 파라미터가 **0.0**으로 반환된다
  (`set_system('empty_region_result',<Result>)`).
- 빈 입력(no input regions)의 동작은 `set_system('no_object_result',<Result>)`로 설정. 필요 시 예외 발생.
- 반환값: 입력이 비어있지 않으면 2 (H_MSG_TRUE).
- Complexity: 리전 면적을 F(또는 Z)라 할 때 평균 실행 복잡도 O(sqrt(F)).
- Parallelization: Multithreading type = reentrant, scope = global, tuple 레벨 자동 병렬화.
- Possible Predecessors: `threshold`, `regiongrowing`, `connection`
- See also: `elliptic_axis`
- Module: Foundation
- **출력값의 수치 범위(min/max)를 명시한 곳은 7개 페이지 어디에도 없음** → "문서에 명시 없음"
- **Attention 절은 7개 페이지 모두 존재하지 않음** → "문서에 명시 없음"
- **참고문헌(References)은 `moments_region_2nd` 한 곳에만 존재**. 나머지 6개는 References 절 자체가 없음.

---

## 1. moments_region_2nd

### 시그니처
```
moments_region_2nd(Regions : : : M11, M20, M02, Ia, Ib)
```
(HDevelop 기준. C++ 클래스 인터페이스는 `double HRegion::MomentsRegion2nd(double* M20, double* M02, double* Ia, double* Ib) const`
- 즉 M11이 반환값이고 나머지 4개가 out 파라미터)

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| M11 | "Product of inertia of the axes through the center parallel to the coordinate axes." (중심을 지나고 좌표축에 평행한 축들의 관성적 / 관성곱) |
| M20 | "Moment of 2nd order (row-dependent)." (행 의존) |
| M02 | "Moment of 2nd order (column-dependent)." (열 의존) |
| Ia | "Length of the major axis of the input region." |
| Ib | "Length of the minor axis of the input region." |

### 설명 원문
> moments_region_2nd calculates the geometric moments M11, M20 and M02 of the input regions in Regions.
> Furthermore the major and minor axis of the input regions are calculated and returned in Ia and Ib.
>
> The covariance matrix is given by:

공분산 행렬 (2x2):
```
[ M20  M11 ]
[ M11  M02 ]
```

> The row-dependent moment of 2nd order is returned in M20 and the column-dependent moment of 2nd order
> is returned in M02. The moment M11 represents the covariance between the row and column coordinates
> of the region points.
>
> Calculation:
> r0 and c0 are the coordinates of the center of gravity of a region R.
> Then the moments M_ij are defined by:

### 수식 (원문 글리프 복원)
```
M_ij = SUM_{(r,c) in R} (r0 - r)^i * (c0 - c)^j
```
> wherein r and c run over all pixels of the region R.

- **정규화 없음**: 면적 F로 나누지 않는다. 순수 합(sum)이다.
- **원점 = 무게중심**: (r0, c0)은 region R의 center of gravity(무게중심) 좌표.
- **p 지수 = row(행), q 지수 = column(열)**: 첫 번째 첨자 i가 (r0 - r)의 지수, 두 번째 첨자 j가 (c0 - c)의 지수.
  따라서 M20 = SUM (r0-r)^2, M02 = SUM (c0-c)^2, M11 = SUM (r0-r)(c0-c).
- 부호 주의: 원문은 (r0 - r), (c0 - c) 순서다. 즉 (중심 - 픽셀). 2차/짝수차에서는 무관하나 홀수차에서는 부호가 반대가 된다.

> Furthermore the length of the major and minor axes are defined by:
```
Ia = h + sqrt( h^2 - M20 * M02 + M11^2 )
Ib = h - sqrt( h^2 - M20 * M02 + M11^2 )
```
> wherein h is defined by:
```
h = (M20 + M02) / 2
```
> The equation for the major and minor axes can be derived from the definition of the moments
> by diagonalizing the covariance matrix and reforming the resulting formula.

- Ia / Ib 정의: **공분산 행렬의 고유값**(대각화 결과)에 해당. Ia는 `+sqrt`, Ib는 `-sqrt` 이므로 **Ia >= Ib**가 항상 성립
  (판별식이 실수인 한). Ia가 큰 값(major), Ib가 작은 값(minor).
- 단, 파라미터 설명은 "Length of the major/minor axis"라고 쓰여 있으나 수식은 주축 방향의 2차 모멘트(고유값)이다.
  문서는 이 둘의 단위 차이를 별도로 언급하지 않음 → "문서에 명시 없음".
- `*` 기호는 원문에서 TeX cmsy 코드 3(asterisk operator)로 렌더링되어 있다(곱셈).

### 참고문헌 (원문 그대로)
```
R. Haralick, L. Shapiro
"Computer and Robot Vision"
Addison-Wesley, 1992, pp. 73-75
```

### Alternatives
`moments_region_2nd_invar`

---

## 2. moments_region_2nd_invar

### 시그니처
```
moments_region_2nd_invar(Regions : : : M11, M20, M02)
```
(Ia, Ib 없음 — 출력 3개)

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| M11 | "Product of inertia of the axes through the center parallel to the coordinate axes." |
| M20 | "Moment of 2nd order (line-dependent)." (2nd는 row-dependent, 여기서는 line-dependent로 표기 — 같은 뜻) |
| M02 | "Moment of 2nd order (column-dependent)." |

### 설명 원문
> The operator moments_region_2nd_invar calculates the **scaled moments** (M20, M02) and the procut [sic]
> of inertia of the axes through the center parallel to the coordinate axes (M11).
>
> Calculation:
> Z0 and S0 are the coordinates of the center of a region R with the area F.
> Then the moments M_ij are defined by:

(원문 오타 "procut" 그대로 표기. Z = Zeile(행), S = Spalte(열)의 독일어 약자로 보이나 문서에 그 설명은 없음 → "문서에 명시 없음")

### 수식 (원문 글리프 복원)
```
M_ij = (1 / F^2) * SUM_{(Z,S) in R} (Z0 - Z)^i * (S0 - S)^j
```
> wherein Z and S run through all pixels of the region R.

- **정규화: 면적 F의 제곱(F^2)으로 나눈다.** (F^1이 아니라 F^2. 지수는 i, j와 무관하게 고정 2)
- **원점 = 리전의 중심 (Z0, S0)**. 원문 표현은 "the coordinates of the center of a region R"
  (2nd 페이지의 "center of gravity"와 달리 여기서는 그냥 "center"로 표기됨).
- **Z(행) 지수 = i (첫 첨자), S(열) 지수 = j (두 번째 첨자)** — moments_region_2nd와 동일한 순서.
- 즉 `moments_region_2nd_invar = moments_region_2nd / F^2`.
  2차 모멘트(i+j=2)에서 F^(1+(i+j)/2) = F^2 이므로 스케일 불변이 된다(단, 이 유도 설명은 문서에 없음 → "문서에 명시 없음").

### Alternatives
`moments_region_2nd`

### 참고문헌
없음 (References 절 자체가 존재하지 않음)

---

## 3. moments_region_2nd_rel_invar

### 시그니처
```
moments_region_2nd_rel_invar(Regions : : : PHI1, PHI2)
```

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| PHI1 | "Moment of 2nd order." |
| PHI2 | "Moment of 2nd order." |

### 설명 원문
> The operator moments_region_2nd_rel_invar calculates the **scaled relative moments** (PHI1, PHI2).
>
> Calculation:
> The moments PHI1 and PHI2 are defined by:

### 수식 (원문 글리프 복원)
```
PHI1 = M20 + M02
PHI2 = (M20 - M02)^2 + 4 * M11^2
```

- PHI1 = 공분산 행렬의 trace, PHI2 = (고유값 차이)^2 에 해당하나, 이 해석은 문서에 없음 → "문서에 명시 없음".
- **여기 쓰인 M20, M02, M11이 `moments_region_2nd`(비정규화)의 값인지 `moments_region_2nd_invar`(F^2 정규화)의 값인지
  문서에 명시 없음.** 다만 연산자 설명이 "scaled relative moments"라고 되어 있고, `moments_region_2nd_invar`의 설명이
  "scaled moments"인 점만 문서에서 확인 가능하다.
- Complexity 절이 이 페이지에는 없음 (다른 6개에는 있음).

### Alternatives
`moments_region_2nd`

### 참고문헌
없음

---

## 4. moments_region_3rd

### 시그니처
```
moments_region_3rd(Regions : : : M21, M12, M03, M30)
```
(출력 순서 주의: M21, M12, M03, M30 — 사전순/차수순이 아님)

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| M21 | "Moment of 3rd order (line-dependent)." |
| M12 | "Moment of 3rd order (column-dependent)." |
| M03 | "Moment of 3rd order (column-dependent)." |
| M30 | "Moment of 3rd order (line-dependent)." |

### 설명 원문
> The operator moments_region_3rd calculates the **translation-invariant central moments**
> (M21, M12, M03, M30) of order p+q.
>
> Calculation:
> x and y are the coordinates of the center of a region R with the area Z.
> Then the moments M_pq are defined by:

### 수식 (원문 글리프 복원)
```
M_pq = SUM_{i=1} M * Z(x_i, y_i) * (x_i - x)^p * (y_i - y)^q
```
> wherein are
```
x = m_10 / m_00
y = m_01 / m_00
```

**중요 주의 (원문 렌더링 상태 그대로 기록):**
- 합 기호 Σ에는 **아래 첨자 `i = 1`만 있고 위쪽 첨자(상한)가 렌더링되어 있지 않다.** 대문자 `M`은 Σ 바로 뒤 베이스라인에
  독립 글리프로 놓여 있다. 즉 문서상 문자 그대로는 "SUM_{i=1} M Z(x_i,y_i) ..." 로 읽힌다.
  (`\sum_{i=1}^{M}` 의도로 보이는 조판 결함이나, **문서에는 M의 정의가 없다** → "문서에 명시 없음". HALCON 23.11
  버전 페이지에서도 동일하게 렌더링됨을 확인.)
- `Z(x_i, y_i)`의 정의도 문서에 없음. 본문에서는 Z를 "the area"로 소개했는데 수식에서는 Z(x_i, y_i) 형태의 함수로 쓰였다
  → 문서 내 불일치. "문서에 명시 없음".
- **정규화 없음** (면적으로 나누는 항이 수식에 없음).
- **원점 = 중심 (x, y)**, 그리고 그 중심은 `x = m10/m00`, `y = m01/m00` 로 명시적으로 정의된다.
  (여기서 m_pq는 비중심 raw 모멘트를 뜻하지만, m_pq 자체의 정의는 이 페이지에 없음 → "문서에 명시 없음".)
- **p 지수 = x (첫 좌표), q 지수 = y (두 번째 좌표)**: `(x_i - x)^p (y_i - y)^q`.
  파라미터 설명에서 M21/M30이 "line-dependent(행)"이고 M12/M03이 "column-dependent(열)"이므로,
  **x = line(행), y = column(열)** 에 대응한다.
- 부호 주의: 여기서는 (픽셀 - 중심) 순서 `(x_i - x)`로, `moments_region_2nd`의 (중심 - 픽셀)과 **반대**다.
  3차(홀수차)이므로 이 차이는 부호에 영향을 준다. 문서는 이 불일치를 언급하지 않음.

### Alternatives
`moments_region_2nd`

### 참고문헌
없음

---

## 5. moments_region_3rd_invar

### 시그니처
```
moments_region_3rd_invar(Regions : : : M21, M12, M03, M30)
```
(출력 이름/순서/설명은 `moments_region_3rd`와 완전히 동일)

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| M21 | "Moment of 3rd order (line-dependent)." |
| M12 | "Moment of 3rd order (column-dependent)." |
| M03 | "Moment of 3rd order (column-dependent)." |
| M30 | "Moment of 3rd order (line-dependent)." |

### 설명 원문
> The operator moments_region_3rd_invar calculates the **scale-invariant moments** (M21, M12, M03, M30).
>
> Calculation:
> Then the moments M_ij are defined by:

### 수식 (원문 글리프 복원)
```
M_pq = mu_pq / mu^3
```
> wherein
```
p + q >= 2
```
> and
```
mu = mu_00 = m_00
```

- **정규화: 중심 모멘트를 mu_00(= m_00 = 면적)의 3제곱으로 나눈다. 지수는 p, q와 무관하게 고정 3.**
  (본문 "Then the moments M_ij" 에서는 첨자를 i,j로 쓰고 수식에서는 p,q로 쓰는 표기 혼용이 있다.)
- 조건 `p + q >= 2` 명시 (원문은 `>` 와 `=` 두 글리프로 조판되어 있음).
- mu_pq(중심 모멘트) 자체의 정의는 이 페이지에 없음 → "문서에 명시 없음"
  (`moments_region_3rd`의 M_pq 정의를 참조해야 하나, 문서는 그 연결을 명시하지 않음).
- 참고: 일반적인 Hu 정규화는 p+q=3에서 mu_00^(1+(p+q)/2) = mu_00^2.5 이지만, **HALCON 문서는 3제곱으로 적혀 있다.**
  (원문 그대로 기록. 다른 해석은 문서에 없음.)

### Alternatives
`moments_region_2nd`

### 참고문헌
없음

---

## 6. moments_region_central

### 시그니처
```
moments_region_central(Regions : : : I1, I2, I3, I4)
```

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| I1 | "Moment of 2nd order." |
| I2 | "Moment of 2nd order." |
| I3 | "Moment of 2nd order." |
| I4 | "Moment of 3rd order." |

(주: I2, I3, I4는 실제로는 3차 중심모멘트를 포함하지만 문서의 파라미터 설명은 위와 같이 적혀 있다. 원문 그대로.)

### 설명 원문
> The operator moments_region_central calculates the **central moments** (I1, I2, I3, I4).
>
> Calculation: Then the moments I_i are defined by:

(원문에 "Then"의 선행 문장이 없다. **mu_pq의 정의는 이 페이지에 존재하지 않는다** → "문서에 명시 없음")

### 수식 (원문 글리프 복원)
```
I1 = mu20 * mu02 - mu11^2

I2 = (mu30 * mu03 - mu21 * mu12)^2
     - 4 * (mu30 * mu12 - mu21^2) * (mu21 * mu03 - mu12^2)

I3 = mu20 * (mu21 * mu03 - mu12^2)
     - mu11 * (mu30 * mu03 - mu21 * mu12)
     + mu02 * (mu30 * mu12 - mu21^2)

I4 = mu30^2 * mu02^3
     - 6 * mu30 * mu21 * mu11 * mu02^2
     + 6 * mu30 * mu12 * mu02 * (2 * mu11^2 - mu20 * mu02)
     + mu30 * mu03 * (6 * mu20 * mu11 * mu02 - 8 * mu11^3)
     + 9 * mu21^2 * mu20 * mu02^2
     - 18 * mu21 * mu12 * mu20 * mu11 * mu02
     + 6 * mu21 * mu03 * mu20 * (2 * mu11^2 - mu20 * mu02)
     + 9 * mu12^2 * mu20^2 * mu02
     - 6 * mu12 * mu03 * mu11 * mu20^2
     + mu03^2 * mu20^3
```

- **정규화 없음**: 어떤 항도 면적으로 나누지 않는다. mu_pq(중심 모멘트)의 다항식일 뿐이다.
- 원점: mu가 중심 모멘트이므로 무게중심 기준이지만, **이 페이지에는 그 정의(원점, 정규화 여부)가 전혀 기술되어 있지 않다**
  → "문서에 명시 없음".
- p/q 지수 순서(mu20이 행인지 열인지)도 이 페이지에는 명시 없음 → "문서에 명시 없음".
  (`moments_region_2nd` / `moments_region_3rd` 페이지의 관례를 따르면 첫 첨자 = 행/line.)
- 조판 주의: 원문 I3 항의 `mu03`은 `\mu_03` 형태로 조판되어 0과 3의 크기가 다르게 렌더링되지만 의미는 mu_03이다.

### Alternatives
`moments_region_2nd`

### 참고문헌
없음

---

## 7. moments_region_central_invar

### 시그니처
```
moments_region_central_invar(Regions : : : PSI1, PSI2, PSI3, PSI4)
```

### 파라미터 설명 (원문)
| 이름 | 원문 설명 |
|---|---|
| PSI1 | "Moment of 2nd order." |
| PSI2 | "Moment of 2nd order." |
| PSI3 | "Moment of 2nd order." |
| PSI4 | "Moment of 2nd order." |

(4개 모두 "Moment of 2nd order."로 동일하게 적혀 있음. 원문 그대로.)

### 설명 원문
> The operator moments_region_central_invar calculates the moments (PSI1, PSI2, PSI3, PSI4) that are
> **invariant under translation and general linear transformations**.
>
> Calculation: Then the moments psi_i are defined by:

### 수식 (원문 글리프 복원)
```
PSI1 = I1 / mu^4
PSI2 = I2 / mu^10
PSI3 = I3 / mu^7
PSI4 = I4 / mu^11
```

- I1..I4는 `moments_region_central`의 출력값(위 6번 참조). 단, **이 페이지에서 I1..I4를 명시적으로 연결해 설명하지는
  않는다** → 연결 근거는 기호 일치뿐.
- `mu`의 정의(= mu_00 = 면적)는 **이 페이지에 없음** → "문서에 명시 없음".
  (`moments_region_3rd_invar` 페이지에는 `mu = mu_00 = m_00`이 명시되어 있음.)
- **정규화 지수: 4 / 10 / 7 / 11** (각 다항식의 차수에 맞춘 affine 정규화).
- 조판 주의: 원문의 PSI2, PSI4 분모는 `\mu^10`, `\mu^11` 형태로 조판되어 지수의 첫 자리(1)만 위첨자 크기로,
  둘째 자리(0, 1)는 본문 크기로 렌더링된다. 지수는 각각 10과 11이다.
- "invariant under translation and general linear transformations" = **아핀 불변**(스케일뿐 아니라 일반 선형 변환에도 불변)
  이라고 문서가 직접 명시. 반면 `*_invar` 계열 중 `moments_region_2nd_invar`/`3rd_invar`는 "scaled" /
  "scale-invariant"로만 기술된다.

### Alternatives
`moments_region_2nd`

### 참고문헌
없음

---

## 정규화 방식 비교표 (핵심)

| 오퍼레이터 | 출력 (순서 그대로) | 원점 | 정규화(나누는 값) | 문서상 불변성 표현 | 지수 규칙(p/q) |
|---|---|---|---|---|---|
| moments_region_2nd | M11, M20, M02, Ia, Ib | 무게중심 (r0, c0) | **없음** (순수 SUM) | "geometric moments" (불변 언급 없음) | i=row(r0-r), j=col(c0-c) |
| moments_region_2nd_invar | M11, M20, M02 | 리전 중심 (Z0, S0) | **F^2** (면적 제곱, 고정) | "scaled moments" | i=row(Z0-Z), j=col(S0-S) |
| moments_region_2nd_rel_invar | PHI1, PHI2 | (해당 없음, M들의 조합) | **없음** (M20/M02/M11 조합만) | "scaled relative moments" | 해당 없음 |
| moments_region_3rd | M21, M12, M03, M30 | 중심 x=m10/m00, y=m01/m00 | **없음** | "translation-invariant central moments" | p=x=line, q=y=column |
| moments_region_3rd_invar | M21, M12, M03, M30 | (mu_pq 기반) | **mu_00^3** (고정 3제곱) | "scale-invariant moments" | p=line, q=column (3rd와 동일) |
| moments_region_central | I1, I2, I3, I4 | 문서에 명시 없음 (mu 기반) | **없음** | "central moments" | 문서에 명시 없음 |
| moments_region_central_invar | PSI1, PSI2, PSI3, PSI4 | 문서에 명시 없음 | **mu^4 / mu^10 / mu^7 / mu^11** (각각 다름) | "invariant under translation and general linear transformations" (아핀 불변) | 문서에 명시 없음 |

### invar vs rel_invar 차이 (문서 근거)
- 문서가 실제로 말하는 것은 다음 표현뿐이다.
  - `moments_region_2nd_invar` : "calculates the **scaled** moments" + 수식에 1/F^2 존재
  - `moments_region_2nd_rel_invar` : "calculates the **scaled relative** moments" + 수식은
    PHI1 = M20+M02, PHI2 = (M20-M02)^2 + 4*M11^2 (나눗셈 없음)
- **즉 문서상 차이는 "면적으로 정규화된 개별 2차 모멘트(invar)" vs "그 2차 모멘트들의 상대적 조합값(rel_invar)"이다.**
- rel_invar가 어떤 변환에 대해 상대 불변(relative invariant)인지, 어떤 가중치(weight)를 갖는지는
  **문서에 명시 없음**.
- 3차 계열에서는 `moments_region_3rd`(정규화 없음) vs `moments_region_3rd_invar`(mu^3로 나눔, "scale-invariant") 이며,
  rel_invar 버전은 존재하지 않는다.

### 값 범위 / 예외 조건
- 값 범위: 7개 전부 **문서에 명시 없음** (파라미터에 Assertion/Value Range 항목 자체가 없음).
- 예외 조건 (7개 공통):
  - 빈 리전 → 모든 출력 0.0 (`set_system('empty_region_result', ...)`로 변경 가능)
  - 빈 입력 → `set_system('no_object_result', ...)` 설정에 따름, 필요 시 예외 발생
  - 0으로 나누는 상황(면적 0, mu_00 = 0)에 대한 별도 언급은 **문서에 명시 없음**
  - Ia/Ib의 sqrt 내부가 음수가 될 경우에 대한 언급도 **문서에 명시 없음**

### 참고문헌 요약
- `moments_region_2nd`: R. Haralick, L. Shapiro, "Computer and Robot Vision", Addison-Wesley, 1992, pp. 73-75
- 나머지 6개: **참고문헌 절 없음**
