# examples — 실제로 컴파일되는 예제

이 폴더의 예제는 **외부 의존성이 0** 입니다. OpenCV 도 MFC 도 필요 없습니다.
공개 헤더 [`include/GlimHalcon.h`](../include/GlimHalcon.h) 하나만 include 합니다.

사용법 전체 설명은 [`docs/05_USER_GUIDE.md`](../docs/05_USER_GUIDE.md) 에 있습니다.

---

## 예제 목록

| 파일 | 무엇을 보여주는가 |
|---|---|
| [`01_quickstart.cpp`](01_quickstart.cpp) | 최소 예제. 마스크 버퍼 → `Region` → `AreaCenter` / `ContLength` / `Circularity` |
| [`02_from_binary_mask.cpp`](02_from_binary_mask.cpp) | 이진 마스크 버퍼로 **5종 특징값 전부** 출력. `stride` 패딩 버퍼, 큰 이미지의 **ROI 부분 전달**, 빈 리전 |
| [`03_from_runs.cpp`](03_from_runs.cpp) | **런렝스**로 직접 만들기, 정렬 안 된/겹치는 런의 자동 정규화, **좌표 목록 → 런 어댑터** |
| [`04_batch_features.cpp`](04_batch_features.cpp) | **배치 API `ComputeFeatures`** 사용법. 배치 결과 = 개별 호출 결과임을 8개 도형에서 대조하고, 잘못된 `stride` 처리도 확인 |
| [`05_largest_blob.cpp`](05_largest_blob.cpp) | **검사기 원샷 API `ComputeLargestBlobFeatures`** / `SelectLargestBlob`. 덩어리 3개에서 최대만 선택, 면적 동률 처리, 전경 없음/널 포인터/`0<stride<width` → `false`, 원샷 == 수동 조합 비트 대조 |

> `04` · `05` 는 단순 예제가 아니라 **회귀 감시**입니다. 값이 하나라도 어긋나면 **exit code 1** 을 돌려주며,
> `ctest` 에 각각 `GlimHalconBatchFeatures` · `GlimHalconLargestBlob` 으로 등록돼 있습니다.

---

## 빌드 — 방법 1: CMake (권장)

저장소 루트에서:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

실행파일은 `build\Release\` 에 생깁니다.

```bat
build\Release\01_quickstart.exe
build\Release\02_from_binary_mask.exe
build\Release\03_from_runs.exe
build\Release\04_batch_features.exe
```

예제 빌드를 끄려면 `-DGLIM_HALCON_BUILD_EXAMPLES=OFF` 를 줍니다.

## 빌드 — 방법 2: 컴파일러 한 줄 (CMake 없이)

의존성이 0 이므로 `cl.exe` 만으로 그대로 빌드됩니다.
**MSVC 개발자 명령 프롬프트**에서 저장소 루트를 기준으로 실행하세요.

```bat
cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
   src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\01_quickstart.cpp ^
   /Fe:01_quickstart.exe
```

`examples\01_quickstart.cpp` 자리를 `02_from_binary_mask.cpp` / `03_from_runs.cpp` 로 바꾸면 나머지도 같습니다.

g++ 도 됩니다.

```bash
g++ -std=c++14 -Wall -Wextra -I include -I src/Core -I src/Feature \
    src/Core/*.cpp src/Feature/*.cpp src/Facade/*.cpp examples/01_quickstart.cpp -o 01_quickstart
```

> 예제 소스는 한글 주석이 있으므로 **UTF-8 BOM** 으로 저장돼 있습니다.
> BOM 을 지우면 Visual Studio 가 CP949 로 오독해 주석 다음 줄을 삼킵니다. 지우지 마세요.

---

## 실측 출력 (MSVC 14.44 / x64 / Release, 2026-07-21)

### 01_quickstart.exe

```
Area        = 16
Row         = 2.500000
Column      = 3.500000
ContLength  = 12.000000
Circularity = 1.000000
```

### 02_from_binary_mask.exe

```
Shape                Area        Row     Column   ContLength   Circular    Compact     Convex
--------------------------------------------------------------------------------------------
Rectangle 20x20       400    14.5000    19.5000      76.0000   0.705396   1.149099   1.000000
Disk R=10             317    20.0000    20.0000      65.9411   1.000000   1.091550   1.000000
L-Shape               300    16.1667    12.8333      75.4142   0.513250   1.508604   0.869565
Padded(st=48)         400    14.5000    19.5000      76.0000   0.705396   1.149099   1.000000
Empty                   0     0.0000     0.0000       0.0000   0.000000   0.000000   0.000000

[ROI 로 잘라 넘기기]
ROI 로컬 좌표   : Area=400  Row=19.5000  Column=29.5000
전역 좌표 환산  : Row=39.5000  Column=129.5000  (ROI 원점 20, 100 을 더함)
```

`Rectangle` 과 `Padded(st=48)` 의 값이 **완전히 같습니다.** stride 패딩 영역의 쓰레기 값이
결과에 섞이지 않는다는 증거입니다.

### 03_from_runs.exe

```
Case                       Area        Row     Column   ContLength   Circular    Compact     Convex
--------------------------------------------------------------------------------------------------
FromRuns (정렬됨)           400    14.5000    19.5000      76.0000   0.705396   1.149099   1.000000
FromRuns (뒤죽박죽)         400    14.5000    19.5000      76.0000   0.705396   1.149099   1.000000
좌표목록 -> 런              400    14.5000    19.5000      76.0000   0.705396   1.149099   1.000000
```

세 줄이 모두 같습니다. 입력 런의 순서·중복·인접이 결과에 영향을 주지 않습니다.

### 04_batch_features.exe

```
Shape                Area        Row     Column   ContLength    Circular     Compact      Convex   batch==each
---------------------------------------------------------------------------------------------------------------
Rectangle 20x20       400    14.5000    19.5000    76.000000 0.705395870 1.149098689 1.000000000   MATCH
Disk R=10             317    20.0000    20.0000    65.941125 1.000000000 1.091549876 1.000000000   MATCH
L-Shape               300    16.1667    12.8333    75.414214 0.513249742 1.508604137 0.869565217   MATCH
Ring 15/7             560    20.0000    20.0000    98.911688 0.792237939 1.390264195 0.789844852   MATCH
SinglePixel             1    20.0000    20.0000     0.000000 0.000000000 1.000000000 1.000000000   MATCH
ThinLine 1x30          30    20.0000    19.5000    58.000000 0.045418771 8.923287143 1.000000000   MATCH
TwoBlobs               89    25.2135    23.7753    44.000000 0.027965230 1.731033538 0.247910864   MATCH
Empty                   0     0.0000     0.0000     0.000000 0.000000000 0.000000000 0.000000000   MATCH

[stride 검증]
stride = width (40)      : IsEmpty=false   (기대: false)
stride = 0               : IsEmpty=false   (기대: false, width 로 간주)
stride = 32 (< width)    : IsEmpty=true    (기대: true, 버퍼 침범 방어)

ALL OK
```

`MATCH` 는 `ComputeFeatures` 의 값 7개가 개별 오퍼레이터 5종 호출 결과와 **비트 단위로 같다**는 뜻입니다
(허용 오차 없이 `==` 로 비교합니다). `Rectangle` 행이 `02_from_binary_mask.exe` 와 같은 값인 것도 확인하세요.

### 05_largest_blob.exe

```
[1] 덩어리 3개 -> 최대만 선택
ThreeBlobs               area=400    row=  29.5000 col=  29.5000 cont=   76.0000 circ=0.705396 comp=1.149099 conv=1.000000

[2] 면적 동률 -> 스캔 순서상 먼저(위쪽) 선택
TieBreak(top wins)       area=100    row=   9.5000 col=   9.5000 cont=   36.0000 circ=0.785950 comp=1.031324 conv=1.000000

[3] 단일 blob -> 값 불변 (SelectLargestBlob 이 통과만)
full (FromMask)          area=400    row=  14.5000 col=  19.5000 cont=   76.0000 circ=0.705396 comp=1.149099 conv=1.000000
largest (선택 후)        area=400    row=  14.5000 col=  19.5000 cont=   76.0000 circ=0.705396 comp=1.149099 conv=1.000000

[4] 전경 없음 -> false + 전 필드 0
   ok=false (기대 false), allZero=true (기대 true)

[5] 잘못된 입력 -> false + 전 필드 0
   null data          : ok=false (기대 false)
   width = 0          : ok=false (기대 false)
   stride=32 (<width) : ok=false (기대 false)
   stride = 0         : ok=true (기대 true, width 로 간주)  area=1600

ALL OK
```

`[1]` 은 25 / 400 / 36 픽셀 덩어리 셋 중 **400 만** 뽑혔음을(무게중심이 큰 사각형 중심 29.5, 29.5),
`[2]` 는 면적 100 동률에서 **위쪽(row 9.5)** 이 선택됐음을,
`[3]` 은 단일 blob 에서 최대 선택 전후 값이 **완전히 동일**함을 보여줍니다.
`[4]`·`[5]` 는 전경 없음·입력 오류가 전부 `false` + 전 필드 0 으로 처리됨을 확인합니다.

실제 검사 마스크(128×128) 4장에 대한 원샷 == 수동 조합 비트 대조 결과는
[`docs/05_USER_GUIDE.md` §3.6](../docs/05_USER_GUIDE.md) 에 있습니다.

---

## OpenCV 를 쓰는 검사기라면

**예제에는 OpenCV 코드를 넣지 않았습니다** (의존성 0 유지).
`cv::Mat` → `Region` 어댑터는 [`docs/05_USER_GUIDE.md` 3.4](../docs/05_USER_GUIDE.md) 에
복붙 가능한 코드로 있습니다. 어댑터는 **사용처 프로젝트 쪽**에 두세요.
