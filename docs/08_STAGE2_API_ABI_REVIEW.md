최신 상태: 2026-11-05 전체 승인 후 로컬 빌드·실행 검증을 진행했습니다. [실행 검증 보고서](10_EXECUTION_VERIFICATION.md) 참조. 아래 내용은 최초 소스 작업 시점 기록입니다.

# 2단계 API/ABI 정적 점검 — 2026-11-05
상태: 정적 점검 및 A2-01/A2-02 소스 보완 완료, 컴파일/링크/실행 검증 대기 (In Progress).
대상: main / cf163674cae94793e7814945849a020315a8fad0 + 미커밋 작업트리, SDK 0.0.1.
최초 점검은 소스 읽기와 문서 작성만 수행. 후속 소스 수정 내역은 문서 하단 참조. 빌드, configure, C++ 테스트, Actions, commit/push 없음.

## 점검 결과
| 영역 | 소스에서 확인한 동작 | 남은 확인 |
|---|---|---|
| DLL export | GLIM_HALCON_EXPORTS를 SHARED target에만 PRIVATE 지정; 공개 함수/Region 클래스 dllexport | 실제 export/import library 링크 미검증 |
| 공개 타입 | STL/OpenCV/MFC 타입이 이미지 API 경계를 통과하지 않음; enum underlying int; POD + opaque handle | C++ mangled API이며 C ABI/다른 컴파일러 호환 보장 아님 |
| packing | GlimHalcon.h 자체 pack(8), GlimImage.h 각 구조체 pack(8), 호출자 pack 복원 | 이전 잘못된 packing 사용처 재컴파일 필요 |
| 버전 | ImageSettings/GroupingSettings structSize 및 apiVersion=1; ImageView structSize 확인 | SDK 0.0.1과 ABI 1은 다른 번호. 런타임 SDK 버전 조회 API는 없음 |
| 입력 | null, 양수 치수/stride, uint64 버퍼 길이, size_t 한계, ROI 범위, 유한 threshold 검사 | caller가 선언한 길이의 실제 메모리 유효성까지 확인하지 못함 |
| 생성 | AnalyzeImage/AnalyzeBmpFile/GroupBlobDefects는 시작 시 output=null; unique_ptr로 실패 정리 | 이미 보유한 핸들 변수를 그대로 넘기면 이전 소유권을 잃으므로 먼저 해제/별도 변수 사용 |
| 조회 | Count 실패 시 0, member 실패 시 -1; feature는 크기가 맞으면 초기화 후 인덱스 확인 | 크기 불일치 때 feature 버퍼는 수정 안 함; 반환 status 먼저 확인 |
| 마스크 | 모든 인자 검사 후 행 유효 픽셀만 0/255 기록; padding 유지 | 실패 시 출력 버퍼 보존; caller가 공유 출력 버퍼 동기화 |
| 해제 | 생성 DLL의 Release 함수로 delete; nullptr 해제 허용 | 이중 해제, 해제 중 조회, 다른 DLL 인스턴스 핸들, 임의 포인터 금지 |
| 동시성 | 호출마다 독립 결과; 조회 시 내부 쓰기 없음 | 입력/설정은 호출 동안 변경 금지; 동일 결과의 release와 read 동시 사용 금지 |
| 예외 | 생성 함수 bad_alloc/기타 catch; legacy core 실패는 일부 false로 축약 | OutOfMemory가 모든 내부 메모리 부족을 구분하지 않음(ProcessingFailed 가능). try/catch는 무효 포인터/SEH 안전 보장 아님 |
| 배포 | x86/x64/v143/Release 분리, Release CRT 배치 설정 | v140 호환, 깨끗한 PC DLL 로딩/설치 실행 미검증 |

## A2-01 / 높음 — 공개 함수 호출 규약 미고정
근거: include/GlimHalcon.h:33-37 export 매크로, include/GlimImage.h:71-117 함수 선언. 소스/CMake에 __cdecl 또는 대응 계약이 없음.
현재는 컴파일러 기본 호출 규약에 의존한다. 예를 들어 x86 DLL은 기본 /Gd, 소비자는 /Gz이면 동일 헤더에서도 함수 선언과 decorated symbol이 달라져 정상 import-library 링크가 실패할 수 있다. 함수 포인터를 강제 변환하면 호출/스택 계약 문제로 확대될 수 있다.
제안: export 매크로(클래스에도 사용)와 별도의 GLIM_HALCON_CALL을 정의하여 공개 자유 함수 선언/정의에 __cdecl을 명시. 기존 Region 멤버 호출 규약은 별도로 다룬다. C ABI로의 변경은 이번 수정에 섞지 않는다.
후속 검증: x86 소비자를 /Gd, /Gz, /Gr로 빌드하여 실제 DLL에 링크·호출하는 검증. x64도 기본 설정과 /Gv 조합을 검토.
근거 문서: [Microsoft 호출 규약](https://learn.microsoft.com/en-us/cpp/build/reference/gd-gr-gv-gz-calling-convention?view=msvc-170).
최초 점검에서 발견했으며 후속 소스 보완 내역은 하단 참조.

## A2-02 / 검증 공백 — 입력 구조체와 DLL 소비자 검사가 부족
현재 AbiPackingTest.cpp는 RegionFeatures/Moments/Topology, BlobFeatures/DefectFeatures 일부 offset/size를 검사한다.
ImageView, ImageSettings, DefectGroupingSettings의 크기/offset/enum 값 및 함수 포인터 호출 규약은 검사하지 않는다.
6개 object target의 compile-only 검사는 DLL export/link/run 검증을 대체하지 않는다.
Windows MSVC pack(8) 기준 수작업 산출값(컴파일 실측 아님):
| 구조체 | x86 크기 | x64 크기 | 주요 offset |
|---|---:|---:|---|
| ImageView | 32 | 40 | data=4/8, dataBytes=8/16, width=16/24 |
| ImageSettings | 56 | 56 | threshold=16, roiHeight=52 |
| DefectGroupingSettings | 48 | 48 | axisAngleDegrees=8, maxInputBlobs=40 |
| BlobFeatures | 264 | 264 | shape=48, moments=104, topology=224 |
| DefectFeatures | 272 | 272 | shape=56, moments=112, topology=232 |
후속: 위 값과 standard-layout/trivially-copyable 여부를 컴파일 검사에 추가. enum 숫자를 ABI 계약으로 고정하고 변경을 검사. DLL consumer executable을 별도 target으로 준비.
실패 시 출력 유지/초기화 계약, 잘못된 structSize/enum/stride/NaN, null 및 경계 인덱스도 회귀 사례를 보강한다.

## 승인 범위와 다음 단계
- 1단계 R1/R2는 소스 수정 상태로 유지. 이번 점검으로 실행 검증 통과로 승격하지 않음.
- 2단계는 A2-01/A2-02 소스 보완 후 실제 컴파일/링크/실행 검증이 남아 In Progress.
- 실제 빌드/링크/실행은 소유자 실행 또는 별도 승인 이후 단계에서 수행.
- 3단계 실제 BMP/정상·불량 데이터 및 합격 기준 준비는 독립적으로 진행 가능.
- git diff --check 통과(README의 줄바꿈 경고 존재). 최초 점검 시점에는 코드를 변경하지 않았다.


## 후속 소스 보완 — 2026-11-05
- A2-01: export와 별도 GLIM_HALCON_CALL(__cdecl)을 공개 free function 선언/정의에 적용. ImageStatus 번호0..7 명시. Region 클래스 멤버 ABI 및 다른 컴파일러 간 호환 보장은 별개다.
- A2-02: ImageView/설정/그룹 설정의 size·offset, enum size, 반환 POD traits 검사를 추가. x86 /Gd,/Gz,/Gr 및 x64 /Gd,/Gv 소비자에서 이미지 분석→그룹화→원본 해제 후 그룹 조회→해제를 검사하는 실행 타깃을 등록했다.
- 위 타깃은 작성만 했으며 실행하지 않았다. 호출 규약/packing/배포 성공으로 판정하지 않는다.
- apps/MfcDemo에 선택형 DLL 소비 프로그램 추가. 상세 범위는 09_MFC_DEMO.md.