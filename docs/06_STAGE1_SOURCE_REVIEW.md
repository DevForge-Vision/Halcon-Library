## 수정 후 상태 — 2026-11-05
R1/R2 소스 수정 및 정적 재점검 완료. 빌드/실행 검증 대기이므로 Projects는 In Progress 유지.
- R1: GlimHalcon.h에서 pack(push,8)/pop으로 타입 정의 자체를 보호. 호출자의 packing을 복원한다. 기존 비표준 packing으로 컴파일한 호출부는 새 헤더로 재컴파일해야 한다.
- R2: maxSteps/잘못된 방향/고립점이 아닌 탐색 중단에서 전체 윤곽을 비우고 false 반환. FeatureContext도 실패 시 비운다. ComputeChecked의 빈 윤곽 검사로 Image API는 ProcessingFailed와 null 결과를 반환하는 경로로 연결된다.
- test/AbiPackingTest.cpp: MSVC pack 1/4/8 × 헤더 순서 2가지의 sizeof/offsetof 및 호출자 packing 복원 compile-only 검사. CMake에 6개 object target 등록. x86/x64는 Release workflow matrix에서 각각 확인하도록 구성.
- test/ContourFailureTest.cpp: 고립 성분 뒤 사각형 성분의 추적을 내부 stepLimit으로 중단하여 이전 성분과 부분 윤곽이 모두 제거되는지, 다음 정상 호출이 복구되는지 검사. 내부 전용 budget 기본값 0은 기존 자동 한계를 사용하며 공개 DLL API는 변경하지 않음.
- git diff --check 통과. 수정 C++ 파일 UTF-8 BOM/CRLF 확인. 테스트 소스만 추가했으며 CMake configure/빌드/테스트/Actions 실행은 하지 않았다.
- 커밋/push/Release 게시 없음. 실제 DLL ABI·수치·설치 검증 완료를 의미하지 않음.
# 1단계 소스 점검 — 2026-11-05
대상: main / cf163674cae94793e7814945849a020315a8fad0 + 현재 미커밋 작업트리.
판정: 1차 정적 점검 완료, 수정 필요 2건으로 단계는 In Progress 유지.
빌드, CMake configure, C++ 테스트, 이미지 실행, HALCON 대조, 설치 실행은 수행하지 않았다. 소스 수정/커밋/푸시도 하지 않았다.

## 확인 범위
- CMakeLists.txt, cmake/Packaging.cmake 및 PackageConfig.cmake: DLL 기본 SHARED, export 매크로 PRIVATE, 소스/테스트/공개 헤더 등록. 실제 DLL export/link 검증은 미실시.
- include/GlimImage.h, GlimHalcon.h: POD 출력/opaque handle, 크기·버전 확인, DLL 내부 생성/해제. C++ ABI이며 C ABI가 아니다. 같은 아키텍처/검증된 툴셋 조합이 필요하다.
- ImageAnalysis.cpp, BmpReader.cpp, Segmentation.cpp: BMP 8-bit palette/24/32-bit 비압축, Gray8/BGR/BGRA 버퍼, stride/크기/ROI 검사, 8연결 Blob, 면적 필터 후 모든 선택 Blob 반환.
- Operators.cpp, FeatureContext.cpp, Region/Contour/ConvexHull/Moments/Holes 코어: 기존 feature 계산 공유 경로, 원본 grayscale의 모집단 평균/편차, 전체 영상 좌표와 ROI 경계.
- DefectGrouping.cpp: 고정 축 투영 구간 거리 + 횡방향 무게중심 범위 + 선택적 길이 제한. 결정적 탐욕 병합. 픽셀 합집합으로 형상/모멘트 재계산, 면적 가중 평균 및 조각 간 차이를 포함한 분산 결합. 빈 간격은 채우지 않음.
- 그룹 결과는 런/feature/멤버 인덱스를 복사하여 원본 ImageResult 해제 후에도 독립적으로 보관. 동시 읽기는 가능하도록 구성되며 읽기 중 해제/잘못된 핸들은 지원하지 않음.
- ImageTest.cpp/DefectTest.cpp는 사례를 정의한 소스만 확인. 실행 성공 증거가 아님.

## 수정 필요
### R1 / 높음: 중첩 공개 구조체 packing이 include 순서에 의존
근거: include/GlimImage.h:4-6,61-63,104-106; include/GlimHalcon.h:172,204,305.
GlimImage.h가 GlimHalcon.h를 pack(8) 안에서 include하지만, 호출자가 이미 pack(1) 또는 pack(4) 아래 GlimHalcon.h를 포함했다면 include guard 때문에 재정의되지 않는다.
따라서 RegionFeatures/RegionTopology의 레이아웃은 이미 다른 packing으로 확정되어 BlobFeatures/DefectFeatures의 중첩 멤버에도 전달된다.
크기 검사는 일부 조합을 VersionMismatch로 막지만 ABI v1 고정 보장은 성립하지 않는다. 기존 Region API에는 크기 검사도 없다.
후속: 타입을 정의하는 GlimHalcon.h 자체에 packing 경계를 두거나 고정 wire 타입을 별도로 정의. include 순서 × pack(1/4/8) × x86/x64의 sizeof/offsetof 검증 소스를 준비하고 소유자 빌드 단계에서 확인.

### R2 / 중간: 윤곽 추적 방어 종료가 부분 윤곽 성공으로 처리될 수 있음
근거: src/Core/Contour.cpp:337-342,392-396; src/Feature/FeatureContext.cpp:82; src/Facade/Operators.cpp:1050.
maxSteps 초과 또는 잘못된 startDir에서 break만 수행하며, 이미 누적한 contour를 보관하고 nonempty이면 true를 반환한다.
ComputeChecked는 nonempty contour/hull/면적/성분 수를 확인하므로 이런 부분 윤곽을 실패로 식별하지 못한다. 그 경로에서는 contLength/convexity/방향 등이 잘못되어도 Ok가 가능하다.
정상 영상에서 방어 분기에 도달하는 구체 사례는 실행 검증하지 않았다. 결함은 방어 분기 발생 시 실패 전파가 없는 것이다.
후속: 정상 닫힘/고립점과 비정상 종료를 구분하여 비정상 시 전체 contour를 비우고 false 반환. 경계 전파와 회귀 케이스 확인.

## 동작 한계 및 검증 공백
- 분석 기본 minArea=12이므로 12픽셀 미만 조각은 그룹화 전에 제거된다. 그룹화가 제거된 조각을 복원하지 못한다. 흠집 recipe에서는 낮은 후보 minArea와 그룹 후 면적 필터를 분리해야 한다.
- maxAlongGap은 빈 픽셀 개수가 아니라 픽셀 중심 투영 구간 거리. 끝 좌표 2와 시작 좌표 5의 gap은 3이다.
- 길이 제한은 병합 제약이며 이미 긴 단일 Blob을 제거/분할하지 않는다.
- 그룹화는 개별 방향 일치/폭/밝기/흠집 여부를 검사하지 않는다. 가까운 별개 결함 오병합은 실제 정상·불량 데이터로 확인해야 한다.
- 고정 축/threshold 기본값은 현장 검증값이 아니다. 16-bit, JPG 파일은 현재 API 지원 밖이다.
- 픽셀 16,777,216, 런 1,000,000, 그룹 입력 4096, 후보 간선 1,000,000 한계. 그룹 후보 검사는 O(N²). 실시간 성능/최대 메모리/P99는 미확인.
- 테스트 보완: packing/include 순서, 비정상 윤곽 실패 전파, 비직교 축/동률, 다중 조각 크기가 다른 밝기 분산, 컬러/BMP 오류 조합, 원본 실제 데이터.
- 기존 HALCON 미결 규약(R-1, MI-1, EA-2, EC-1, OR-2/3, HC-1/3, DR-2)은 이번 점검으로 해결되지 않았다.

## 정적 검사와 다음 상태
git diff --check 통과. 점검 대상 신규 이미지/그룹화 헤더·소스 및 Operators/Contour의 UTF-8 BOM 확인.
기존 작업트리 변경은 보존. 빌드/런타임 검증 통과로 간주하지 않는다.
1단계: 점검 결과 기록 완료, R1/R2 수정 및 재점검 대기 (In Progress).
2단계: API/ABI·예외/입력 경계 상세 점검에서 R1/R2를 우선 처리.
Projects: https://github.com/orgs/DevForge-Vision/projects/1
Issue: https://github.com/DevForge-Vision/Halcon-Library/issues/2

