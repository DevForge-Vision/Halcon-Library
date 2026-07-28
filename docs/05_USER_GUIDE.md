# 05. 사용 매뉴얼 — 내 검사기 데이터를 이 라이브러리에 넣는 법

> 작성 2026-07-21. 대상: Phase 1 오퍼레이터 5종
> `AreaCenter` · `ContLength` · `Circularity` · `Compactness` · `Convexity`
> 및 배치 API `ComputeFeatures` (§4.6), 검사기 원샷 API `ComputeLargestBlobFeatures` (§3.6)
>
> 이 문서는 **사용법**만 다룹니다.
> 각 값의 **정의와 수식**은 [`02_DEFINITIONS.md`](02_DEFINITIONS.md),
> **구현 근거와 검증 결과**는 [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) 를 보세요.
> 이 문서에서 수식을 다시 정의하지 않습니다.

## 목차

1. [5분 시작하기](#1-5분-시작하기)
2. [빌드 / 링크 방법](#2-빌드--링크-방법)
3. [⭐ 데이터를 넣는 법](#3--데이터를-넣는-법)
4. [오퍼레이터 레퍼런스 5종 + 배치 API](#4-오퍼레이터-레퍼런스-5종--배치-api)
5. [⚠ 값 해석 시 함정](#5--값-해석-시-함정)
6. [성능 가이드](#6-성능-가이드)
7. [HALCON 에서 이식하기](#7-halcon-에서-이식하기)
8. [FAQ / 문제 해결](#8-faq--문제-해결)

---

## 1. 5분 시작하기

공개 헤더는 [`include/GlimHalcon.h`](../include/GlimHalcon.h) **하나뿐**입니다.
OpenCV·MFC·HALCON SDK 어느 것도 필요 없습니다.

```cpp
#include "GlimHalcon.h"

#include <cstdio>
#include <vector>

using namespace glim::halcon;

int main()
{
	// 1) 이진 마스크. row = y(아래로 증가), col = x(오른쪽으로 증가), 좌상단 (0,0), 0-based
	const int width = 8;
	const int height = 6;
	std::vector<unsigned char> mask(static_cast<size_t>(width) * height, 0);

	for (int row = 1; row <= 4; ++row)			// row 1..4, col 2..5 = 4x4 정사각형
	{
		for (int col = 2; col <= 5; ++col)
			mask[static_cast<size_t>(row) * width + col] = 255;
	}

	// 2) Region 생성 (stride = 한 행의 바이트 수. 0 이하이면 width 로 간주)
	Region region = Region::FromMask(&mask[0], width, height, width);

	// 3) 값 호출. 출력은 참조 인자다 (HALCON 시그니처와 1:1)
	long area = 0;
	double centerRow = 0.0;
	double centerColumn = 0.0;
	AreaCenter(region, area, centerRow, centerColumn);

	double contLength = 0.0;
	ContLength(region, contLength);

	double circularity = 0.0;
	Circularity(region, circularity);

	std::printf("Area        = %ld\n", area);
	std::printf("Row         = %.6f\n", centerRow);
	std::printf("Column      = %.6f\n", centerColumn);
	std::printf("ContLength  = %.6f\n", contLength);
	std::printf("Circularity = %.6f\n", circularity);
	return 0;
}
```

**실측 출력** (MSVC 14.44 / x64 / Release, 2026-07-21 — [`examples/01_quickstart.cpp`](../examples/01_quickstart.cpp) 실행 결과 그대로):

```
Area        = 16
Row         = 2.500000
Column      = 3.500000
ContLength  = 12.000000
Circularity = 1.000000
```

읽는 법:

- `Area = 16` — 4×4 이므로 픽셀 개수 16.
- `Row = 2.5`, `Column = 3.5` — row 1..4 의 중심 2.5, col 2..5 의 중심 3.5. **무게중심이므로 소수**입니다.
- `ContLength = 12` — 둘레 픽셀 12개를 **픽셀 중심끼리 이은 닫힌 체인**의 길이. 사각형 둘레 16 이 아닙니다(§5.2).
- `Circularity = 1.000000` — **1.0 에서 클리핑된 값**입니다. 원 계산값은 1.13 입니다(§5.3).

> 알아야 할 규약 하나: **예외는 절대 밖으로 나오지 않습니다.**
> 입력이 이상하거나 빈 리전이면 모든 출력이 조용히 `0` 이 됩니다. 반환값으로 성패를 알 수 없으므로,
> 필요하면 `region.IsEmpty()` 로 직접 확인하세요.

---

## 2. 빌드 / 링크 방법

### 2.1 소스 구성

```
include/GlimHalcon.h              ← 사용처가 include 하는 유일한 헤더
src/Core/Region.{h,cpp}
src/Core/Contour.{h,cpp}
src/Core/ConvexHull.{h,cpp}
src/Feature/FeatureContext.{h,cpp}
src/Facade/Operators.cpp
```

`.cpp` 는 **6개**입니다. 외부 라이브러리·전처리기 정의·링크 옵션이 하나도 필요 없습니다.

### 2.2 방법 A — 검사기 프로젝트에 소스를 직접 넣기 (가장 권장)

MFC 검사기에 붙일 때는 **static lib 를 만들지 말고 `.cpp` 6개를 프로젝트에 그대로 추가**하는 쪽이
사고가 훨씬 적습니다. 런타임 라이브러리(`/MD` vs `/MT`) 불일치, 디버그/릴리즈 CRT 혼용,
툴셋 차이 같은 링크 사고가 **원천적으로 발생하지 않기** 때문입니다.

절차:

1. 프로젝트에 `src\Core\*.cpp`, `src\Feature\*.cpp`, `src\Facade\Operators.cpp` 6개 추가
2. C/C++ ▸ 일반 ▸ **추가 포함 디렉터리**에 `include`, `src\Core`, `src\Feature` 추가
3. **⚠ 추가한 6개 파일 각각**: C/C++ ▸ 미리 컴파일된 헤더 ▸ **"미리 컴파일된 헤더 사용 안 함"**
   (MFC 프로젝트는 `stdafx.h` 강제 포함이 걸려 있어 그대로 두면 `C1010: unexpected end of file
   while looking for precompiled header` 가 납니다)

이 방법이면 x86/x64, 멀티바이트/유니코드, Debug/Release 어느 조합이든 그냥 됩니다.
라이브러리 코드는 `TCHAR`·`CString`·Windows API 를 전혀 쓰지 않으므로 문자셋 설정과 무관합니다.

### 2.3 방법 B — static lib 로 만들어 링크

```bash
# VS 2022 (v143) x64
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# VS 2015 (v140) x86  — 검사기가 x86 이면 이쪽
cmake -S . -B build32 -G "Visual Studio 14 2015" -A Win32
cmake --build build32 --config Release

# VS 2022 IDE 를 쓰되 툴셋만 v140 으로 맞추기
cmake -S . -B build140 -G "Visual Studio 17 2022" -A Win32 -T v140
```

만들어진 `GlimHalcon.lib` 를 링크하고, 사용처에 `include` 디렉터리를 추가하면 끝입니다.

> 검증 실적: `cmake -G "Visual Studio 18 2026" -A x64` + `--build --config Release` 로
> 라이브러리·테스트·예제 3종이 전부 빌드되고 `ctest` 가 통과하는 것을 확인했습니다(2026-07-21, CMake 4.3.0).
> `Visual Studio 14 2015` 제너레이터는 CMake 4.x 에서 **deprecated** 로 표시됩니다.
> VS2015 툴셋이 필요하면 최신 제너레이터 + `-T v140` 조합을 쓰거나, §2.2 방식을 쓰세요.

#### ⚠ 이 방법에서 실제로 밟는 함정 4가지

| 증상 | 원인 | 해결 |
|---|---|---|
| `LNK2038: mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match 'MD_DynamicRelease'` | 라이브러리와 검사기의 **런타임 라이브러리 설정 불일치**. CMake/MSVC 기본은 `/MD`(Release)·`/MDd`(Debug) 인데, MFC 검사기가 `/MT` 로 설정돼 있으면 충돌 | 양쪽을 맞춘다. CMake 쪽: `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`(=/MT) 또는 `MultiThreadedDLL`(=/MD). MFC "공유 DLL에서 MFC 사용" 이면 보통 `/MD` |
| `LNK2038: mismatch detected for '_ITERATOR_DEBUG_LEVEL': value '0' doesn't match value '2'` | **Debug 검사기에 Release 라이브러리**를 링크(또는 반대). 이 라이브러리는 `std::vector`·`std::shared_ptr` 를 쓰므로 STL ABI 가 구성마다 다르다 | Debug 는 Debug 빌드 lib, Release 는 Release 빌드 lib 를 링크한다. **Debug/Release 라이브러리를 각각 따로 빌드해서 따로 링크** |
| `LNK1112: module machine type 'x64' conflicts with target machine type 'X86'` | 비트수 불일치 | 검사기가 x86(코터 마스터, 슬리터 마스터·라벨서버)이면 `-A Win32`, x64(바타입, 커팅면)이면 `-A x64` 로 별도 빌드 |
| `C2065`·이상한 위치의 구문 오류 | 사용처가 `windows.h` 를 먼저 include 하고 `min`/`max` 매크로가 살아 있는 상태 | `GlimHalcon.h` 자체는 표준 헤더도 include 하지 않아 영향받지 않는다. 다만 라이브러리 `.cpp` 를 프로젝트에 직접 넣는 경우(§2.2) `stdafx.h` 강제 포함이 걸리면 문제가 될 수 있으니 §2.2 의 3번을 반드시 적용 |

**멀티바이트/유니코드**: `CMakeLists.txt` 는 `-D_MBCS` 를 정의하지만, 라이브러리 코드에는
`TCHAR` 계열이 전혀 없어서 실질적인 영향이 없습니다. 검사기가 유니코드여도 그대로 링크됩니다.

**툴셋 혼용**: v140 으로 빌드한 lib 를 v143 프로젝트에 링크하는 것은 STL 컨테이너가
경계를 넘나들지 않으므로 대체로 동작하지만(이 라이브러리의 공개 경계는 `long`/`double`/`unsigned char*` 뿐입니다),
권장하지 않습니다. **검사기와 같은 툴셋으로 빌드하세요.** 확신이 없으면 §2.2 로 가세요.

### 2.4 방법 C — 컴파일러 한 줄 (CMake 없이 확인만)

```bat
cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature /I test ^
   src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp test\TestMain.cpp ^
   /Fe:glimtest.exe
```

`glimtest.exe` 의 **exit code 가 0** 이면 기준 도형 검증 통과입니다.

### 2.5 소스 인코딩

라이브러리·예제의 `.cpp/.h` 는 한글 주석이 있어 **UTF-8 BOM** 으로 저장돼 있습니다.
BOM 을 지우면 Visual Studio 가 CP949 로 오독해 **주석 다음 줄을 통째로 삼킵니다**
(컴파일은 성공하는데 코드가 사라지는 유형의 사고입니다). 편집기 설정을 확인하세요.

---

## 3. ⭐ 데이터를 넣는 법

이 장이 이 문서의 핵심입니다.

### 3.0 좌표계 규약 — 먼저 이것부터 못 박습니다

```
row  = y : 위에서 아래로 증가          (0 .. height-1)
col  = x : 왼쪽에서 오른쪽으로 증가    (0 .. width-1)
원점 = 이미지 좌상단 (row=0, col=0)
0-based
```

| | 순서 | 예 |
|---|---|---|
| **이 라이브러리 / HALCON** | **(row, col) = (y, x)** | `AreaCenter(region, area, row, column)` |
| OpenCV `cv::Point` | (x, y) | `cv::Point(col, row)` |
| OpenCV `cv::Mat::at` | (y, x) | `mat.at<uchar>(row, col)` ← 이건 같은 순서 |

> **가장 흔한 사고**: `AreaCenter` 가 돌려준 `(row, column)` 을 그대로 `cv::Point` 나
> `MoveTo(x, y)` 에 넣는 것. **x = column, y = row** 로 바꿔서 넣어야 합니다.

버퍼 규약:

- **행 우선(row-major)**, 좌상단부터 저장
- **0 이 아닌 모든 값이 전경**입니다. 255 든 1 든 17 이든 전부 같습니다. 임계값 개념이 없습니다
- `stride` = 한 행의 **바이트 수**. `stride <= 0` 이면 `width` 로 간주합니다
- 라이브러리는 **8bit 단일 채널만** 받습니다. 16bit·컬러 버퍼는 사용처에서 미리 이진화해야 합니다

### 3.1 공개 입력 API — 이게 전부입니다

```cpp
class Region
{
public:
	Region();											// 빈 리전
	Region(const Region& other);						// 복사는 O(1) (내부 데이터 공유)
	Region& operator=(const Region& other);

	static Region FromMask(const unsigned char* data, int width, int height, int stride);
	static Region FromRuns(const int* rows, const int* colBegins, const int* colEnds, int count);

	bool IsEmpty() const;
};
```

Region 을 만드는 길은 **`FromMask` 와 `FromRuns` 둘뿐**입니다.
그 외 형태(좌표 목록, `cv::Mat`, `CRect` 목록 등)는 이 둘 중 하나로 **사용처에서 변환**합니다.
아래에 케이스별 변환 코드가 있습니다.

### 3.2 케이스 A — 이진 마스크 버퍼 (`unsigned char*` + width/height/stride)

가장 직접적인 경로입니다.

```cpp
// 검사기가 이미 들고 있는 이진화 결과 버퍼
unsigned char* maskData = ...;		// 0 = 배경, 그 외 = 전경
int width  = 1024;
int height = 768;
int stride = 1024;					// 행 패딩이 있으면 그 값. 없으면 width (또는 0)

glim::halcon::Region region = glim::halcon::Region::FromMask(maskData, width, height, stride);
```

- **패딩(stride > width)** 이 있어도 그대로 넘기면 됩니다. `width` 열까지만 읽으므로
  패딩 영역에 쓰레기 값이 있어도 결과에 섞이지 않습니다.
  ([`examples/02_from_binary_mask.cpp`](../examples/02_from_binary_mask.cpp) 의 `Padded(st=48)` 행이
  패딩 없는 `Rectangle` 과 **완전히 같은 값**을 내는 것으로 실증돼 있습니다)
- Windows DIB 처럼 **bottom-up** 으로 저장된 버퍼를 그대로 넘기면 row 가 상하 반전됩니다.
  `Row` 값이 뒤집혀 나오므로, 뒤집힌 버퍼는 사용처에서 정방향으로 맞춰 넘기거나 결과를 보정하세요.
- **`stride` 규약**
  | `stride` | 동작 |
  |---|---|
  | `stride <= 0` | `width` 로 간주합니다 (필요 버퍼 = `width × height`) |
  | `stride >= width` | 정상. 패딩 영역은 읽지 않습니다 |
  | `0 < stride < width` | **빈 리전을 반환합니다.** 버퍼를 읽지 않습니다 |

  `0 < stride < width` 는 행 우선 버퍼로 성립할 수 없는 값입니다. 그대로 읽으면 각 행이 다음 행을
  침범하고 마지막 행에서는 **버퍼 끝을 넘습니다**. `width` 로 보정해도 읽는 총량은 `width × height` 라
  버퍼(`stride × height`)를 여전히 넘으므로 보정으로는 안전해지지 않습니다.
  그래서 **읽기를 시도하지 않고 빈 리전을 돌려줍니다** (`data == 0` / `width <= 0` 과 같은 취급).
  조용히 틀린 값이 나오는 대신 `IsEmpty() == true` 로 드러나므로, 값이 전부 0 이면 §5.6 을 보세요.

### 3.3 케이스 B — 좌표 목록 / 런렝스 목록

#### B-1. 이미 런렝스로 blob 을 들고 있다면 (가장 빠른 경로)

FChain 처럼 런 단위로 blob 을 관리하는 검사기라면, **마스크 이미지를 다시 만들 필요가 없습니다.**

```cpp
// colEnd 는 끝 포함(inclusive). col 10..29 는 colBegin=10, colEnd=29 (개수 20)
std::vector<int> rows;
std::vector<int> colBegins;
std::vector<int> colEnds;

for (size_t i = 0; i < blob.runCount; ++i)
{
	rows     .push_back(blob.runs[i].y);
	colBegins.push_back(blob.runs[i].xStart);
	colEnds  .push_back(blob.runs[i].xEnd);		// ⚠ 끝 포함인지 확인. 배타적이면 -1
}

Region region = Region::FromRuns(&rows[0], &colBegins[0], &colEnds[0], static_cast<int>(rows.size()));
```

내부에서 **정렬 → 겹침/인접 병합 → 정규형** 으로 만들기 때문에:

- 런의 순서가 뒤죽박죽이어도 됩니다
- 같은 행에 겹치는 런, 인접한 런(`끝+1 == 다음 시작`)이 있어도 병합됩니다
- `colEnd < colBegin` 인 잘못된 런은 조용히 버려집니다
- 좌표가 음수여도 동작합니다 (내부에서 바운딩 박스 기준으로 처리)

([`examples/03_from_runs.cpp`](../examples/03_from_runs.cpp) 가 정렬된 런 / 뒤죽박죽 런 /
좌표 목록 세 경로에서 **완전히 같은 값**이 나오는 것을 실증합니다)

#### B-2. 픽셀 좌표 목록밖에 없다면

`(row, col)` 목록을 런으로 접어서 넘깁니다.

```cpp
struct PixelPoint { int row; int col; };

bool PixelLess(const PixelPoint& a, const PixelPoint& b)
{
	if (a.row != b.row) return a.row < b.row;
	return a.col < b.col;
}

Region MakeFromPointList(std::vector<PixelPoint>& points)
{
	if (points.empty())
		return Region();

	std::sort(points.begin(), points.end(), PixelLess);

	std::vector<int> rows, colBegins, colEnds;

	size_t i = 0;
	while (i < points.size())
	{
		const int row = points[i].row;
		const int begin = points[i].col;
		int end = points[i].col;

		size_t k = i + 1;
		while (k < points.size() && points[k].row == row && points[k].col <= end + 1)
		{
			if (points[k].col > end)
				end = points[k].col;
			++k;
		}

		rows.push_back(row);
		colBegins.push_back(begin);
		colEnds.push_back(end);
		i = k;
	}

	return Region::FromRuns(&rows[0], &colBegins[0], &colEnds[0], static_cast<int>(rows.size()));
}
```

> 사실 정렬조차 하지 않고 **픽셀 하나당 런 하나**(`colBegin == colEnd`)로 넣어도 결과는 같습니다.
> 내부 정규화가 전부 병합해 줍니다. 다만 런 개수가 픽셀 수만큼 늘어나 정렬 비용이 커집니다.
> 픽셀이 수만 개 이상이면 위처럼 접어서 넘기세요.

### 3.4 케이스 C — OpenCV `cv::Mat` 을 쓰는 검사기

**라이브러리는 OpenCV 에 의존하지 않습니다.** 앞으로도 추가하지 않습니다
([`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §2.1 — OpenCV 함수를 쓰면 오히려 값이 틀립니다).
따라서 어댑터는 **사용처 프로젝트 쪽**에 두세요. 아래 코드를 검사기 프로젝트에 복사해 쓰면 됩니다.

```cpp
// ── 검사기 프로젝트 쪽 파일 (예: GlimHalconCvAdapter.h) ──────────────────────
#include <opencv2/opencv.hpp>
#include "GlimHalcon.h"

// CV_8UC1 이진 마스크 → Region
//   mat.data / mat.step 을 그대로 넘긴다. 복사가 발생하지 않는다.
//   ⚠ ROI(부분 Mat)여도 step 은 원본 행 바이트 수이므로 그대로 쓰면 된다.
inline glim::halcon::Region ToRegion(const cv::Mat& mask)
{
	if (mask.empty() || mask.type() != CV_8UC1)
		return glim::halcon::Region();		// 빈 리전

	return glim::halcon::Region::FromMask(
		mask.data,
		mask.cols,
		mask.rows,
		static_cast<int>(mask.step));		// step = 한 행의 바이트 수 = stride
}
```

사용 예:

```cpp
cv::Mat gray = ...;								// CV_8UC1 원본
cv::Mat mask;
cv::threshold(gray, mask, 128, 255, cv::THRESH_BINARY);

glim::halcon::Region region = ToRegion(mask);

long area = 0;
double row = 0.0, column = 0.0;
glim::halcon::AreaCenter(region, area, row, column);

// 결과를 OpenCV 로 되돌릴 때는 순서를 바꾼다
cv::Point center(static_cast<int>(column + 0.5), static_cast<int>(row + 0.5));	// (x, y)
```

#### C-1. `cv::connectedComponents` 로 blob 을 나눠서 넘기기

**중요**: 이 라이브러리에는 HALCON 의 `connection` 에 해당하는 오퍼레이터가 **없습니다.**
한 `Region` 에 여러 덩어리를 담으면 그것들을 **하나의 리전으로 취급**합니다(§5.5).
blob 별 값이 필요하면 **blob 하나당 Region 하나**를 만들어야 합니다.

```cpp
cv::Mat labels, stats, centroids;
const int count = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);

for (int label = 1; label < count; ++label)		// 0 은 배경
{
	// blob 의 바운딩 박스만 잘라서 처리한다 (성능상 매우 중요 — §6.3)
	const cv::Rect box(stats.at<int>(label, cv::CC_STAT_LEFT),
					   stats.at<int>(label, cv::CC_STAT_TOP),
					   stats.at<int>(label, cv::CC_STAT_WIDTH),
					   stats.at<int>(label, cv::CC_STAT_HEIGHT));

	cv::Mat blobMask = (labels(box) == label);		// CV_8UC1, 0/255, 연속 메모리

	glim::halcon::Region region = glim::halcon::Region::FromMask(
		blobMask.data, blobMask.cols, blobMask.rows, static_cast<int>(blobMask.step));

	long area = 0;
	double localRow = 0.0, localColumn = 0.0;
	glim::halcon::AreaCenter(region, area, localRow, localColumn);

	// ⚠ 결과는 크롭 로컬 좌표다. 전역 좌표로 되돌린다
	const double globalRow    = localRow    + box.y;
	const double globalColumn = localColumn + box.x;

	double circularity = 0.0, compactness = 0.0, convexity = 0.0;
	glim::halcon::Circularity(region, circularity);
	glim::halcon::Compactness(region, compactness);
	glim::halcon::Convexity  (region, convexity);
}
```

#### C-2. `cv::findContours` 결과밖에 없다면

`findContours` 는 **윤곽 좌표**만 줍니다. 이 라이브러리는 **채워진 리전**을 받으므로,
`cv::drawContours(..., cv::FILLED)` 로 마스크를 만들어 넘기세요.
윤곽 좌표만 `FromRuns` 에 넣으면 **속이 빈 리전**이 되어 `Area` 가 둘레 픽셀 수로 나옵니다.

```cpp
cv::Mat filled = cv::Mat::zeros(mask.size(), CV_8UC1);
cv::drawContours(filled, contours, index, cv::Scalar(255), cv::FILLED);
glim::halcon::Region region = ToRegion(filled);
```

### 3.5 케이스 D — 큰 이미지의 ROI 만 넘기기

시작 픽셀 포인터를 옮기고 `stride` 는 **원본 그대로** 둡니다.

```cpp
const unsigned char* roiStart = imageData
	+ static_cast<size_t>(roiRow) * imageStride
	+ static_cast<size_t>(roiCol);

Region region = Region::FromMask(roiStart, roiWidth, roiHeight, imageStride);	// stride 는 원본!
```

결과 좌표는 **ROI 로컬 좌표**입니다. 전역 좌표가 필요하면 ROI 원점을 더하세요.
실측 예([`examples/02_from_binary_mask.cpp`](../examples/02_from_binary_mask.cpp)):

```
ROI 로컬 좌표   : Area=400  Row=19.5000  Column=29.5000
전역 좌표 환산  : Row=39.5000  Column=129.5000  (ROI 원점 20, 100 을 더함)
```

> `Area` · `Circularity` · `Compactness` · `Convexity` 는 **평행이동에 불변**이므로 ROI 크롭의 영향을 받지 않습니다.
> 좌표(`Row`/`Column`)만 보정하면 됩니다.
> 단, **ROI 경계에서 blob 이 잘리면** 잘린 모양의 값이 나옵니다. blob 바운딩 박스보다 넉넉하게 잡으세요.

### 3.6 ⭐ 케이스 E — 검사기 원샷: 이진화 crop 한 장 → 특징값 7종

실시간 검사기가 **이진화된 crop 한 장**(예: 128×128)을 던지고 "그 안의 불량 하나"의 특징값을
받고 싶을 때 쓰는 **가장 짧은 경로**입니다. 이 한 함수가 전부입니다.

```cpp
#include "GlimHalcon.h"

using namespace glim::halcon;

// 검사기가 이미 만든 이진화 결과 (0 = 배경, 그 외 = 전경). 128x128 이든 임의 크기든 상관없습니다.
const unsigned char* mask = ...;
int width  = 128;
int height = 128;
int stride = 128;					// 한 행의 바이트 수. 패딩 없으면 width (또는 0)

RegionFeatures f;
if (ComputeLargestBlobFeatures(mask, width, height, stride, f))
{
	// f.area / f.row / f.column / f.contLength / f.circularity / f.compactness / f.convexity
	if (f.circularity < 0.3 && f.convexity < 0.8)
		nDefectType = DEFECT_SCRATCH;
}
else
{
	// 입력이 잘못됐거나(널 포인터 / width·height <= 0 / 0 < stride < width)
	// 전경이 하나도 없다. 이때 f 는 전 필드 0.
}
```

이 함수가 하는 일:

1. `Region::FromMask` 로 마스크를 리전으로 만들고
2. `SelectLargestBlob` 으로 **면적이 가장 큰 연결성분(8-연결) 하나만** 고른 뒤
3. `ComputeFeatures` 로 특징값 7종을 계산합니다.

**왜 "최대 blob 선택" 단계가 필요한가:** `Region::FromMask` 는 마스크 전체를 **하나의 리전**으로
만듭니다. crop 안에 노이즈 점이나 잔여 덩어리가 여럿 있으면 그것들이 전부 합쳐져 값이 섞입니다
(이 라이브러리에는 HALCON `connection` 오퍼레이터가 없습니다 — §5.5). `ComputeLargestBlobFeatures` 는
그 사이에 최대 성분 선택을 끼워, **"crop 한 장 = 불량 하나"** 라는 검사기의 전제에 맞춥니다.

> **동률 규칙**: 최대 면적이 같은 성분이 둘 이상이면 **스캔 순서(위→아래, 왼→오른쪽)상 먼저 나온 것**을 택합니다.

#### ⚠ 이 함수만 `bool` 을 반환합니다 — 이유

나머지 오퍼레이터 5종과 `ComputeFeatures` 는 모두 `void` 이고 "실패도 조용히 0" 규약입니다.
`ComputeLargestBlobFeatures` **하나만** `bool` 을 돌려줍니다. 이유:

- 다른 함수들은 **이미 만들어진 `Region`** 을 받습니다. Region 은 생성 시 검증을 통과한 값이라
  "실패" 라는 개념이 사실상 빈 리전 하나뿐이고, "0 반환" 으로 충분했습니다.
- 그러나 이 함수는 **원시 포인터와 치수를 직접 받습니다.** 널 포인터·잘못된 stride 처럼
  입력 자체가 틀릴 수 있고, 그것은 **"면적이 0 인 정상 마스크(전경 없음)"** 와 의미가 다릅니다.
- 검사기가 이 둘을 구분하지 못하면 **조용히 틀린 판정**을 내립니다. 그래서 여기서만 성패를 `bool` 로
  명시합니다. 값 규약(실패 시 전 필드 0)은 기존과 동일합니다.

낱개 부품이 필요하면 `SelectLargestBlob(const Region&)` 을 직접 쓸 수도 있습니다
(마스크 대신 이미 만든 `Region`, 또는 `FromRuns` 로 만든 리전에서 최대 성분만 고르고 싶을 때).

#### 단일 blob 마스크에서는 값이 변하지 않습니다

crop 이 이미 단일 성분이면 최대 blob 선택은 **아무것도 바꾸지 않습니다**(같은 리전을 그대로 반환).
실제 검사 마스크 4장으로 확인한 결과입니다 (실측):

| 마스크(요약) | area | row | column | contLength | circularity | compactness | convexity |
|---|---|---|---|---|---|---|---|
| 077 k2.0 | 2065 | 82.259564 | 69.057627 | 206.225397 | 0.430920 | 1.638907 | 0.956905 |
| 077 k2.5 | 1635 | 82.522936 | 67.632416 | 183.740115 | 0.389930 | 1.643162 | 0.961765 |
| 078 k2.5 (링/도넛) | 1148 | 86.398955 | 65.833624 | 352.894444 | 0.143537 | 8.632526 | 0.431417 |
| 553 k2.0 | 6977 | 93.738140 | 68.202666 | 376.124892 | 0.461344 | 1.613562 | 0.961682 |

`ComputeLargestBlobFeatures` 결과 = `FromMask` + `ComputeFeatures` 직접 조합 결과와
**비트 단위로 동일**합니다. 078 k2.5 는 구멍이 있는 링 형상이며, 구멍 처리(§5.6/구멍 제외)도
최대 blob 선택 도입 후 그대로 유지됩니다.

예제: [`examples/05_largest_blob.cpp`](../examples/05_largest_blob.cpp)

---

## 4. 오퍼레이터 레퍼런스 5종 + 배치 API

정의·수식은 [`02_DEFINITIONS.md`](02_DEFINITIONS.md) 의 해당 절을 보세요. 여기서는 **쓰는 법**만 씁니다.

공통 규약:

- 출력은 **참조 인자**입니다. 함수 진입 즉시 `0` 으로 초기화됩니다.
- **예외를 던지지 않습니다.** 내부에서 잡고 `0` 으로 축약합니다.
- **빈 리전이면 전부 `0`** 입니다 (`Compactness` 도 `0`. 단 단일 픽셀은 `1.0` — §5.4).

### 4.1 `AreaCenter` — 면적과 무게중심

```cpp
void AreaCenter(const Region& region, long& area, double& row, double& column);
```

| 인자 | 의미 |
|---|---|
| `area` (out) | 리전에 속한 **픽셀 개수**. 기하 면적이 아닙니다 |
| `row` (out) | 무게중심의 row 좌표 (y). 소수입니다 |
| `column` (out) | 무게중심의 column 좌표 (x). 소수입니다 |

- 범위: `area >= 0`, `row`/`column` 은 리전의 바운딩 박스 안
- HALCON 대비 주의점: HALCON `area_center` 와 동일 정의입니다. 다만 HALCON 은 리전 튜플을 넘기면
  리전마다 값을 돌려주지만, 여기서는 **Region 하나당 호출 한 번**입니다
- 성능: **O(런 수)** 입니다. 픽셀을 순회하지 않습니다. 5종 중 압도적으로 쌉니다

```cpp
long area = 0;
double row = 0.0, column = 0.0;
AreaCenter(region, area, row, column);
if (area < m_nMinDefectArea)
	return;			// 너무 작은 blob 은 버린다
```

정의: [`02_DEFINITIONS.md` §1](02_DEFINITIONS.md)

### 4.2 `ContLength` — 윤곽 길이

```cpp
void ContLength(const Region& region, double& contLength);
```

| 인자 | 의미 |
|---|---|
| `contLength` (out) | 외곽 윤곽 체인의 길이. 축평행 스텝 = 1, 대각 스텝 = √2 |

- 범위: `contLength >= 0`
- **구멍(hole)의 윤곽은 포함하지 않습니다.** 도넛 모양이어도 바깥 둘레만 셉니다
- **단일 픽셀은 `0.0`** 입니다 (스텝이 없으므로). 경계 둘레 모델이 아닙니다
- 연결성분이 여러 개면 각 성분의 외곽 윤곽 길이를 **합산**합니다
- ⚠ HALCON 대비 주의점 2가지 — §5.2 를 반드시 읽으세요
  - 디지털 원의 체인 길이는 연속 원주(`2πR`)보다 **약 +5.5%** 큽니다
  - 체인을 **닫힌 경로**로 봅니다(마지막 점 → 첫 점 복귀 1스텝 포함). 정사각형 100×100 에서 396 이 나옵니다.
    열린 경로로 보는 해석이면 395 입니다. 상대오차 0.25%
    ([`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §6 R-1 — HALCON 실측으로 확정 대기 중)

정의: [`02_DEFINITIONS.md` §2](02_DEFINITIONS.md)

### 4.3 `Circularity` — 원형도

```cpp
void Circularity(const Region& region, double& circularity);
```

| 인자 | 의미 |
|---|---|
| `circularity` (out) | 면적을 "중심에서 가장 먼 윤곽 픽셀"을 반지름으로 하는 원 넓이로 나눈 값 |

- 범위: **`0 <= circularity <= 1`** — 위쪽으로 **1.0 에서 클리핑**됩니다
- 원에 가까울수록 1 에 가깝고, 길쭉하거나 가지가 뻗을수록 작아집니다
- **단일 픽셀은 `0`** 입니다 (최대 거리가 0 → 0 나눗셈 방어)
- ⚠ **클리핑이 실제로 자주 발동합니다.** 작은 원판·작은 정사각형에서 계산값이 1 을 넘어
  1.0 으로 잘립니다. 즉 **"1.0 = 완전한 원"이 아니라 "1.0 이상"** 이라는 뜻입니다(§5.3)

```cpp
double circularity = 0.0;
Circularity(region, circularity);
if (circularity < 0.3)
	nDefectType = DEFECT_SCRATCH;		// 길쭉한 형태
```

정의: [`02_DEFINITIONS.md` §3](02_DEFINITIONS.md)

### 4.4 `Compactness` — 조밀도

```cpp
void Compactness(const Region& region, double& compactness);
```

| 인자 | 의미 |
|---|---|
| `compactness` (out) | 윤곽 길이의 제곱을 면적으로 정규화한 값 |

- 범위: **`compactness >= 1.0` 또는 `== 0`** — 아래쪽으로 **1.0 에서 클리핑**됩니다
  (`Circularity` 와 클리핑 **방향이 반대**입니다. 헷갈리기 쉬우니 주의)
- 값이 **클수록** 복잡·길쭉한 형태입니다. 원형에 가까울수록 1 에 가깝습니다
- 빈 리전 = `0`, **단일 픽셀 = `1.0`** (빈 리전과 다릅니다)
- ⚠ `ContLength` 를 제곱해 쓰므로 **체인 길이 편향이 제곱으로 증폭**됩니다.
  이상적인 원판이 1.0 이 아니라 **약 1.10** 입니다(§5.2). 임계값을 1.0 기준으로 잡으면 안 됩니다

정의: [`02_DEFINITIONS.md` §4](02_DEFINITIONS.md)

### 4.5 `Convexity` — 볼록도

```cpp
void Convexity(const Region& region, double& convexity);
```

| 인자 | 의미 |
|---|---|
| `convexity` (out) | 원본 면적 ÷ convex hull 면적. 둘 다 **픽셀 개수**입니다 |

- 범위: `0 <= convexity <= 1` (볼록한 도형이면 정확히 `1.0`)
- 오목한 부분이 많을수록 작아집니다. L자 모양·별 모양 검출에 유용합니다
- ⚠ **1.0 클램프를 일부러 넣지 않았습니다.** 1 을 넘는 값이 나온다면 그것은 내부 계산 회귀이며,
  감추지 않고 그대로 노출합니다([`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §2.3).
  운영 코드에서 1 을 넘는 값을 보면 **버그로 신고**해 주세요
- 5종 중 **가장 비쌉니다** (윤곽 추적 + convex hull + hull 래스터화)

정의: [`02_DEFINITIONS.md` §5](02_DEFINITIONS.md)

### 4.6 `ComputeFeatures` — 5종을 한 번에 (배치 API)

```cpp
struct RegionFeatures
{
	long   area;			// AreaCenter  의 Area
	double row;				// AreaCenter  의 Row    (y)
	double column;			// AreaCenter  의 Column (x)
	double contLength;		// ContLength
	double circularity;		// Circularity
	double compactness;		// Compactness
	double convexity;		// Convexity
};

void ComputeFeatures(const Region& region, RegionFeatures& features);
```

HALCON 에 대응하는 오퍼레이터가 **없습니다.** 성능을 위해 이 라이브러리가 추가한 함수입니다.

```cpp
RegionFeatures f;
ComputeFeatures(region, f);

if (f.area >= m_nMinDefectArea && f.circularity < 0.3 && f.convexity < 0.8)
	nDefectType = DEFECT_SCRATCH;
```

- **값은 개별 오퍼레이터 5종을 각각 호출한 것과 완전히 동일합니다.** 내부에서 같은 계산 함수를
  호출하기 때문입니다 (근사·재구현이 아닙니다)
- **왜 쓰는가**: 개별로 5종을 다 부르면 윤곽 추적이 **4번** 반복됩니다.
  `ComputeFeatures` 는 내부 캐시(`FeatureContext`)를 하나만 만들어 **윤곽 추적 1회 + hull 1회**로 끝냅니다 (§6.2)
- 진입 즉시 **전 필드가 `0` 으로 초기화**됩니다. 빈 리전·실패도 전 필드 `0` 입니다 (개별 오퍼레이터와 같은 규약)
- **예외를 던지지 않습니다**
- 값이 **하나만** 필요하면 개별 오퍼레이터가 더 쌉니다. `ComputeFeatures` 는 가장 비싼
  `Convexity`(hull 래스터화)를 항상 포함해 계산합니다
- 구조체는 POD 입니다. 필드가 추가되는 경우 **끝에만** 붙습니다. 다만 그때는
  `sizeof(RegionFeatures)` 가 바뀌므로 **라이브러리와 검사기를 함께 재컴파일**해야 합니다
  (같은 lib 에 옛 헤더로 컴파일된 호출부를 섞으면 조용히 깨집니다)

예제: [`examples/04_batch_features.cpp`](../examples/04_batch_features.cpp)

### 4.7 검사기 연동 API — `SelectLargestBlob` · `ComputeLargestBlobFeatures`

HALCON 원명 대응이 없는, 검사기 편의용 함수 2종입니다. 자세한 사용법은 §3.6 을 보세요.

```cpp
// 최대 연결성분(8-연결) 하나만 남긴 리전을 돌려준다. 동률이면 스캔 순서상 먼저인 것.
Region SelectLargestBlob(const Region& region);

// 이진 마스크 한 장 → 최대 blob → 특징값 7종. 원샷.
// 반환 true = 계산 성공 / false = 입력 오류 또는 전경 없음 (features 는 전 필드 0).
bool ComputeLargestBlobFeatures(const unsigned char* data,
                                int width, int height, int stride,
                                RegionFeatures& features);
```

- `ComputeLargestBlobFeatures` 는 `FromMask` → `SelectLargestBlob` → `ComputeFeatures` 조합입니다.
  값은 세 함수를 손으로 조합한 것과 **완전히 동일**합니다(계산식을 재사용하며 재구현하지 않습니다).
- **이 함수만 `bool` 을 반환**하는 이유(원시 포인터/치수를 직접 받아 입력 오류와 전경 없음을 구분)는 §3.6 에 있습니다.
- 호출당 힙 할당 실측과 개선 방향은 §6.5 를 보세요.

### 4.8 2차 모멘트 계열 5종 (2026-07-28 추가)

```cpp
void MomentsRegion2nd     (const Region&, double& m11, double& m20, double& m02,
                           double& ia, double& ib);
void MomentsRegion2ndInvar(const Region&, double& m11, double& m20, double& m02);
void EllipticAxis         (const Region&, double& ra, double& rb, double& phi);
void Eccentricity         (const Region&, double& anisometry, double& bulkiness,
                           double& structureFactor);
void OrientationRegion    (const Region&, double& phi);
```

검사기에서 실제로 쓰는 것은 대개 아래 4개입니다.

| 값 | 뜻 | 검사기에서 쓰는 법 |
|---|---|---|
| `ra` / `rb` | 같은 방향·같은 종횡비를 갖는 **등가 타원의 주/부 반지름** | 불량의 "길이 × 두께" 근사 |
| `anisometry` | `ra / rb`. 원이면 1.0, 길쭉할수록 큼 | **스크래치 판별.** 상한이 없다 |
| `phi` | 주축과 x축의 각 [라디안], `-π/2 < φ ≤ π/2` | 불량의 기울기 |
| `orientation` | 방향까지 확정한 각, `-π ≤ φ < π` | 화살표를 그릴 때 |

```cpp
double ra = 0.0, rb = 0.0, phi = 0.0;
EllipticAxis(region, ra, rb, phi);

double aniso = 0.0, bulk = 0.0, sf = 0.0;
Eccentricity(region, aniso, bulk, sf);

if (aniso > 5.0 && f.area > 100)
    nDefectType = DEFECT_SCRATCH;       // 5배 이상 길쭉하다
```

#### ⚠ 반드시 알아야 할 함정 4가지

**1. `MomentsRegion2nd` 는 면적으로 나누지 않습니다 (정규화 없음).**
값이 도형 크기의 **4제곱**으로 커집니다. 100×100 정사각형에서 `M20 = 8,332,500` 입니다.
크기에 무관한 값이 필요하면 `MomentsRegion2ndInvar`(= 이 값 ÷ `Area²`)를 쓰세요.

| 도형 | `MomentsRegion2nd` 의 `M20` | `MomentsRegion2ndInvar` 의 `M20` |
|---|---|---|
| 정사각형 100×100 | 8 332 500 | 0.083325 |
| 정사각형 50×50 | 520 625 | 0.0833 |

**2. 픽셀은 "면적 1의 사각형" 이 아니라 "무한소 점" 입니다.**
그래서 **1픽셀 두께 리전과 단일 픽셀은 `rb = 0`** 이 됩니다.
`0` 으로 나눌 수 있으니 사용처에서 방어하세요. 라이브러리는 내부에서 방어합니다.

**3. 🔴 `anisometry` 는 `rb = 0` 일 때 `1.0` 이 아니라 `0` 입니다.**
직관과 반대이고 HALCON 문서의 Assertion(`>= 1.0`)과도 어긋나지만, **HALCON 의 실제 동작**입니다
(원문 Attention 이 명시). 이 라이브러리는 그것을 그대로 재현합니다.

> **그래서 `anisometry > 5.0` 같은 조건만 쓰면 1픽셀 두께 스크래치가 통째로 빠집니다.**
> 가장 길쭉한 불량이 `0` 을 내기 때문입니다. `rb == 0` 을 **별도로** 처리하세요.
>
> ```cpp
> const bool isLine = (rb <= 0.0) && (ra > 0.0);   // 1픽셀 두께 = 극단적으로 길쭉함
> if (isLine || aniso > 5.0)
>     nDefectType = DEFECT_SCRATCH;
> ```

**4. `structureFactor` 는 축퇴 리전에서 `-1.0`, 빈 리전에서 `0.0` 입니다.**

| 도형 | `anisometry` | `bulkiness` | `structureFactor` |
|---|---|---|---|
| 빈 리전 | 0 | 0 | **0** |
| 단일 픽셀 / 1픽셀 두께 직선 | 0 | 0 | **-1.0** |
| 정사각형 100×100 | 1.0 | 1.0471 | 0.0471 |

`0` 과 `-1.0` 이 다른 뜻입니다. `structureFactor == 0` 으로 빈 리전을 판정하지 마세요.

> 이 `-1.0` 은 **미결 쟁점 `EC-1`** 입니다(HALCON 실측 대기). 원문 문장이 모호해
> `0` 일 가능성도 있습니다. 판정이 바뀌면 값이 `0` 으로 바뀔 수 있으니
> **이 값에 임계값을 직접 걸지 마세요.**

#### 배치 API

```cpp
RegionMoments m;
ComputeMoments(region, m);                          // 2차 5종을 한 번에

RegionFeatures f;
ComputeFeaturesAndMoments(region, f, m);            // 1차 7값 + 2차 15값을 한 번에 (가장 쌈)
```

- `ComputeFeatures` + `ComputeMoments` 를 따로 부르면 윤곽 추적이 **2회** 일어납니다.
  `ComputeFeaturesAndMoments` 는 **1회**입니다. 값은 어느 경로로 부르든 완전히 동일합니다.
- `RegionMoments` 는 `RegionFeatures` 와 **별개 구조체**입니다.
  기존 `RegionFeatures` 의 레이아웃은 **바뀌지 않았습니다** — 이미 쓰고 있는 코드는
  재컴파일 없이 그대로 동작합니다.

정의: [`02_DEFINITIONS.md` §6~§10](02_DEFINITIONS.md) ·
기댓값: [`03_VERIFICATION.md` §3B](03_VERIFICATION.md)

### 4.9 한 번에 보기 — 실측 기준값

[`examples/02_from_binary_mask.cpp`](../examples/02_from_binary_mask.cpp) 실행 결과 (MSVC 14.44 / x64 / Release):

| 도형 | Area | Row | Column | ContLength | Circularity | Compactness | Convexity |
|---|---|---|---|---|---|---|---|
| 정사각형 20×20 | 400 | 14.5000 | 19.5000 | 76.0000 | 0.705396 | 1.149099 | 1.000000 |
| 원판 R=10 | 317 | 20.0000 | 20.0000 | 65.9411 | 1.000000 | 1.091550 | 1.000000 |
| L자 | 300 | 16.1667 | 12.8333 | 75.4142 | 0.513250 | 1.508604 | 0.869565 |
| stride 패딩 정사각형 | 400 | 14.5000 | 19.5000 | 76.0000 | 0.705396 | 1.149099 | 1.000000 |
| 빈 리전 | 0 | 0.0000 | 0.0000 | 0.0000 | 0.000000 | 0.000000 | 0.000000 |

이 표에서 바로 읽어야 할 것:

- 원판의 `ContLength` 65.94 는 `2πR = 62.83` 보다 **큽니다** (§5.2)
- 원판의 `Compactness` 1.0916 은 **1.0 이 아닙니다** (§5.2)
- 원판의 `Circularity` 1.000000 은 **클리핑된 값**입니다 (계산값 1.0090) (§5.3)
- L자의 `Convexity` 0.8696 — 오목한 도형이 1 보다 확실히 작게 나옵니다
- 빈 리전은 전부 0 이며 예외가 아닙니다

---

## 5. ⚠ 값 해석 시 함정

검사기에서 **임계값을 잡기 전에** 이 장을 반드시 읽으세요.
근거는 전부 [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) 에 있습니다.

### 5.1 요약 표

| # | 함정 | 결과 |
|---|---|---|
| 1 | 디지털 원의 체인 둘레는 `2πR` 보다 **+5.5%** | 원판 `Compactness` ≈ **1.10** (1.0 아님) |
| 2 | `Circularity` 는 **위로** 1.0 클리핑 | 1.0 은 "완전한 원"이 아니라 "1.0 이상" |
| 3 | `Compactness` 는 **아래로** 1.0 클리핑 | 방향이 `Circularity` 와 반대 |
| 4 | 단일 픽셀 | `ContLength=0`, `Circularity=0`, **`Compactness=1.0`**, `Convexity=1.0` |
| 5 | 한 Region 에 여러 덩어리 | 하나의 리전으로 합산 처리 (HALCON tuple 과 다름) |
| 6 | 구멍(hole) | `ContLength` 에 포함되지 않음 |
| 7 | 실패해도 예외 없음 | 전부 조용히 `0`. 반환값으로 성패를 알 수 없음 |

### 5.2 🔴 가장 중요 — 디지털 원의 둘레는 `2πR` 이 아닙니다 (+5.5%)

> **반지름 `R` 인 디지털 원의 8연결 체인 둘레는 연속 원주보다 약 +5.5% 큽니다.**
> 이것은 버그도 오차도 아니고 **이산 기하의 원리적 성질**입니다.

```
옥탄트(45°) 1개당 이동량 : 축평행 0.414R × 1 + 대각 0.414R × √2 = 0.828R
전체 8 옥탄트            : 8 × 0.828R = 6.63R
연속 원주                : 2πR       = 6.283R

비율 = 6.63 / 6.283 ≈ 1.055   →   +5.5%
```

**검사기 임계값에 미치는 영향:**

| 값 | 영향 | 실측 근거 |
|---|---|---|
| `ContLength` | 곡선 경계일수록 커진다. 직선 경계는 영향 없다 | R=50 원판: 실측 **329.71**, `2πR` = 314.16 |
| `Compactness` | **`L²` 이므로 편향이 제곱으로 증폭**. 이상적인 원판이 **1.0 이 아니라 ≈1.10** | R=50 원판: 실측 **1.1027** / R=10 원판: 실측 **1.0916** |
| `Circularity` | 영향 없음 (`L` 을 쓰지 않음) | — |
| `Convexity` | 영향 없음 | — |

> ### 🔴 절대 하지 말 것
> **`Compactness < 1.05` 를 "원형이다" 판정 기준으로 쓰면 진짜 원도 전부 탈락합니다.**
> 원형 판정은 `Compactness` 가 아니라 `Circularity` 를 쓰거나,
> 임계값을 **실제 샘플 이미지에서 측정한 값**으로 잡으세요.
> 교과서의 연속 기하 공식(`4πA/L² = 1`)으로 임계값을 계산하면 안 됩니다.

같은 이유로, **기댓값을 손으로 계산할 때도 연속 기하 공식을 쓰면 안 됩니다.**
이 저장소의 검증 문서 초판이 실제로 이 실수를 했고, 실측으로 4건을 보정했습니다
([`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §3~§4).

### 5.3 `Circularity` 의 1.0 은 "완전한 원"이 아닙니다

`Circularity` 는 계산값이 1 을 넘으면 **1.0 으로 잘립니다**. 그리고 이 클리핑은 **실제로 자주 발동합니다.**

실측 예:

| 도형 | 원 계산값 | 출력 |
|---|---|---|
| 4×4 정사각형 (예제 01) | 16 / (4.5·π) = **1.132** | `1.000000` |
| 원판 R=10 (예제 02) | 317 / (100·π) = **1.009** | `1.000000` |
| 원판 R=50 (기준도형 S2) | 7845 / (2500·π) = **0.9989** | `0.998856` (클리핑 미발동) |

작은 도형일수록 클리핑에 걸리기 쉽습니다.
따라서 **`circularity == 1.0` 을 "원이다"로 해석하면 작은 사각형도 원으로 판정됩니다.**
작은 blob(수십 픽셀)에서는 `Circularity` 의 변별력이 떨어진다고 보고, 면적 하한을 함께 거세요.

### 5.4 단일 픽셀 · 얇은 구조

| 도형 | Area | ContLength | Circularity | Compactness | Convexity |
|---|---|---|---|---|---|
| 단일 픽셀 | 1 | **0.0** | **0.0** | **1.0** | 1.0 |
| 빈 리전 | 0 | 0.0 | 0.0 | **0.0** | 0.0 |
| 1×100 가로선 | 100 | 198.0 | 0.012991 | 31.1976 | 1.0 |

- 단일 픽셀과 빈 리전의 `Compactness` 가 **다릅니다** (`1.0` vs `0.0`). 0 으로 구분하면 안 됩니다
- 1픽셀 두께 구조에서 `ContLength` 는 왕복하므로 길이의 약 2배가 나옵니다 (100 픽셀 선 → 198)
- 노이즈성 1~2 픽셀 blob 은 특징값을 보기 전에 **면적으로 먼저 걸러내는** 편이 안전합니다

### 5.5 한 Region 에 여러 덩어리를 담으면

이 라이브러리에는 HALCON 의 `connection` 에 해당하는 분리 오퍼레이터가 **없습니다.**
덩어리가 여럿인 마스크를 그대로 넘기면:

- `Area` = 전체 픽셀 수 합
- `Row`/`Column` = 전체의 **공통 무게중심** (덩어리 사이 빈 공간에 찍힐 수 있음)
- `ContLength` = 각 덩어리 외곽 길이의 **합**
- `Convexity` = 모든 덩어리를 감싸는 **하나의 hull** 기준 → 매우 작은 값

즉 **blob 별 값이 필요하면 blob 하나당 Region 하나**를 만들어야 합니다 (§3.4 C-1).

### 5.6 결과가 전부 0 일 때

예외가 없으므로 실패가 조용합니다. 아래를 순서대로 확인하세요.

1. `region.IsEmpty()` 가 `true` 인가 → 마스크가 전부 0 이거나 포인터/크기가 잘못됨
2. `FromMask` 에 `data == 0`, `width <= 0`, `height <= 0` 을 넘기지 않았는가
2-1. `FromMask` 의 `stride` 가 **`width` 보다 작지 않은가** (`0 < stride < width` 는 빈 리전입니다 — §3.2).
   `stride` 를 "픽셀 수" 가 아니라 "바이트 수" 로 넘겼는지, 컬러 버퍼의 `step` 을 잘못 넘겼는지 확인하세요
3. `FromRuns` 에 `count <= 0` 이거나 배열 포인터 중 하나가 널이 아닌가
4. 마스크의 전경 값이 정말 0 이 아닌 값인가 (임계값 개념이 없으므로 0 은 무조건 배경)

---

## 6. 성능 가이드

### 6.1 오퍼레이터별 비용

| 오퍼레이터 | 내부에서 하는 일 | 비용 |
|---|---|---|
| `AreaCenter` | 런 누적만 | **O(런 수)** — 매우 쌈. 픽셀 순회 없음 |
| `ContLength` | 8연결 성분 라벨링 + Moore 경계추적 + 체인 합 | 중간. 메모리는 **O(런 수)** (§6.3) |
| `Circularity` | 위 + 중심~윤곽 최대거리 | `ContLength` 와 비슷 |
| `Compactness` | 위와 동일 (`ContLength` 재사용) | `ContLength` 와 비슷 |
| `Convexity` | 위 + convex hull + hull 래스터화 | **가장 비쌈** |
| `ComputeFeatures` | 위 전부를 **캐시 하나로 공유** | `Convexity` 한 번과 거의 같음. 5종을 다 쓸 때 가장 쌈 (§6.2) |

`Region::FromMask` 자체는 전체 픽셀을 한 번 훑습니다(O(width × height)).
`FromRuns` 는 정렬 비용(O(n log n))만 듭니다.

### 6.2 🔴 2종 이상 필요하면 `ComputeFeatures` 를 쓰세요

내부에 `FeatureContext` 라는 지연계산 캐시가 있습니다. area/center · contour · convex hull 을
**처음 요청될 때 1회만** 계산하고 재사용합니다.

**그런데 이 캐시는 `FeatureContext` 인스턴스 하나 안에서만 유효합니다.**
개별 오퍼레이터는 **호출할 때마다 `FeatureContext` 를 스택에 새로 만들고 버립니다.**

```cpp
// 개별 호출 — 윤곽 추적이 4번 일어납니다 (캐시가 호출 사이에 공유되지 않음)
ContLength (region, contLength);		// 윤곽 추적 1회
Circularity(region, circularity);		// 윤곽 추적 2회
Compactness(region, compactness);		// 윤곽 추적 3회
Convexity  (region, convexity);			// 윤곽 추적 4회 + hull
```

```cpp
// 배치 호출 — 윤곽 추적 1회 + hull 1회로 끝납니다. 값은 위와 완전히 동일합니다.
RegionFeatures f;
ComputeFeatures(region, f);
```

| 호출 방식 | 윤곽 추적 | convex hull |
|---|---|---|
| 개별 5종을 각각 호출 | **4회** | 1회 |
| `ComputeFeatures` 1회 | **1회** | 1회 |

> **값이 달라지지 않습니다.** 두 경로가 라이브러리 내부의 **같은 계산 함수**를 호출하기 때문입니다.
> 계산식이 복제돼 있지 않으므로 어긋날 여지가 구조적으로 없습니다.
> [`examples/04_batch_features.cpp`](../examples/04_batch_features.cpp) 가 8개 도형에 대해
> 두 경로의 값 7개가 **비트 단위로 같은지** 매 빌드마다 검사합니다(ctest 등록됨).

**나머지 최적화:**

- 값 하나만 필요하면 개별 오퍼레이터가 더 쌉니다. `ComputeFeatures` 는 항상 5종을 전부 계산합니다
  (특히 가장 비싼 `Convexity` 의 hull 래스터화를 포함합니다)
- **면적으로 먼저 거른다.** `AreaCenter` 는 거의 공짜이므로, 면적 하한에 미달하는 blob 은
  나머지를 아예 호출하지 않는다 — 실시간 경로에서 가장 효과가 큰 최적화입니다

```cpp
long area = 0;
double row = 0.0, column = 0.0;
AreaCenter(region, area, row, column);		// 싸다 (O(런 수), 윤곽 추적 없음)

if (area < m_nMinDefectArea)
	return;									// 비싼 계산을 통째로 건너뛴다

RegionFeatures f;
ComputeFeatures(region, f);					// 여기서 윤곽 추적 1회 + hull 1회
```

> `AreaCenter` 로 거른 뒤 `ComputeFeatures` 를 부르면 면적/중심이 두 번 계산되지만,
> 그 비용은 O(런 수)라 윤곽 추적 대비 무시할 수준입니다. 거르는 이득이 훨씬 큽니다.

### 6.3 리전은 작게 — 다만 라벨 이미지 문제는 해소됐습니다

> **이 절은 2026-07-21 에 갱신됐습니다.**
> 이전 판은 "윤곽 계열이 바운딩 박스 크기의 `int` 라벨 이미지를 할당하므로
> 풀프레임 리전이면 약 1.3 GB" 라고 경고했습니다. **그 구현은 교체됐습니다.**

현재 윤곽 추적(`ContLength`/`Circularity`/`Compactness`/`Convexity`/`ComputeFeatures`)은
**라벨 이미지를 만들지 않습니다.** 대신 런 조회 인덱스(행 이진탐색 → 행 안에서 런 이진탐색)로
"이 좌표가 리전에 속하는가" 를 판정합니다.

| 항목 | 이전 (dense 라벨 이미지) | 현재 (런 이진탐색) |
|---|---|---|
| 메모리 | `(bboxW+2) × (bboxH+2) × 4바이트` — **바운딩 박스 크기** | **O(런 수)** — 바운딩 박스와 무관 |
| 풀프레임 53580×6144 리전 | 약 **1.3 GB** (사실상 불가) | 런 개수에 비례. 픽셀 몇 개짜리면 수십 바이트 |
| 멤버십 판정 | O(1) (배열 인덱싱) | **O(log n)** ← 이것이 대가입니다 |

**트레이드오프를 정직하게 적자면**: 판정 비용이 상수시간에서 로그시간으로 올라갔습니다.
경계 픽셀 하나당 최대 8회 판정하므로, 런이 매우 많은(수십만) 리전에서는
이전 구현보다 느릴 수 있습니다. 그 대신 **메모리가 입력 크기와 무관하게 폭발하지 않습니다.**
실시간 검사 경로에서는 "느려짐" 보다 "1.3 GB 할당 실패" 가 훨씬 치명적이므로 이쪽을 골랐습니다.

> **그래도 blob 바운딩 박스로 크롭해서 넘기는 편이 좋습니다** (§3.4 C-1 / §3.5).
> `Region::FromMask` 자체가 `width × height` 를 전부 훑기 때문에,
> 풀프레임 마스크를 통째로 넘기면 리전 생성 단계에서 시간이 듭니다.
> (`FromRuns` 로 넣으면 이 비용도 없습니다.)

### 6.4 8스레드 실시간 검사 경로에서

**소스를 읽고 확인한 사실:**

- **전역/정적 캐시가 없습니다.** `FeatureContext` 는 각 오퍼레이터 호출의 **스택 지역 변수**입니다.
  `Contour.cpp` / `ConvexHull.cpp` 도 가변 전역 상태를 두지 않습니다
  (`ComputeContourLength` 안의 `static const double kSqrt2` 는 읽기 전용 상수입니다)
- `RegionData` 는 생성 시 `Assign` 으로 채워진 뒤 **변경되지 않습니다.** 오퍼레이터는 읽기만 합니다
- `Region` 의 복사는 `std::shared_ptr` 복사(O(1))이며, 참조계수 증감은 원자적입니다

**따라서 (코드 근거 기반 판단):**

| 패턴 | 판단 |
|---|---|
| 스레드마다 **각자 만든** Region 에 오퍼레이터 호출 | ✅ 안전 |
| 여러 스레드가 **같은 Region 객체를 동시에 읽기**(오퍼레이터 호출 / 복사 생성) | ✅ 안전 (내부 데이터 불변, 쓰기 없음) |
| 한 스레드가 Region 에 **대입**(`operator=`)하는 동안 다른 스레드가 그 객체를 읽음 | ❌ **안전하지 않음** — 같은 `shared_ptr` 인스턴스에 대한 동시 읽기/쓰기 |

> ⚠ **미검증**: 위 판단은 **소스 검토 결과이며, 멀티스레드 스트레스 테스트를 수행한 적은 없습니다.**
> 저장소의 테스트(`test/TestMain.cpp`)는 단일 스레드입니다.
> 8스레드 실검사 경로에 투입하기 전에 사용처에서 부하 테스트를 한 번 거치시길 권합니다.

**권장 패턴 — 스레드당 완전 분리:**

```cpp
// 각 스레드가 자기 blob 을 크롭 → 자기 Region 생성 → 자기 값 계산
// Region 을 스레드 간에 공유하지 않으면 동기화 자체가 필요 없습니다.
void InspectThread(int nThreadIndex)
{
	for (each blob assigned to this thread)
	{
		Region region = Region::FromMask(cropData, cropWidth, cropHeight, cropStride);

		long area = 0;
		double row = 0.0, column = 0.0;
		AreaCenter(region, area, row, column);
		if (area < m_nMinDefectArea)
			continue;

		double circularity = 0.0;
		Circularity(region, circularity);
		// ...
	}
}
```

메모리 할당(`std::vector`)이 호출마다 발생하므로, 초당 수만 blob 규모라면
힙 경합이 병목이 될 수 있습니다. 현재 API 로는 버퍼 재사용 수단이 없습니다 (개선 후보).

### 6.5 🔴 `ComputeLargestBlobFeatures` 의 호출당 힙 할당 (검사기 필독)

검사기는 프레임당 수백 개 crop 을 이 함수에 던집니다. **호출당 힙 할당 비용을 실측**해 둡니다.

측정 조건: 128×128 마스크, 단일 원판 blob(반경 30, 면적 2821), 전역 `operator new` 카운트,
MSVC 14.50 / x64 / `/O2` (2026-07-21).

| 구간 | 호출당 힙 할당 |
|---|---|
| `Region::FromMask` | **16** |
| `SelectLargestBlob` | **7** (성분이 하나뿐이면 원본을 그대로 반환하는 단축 경로) |
| `ComputeFeatures` | **28** |
| **`ComputeLargestBlobFeatures` 합계** | **51** |

> 할당 횟수는 입력에 따라 달라집니다(런 개수·성분 개수만큼 벡터가 자랍니다).
> 위 51 은 "전형적인 단일 blob 128×128 crop" 의 실측값이며, 규모의 감을 잡는 용도입니다.
> 성분이 여럿이면 `SelectLargestBlob` 이 선택된 성분으로 새 리전을 만들어 몇 개 더 늘어납니다.

**비용이 어디서 나오는가 (정직하게):**

1. **`FromMask` 가 `width × height` 를 전부 훑습니다.** 128×128 이면 16384 픽셀 스캔 + 런 벡터 성장.
2. **연결성분 라벨링이 두 번 일어납니다.** 한 번은 `SelectLargestBlob` 안에서(최대 성분을 고르려고),
   또 한 번은 `ComputeFeatures` → 윤곽 추적 안에서(같은 라벨링을 다시). 값 계산 경로가 리전을
   새로 받기 때문에 앞서 만든 라벨 정보를 이어받지 못합니다.
3. **임시 `Region`/`RegionData`/`shared_ptr` 이 매 단계 새로 할당됩니다.**
   `FromMask` 가 리전 하나, `SelectLargestBlob` 이 리전 하나를 더 만듭니다(각각 `Impl` +
   `shared_ptr` 제어블록 + 런 벡터).

**개선 방향 (이번 범위 밖 — 구현하지 않음):**

- **재사용 가능한 워크스페이스/컨텍스트 API** 를 두면 위 3가지를 모두 줄일 수 있습니다.
  호출자가 스레드마다 워크스페이스 하나를 들고 다니며 넘기면,
  (a) 라벨링을 **한 번만** 하고 그 결과로 고른 성분을 곧장 특징값 계산에 넘겨 **이중 라벨링을 제거**,
  (b) 내부 벡터 버퍼를 **호출 간 재사용**해 힙 할당을 상수화할 수 있습니다.
- 현재 API 는 이 수단을 제공하지 않습니다. 초당 수만 crop 규모에서 힙 경합이 실측 병목으로
  확인되면 그때 워크스페이스 API 추가를 검토하세요(파괴적 변경 없이 **추가**로 가능합니다).
- 지금 당장 할 수 있는 완화책: **면적으로 먼저 거르기**(§6.2)는 이 함수에는 적용되지 않습니다
  (내부에서 이미 전부 계산하므로). 대신 crop 을 넘기기 전에 **crop 크기를 blob 바운딩 박스로
  최소화**하면 `FromMask` 스캔 비용이 줄어듭니다.

---

## 7. HALCON 에서 이식하기

### 7.1 한 줄 대응표

| HDevelop | 이 라이브러리 |
|---|---|
| `area_center (Region, Area, Row, Column)` | `AreaCenter(region, area, row, column);` |
| `contlength (Region, ContLength)` | `ContLength(region, contLength);` |
| `circularity (Region, Circularity)` | `Circularity(region, circularity);` |
| `compactness (Region, Compactness)` | `Compactness(region, compactness);` |
| `convexity (Region, Convexity)` | `Convexity(region, convexity);` |
| (대응 HALCON 오퍼레이터 없음) | `RegionFeatures f; ComputeFeatures(region, f);` — 위 5종을 한 번에 (§4.6) |
| `threshold (Image, Region, 128, 255)` | **대응 없음.** 사용처에서 이진화해 마스크 버퍼를 만든 뒤 `Region::FromMask` |
| `connection (Region, ConnectedRegions)` | **대응 없음.** 사용처에서 연결성분을 나눠 **blob 당 Region 하나**를 만든다 (§3.4 C-1). 단, **최대 성분 하나만** 필요하면 `SelectLargestBlob` / `ComputeLargestBlobFeatures` 로 끝낼 수 있다 (§3.6) |
| `select_shape (..., 'area', 'max', ...)` 로 최대 blob 하나 고르기 | `SelectLargestBlob(region)` 또는 원샷 `ComputeLargestBlobFeatures(mask, w, h, stride, f)` (§3.6) |
| `gen_region_runs (Region, Row, ColBegin, ColEnd)` (역방향) | `Region::FromRuns(rows, colBegins, colEnds, count)` — `ColEnd` 끝 포함 규약이 같다 |
| `select_shape (Regions, Selected, 'area', 'and', 100, 99999)` | **대응 없음.** `AreaCenter` 로 값을 받아 사용처에서 `if` 로 거른다 |
| `smallest_rectangle1` 등 나머지 | **미구현** (Phase 2 이후) |

### 7.2 전형적인 HDevelop 스크립트 → C++

```
* HDevelop
threshold (Image, Dark, 0, 128)
connection (Dark, Blobs)
select_shape (Blobs, Big, 'area', 'and', 100, 999999)
area_center (Big, Area, Row, Column)
circularity (Big, Circularity)
```

```cpp
// 이 라이브러리 (+ 사용처의 OpenCV)
cv::Mat mask;
cv::threshold(image, mask, 128, 255, cv::THRESH_BINARY_INV);		// threshold

cv::Mat labels, stats, centroids;
const int count = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);	// connection

for (int label = 1; label < count; ++label)
{
	const cv::Rect box(stats.at<int>(label, cv::CC_STAT_LEFT),
					   stats.at<int>(label, cv::CC_STAT_TOP),
					   stats.at<int>(label, cv::CC_STAT_WIDTH),
					   stats.at<int>(label, cv::CC_STAT_HEIGHT));

	cv::Mat blobMask = (labels(box) == label);

	glim::halcon::Region region = glim::halcon::Region::FromMask(
		blobMask.data, blobMask.cols, blobMask.rows, static_cast<int>(blobMask.step));

	long area = 0;
	double row = 0.0, column = 0.0;
	glim::halcon::AreaCenter(region, area, row, column);

	if (area < 100)
		continue;											// select_shape

	double circularity = 0.0;
	glim::halcon::Circularity(region, circularity);			// circularity

	const double globalRow    = row    + box.y;				// ⚠ 좌표 보정
	const double globalColumn = column + box.x;
}
```

### 7.3 값이 HALCON 과 다를 때

| 차이 | 원인 후보 |
|---|---|
| `ContLength` 가 1 픽셀 차이 | 체인 닫힘 해석 (R-1). 정사각형에서 396 vs 395 |
| `ContLength` 가 곡선에서 크다 | 정상입니다 (§5.2). HALCON 도 같은 체인 정의이므로 같은 편향이 있어야 합니다 |
| 도넛에서 `ContLength` 가 작다 | 정상입니다. 구멍 윤곽 제외 |
| `Convexity` 가 미묘하게 다르다 | hull 래스터화 규칙(경계 포함) 차이 가능성 |
| 4-이웃 설정을 쓰는 HALCON 코드와 다르다 | 본 구현은 **8-이웃 고정** (R-2). `set_system('neighborhood', 4)` 대응 없음 |

값이 다르면 [이슈 → 값 불일치 리포트](../CONTRIBUTING.md) 로 신고해 주세요.
**HALCON 실측값이 있으면 R-1 / R-2 를 한 번에 확정할 수 있습니다.**

---

## 8. FAQ / 문제 해결

**Q. OpenCV 가 있어야 하나요?**
아니요. Phase 1 은 외부 의존성이 **0** 입니다. 오히려 OpenCV 함수를 쓰면 값이 틀립니다
([`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §2.1). `cv::Mat` 어댑터는 사용처에 두세요(§3.4).

**Q. HALCON 라이선스가 필요한가요?**
아니요. HALCON SDK 나 dongle 없이 동작합니다. 이 라이브러리는 HALCON 문서의 **정의를 재현**한 것입니다.

**Q. `Region` 을 만든 뒤 원본 마스크 버퍼를 해제해도 되나요?**
네. `FromMask` 는 런렝스로 **복사·변환**합니다. 이후 원본 버퍼와의 연결은 없습니다.

**Q. `Region` 복사가 비싼가요?**
아니요. 내부적으로 `shared_ptr` 공유라 **O(1)** 입니다. 함수 인자로 값 전달해도 됩니다
(그래도 `const Region&` 를 권장합니다).

**Q. 리전을 수정(팽창/침식/합집합)할 수 있나요?**
없습니다. `Region` 은 **불변**이고 형태 연산 오퍼레이터가 없습니다. 마스크 단계에서 처리하세요.

**Q. 값을 못 구했는지 어떻게 알죠?**
현재 API 로는 알 수 없습니다. 실패·빈 리전·정상 0 이 전부 `0` 으로 보입니다.
`region.IsEmpty()` 로 사전 확인하고, 면적 0 을 무효로 취급하세요.

**Q. `area` 가 `long` 인데 오버플로 위험은?**
Windows 에서 `long` 은 32비트이므로 약 21억 픽셀에서 넘칩니다. 53580×6144 = 약 3.3억이므로
단일 프레임 규모에서는 문제가 없습니다.

**Q. 이미지 좌표가 음수여도 되나요?**
`FromRuns` 는 음수 좌표를 받습니다. 바운딩 박스 크기에 비례하는 내부 버퍼는 더 이상 없으므로(§6.3)
좌표 범위가 넓어도 메모리가 폭발하지 않습니다.

**Q. 5종을 다 뽑는데 느립니다.**
개별로 5번 부르면 윤곽 추적이 4번 반복됩니다. `ComputeFeatures` 하나로 바꾸세요(§4.6 / §6.2).
값은 완전히 동일합니다.

**Q. `C1010: unexpected end of file while looking for precompiled header` 가 납니다.**
MFC 프로젝트에 라이브러리 `.cpp` 를 직접 추가했을 때 발생합니다.
해당 파일들의 속성에서 **"미리 컴파일된 헤더 사용 안 함"** 으로 바꾸세요 (§2.2).

**Q. `LNK2038` 이 납니다.**
런타임 라이브러리(`/MD` vs `/MT`) 또는 Debug/Release 불일치입니다. §2.3 의 표를 보세요.
가장 확실한 해결은 §2.2(소스 직접 포함)입니다.

**Q. 한글 주석이 깨지고 코드가 사라집니다.**
소스의 **UTF-8 BOM 이 제거된** 경우입니다. BOM 을 유지하세요 (§2.5).

**Q. 다른 특징값(`elliptic_axis`, `orientation_region` 등)은 언제 되나요?**
Phase 2 계획입니다. 순서는 [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) §8 을 보세요.
이 저장소는 **"값이 틀린 40종보다 값이 맞는 5종"** 원칙으로 운영됩니다([`CONTRIBUTING.md`](../CONTRIBUTING.md)).

---

## 관련 문서

| 문서 | 내용 |
|---|---|
| [`02_DEFINITIONS.md`](02_DEFINITIONS.md) | 각 특징값의 정의와 수식 (구현의 유일한 근거) |
| [`03_VERIFICATION.md`](03_VERIFICATION.md) | 기준 도형 S1~S7 과 손계산 기댓값 |
| [`04_IMPLEMENTATION_NOTES.md`](04_IMPLEMENTATION_NOTES.md) | 구현 구조·검증 결과·남은 리스크(R-1/R-2) |
| [`examples/README.md`](../examples/README.md) | 예제 3종 설명과 실측 출력 |
| [`CONTRIBUTING.md`](../CONTRIBUTING.md) | 기여 규칙·이슈 템플릿 |
