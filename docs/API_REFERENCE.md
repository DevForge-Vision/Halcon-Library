# GlimHalcon SDK 공식 API Reference

문서 버전 1.0 · 2026-11-06 · 이미지 API v1 · SDK 0.0.1 소스 계열

이 문서는 저장소 구현을 기준으로 한 사용자 계약이다. HALCON 제품의 공식 문서가 아니다. HALCON 실물 수치 일치와 현장 검출 정확도는 별도 검증 대상이다.

## 1. 시작하기

공개 헤더 `GlimImage.h`, 네임스페이스 `glim::halcon`, 링크 파일 `GlimHalcon.lib`, 실행 파일 `GlimHalcon.dll`. [툴셋 연결 방법](TOOLSET_GUIDE.md)을 먼저 적용한다. C++14 이상이며 MFC 프로그램에서도 그대로 호출한다.

```text
원본 메모리 또는 BMP → AnalyzeImage / AnalyzeBmpFile → ImageResult
  → GetBlobCount → GetBlobFeatures → Blob별 수치
  → GroupBlobDefects → DefectResult → GetDefectFeatures → 그룹별 수치
  → CopyBlobMask / CopyDefectMask → 표시용 0/255 마스크
  → ReleaseImageResult / ReleaseDefectResult
```

모든 이미지 함수는 동기식이다. UI에서 오래 걸릴 입력은 작업 스레드에서 호출하고 화면 갱신은 UI 스레드에서 수행한다. 서로 다른 결과는 독립적이며 같은 결과의 동시 읽기를 허용한다. 읽는 동안 Release를 호출하면 안 된다.

## 2. 공통 반환값과 소유권

Release 함수만 `void`, 나머지 이미지/그룹 함수는 `ImageStatus`를 반환한다.

| 값 | 이름 | 의미와 조치 |
|---:|---|---|
| 0 | Ok | 호출 성공. Blob 0개도 정상 결과 |
| 1 | InvalidArgument | null, 범위 밖 인덱스/ROI/설정, 부족한 출력 버퍼 등 확인 |
| 2 | UnsupportedFormat | 지원하지 않는 픽셀/BMP 형식 또는 일부 손상 BMP |
| 3 | FileError | 파일 열기/읽기 실패 |
| 4 | ResourceLimit | 픽셀·Blob·run·그룹 후보 수 제한 초과. 부분 결과 없음 |
| 5 | ProcessingFailed | 내부 계산 실패 또는 처리 중 예외 |
| 6 | OutOfMemory | 메모리 할당 실패 |
| 7 | VersionMismatch | 구조체 크기 또는 apiVersion 불일치. 헤더/DLL 짝 확인 |

`ImageView.structSize=sizeof(ImageView)`. 설정은 `GetDefault...`로 초기화하고 일부만 변경한다. `bytes` 인자는 항상 해당 구조체의 `sizeof(...)`이며 정확히 일치해야 한다. 임의 구조체 재정의/필드 추가/pack 변경을 하지 않는다.

Analyze/Group 호출은 출력 포인터가 유효하면 먼저 `*result=nullptr`로 만든다. 기존 핸들을 출력 변수에 덮어쓰면 누수이므로 먼저 해제한다. 성공 핸들은 라이브러리 소유이며 `delete`/`free` 금지. Release에 null은 허용하지만 이중 해제, 다른 DLL의 핸들, 해제 후 사용은 금지한다. 임의 주소의 안전성을 검사하는 API가 아니다.

출력 feature는 성공 시에만 사용한다. 잘못된 bytes/null 때문에 실패하면 기존 출력 내용이 그대로일 수 있다. count 조회는 count 포인터가 유효하면 0으로 초기화한다. 입력 이미지 버퍼는 호출이 끝날 때까지 읽기 가능한 상태로 유지하고 동시에 변경하지 않는다. 결과 생성 후 입력 버퍼는 해제 가능하다.

## 3. 입력 구조체

### ImageView

| 필드 | 전달값 |
|---|---|
| structSize | `sizeof(ImageView)` |
| data | 좌상단 첫 픽셀 포인터, 호출자 소유 |
| dataBytes | data에서 읽을 수 있는 바이트 수 |
| width, height | 양수, 원본 이미지 폭/높이 |
| strideBytes | 양수, 한 행 시작부터 다음 행 시작까지 바이트 수 |
| format | `Gray8=1`, `Bgr24=3`, `Bgra32=4` |

행 순서는 top-down. 음수 stride는 지원하지 않는다. `strideBytes >= width*채널수`, `dataBytes >= (height-1)*strideBytes + width*채널수`. 입력은 최대 16,777,216픽셀. 16bit/RGB 순서 메모리는 직접 지원하지 않으므로 호출 전에 변환한다. BGRA의 alpha는 무시한다. 컬러의 grayscale은 `(29*B + 150*G + 77*R + 128) >> 8`이다.

### ImageSettings

| 필드 | 기본값 | 허용값/설명 |
|---|---:|---|
| structSize / apiVersion | sizeof / 1 | 기본 설정 함수가 채움 |
| thresholdMode | LocalMean | Fixed=0, LocalMean=1, NonZeroMask=2 |
| foreground | Bright | Bright=0, Dark=1 |
| threshold | 10 | 유한값 0..255 |
| backgroundRadius | 9 | 1..1024, 평균 창 반경 |
| minArea | 12 | 1 이상, 이보다 작은 Blob 제외 |
| maxArea | 0 | 0=상한 없음, 나머지는 minArea 이상 |
| maxBlobs | 4096 | 1..100000, 면적 필터 통과 Blob 수 상한 |
| roiX / roiY | 0 / 0 | 원본 좌표, 음수 불가 |
| roiWidth / roiHeight | 0 / 0 | 둘 다 0이고 x/y도 0이면 전체. 그 외 양수이며 원본 안에 포함 |

Fixed: Bright는 `gray > threshold`, Dark는 `gray < threshold`. 등호는 포함하지 않는다.

LocalMean: ROI 안의 중심 픽셀을 포함한 `(2r+1)` 창을 ROI 경계에서 자른 평균을 사용한다. Bright는 `gray > mean+threshold`, Dark는 `gray < mean-threshold`.

NonZeroMask: grayscale이 0이 아닌 픽셀을 전경으로 취급한다. foreground/threshold는 분할에 쓰이지 않지만 설정값 검증은 동일하게 적용한다. 이미 이진화한 마스크에는 이 모드를 사용한다. 전경 연결성은 8-연결이다.

### DefectGroupingSettings

| 필드 | 기본값 | 허용값/설명 |
|---|---:|---|
| structSize / apiVersion | sizeof / 1 | 기본 설정 함수가 채움 |
| axisAngleDegrees | 90 | 유한값 [0,180), 0=오른쪽, 90=아래쪽 |
| maxAlongGap | 12 | 0..16777216, 축 투영 픽셀 중심 구간 사이 거리의 허용 상한 |
| maxAcrossOffset | 4 | 0..16777216, 그룹 전체의 횡방향 Blob 무게중심 범위 상한 |
| maxGroupLength | 0 | 0=길이 제한 끔, 그 외 0..16777216의 축 투영 범위 상한 |
| maxInputBlobs | 4096 | 1..4096. 초과하면 전체 실패 |

거리 조건은 상한을 포함한다. 작은 gap/offset 순으로 병합하고 전체 그룹 조건을 재검사하는 결정적 greedy 방식이다. 단순 연결성분이나 최적 군집화와 같지 않다. 최대 후보 edge는 1,000,000개이며 초과하면 ResourceLimit. 원래 Blob 픽셀의 합집합만 만들고 끊긴 틈을 채우지 않는다.

## 4. 이미지 함수 상세

아래의 타입은 모두 `glim::halcon` 안에 있다. 선언의 내보내기 매크로는 생략했다. 실제 호출 규약은 `GLIM_HALCON_CALL`이다.

### GetDefaultImageSettings
```cpp
ImageStatus GetDefaultImageSettings(ImageSettings* settings, std::uint32_t bytes);
```
입력: 쓸 수 있는 settings 포인터와 `sizeof(ImageSettings)`. 출력: 3절의 기본값 및 크기/버전. null은 InvalidArgument, 크기 불일치는 VersionMismatch. 성공 후 threshold/minArea 등을 바꾼다.

### AnalyzeImage
```cpp
ImageStatus AnalyzeImage(const ImageView* image, const ImageSettings* settings, ImageResult** result);
```
입력: 이미지 view, 초기화된 설정, `ImageResult* raw=nullptr`의 주소 `&raw`. 출력: Blob 수/feature/마스크를 보관하는 새 핸들. 영상 데이터는 호출 중 읽어 내부 grayscale로 복사한다. 원본을 수정하지 않는다. 픽셀/설정/자원/처리 실패는 2절 상태값으로 반환한다.

### AnalyzeBmpFile
```cpp
ImageStatus AnalyzeBmpFile(const char* path, const ImageSettings* settings, ImageResult** result);
```
입력: null 종료 BMP 경로, 설정, 결과 포인터 주소. Windows 경로는 현재 ANSI 코드 페이지. 비압축 8bit palette/24bit BGR/32bit BGRX, DIB 크기 40/108/124 지원. BMP top-down/bottom-up을 내부 top-down으로 정규화한다. JPG/PNG/압축 BMP는 지원하지 않는다. 출력과 수명은 AnalyzeImage와 같다. 파일명을 바꾸는 것만으로 포맷 변환되지 않는다.

### GetBlobCount
```cpp
ImageStatus GetBlobCount(const ImageResult* result, int* count);
```
입력: 살아 있는 결과와 int 출력 주소. 출력: 면적 필터를 통과한 Blob 수. 두 인자 모두 필수. 0개면 순회를 생략한다.

### GetBlobFeatures
```cpp
ImageStatus GetBlobFeatures(const ImageResult* result, int index,
    BlobFeatures* features, std::uint32_t bytes);
```
입력: 결과, `0 <= index < count`, 출력 구조체, `sizeof(BlobFeatures)`. 출력: 선택 Blob의 bbox/gray/shape/moments/topology 사본. index를 영구 결함 ID나 면적순 순위로 사용하지 않는다. 잘못된 index는 InvalidArgument, bytes 불일치는 VersionMismatch.

### CopyBlobMask
```cpp
ImageStatus CopyBlobMask(const ImageResult* result, int index,
    unsigned char* data, std::uint64_t bytes, int stride);
```
입력: 결과, Blob index, 호출자 소유 출력 버퍼, 버퍼 크기, 양수 stride. `stride >= 원본 width`, `bytes >= (원본 height-1)*stride+원본 width`. 출력: **원본 전체 크기** Gray8 마스크, 선택 픽셀=255/나머지=0. bbox crop 크기가 아니다. 행 padding은 수정하지 않는다. 예: 128×128이면 stride128, bytes16384.

### ReleaseImageResult
```cpp
void ReleaseImageResult(ImageResult* result);
```
입력: 해당 DLL에서 만든 살아 있는 핸들 또는 null. 출력 없음. 호출 후 호출자의 포인터는 자동 null이 되지 않으므로 직접 null로 만들거나 RAII를 사용한다.

## 5. 그룹 함수 상세

### GetDefaultDefectGroupingSettings
```cpp
ImageStatus GetDefaultDefectGroupingSettings(DefectGroupingSettings* settings, std::uint32_t bytes);
```
입력: 쓸 수 있는 설정과 `sizeof(DefectGroupingSettings)`. 출력: 3절 기본값. 오류 규칙은 GetDefaultImageSettings와 같다.

### GroupBlobDefects
```cpp
ImageStatus GroupBlobDefects(const ImageResult* source,
    const DefectGroupingSettings* settings, DefectResult** result);
```
입력: 이미지 결과, 그룹 설정, 새 그룹 핸들을 받을 주소. 출력: 독립 snapshot. 함수 성공 후 source를 해제해도 그룹 조회 가능. 단일 Blob도 그룹 하나가 된다. 0 Blob은 0개 그룹인 성공 결과. 설정/자원 오류 시 부분 그룹을 반환하지 않는다.

### GetDefectCount
```cpp
ImageStatus GetDefectCount(const DefectResult* result, int* count);
```
입력: 살아 있는 그룹 결과, 출력 int 주소. 출력: 그룹 수. null은 InvalidArgument.

### GetDefectFeatures
```cpp
ImageStatus GetDefectFeatures(const DefectResult* result, int index,
    DefectFeatures* features, std::uint32_t bytes);
```
입력: 결과, `0 <= index < 그룹 수`, 출력 구조체, `sizeof(DefectFeatures)`. 출력: 합집합 픽셀에서 다시 계산한 그룹 feature. 면적에 틈의 픽셀은 포함하지 않는다. 평균/표준편차는 원래 픽셀 분포 기준이며 조각별 평균의 단순 평균이 아니다.

### GetDefectMemberBlobIndex
```cpp
ImageStatus GetDefectMemberBlobIndex(const DefectResult* result, int index,
    int memberIndex, int* blobIndex);
```
입력: 그룹 결과, 그룹 index, `0 <= memberIndex < memberCount`, 출력 int 주소. 출력: 그룹 생성에 사용한 **원래 ImageResult의 Blob index**. 원본을 해제한 뒤에도 번호 조회는 가능하지만 그 번호로 원본 feature를 조회하려면 원본 핸들을 보관해야 한다. 범위 오류/null은 InvalidArgument.

### CopyDefectMask
```cpp
ImageStatus CopyDefectMask(const DefectResult* result, int index,
    unsigned char* data, std::uint64_t bytes, int stride);
```
입력과 출력 버퍼 규칙은 CopyBlobMask와 같다. index는 그룹 번호. 출력은 그룹 모든 조각의 원본 크기 합집합 마스크다. 끊어진 간격은 그대로 0이다.

### ReleaseDefectResult
```cpp
void ReleaseDefectResult(DefectResult* result);
```
그룹 핸들을 해제한다. null 허용, 이중 해제 금지. 원본 ImageResult의 해제 순서와 독립적이다.

## 6. 결과 필드 사전

좌표는 원본 이미지 기준이며 ROI 좌상단을 빼지 않는다. 길이는 pixel, 면적은 픽셀 개수다. mm 변환은 카메라 보정값을 사용자가 적용한다.

| BlobFeatures / DefectFeatures 필드 | 결과 의미 |
|---|---|
| structSize | 반환 구조체 크기 |
| blobIndex / defectIndex | 해당 결과 내 0부터 시작하는 번호 |
| memberCount | DefectFeatures에만 존재, 원본 Blob 조각 수 |
| x, y, width, height | 축 정렬 bbox, width=maxX-minX+1 |
| touchesImageBorder | 원본 영상 경계에 닿으면 1 |
| touchesRoiBorder | 분석 ROI 경계에 닿으면 1 |
| meanGray | 원본 grayscale의 픽셀 평균, 0..255 |
| deviationGray | 모집단 표준편차, 분모 N |
| shape | 아래 RegionFeatures |
| moments | 아래 RegionMoments |
| topology | 아래 RegionTopology |

| RegionFeatures 필드 | 의미 |
|---|---|
| area | 전경 픽셀 수(long) |
| row / column | 무게중심 y / x |
| contLength | 외곽 윤곽 길이, 구멍 윤곽 제외 |
| circularity | 원형도, 0..1 |
| compactness | compactness, 비어 있지 않은 일반 영역은 1 이상 |
| convexity | 원본 면적 / convex hull 면적 |

| RegionMoments 필드 | 의미 |
|---|---|
| m11 / m20 / m02 | 중심 기준 교차/행제곱/열제곱 모멘트의 합, 면적 정규화 없음 |
| ia / ib | 위 모멘트 행렬의 큰/작은 고유값 |
| m11Invar / m20Invar / m02Invar | 각 모멘트 / area² |
| ra / rb | 등가 타원 주/부 반지름, 전체 축 길이가 아님 |
| phi | 주축 각도(rad), 기본 범위 (-π/2,π/2] |
| anisometry | ra/rb. rb=0일 때 0 |
| bulkiness | π*ra*rb/area |
| structureFactor | anisometry*bulkiness-1. 비어 있지 않은 선형 영역에서 -1 가능 |
| orientation | 방향각(rad), 기본 범위 [-π,π) |

moments.phi의 주축 벡터는 `(x,y)=(cos(phi),-sin(phi))`이다. 그룹 설정의 axisAngleDegrees(화면 아래가 +90도)와 부호/단위가 다르다. 대칭 도형의 방향은 유일하지 않을 수 있다.

| RegionTopology 필드 | 의미 |
|---|---|
| areaHoles | 둘러싸인 배경 픽셀 수 |
| numConnected | 전경 8-연결 성분 수. 그룹화 후에도 끊어진 조각 수 유지 |
| numHoles | 배경 구멍 개수, 기본 배경 4-연결 |
| eulerNumber | numConnected-numHoles, 음수 가능 |
| diameterRow1 / diameterColumn1 | 가장 먼 픽셀 중심 쌍의 첫 y/x |
| diameterRow2 / diameterColumn2 | 가장 먼 픽셀 중심 쌍의 두 번째 y/x |
| diameter | 두 중심의 유클리드 거리. 단일 픽셀이면 0 |

수식 및 HALCON 대조 미결 사항은 [정의서](02_DEFINITIONS.md)를 참조한다. 묶인 흠집의 ra/rb는 그룹 전체 분포의 등가 타원 값이며 실제 선폭/곡선 길이 측정값으로 바로 해석하지 않는다.

## 7. 복사해서 사용하는 BMP 예제

```cpp
#include "GlimImage.h"
#include <memory>
#include <cstdio>

int AnalyzeFile(const char* path)
{
    using namespace glim::halcon;
    try
    {
        ImageSettings settings = {};
        if (GetDefaultImageSettings(&settings, sizeof(settings)) != ImageStatus::Ok) return 1;
        settings.threshold = 10;
        settings.minArea = 12;
        ImageResult* raw = nullptr;
        const auto status = AnalyzeBmpFile(path, &settings, &raw);
        std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)> result(raw, &ReleaseImageResult);
        if (status != ImageStatus::Ok) return static_cast<int>(status);
        int count = 0;
        if (GetBlobCount(result.get(), &count) != ImageStatus::Ok) return 1;
        for (int i = 0; i < count; ++i)
        {
            BlobFeatures blob = {};
            if (GetBlobFeatures(result.get(), i, &blob, sizeof(blob)) != ImageStatus::Ok) return 1;
            std::printf("Blob=%d Area=%ld Center=(%.2f,%.2f) Mean=%.2f\n",
                i, blob.shape.area, blob.shape.row, blob.shape.column, blob.meanGray);
        }
        return 0;
    }
    catch (...) { return 1; }
}
```

OpenCV 사용처는 CV_8UC1/3/4의 data/cols/rows/step을 ImageView에 연결할 수 있다. step의 int 범위와 실제 연속 접근 가능 바이트를 검증하고, ROI Mat라면 부모 메모리 수명도 유지한다. SDK에는 OpenCV 타입을 전달하지 않는다.

## 8. 입력과 결과가 확정된 그룹 예제

[컴파일 가능한 전체 예제](../examples/07_api_contract.cpp)는 8×8 Gray8에 `(x=3,y=1,2,5,6)` 네 픽셀을 200으로 채운다. NonZeroMask, minArea1로 분석하고 axis90/maxAlongGap3/maxAcrossOffset0으로 그룹화한다.

예상 출력:
```text
Blobs=2 Defects=1 Members=2 Area=4 Row=3.5 Column=3.0 Mean=200.0
```

두 Blob 면적은 각 2. 그룹 bbox는 (3,1,1,6), 표준편차0, numConnected2. y=3,4의 두 빈 픽셀은 mask에서 여전히 0이다. gap은 빈 픽셀 개수2가 아니라 픽셀 중심 구간 사이 거리 `5-2=3`이다. 예제는 source 해제 후 그룹 읽기, 합집합 mask와 수치까지 검사하며 CTest `GlimApiDocumentExample`로 실행된다.

## 9. 저수준 Region API

이미 분할된 리전의 개별 형상만 필요하면 `GlimHalcon.h`를 사용한다. 이 API는 ImageStatus 계약과 다르며 빈 리전/계산 실패를 대체로 0으로 축약한다. 오류 구분이 필요한 신규 이미지 연동에는 위 이미지 API를 권장한다.

| 함수 | 입력 → 출력 |
|---|---|
| Region::FromMask(data,width,height,stride) | Gray8 비영(전경) 마스크 → Region. stride<=0이면 width |
| Region::FromRuns(rows,colBegins,colEnds,count) | 끝 포함 run 배열 → 정규화 Region |
| Region::IsEmpty() | 리전 → 비었으면 true |
| AreaCenter(region,area,row,column) | Region → 면적, 중심 y/x |
| ContLength(region,value) | Region → 외곽 길이 |
| Circularity(region,value) | Region → 원형도 |
| Compactness(region,value) | Region → compactness |
| Convexity(region,value) | Region → convexity |
| MomentsRegion2nd(region,m11,m20,m02,ia,ib) | Region → 원시 중심 모멘트/고유값 |
| MomentsRegion2ndInvar(region,m11,m20,m02) | Region → 면적² 정규화 모멘트 |
| EllipticAxis(region,ra,rb,phi) | Region → 타원 반지름/각도 |
| Eccentricity(region,anisometry,bulkiness,structureFactor) | Region → 타원 비율 기반 3값 |
| OrientationRegion(region,phi) | Region → 방향각 |
| AreaHoles(region,area) | Region → 구멍 픽셀 수 |
| ConnectAndHoles(region,numConnected,numHoles) | Region → 연결/구멍 수 |
| EulerNumber(region,value) | Region → Euler 수 |
| DiameterRegion(region,row1,column1,row2,column2,diameter) | Region → 극점 좌표/거리 |
| ComputeFeatures(region,features) | Region → RegionFeatures |
| ComputeMoments(region,moments) | Region → RegionMoments |
| ComputeTopology(region,topology) | Region → RegionTopology |
| ComputeFeaturesAndMoments(region,features,moments) | Region → 두 구조체 |
| ComputeFeaturesMomentsAndTopology(region,features,moments,topology) | Region → 세 구조체 |
| SelectLargestBlob(region) | 최대 면적 8-연결 성분 Region 반환. 동률은 스캔순 |
| ComputeLargestBlobFeatures(data,width,height,stride,features) | 마스크 → 최대 Blob shape, bool 성공 여부 |

Region의 생성/복사/소멸은 라이브러리 내부 구현을 사용한다. 위 출력 인자는 참조이며 정수는 long, 실수는 double이다. 정확한 선언은 [공개 헤더](../include/GlimHalcon.h), 상세 사용 예는 [기존 가이드](05_USER_GUIDE.md)를 참조한다.
