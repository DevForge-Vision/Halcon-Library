# Halcon 13 공식 문서 원문 수집 — 기하/런렝스 오퍼레이터 7종

출처: https://www.mvtec.com/doc/halcon/13/en/&lt;operator&gt;.html (2026-07-20 수집, HTML 원문 직접 파싱)

주의: MVTec 문서의 수식은 텍스트가 아니라 MetaPost SVG(글리프 참조)로 렌더링되어 있어, SVG 글리프 코드(TeX 폰트 인코딩: pcrr8r=ASCII, cmr10, cmmi10, cmsy10)를 디코딩하여 복원했다. 복원한 수식에는 그 사실을 명시한다.

---

## 1. inner_circle

### 시그니처
```
inner_circle(Regions : : : Row, Column, Radius)
```
C: `Herror inner_circle(const Hobject Regions, double* Row, double* Column, double* Radius)`

| 파라미터 | 종류 | 타입 | 원문 설명 |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined." |
| Row | output_control | circle.center.y(-array), real | "Line index of the center." |
| Column | output_control | circle.center.x(-array), real | "Column index of the center." |
| Radius | output_control | circle.radius(-array), real | "Radius of the inner circle." / Assertion: `Radius >= 0` |

### Description (원문)
> "The operator inner_circle determines the largest inner circle of a region. This is the biggest discrete circle region that completely fits into the region. For this circle the center (Row, Column) and the radius (Radius) are calculated."
>
> "If the position of the circle is ambiguous, the "first possible" position (as far upper left as possible) is returned."
>
> "The output of the operator is chosen in such a way that it can be used as an input for the operators disp_circle, gen_circle, and gen_ellipse_contour_xld."
>
> "If several regions are passed in Regions corresponding tuples are returned as output parameters. In case of an empty input region all parameters have the value 0.0 if no other behavior was set with set_system."

### Attention (원문)
> "If several inner circles are present at a region only the most upper left solution is returned."

### Complexity (원문)
> "If F is the area of the region and R is the radius of the inner circle the runtime complexity is O(sqrt(F) * R)."

### Result / 예외
> "The operator inner_circle returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in case of empty input (no input regions available) is set via the operator set_system('no_object_result',<Result>), the behavior in case of empty region is set via set_system('empty_region_result',<Result>)."

### 좌표 관례 정리
- Row/Column은 **원(내접원)의 중심 좌표**(실수형, discrete circle region 기준).
- Radius는 내접원의 반지름. inner_circle에는 smallest_circle과 같은 "+0.5 보정" 언급이 **문서에 없음**.
- 픽셀 중심 좌표 기준이라는 명시적 문장은 inner_circle 페이지에 **문서에 명시 없음** (smallest_circle/smallest_rectangle1/2에는 명시됨).

---

## 2. inner_rectangle1

### 시그니처
```
inner_rectangle1(Regions : : : Row1, Column1, Row2, Column2)
```

| 파라미터 | 종류 | 타입 | 원문 설명 |
|---|---|---|---|
| Regions | input_object | region(-array) | "Region to be examined." |
| Row1 | output_control | rectangle.origin.y(-array), integer (Hlong) | "Row coordinate of the upper left corner point." |
| Column1 | output_control | rectangle.origin.x(-array), integer | "Column coordinate of the upper left corner point." |
| Row2 | output_control | rectangle.corner.y(-array), integer | "Row coordinate of the lower right corner point." |
| Column2 | output_control | rectangle.corner.x(-array), integer | "Column coordinate of the lower right corner point." |

### Description (원문 전문)
> "The operator inner_rectangle1 determines the largest axis-parallel rectangle that fits into a region. The rectangle is described by the coordinates of the corner pixels (Row1, Column1, Row2, Column2)."
>
> "In the documentation of this chapter (Regions / Features), you can find an image illustrating regions which vary in the width and height of their inner rectangle."
>
> "If more than one region is passed in Regions the results are stored in tuples, the index of a value in the tuple corresponding to the index of the input region. For empty regions all parameters have the value 0 if no other behavior was set (see set_system)."

### 최대 내접 사각형의 기준
- 문서 원문은 "the largest axis-parallel rectangle that fits into a region"이라고만 서술한다.
- **면적 최대인지, 둘레 최대인지, 다른 기준인지는 문서에 명시 없음.** 알고리즘 서술도 없음.
- Attention/Note 섹션 **없음**. Complexity 섹션 **없음**.

### 좌표 관례
- 두 점은 "corner pixels"(모서리 **픽셀**)이며 정수형. 즉 Row1/Column1 = 좌상단 픽셀, Row2/Column2 = 우하단 픽셀.
- 픽셀 자체를 가리키므로 실질적으로 **inclusive**로 읽히지만, "inclusive/exclusive"라는 명시적 단어는 **문서에 명시 없음**.
- 폭/높이 계산식(예: Column2-Column1+1)에 대한 언급도 **문서에 명시 없음**.

### Result / 예외
> "The operator inner_rectangle1 returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in case of empty input (no input regions available) is set via the operator set_system('no_object_result',<Result>). The behavior in case of empty region (the region is the empty set) is set via set_system('no_object_result',<Result>). If necessary an exception is raised."
>
> (원문 그대로: 빈 region의 경우에도 `no_object_result`라고 적혀 있음 — 다른 오퍼레이터의 `empty_region_result`와 다르며 문서상 표기가 그러함.)

---

## 3. smallest_circle

### 시그니처
```
smallest_circle(Regions : : : Row, Column, Radius)
```

| 파라미터 | 종류 | 타입 | 원문 설명 |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined." |
| Row | output_control | circle.center.y(-array), real | "Line index of the center." |
| Column | output_control | circle.center.x(-array), real | "Column index of the center." |
| Radius | output_control | circle.radius(-array), real | "Radius of the surrounding circle." / Assertion: `Radius >= 0` |

### Description (원문)
> "The operator smallest_circle determines the smallest surrounding circle of a region, i.e., the circle with the smallest area of all circles containing the region. For this circle the center (Row, Column) and the radius (Radius) are calculated. The operator is applied when, for example, the location and size of circular objects (e.g., coins) which, however, are not homogeneous inside or have broken edges due to bad segmentation, has to be determined. The output of the operator is selected in such a way that it can be used as input for the operators disp_circle and gen_circle."
>
> "If several regions are passed in Regions corresponding tuples are returned as output parameter. In case of empty region all parameters have the value 0.0 if no other behavior was set (see set_system)."

### Attention (원문 — 매우 중요, +0.5 보정)
> "Internally, the calculation is based on the center coordinates of the region pixels. To take into account that pixels are not just infinitely small points but have a certain area, the calculated radius is enlarged by 0.5 before it is returned in Radius. This, in most cases, gives acceptable results. However, in the worst case (pixel diagonal) this enlargement is not sufficient. If one wants to ensure that the border of the input region completely lies within the circle, one had to enlarge the radius by [1/√2] instead of 0.5. Consequently, the value returned in Radius must be corrected by [1/√2 − 0.5]. However, this would also be only an upper bound, i.e., the circle with the corrected radius would be slightly too big in most cases."
>
> 대괄호 두 수식은 SVG 이미지로 되어 있어 글리프 디코딩으로 복원함: 첫 번째 = `1/sqrt(2)`, 두 번째 = `1/sqrt(2) - 0.5`.
>
> 그림 캡션: "The smallest surrounding circle of a region. Note that the calculation is based on the center coordinates of the region pixels and that 0.5 is added to the resulting radius."

### Complexity (원문 그대로, 괄호 오타 포함)
> "If F is the area of the region, then the mean runtime complexity is O(sqrt(F)."

### Result / 예외
> "The operator smallest_circle returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in case of empty input ... set_system('no_object_result',<Result>). The behavior in case of empty region (the region is the empty set) is set via set_system('empty_region_result',<Result>). If necessary an exception is raised."

---

## 4. smallest_rectangle1

### 시그니처
```
smallest_rectangle1(Regions : : : Row1, Column1, Row2, Column2)
```

| 파라미터 | 종류 | 타입 | 원문 설명 |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined." |
| Row1 | output_control | rectangle.origin.y(-array), integer (Hlong) | "Line index of upper left corner point." |
| Column1 | output_control | rectangle.origin.x(-array), integer | "Column index of upper left corner point." |
| Row2 | output_control | rectangle.corner.y(-array), integer | "Line index of lower right corner point." |
| Column2 | output_control | rectangle.corner.x(-array), integer | "Column index of lower right corner point." |

### Description (원문 전문)
> "The operator smallest_rectangle1 calculates the surrounding rectangle of all input regions (parallel to the coordinate axes). The surrounding rectangle is described by the coordinates of the corner pixels (Row1, Column1, Row2, Column2). **The calculation of the rectangle is based on the center coordinates of the region pixels.**"
>
> 그림 캡션: "The smallest axis aligned surrounding rectangle of a region. Note that the calculation is based on the center coordinates of the region pixels."
>
> "If more than one region is passed in Regions, the results are stored in tuples, the index of a value in the tuple corresponding to the index of a region in the input. In case of empty region all parameters have the value 0 if no other behavior was set (see set_system)."

### Attention (원문)
> "In case of empty region the result of Row1, Column1, Row2 and Column2 (all are 0) can lead to confusion."

### 좌표 관례 (inclusive 여부)
- 출력은 "coordinates of the corner pixels" = **모서리 픽셀의 좌표**이며 정수. 따라서 Row2/Column2는 영역에 속한 **마지막 픽셀**을 가리킨다(실질 inclusive).
- 단, "inclusive"/"exclusive"라는 단어 자체는 **문서에 명시 없음**.
- 계산이 픽셀 **중심** 좌표 기준임은 명시됨 → 즉 픽셀의 바깥 경계(예: Row2+1)까지 확장한 값이 아니다. 실제 폭/높이를 픽셀 수로 쓰려면 +1이 필요하지만 그 문장은 **문서에 명시 없음**.

### Complexity (원문)
> "If F is the area of the region the mean runtime complexity is O(sqrt(F))."

### Result / 예외
> "The operator smallest_rectangle1 returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in case of empty input (no input regions available) is set via the operator set_system('no_object_result',<Result>). The behavior in case of empty region (the region is the empty set) is set via set_system('empty_region_result',<Result>). If necessary an exception is raised."

Possible Predecessors: threshold, regiongrowing, connection, runlength_features
Possible Successors: disp_rectangle1, gen_rectangle1

---

## 5. smallest_rectangle2

### 시그니처
```
smallest_rectangle2(Regions : : : Row, Column, Phi, Length1, Length2)
```

| 파라미터 | 종류 | 타입(semantic type) | 원문 설명 / Assertion |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined." |
| Row | output_control | rectangle2.center.y(-array), real | "Line index of the center." |
| Column | output_control | rectangle2.center.x(-array), real | "Column index of the center." |
| Phi | output_control | rectangle2.angle.rad(-array), real | "Orientation of the surrounding rectangle (arc measure)" / Assertion: `- pi / 2 < Phi && Phi <= pi / 2` |
| Length1 | output_control | **rectangle2.hwidth**(-array), real | "First radius (half length) of the surrounding rectangle." / Assertion: `Length1 >= 0.0` |
| Length2 | output_control | **rectangle2.hheight**(-array), real | "Second radius (half width) of the surrounding rectangle." / Assertion: `Length2 >= 0.0 && Length2 <= Length1` |

### Description (원문 전문)
> "The operator smallest_rectangle2 determines the smallest surrounding rectangle of a region, i.e., the rectangle with the smallest area of all rectangles containing the region. For this rectangle the center, the inclination and the two radii are calculated. **The calculation of the rectangle is based on the center coordinates of the region pixels.**"
>
> 그림 캡션: "The smallest surrounding rectangle of a region. Note that the calculation is based on the center coordinates of the region pixels."
>
> "The operator is applied when, for example, the location of a scenery of several regions (e.g., printed text on a rectangular paper or in rectangular print (justified lines)) must be found. The parameters of smallest_rectangle2 are chosen in such a way that they can be used directly as input for the operators disp_rectangle2 and gen_rectangle2."
>
> "If more than one region is passed in Regions the results are stored in tuples ... In case of empty region all parameters have the value 0.0 if no other behavior was set (see set_system)."

### 핵심 좌표 관례
- **Length1, Length2는 반변(half length / half width)이다.** semantic type도 `hwidth`, `hheight`. 전체 변 길이는 각각 `2*Length1`, `2*Length2`.
- Length1 >= Length2가 항상 보장된다(Assertion). 즉 Length1이 장반경, Length2가 단반경.
- Phi 범위: `-pi/2 < Phi <= pi/2` (라디안, arc measure).
- Row/Column은 사각형 **중심**(실수). 픽셀 중심 좌표 기준 계산이므로 smallest_circle 같은 +0.5 보정은 **하지 않는다**(보정 언급 자체가 문서에 없음 → 픽셀 면적 미고려).
- Phi의 부호 규약(영상좌표계에서 시계/반시계)에 대한 별도 설명은 이 페이지에 **문서에 명시 없음**.

### Complexity (원문)
> "If F is the area of the region and N is the number of supporting points of the convex hull, the runtime complexity is O(sqrt(F) + N^2)."

### Result / 예외
> "The operator smallest_rectangle2 returns the value 2 (H_MSG_TRUE) if the input is not empty. ... empty input → set_system('no_object_result',<Result>), empty region → set_system('empty_region_result',<Result>). If necessary an exception is raised."

---

## 6. runlength_distribution

### 시그니처
```
runlength_distribution(Region : : : Foreground, Background)
```

| 파라미터 | 종류 | 타입 | 원문 설명 |
|---|---|---|---|
| Region | input_object | **region (단일, 배열 아님)** | "Region to be examined." |
| Foreground | output_control | integer-array | "Length distribution of the region (foreground)." |
| Background | output_control | integer-array | "Length distribution of the background." |

### Description (원문 전문)
> "The operator runlength_distribution calculates the distribution of the runs of a region of the fore- and background. The frequency of the occurrence of a certain length is calculated. **Runs of infinite length are not counted. Therefore the background are the holes of the region.** As many values are passed as set by the maximum length of fore- or background, respectively. The length of both tuples usually differs. **The first entry of the tuples is always 0 (no runs of the length 0).** If there are no blanks the empty tuple is passed at Background. Analogously the empty tuple is passed in case of an empty region at Foreground."

### 히스토그램 인덱스 정의
- 튜플의 인덱스 i = **런 길이 i의 발생 빈도**. 즉 인덱스는 0부터 시작하며 **index 0은 항상 0**("no runs of the length 0"). 실질적 데이터는 index 1부터.
- 튜플 길이 = 각각 전경/배경의 **최대 런 길이**에 의해 결정 ("As many values are passed as set by the maximum length of fore- or background, respectively"). 두 튜플 길이는 보통 다르다.

### Foreground / Background 구분
- Foreground = region 자체의 런.
- Background = region의 **구멍(holes)**. 무한 길이 런(영상 경계까지 이어지는 배경)은 세지 않기 때문에 결과적으로 배경은 구멍만 남는다.
- 구멍(blank)이 없으면 Background는 빈 튜플, 빈 region이면 Foreground가 빈 튜플.

### Complexity (원문)
> "If n is the number of runs of the region the runtime complexity is O(n)."

### Result / 예외 (원문)
> "The operator runlength_distribution returns the value 2 (H_MSG_TRUE) if the input is not empty. The behavior in case of empty input (no input regions available) is set via the operator set_system('no_object_result',<Result>). **If more than one region is passed an exception is raised.**"

Possible Predecessors: threshold, select_obj

---

## 7. runlength_features

### 시그니처
```
runlength_features(Regions : : : NumRuns, KFactor, LFactor, MeanLength, Bytes)
```
C: `Herror runlength_features(const Hobject Regions, Hlong* NumRuns, double* KFactor, double* LFactor, double* MeanLength, Hlong* Bytes)`

| 파라미터 | 종류 | 타입 | 원문 설명 / Assertion |
|---|---|---|---|
| Regions | input_object | region(-array) | "Regions to be examined." |
| NumRuns | output_control | integer(-array) (Hlong) | "Number of runs." / `0 <= NumRuns` |
| KFactor | output_control | real(-array) (double) | "Storing factor in relation to a square." / `0 <= KFactor` |
| LFactor | output_control | real(-array) (double) | "Mean number of runs per line." / `0 <= LFactor` |
| MeanLength | output_control | real(-array) (double) | "Mean length of runs." |
| Bytes | output_control | integer(-array) (Hlong) | "Number of bytes necessary for coding the region." |

### Description (원문 전문)
> "The operator runlength_features calculates for every input region from Regions the number of runs necessary for storing this region with the aid of runlength coding. Furthermore the so-called "K-factor" is determined, which indicates by how much the number of runs differs from the ideal of the square in which this value is 1.0."
>
> "The K-factor (KFactor) is calculated according to the formula:
>
> **KFactor = NumRuns / sqrt(Area)**
>
> wherein Area indicates the area of the region. It should be noted that the K-factor can be smaller than 1.0 (in case of long horizontal regions)."
>
> "The L-factor (LFactor) indicates the mean number of runs for each line index occurring in the region."
>
> "MeanLength indicates the mean length of the runs."
>
> "The parameter Bytes indicates how many bytes are necessary for coding the region with runlengths."

> K-factor 수식은 페이지에 MetaPost SVG로 삽입되어 있으며(텍스트 아님), 글리프 시퀀스를 디코딩한 결과 `KFactor` `=` `NumRuns`(분자, 분수선 존재) `√Area`(분모, 라디칼+오버바)로 확인됨. 즉 `KFactor = NumRuns / √Area`.

### 각 값의 정확한 정의 정리
- **NumRuns**: 런렝스 코딩으로 이 region을 저장하는 데 필요한 런의 개수. (수식 없음, 정의 그대로)
- **KFactor** = `NumRuns / sqrt(Area)` — Area는 region의 면적. 정사각형이 이상적일 때 1.0. 가로로 긴 영역에서는 1.0보다 작아질 수 있음.
- **LFactor**: **"the mean number of runs for each line index occurring in the region"** — 파라미터 설명은 "Mean number of runs per line."
  - **분모에 대한 원문 문장 그대로**: "The L-factor (LFactor) indicates the mean number of runs **for each line index occurring in the region**."
  - 즉 분모는 이미지 전체 행 수가 아니라 **region이 실제로 점유하는(등장하는) line index의 개수**로 읽힌다. 명시적인 수식(분수 형태)은 **문서에 명시 없음** — 서술 문장뿐임.
- **MeanLength**: "indicates the mean length of the runs." 명시 수식 **문서에 명시 없음** (Area/NumRuns라는 언급 없음).
- **Bytes**: "indicates how many bytes are necessary for coding the region with runlengths." 바이트 계산식(런당 몇 바이트인지) **문서에 명시 없음**.

### Attention (원문)
> "All features calculated by the operator runlength_features are not rotation invariant because the runlength coding depends on the direction. The operator runlength_features does not serve for calculating shape features but for controlling and analysing the efficiency of the runlength coding."

### Complexity (원문)
> "The mean runtime complexity is O(1)."

### Result / 예외 (원문)
> "The operator runlength_features returns the value 2 (H_MSG_TRUE) if the input is not empty. If necessary an exception is raised."
>
> (set_system 관련 빈 region 처리 문장은 이 페이지에 **없음**.)

Possible Predecessors: threshold, regiongrowing, connection

---

## 구현 시 핵심 주의사항 요약

1. **픽셀 중심 좌표 기준**: smallest_rectangle1 / smallest_rectangle2 / smallest_circle 모두 "based on the center coordinates of the region pixels". OpenCV의 boundingRect(x, y, w, h)는 픽셀 경계 기반이므로 직접 대응하지 않는다. Halcon Row2 = OpenCV (y + h - 1)에 해당.
2. **smallest_circle만 반지름 +0.5 보정**을 내부에서 수행한다. inner_circle, smallest_rectangle2에는 그런 보정 언급이 없다.
3. **smallest_rectangle2의 Length1/Length2는 반변**(hwidth/hheight). OpenCV minAreaRect의 size는 전체 변이므로 2배 차이. Phi 범위는 `(-pi/2, pi/2]`이고 Length1 >= Length2 보장.
4. **inner_rectangle1의 "largest" 기준은 문서 미정의** — 면적 최대라고 단정할 수 없다. 자체 구현 시 면적 최대 알고리즘을 쓰면 Halcon과 결과가 다를 수 있다.
5. **runlength 히스토그램 index 0은 항상 0**. Background는 구멍만 포함(무한 런 제외). Region 입력은 단일 region만 허용(복수 입력 시 예외).
6. **KFactor = NumRuns / sqrt(Area)** 이며, LFactor의 분모는 "each line index occurring in the region"(영상 전체 행 수가 아님). MeanLength, Bytes의 수식은 문서에 없다.
