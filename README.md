<div align="center">

# Halcon-Library

### HALCON 13 · Regions ▸ Features 자체 구현 엔진

**MVTec HALCON 의 리전 특징값 오퍼레이터를, SDK 없이 C++ 로 재현합니다.**

![C++](https://img.shields.io/badge/C%2B%2B-14-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Windows%20x86%20%7C%20x64-0078D6?style=flat-square&logo=windows&logoColor=white)
![Dependencies](https://img.shields.io/badge/dependencies-none-2EA043?style=flat-square)
![Header](https://img.shields.io/badge/public%20header-1-2EA043?style=flat-square)
![Operators](https://img.shields.io/badge/구현-14%20%2F%2040-informational?style=flat-square)
![Tests](https://img.shields.io/badge/1차%205종-PASS%2070%20%2F%20FAIL%200-2EA043?style=flat-square)
![Pending](https://img.shields.io/badge/2차%205종%20·%203차%204종-빌드%20검증%20대기-orange?style=flat-square)

</div>

---

## 이게 뭔가

검사 프로그램이 불량 Blob 의 **형상 특징값**(면적·원형도·볼록도·장단축·방향 등)을
HALCON 런타임 라이선스 없이 **HALCON 과 같은 수치로** 얻게 해주는 C++ 라이브러리입니다.

| | |
|---|---|
| 배포 형태 | **static lib** (`GlimHalcon.lib`) 또는 **DLL** (`GlimHalcon.dll` + import lib) |
| 공개 면적 | 헤더 **1개** — `include/GlimHalcon.h` |
| 의존성 | **없음.** 표준 라이브러리만 씁니다. OpenCV·Halcon 을 링크하지 않습니다 |
| 표준 / 문자셋 | C++14 / **멀티바이트**(`_MBCS`). 유니코드 매크로를 켜지 않습니다 |
| 스레드 | 오퍼레이터는 **무상태 순수 함수**. `Region` 을 스레드마다 따로 만들면 락 없이 병렬 호출 가능 |

> **성공 기준은 하나뿐입니다 — HALCON 실측값과 일치하는가.**
> 구현 개수는 성과가 아닙니다. 값이 틀린 40종보다 값이 맞는 5종이 낫습니다.

---

## 1. 검사기 프로젝트에 붙이기

### 1-1. static lib 로 링크 (권장)

DLL 보다 이쪽을 권장합니다. **CRT 버전·`/MT`·`/MD` 불일치 문제가 원천적으로 없고**,
배포 시 DLL 을 따라다니게 할 필요가 없습니다.

**① 빌드해서 산출물 얻기**

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

```
build\Release\GlimHalcon.lib      ← 이 파일
include\GlimHalcon.h              ← 이 헤더
```

**② Visual Studio 프로젝트 속성** (검사기 프로젝트에서, 구성·플랫폼 모두 동일하게)

| 속성 페이지 | 항목 | 값 |
|---|---|---|
| C/C++ ▸ 일반 | 추가 포함 디렉터리 | `C:\glim\pgm\Halcon-Library\include` |
| 링커 ▸ 일반 | 추가 라이브러리 디렉터리 | `...\build\Release` |
| 링커 ▸ 입력 | 추가 종속성 | `GlimHalcon.lib` |

**③ 반드시 맞춰야 하는 것 — 안 맞으면 링크가 깨집니다**

| 항목 | 설명 |
|---|---|
| **플랫폼** | 검사기가 x86 이면 라이브러리도 x86 (`-A Win32`). 섞이면 `LNK1112` |
| **런타임 라이브러리** | C/C++ ▸ 코드 생성 ▸ 런타임 라이브러리가 양쪽 동일해야 합니다 (`/MD` ↔ `/MD`, `/MT` ↔ `/MT`). 다르면 `LNK2038` |
| **플랫폼 도구 집합** | 되도록 같은 툴셋(v140 / v143 …)으로. 다르면 표준 라이브러리 심볼이 어긋날 수 있습니다 |
| **문자셋** | 라이브러리는 `_MBCS` 로 빌드됩니다. 공개 헤더에 문자열 타입이 없어 실제 영향은 없지만, 맞춰 두는 편이 안전합니다 |

> `LNK2019: ComputeCentralMoments2nd 외부 기호를 확인할 수 없습니다` 가 뜨면
> 라이브러리 쪽에 `src/Core/Moments.cpp` 가 빠진 것입니다. CMake 로 재생성하면 해결됩니다.

### 1-2. DLL 로 링크

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DGLIM_HALCON_BUILD_SHARED=ON
cmake --build build --config Release
```

```
build\Release\GlimHalcon.dll      ← 실행 파일 옆에 복사
build\Release\GlimHalcon.lib      ← import lib. 링커 ▸ 입력에 추가
```

사용처에서는 **아무것도 정의하지 않습니다.** 헤더가 알아서 `dllimport` 로 동작합니다.

> **DLL 을 쓸 때 지켜야 할 것**
> 공개 헤더에 `std::` 타입이 하나도 없고, `Region` 의 생성·복사·소멸이 전부 DLL 안에서
> 일어나도록 설계했습니다. 그래서 CRT 가 달라도 힙이 섞이지 않습니다.
> 대신 **헤더에 인라인 함수를 추가하지 마세요.** 사용처에서 인라인으로 할당·해제가 일어나는
> 순간 이 보장이 깨집니다.

---

## 2. 호출하기

### 2-1. 가장 흔한 경우 — 이진 마스크 crop 한 장

검사기가 이진화한 128×128 crop 을 던지면, 그 안의 **최대 blob** 하나를 골라 특징값 7개를 돌려줍니다.

```cpp
#include "GlimHalcon.h"

using namespace glim::halcon;

RegionFeatures f;
if (ComputeLargestBlobFeatures(pMask, 128, 128, 128, f))
{
	// f.area          면적 (픽셀 개수)
	// f.row, f.column 무게중심 (HALCON 관례 = y, x)
	// f.contLength    윤곽 길이 (구멍 제외)
	// f.circularity   원형도  0~1
	// f.compactness   조밀도  >= 1
	// f.convexity     볼록도  <= 1

	if (f.convexity < 0.85)
		nDefectType = DEFECT_TEAR;		// 볼록도가 낮으면 찢김
}
else
{
	// 입력이 잘못됐거나(널 포인터 / w,h <= 0 / 0 < stride < width)
	// 전경이 하나도 없다. 이때 f 는 전 필드 0 이다.
}
```

**`stride` 규약**

| 값 | 동작 |
|---|---|
| `0` 이하 | `width` 로 간주 |
| `>= width` | 그대로 사용 (행 패딩이 있는 버퍼) |
| `0 < stride < width` | **빈 리전 반환.** 성립할 수 없는 값이라 읽지 않습니다 |

### 2-2. 마스크에서 `Region` 을 만들어 개별 오퍼레이터 호출

기존 HALCON 스크립트를 1:1 로 옮길 때 씁니다. 함수 이름이 HALCON 원명과 대응됩니다.

```cpp
Region region = Region::FromMask(pMask, width, height, stride);

long   area;  double row, column;
AreaCenter(region, area, row, column);			// HALCON: area_center

double ra, rb, phi;
EllipticAxis(region, ra, rb, phi);				// HALCON: elliptic_axis

double aniso, bulk, sf;
Eccentricity(region, aniso, bulk, sf);			// HALCON: eccentricity
```

`Region` 은 값 타입입니다. 복사는 O(1)(내부 데이터 공유), 소멸은 자동입니다. `delete` 하지 마세요.

### 2-3. 값이 2개 이상 필요하면 배치 API 를 쓰세요

개별로 호출하면 **같은 중간산물을 매번 다시 계산합니다.**
1차 5종을 따로 부르면 윤곽 추적만 4번 돕니다. 배치 API 는 리전당 1회입니다.

```cpp
RegionFeatures f;
RegionMoments  m;
ComputeFeaturesAndMoments(region, f, m);		// 값은 개별 호출과 완전히 동일
```

---

## 3. 검사 결과를 CSV 로 쓰기

실제 목적지가 CSV 라면 이 형태가 됩니다. 멀티바이트 MFC 기준입니다.

```cpp
#include "GlimHalcon.h"
using namespace glim::halcon;

// 헤더는 파일을 새로 만들 때 한 번만
static void WriteCsvHeader(FILE* fp)
{
	fprintf(fp,
		"Frame,Lane,BlobNo,"
		"Area,Row,Column,ContLength,Circularity,Compactness,Convexity,"
		"M11,M20,M02,Ia,Ib,Ra,Rb,Phi,Anisometry,Bulkiness,StructureFactor,Orientation\n");
}

// blob 한 개를 한 줄로
static void WriteCsvRow(FILE* fp, int frame, int lane, int blobNo,
						const RegionFeatures& f, const RegionMoments& m)
{
	// %.9g : 유효자리를 보존하면서 지수표기 남발을 막는다.
	//        %f 로 쓰면 M20 같은 큰 값에서 자리수가 잘리고, 엑셀에서 되돌릴 수 없다.
	fprintf(fp,
		"%d,%d,%d,"
		"%ld,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,"
		"%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n",
		frame, lane, blobNo,
		f.area, f.row, f.column, f.contLength, f.circularity, f.compactness, f.convexity,
		m.m11, m.m20, m.m02, m.ia, m.ib,
		m.ra, m.rb, m.phi, m.anisometry, m.bulkiness, m.structureFactor, m.orientation);
}

// 검사 루프
void CInspector::SaveBlobFeatures(int frame, int lane,
								  const unsigned char* pMask, int w, int h, int stride)
{
	Region region = Region::FromMask(pMask, w, h, stride);
	Region blob   = SelectLargestBlob(region);		// 최대 blob 하나만

	RegionFeatures f;
	RegionMoments  m;
	ComputeFeaturesAndMoments(blob, f, m);

	FILE* fp = fopen(m_strCsvPath, "at");			// 멀티바이트 경로
	if (fp == NULL)
		return;

	if (_filelength(_fileno(fp)) == 0)
		WriteCsvHeader(fp);

	WriteCsvRow(fp, frame, lane, 0, f, m);
	fclose(fp);
}
```

**CSV 로 쓸 때 실수하기 쉬운 것 3가지**

| | |
|---|---|
| **`%f` 로 쓰지 마세요** | `M20` 은 큰 면적에서 `1e17` 규모까지 갑니다. `%f` 는 자리수를 잘라버려 되돌릴 수 없습니다. `%.9g` 또는 `%.17g` 를 쓰세요 |
| **프레임당 열지 마세요** | 매 프레임 `fopen`/`fclose` 는 실시간 경로에서 비쌉니다. 핸들을 유지하거나 메모리에 모아 배치로 flush 하세요 |
| **`Anisometry = 0` 을 필터로 지우지 마세요** | 아래 §5 를 보세요. 가장 길쭉한 불량이 0 을 냅니다 |

---

## 4. 구현된 오퍼레이터 — 14 / 40

### 4-1. 원문 그룹 G1~G5 — 40종을 어떻게 나눴는가

HALCON 13 Operator Reference 의 `Regions ▸ Features` 40종을 **원문 아카이브 5개 그룹**으로
나눠 수집했습니다. 이 분류는 편의상 붙인 것이 아니라, **원문에서 공통 사항·수식 규약을
공유하는 단위**입니다(예: G4 는 7종 전부가 같은 정규화 비교표를 씁니다).

| 그룹 | 분류 | 원문 | 종수 | 구현 | 진행 |
|:--:|---|---|:--:|:--:|---|
| **G1** | 기본 / 크기 | [`G1_basic_size.md`](docs/reference/halcon13/G1_basic_size.md) | 8 | **6** | `██████░░` 75% |
| **G2** | 형상 계수 | [`G2_shape_factors.md`](docs/reference/halcon13/G2_shape_factors.md) | 8 | **6** | `██████░░` 75% |
| **G3** | 외접 / 내접 / 런렝스 | [`G3_geometry_runlength.md`](docs/reference/halcon13/G3_geometry_runlength.md) | 7 | 0 | `░░░░░░░` 0% |
| **G4** | 모멘트 | [`G4_moments.md`](docs/reference/halcon13/G4_moments.md) | 7 | **2** | `██░░░░░` 29% |
| **G5** | 선택 / 관계 | [`G5_select_relation.md`](docs/reference/halcon13/G5_select_relation.md) | 10 | 0 | `░░░░░░░░░░` 0% |
| | **합계** | 2,094줄 | **40** | **14** | **35%** |

> **구현 순서는 그룹 순서가 아닙니다.** 그룹은 *원문의* 묶음이고, 구현은 그것을 가로질러
> **"값을 해석적으로 검증할 수 있는 5종 조합"** 단위(배치)로 진행합니다.
> 그래서 1차 배치는 G1 2종 + G2 3종이었고, 3차 배치는 G1 4종이었습니다.

<details>
<summary><b>그룹별 40종 전체 목록 펼치기</b></summary>

<br/>

**G1 — 기본 / 크기 (6 / 8)**

| 오퍼레이터 | 상태 | 비고 |
|---|:--:|---|
| `area_center` | ✅ | 1차 배치 |
| `area_holes` | ✅ | 3차 배치 / 배경 4-연결 (미결 `HC-1`·`HC-3`) |
| `contlength` | ✅ | 1차 배치 / 구멍 제외 |
| `diameter_region` | ✅ | 3차 배치 / hull + 회전 캘리퍼스 (판정불가 `DR-2`) |
| `connect_and_holes` | ✅ | 3차 배치 / 전경 8 ↔ 배경 4 |
| `euler_number` | ✅ | 3차 배치 / `NumConnected − NumHoles` 코어 공유 |
| `get_region_thickness` | ⬜ | 단일 리전만, 첫 성분만 |
| `region_features` | ⬜ | 63 feature 파사드 |

**G2 — 형상 계수 (6 / 8)**

| 오퍼레이터 | 상태 | 비고 |
|---|:--:|---|
| `circularity` | ✅ | 1차 배치 / `min` 클리핑 |
| `compactness` | ✅ | 1차 배치 / `max` 클리핑 (방향 반대) |
| `convexity` | ✅ | 1차 배치 / `F_o / F_c` |
| `eccentricity` | ✅ | 2차 배치 / `Rb=0` 방어 (미결 `EC-1`) |
| `elliptic_axis` | ✅ | 2차 배치 / Phi 부호 함정 (미결 `EA-2`) |
| `orientation_region` | ✅ | 2차 배치 / 미결 `OR-2` |
| `rectangularity` | ⬜ | **닫힌 수식 없음 — 최난도** |
| `roundness` | ⬜ | 4출력 |

**G3 — 외접 / 내접 / 런렝스 (0 / 7)**

| 오퍼레이터 | 상태 | 비고 |
|---|:--:|---|
| `inner_circle` | ⬜ | |
| `inner_rectangle1` | ⬜ | "largest" 기준 원문 미명시 |
| `smallest_circle` | ⬜ | radius **+0.5 보정** |
| `smallest_rectangle1` | ⬜ | |
| `smallest_rectangle2` | ⬜ | Length = **반변** (OpenCV 대비 2배) |
| `runlength_distribution` | ⬜ | index 0 은 항상 0 |
| `runlength_features` | ⬜ | LFactor 정의 모호 |

**G4 — 모멘트 (2 / 7)**

| 오퍼레이터 | 상태 | 비고 |
|---|:--:|---|
| `moments_region_2nd` | ✅ | 2차 배치 / **정규화 없음** (5출력) |
| `moments_region_2nd_invar` | ✅ | 2차 배치 / `F²` 정규화 (미결 `MI-1`) |
| `moments_region_2nd_rel_invar` | ⬜ | PHI1, PHI2 — **다음 배치 후보** |
| `moments_region_3rd` | ⬜ | 부호 규약이 여기서부터 값을 바꾼다 |
| `moments_region_3rd_invar` | ⬜ | `mu_00^3` 정규화 |
| `moments_region_central` | ⬜ | I1~I4 |
| `moments_region_central_invar` | ⬜ | PSI1~PSI4, 아핀 불변 |

**G5 — 선택 / 관계 (0 / 10)**

| 오퍼레이터 | 상태 | 비고 |
|---|:--:|---|
| `select_shape` | ⬜ | **63 feature 이름 체계의 근간** |
| `select_shape_proto` | ⬜ | Operation 파라미터 없음 |
| `select_shape_std` | ⬜ | |
| `select_region_spatial` | ⬜ | |
| `select_region_point` | ⬜ | |
| `spatial_relation` | ⬜ | |
| `hamming_distance` | ⬜ | Similarity 수식 미확보 |
| `hamming_distance_norm` | ⬜ | 출력명은 `Distance` |
| `find_neighbors` | ⬜ | MaxDistance 범위 1~255 |
| `get_region_index` | ⬜ | |

정의·검증·구현 3단 상태는 [`docs/00_OVERVIEW.md` §7](docs/00_OVERVIEW.md) 이 원본입니다.

</details>

### 4-2. 배치별 구현 현황

### 1차 배치 · 기본 형상 ✅ 실측 통과 (PASS 70 / FAIL 0)

| 오퍼레이터 | C++ 함수 | 출력 |
|---|---|---|
| `area_center` | `AreaCenter` | 면적 · 무게중심 |
| `contlength` | `ContLength` | 윤곽 길이 (**구멍 제외**) |
| `circularity` | `Circularity` | `min(1, F/(max²π))` |
| `compactness` | `Compactness` | `max(1, L²/(4Fπ))` |
| `convexity` | `Convexity` | `F_o / F_c` |

### 2차 배치 · 2차 모멘트 계열 🟡 빌드 검증 대기

| 오퍼레이터 | C++ 함수 | 출력 | 정규화 |
|---|---|---|---|
| `moments_region_2nd` | `MomentsRegion2nd` | `M11 M20 M02 Ia Ib` | **없음** (순수 합) |
| `moments_region_2nd_invar` | `MomentsRegion2ndInvar` | `M11 M20 M02` | **`F²`** |
| `elliptic_axis` | `EllipticAxis` | `Ra Rb Phi` | 내부 **`F`** |
| `eccentricity` | `Eccentricity` | `Anisometry Bulkiness StructureFactor` | — |
| `orientation_region` | `OrientationRegion` | `Phi` (`-π ≤ φ < π`) | — |

### 3차 배치 · 구멍 검출 계열 🟡 빌드 검증 대기

| 오퍼레이터 | C++ 함수 | 출력 | 연결성 |
|---|---|---|---|
| `area_holes` | `AreaHoles` | `Area` (구멍 픽셀 총합) | 배경 **4-연결** |
| `connect_and_holes` | `ConnectAndHoles` | `NumConnected NumHoles` | 전경 **8** ↔ 배경 **4** |
| `euler_number` | `EulerNumber` | `EulerNumber` (**음수 가능**) | 위와 같은 코어 |
| `diameter_region` | `DiameterRegion` | `Row1 Column1 Row2 Column2 Diameter` | — (hull) |

> **전경과 배경의 연결성은 서로 반대여야 합니다.** 둘 다 8-연결로 잡으면 대각으로만 닫힌
> 고리에서 "구멍 안이 바깥과 통하는" 위상적 모순이 생겨, 세 값이 동시에 틀립니다.
> 원문에 명시가 없어 미결(`HC-1`)로 등록하고 내부 스위치로 뒤집을 수 있게 뒀습니다.

### HALCON 원명 대응이 없는 추가 API 7개

| 함수 | 용도 |
|---|---|
| `ComputeFeatures(region, RegionFeatures&)` | 1차 5종의 값 7개를 한 번에 |
| `ComputeMoments(region, RegionMoments&)` | 2차 5종의 값 15개를 한 번에 |
| `ComputeTopology(region, RegionTopology&)` | 3차 4종의 값 9개를 한 번에 |
| `ComputeFeaturesAndMoments(region, f, m)` | 7값 + 15값. 윤곽 추적 1회 · hull 1회 · 모멘트 1회 |
| `ComputeFeaturesMomentsAndTopology(region, f, m, t)` | 위에 9값까지. 컨텍스트 하나로 전부 |
| `SelectLargestBlob(region)` | 최대 연결성분(8-연결) 하나만 남긴 리전. 동률이면 스캔 순서상 먼저인 것 |
| `ComputeLargestBlobFeatures(data, w, h, stride, f)` | 검사기 원샷. **이 함수만 `bool` 반환** |

> **왜 `ComputeLargestBlobFeatures` 만 `bool` 인가**
> 다른 함수는 이미 검증을 통과한 `Region` 을 받으므로 "실패" 가 사실상 빈 리전뿐입니다.
> 그러나 이 함수는 원시 포인터와 치수를 직접 받아 **입력 자체가 틀릴 수 있고**,
> 그것은 "전경이 없는 정상 리전(면적 0)" 과 의미가 다릅니다. 검사기가 둘을 구분하지 못하면
> 조용히 틀린 판정을 내립니다.

전체 40종 현황은 [`docs/00_OVERVIEW.md`](docs/00_OVERVIEW.md) 에 있습니다.

---

## 5. 값 규약 — 모르면 오판합니다

### 빈 리전 / 실패는 전부 `0`

HALCON 규약을 그대로 따릅니다. 예외를 공개 API 밖으로 던지지 않습니다.

### ⚠ `Anisometry` 는 1픽셀 두께 리전에서 `0` 입니다

픽셀을 면적 1의 사각형이 아니라 **무한소 점**으로 보기 때문에 `Rb = 0` 이 되고,
`Anisometry = Ra/Rb` 가 정의될 수 없어 `0` 이 됩니다. **HALCON 실제 동작을 재현한 것입니다.**

```cpp
if (m.anisometry > 5.0)  nType = SCRATCH;   // ✗ 가장 길쭉한 1px 스크래치를 놓칩니다
```

```cpp
if (m.rb == 0.0 || m.anisometry > 5.0)  nType = SCRATCH;   // ✓
```

같은 이유로 `StructureFactor` 는 이때 `-1.0` 입니다. 자세한 내용은
[`docs/05_USER_GUIDE.md` §4.8](docs/05_USER_GUIDE.md).

### ⚠ `RegionFeatures` · `RegionMoments` · `RegionTopology` 는 POD 이고, 레이아웃이 곧 ABI 입니다

필드를 추가하면 `sizeof` 가 바뀌어, **옛 헤더로 컴파일된 호출부가 링크는 되면서 스택을 넘겨 씁니다.**

- 필드는 **끝에만** 추가합니다 (중간 삽입·순서 변경·타입 변경 금지)
- 필드를 추가한 버전으로 올릴 때는 **라이브러리와 검사기를 함께 재컴파일**합니다

---

## 6. 내부가 어떻게 돌아가는가

```
┌──────────────────────────────────────────────────────────┐
│  Facade    namespace glim::halcon                        │  ← 사용처가 보는 전부
│            AreaCenter() EllipticAxis() …                 │     HALCON 이름 그대로
├──────────────────────────────────────────────────────────┤
│  Feature   FeatureContext   중간산물 캐시(지연계산)        │  ← 성능의 핵심
├──────────────────────────────────────────────────────────┤
│  Core      Region (런렝스, 불변)                          │
│            Contour · ConvexHull · Moments · Holes        │
└──────────────────────────────────────────────────────────┘
        전 계층 표준 라이브러리만 사용 — 외부 의존성 없음
```

| 요소 | 어떻게 구현했나 |
|---|---|
| **`Region`** | 픽셀 배열이 아니라 **런렝스**(row, colBegin, colEnd) 목록입니다. 정렬·중복·인접을 정규화해 보관하므로, 면적과 모멘트를 **O(면적)이 아니라 O(런 수)** 로 계산합니다. PIMPL + 내부 공유라 복사가 O(1) 입니다 |
| **윤곽 추적** | 8-연결 런 라벨링(union-find) 후 Moore 경계 추적. 픽셀 소속 판정을 dense 라벨 이미지가 아니라 **런 이진탐색**으로 하기 때문에 메모리가 O(런 수) 입니다(풀프레임에서 1.3GB 짜리 라벨 이미지가 사라집니다) |
| **`ContLength`** | 체인 스텝을 직교 1 · 대각 √2 로 누적합니다. **구멍 윤곽은 제외**합니다(원문 명시) |
| **`ConvexHull`** | monotone chain 으로 껍질을 구한 뒤 **스캔라인 래스터화해 픽셀을 셉니다.** 폴리곤 면적을 쓰면 정사각형에서 `convexity = 1.0203` 이 나와 HALCON 자신의 Assertion(≤1)을 위반합니다 |
| **2차 모멘트** | 런당 상수시간(등차수열 합 + Faulhaber 제곱합). 무게중심에 가장 가까운 **격자점으로 좌표를 먼저 옮기고** 소수부만 보정합니다 — 원시 모멘트를 그대로 빼면 큰 이미지에서 자리수가 통째로 날아갑니다 |
| **`Ia` / `Ib`** | 공분산 고유값을 원문 형태가 아니라 **항등 변형** `((M20−M02)/2)² + M11²` 으로 계산합니다. 원문 형태는 큰 두 수의 차라 대면적에서 유효자리가 전멸하고, 부동소수 오차로 음수가 되어 `sqrt` 가 NaN 을 냅니다. 값은 대수적으로 같습니다 |
| **구멍 검출** | 배경도 **런렝스**로 표현하고 "바깥" 가상 노드 하나로 union-find 합니다. 배경 런 수는 전경 런 수의 2배 이하라 메모리가 O(런 수)이고, 바운딩박스 크기와 무관합니다. `area_holes`·`connect_and_holes`·`euler_number` 가 이 결과 **하나**를 나눠 쓰므로 `Euler = NC − NH` 가 구조적으로 깨질 수 없습니다 |
| **`diameter_region`** | 최대 거리 쌍은 반드시 hull 정점 쌍이므로 hull 위에서 **회전 캘리퍼스**로 찾습니다. 비교는 **제곱거리 정수**로 하고 `sqrt` 는 마지막 한 번뿐입니다 — 부동소수로 비교하면 동률 판정이 흔들려 결과가 결정론적이지 않게 됩니다 |
| **`FeatureContext`** | 윤곽·hull·모멘트·최원점·구멍을 **리전당 1회만** 계산하고 캐시합니다. 배치 API 가 빠른 이유가 이것입니다 |
| **OpenCV 미사용** | 편의상 뺀 게 아니라 **값이 달라서** 뺐습니다. `cv::arcLength` 는 근사 폴리곤, `cv::contourArea` 는 폴리곤 면적, `cv::minAreaRect` 는 HALCON 대비 2배(반변 규약)입니다 |

설계 제약과 근거는 [`docs/01_ARCHITECTURE.md`](docs/01_ARCHITECTURE.md),
구현 세부는 [`docs/04_IMPLEMENTATION_NOTES.md`](docs/04_IMPLEMENTATION_NOTES.md).

---

## 7. 빌드

```bash
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release

cd build && ctest -C Release --output-on-failure
```

| 옵션 | 기본 | 설명 |
|---|---|---|
| `-A x64` / `-A Win32` | — | 검사기 플랫폼에 맞춥니다 |
| `-DGLIM_HALCON_BUILD_SHARED=ON` | `OFF` | DLL 로 빌드 (기본은 static lib) |
| `-DGLIM_HALCON_BUILD_EXAMPLES=OFF` | `ON` | 예제 실행파일 제외 |

생성기는 설치된 Visual Studio 에 맞춰 바꿉니다 (`Visual Studio 17 2022` 등).
실행 가능한 예제 5개가 [`examples/`](examples/) 에 있습니다.

---

## 8. 문서

> ### 📚 [문서 허브 — `docs/README.md`](docs/README.md) 가 진입점입니다.
> 읽는 순서 · 카탈로그 · **오퍼레이터별 정의/검증/원문 역인덱스**가 그곳에 있습니다.

| 문서 | 내용 |
|---|---|
| [`docs/05_USER_GUIDE.md`](docs/05_USER_GUIDE.md) | **사용처(검사기) 관점의 API 매뉴얼** |
| [`CLAUDE.md`](CLAUDE.md) | **개발 참여 전 필수.** AI 에이전트 / 신규 참여자 지침 |
| [`docs/00_OVERVIEW.md`](docs/00_OVERVIEW.md) | 범위 · 원칙 · 함정 14건 · 40종 현황표 |
| [`docs/01_ARCHITECTURE.md`](docs/01_ARCHITECTURE.md) | 설계 제약과 적용 패턴 |
| [`docs/02_DEFINITIONS.md`](docs/02_DEFINITIONS.md) | **특징값 정의서 — 구현의 유일한 근거** |
| [`docs/03_VERIFICATION.md`](docs/03_VERIFICATION.md) | 기준 도형 · 기댓값 손계산 · 검증 스크립트 |
| [`docs/04_IMPLEMENTATION_NOTES.md`](docs/04_IMPLEMENTATION_NOTES.md) | 구현 노트 · 실측 결과 |
| [`docs/reference/halcon13/`](docs/reference/halcon13/README.md) | HALCON 13 원문 아카이브 |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | 빌드·테스트·기여 절차 |

---

<div align="center">
<sub>HALCON 은 MVTec Software GmbH 의 상표입니다. 이 프로젝트는 MVTec 과 무관하며,<br/>
공개된 오퍼레이터 <b>명세</b>를 근거로 한 독립 구현입니다. HALCON 코드를 포함하지 않습니다.</sub>
</div>
