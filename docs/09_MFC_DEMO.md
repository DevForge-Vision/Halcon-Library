최신 상태: 2026-11-05 전체 승인 후 로컬 빌드·실행 검증을 진행했습니다. [실행 검증 보고서](10_EXECUTION_VERIFICATION.md) 참조. 아래 내용은 최초 소스 작업 시점 기록입니다.

# MFC 이미지 검사 데모 — SDK 0.0.1

후속 실행 검증: [v0.0.2 MFC 이미지·Blob·그룹화 UI 확인](14_MFC_UI_VERIFICATION.md).

최신 상태(2026-11-06): v0.0.2의 v141/v142/v143 × x86/x64 MFC 6조합이 빌드·ZIP 시작·설치 후 시작 검사를 통과했습니다. v140은 엔진 SDK만 제공합니다. [툴셋 회귀](12_V002_REGRESSION.md), [설치·제거 검증](13_INSTALLER_VERIFICATION.md)을 참조하세요. 아래 내용은 최초 소스 작업 시점의 역사적 기록이며, 미빌드·미배포 표현은 현재 상태가 아닙니다.

2026-11-05: 소스 구현 및 정적 점검. 빌드/configure/실행/Actions/commit/push 하지 않음.

## 사용 흐름
Open BMP → 원본 픽셀 스냅샷(BGR24) → AnalyzeImage → Blob features → GroupBlobDefects → 결과 목록/선택 마스크.
원본 파일을 분석 때 다시 읽지 않아 미리보기와 처리 픽셀을 동일하게 유지한다. 화면 디코더는 Windows ATL CImage이며 SDK의 AnalyzeBmpFile 디코더 검증과는 별개다.

- 원본/선택 마스크를 종횡비 유지 표시한다.
- Contrast, Min area, Gap, Across를 수정한다. LocalMean/Bright, 배경 반경9, 수직 그룹 축90도는 현재 고정이다.
- Blobs / Grouped defects 전환. Index, Parts, Area, X/Y, Width/Height, Mean, StdDev 표시. 나머지 shape/moments/topology도 서비스 결과에 보존한다.
- 선택 마스크는 실제 Blob 또는 그룹 구성 픽셀의 합집합이다. 끊어진 빈 공간을 채우지 않는다.
- 분석은 std::async 작업 스레드. UI 타이머에서 완료 결과를 받고 DLL 전용 Release로 핸들을 해제한다. 처리 중 재실행/파일 변경/닫기를 막는다. 취소 기능은 아직 없다.
- 16,777,216픽셀까지. 픽셀 스냅샷, 작업 입력 복사, 미리보기, 결과/마스크가 메모리를 사용한다. 실시간 처리용 메모리 최적화 전 단계다.

## 구조
- apps/MfcDemo/Demo.cpp: MFC Dialog/View, 입력 확인, GDI 표시, 비동기 작업 조정.
- apps/MfcDemo/AnalysisService.*: MFC와 분리된 SDK 호출 서비스, 결과 소유권.
- apps/MfcDemo/Demo.rc, resource.h: 대화상자 리소스.
- apps/MfcDemo/CMakeLists.txt: GlimMfcDemo 선택 타깃, DLL 링크/실행폴더 복사.

## 개발 환경과 향후 생성 방법
VS2022 Desktop C++ + MFC(v143), Windows SDK, x64 권장. C++14, 멀티바이트, shared MFC/CRT. VS2015 기반 라이브러리 소비 호환은 별도 검증 대상.
다음 명령은 사용자가 빌드를 허용한 이후 사용할 안내이며 이번 작업에서는 실행하지 않았다.

```powershell
cmake -S . -B build-mfc -G "Visual Studio 17 2022" -A x64 -DGLIM_HALCON_BUILD_SHARED=ON -DGLIM_HALCON_BUILD_MFC_DEMO=ON
cmake --build build-mfc --config Release --target GlimMfcDemo
```

예상 출력: build-mfc/apps/MfcDemo/Release/GlimMfcDemo.exe 및 같은 폴더의 GlimHalcon.dll.
MFC 옵션 기본 OFF. 기존 SDK 릴리스 파이프라인/설치기는 GUI 실행파일을 포함하지 않는다. GUI 배포는 MFC 런타임 포함 정책과 설치 검증 후 추가한다.

## 다음 검증
1. 허가 후 DLL/API consumer 및 MFC 컴파일·링크.
2. 실제 원본 BMP로 입력/미리보기 일치, Blob/그룹 마스크·feature 확인.
3. 반복 열기/분석, 오류 BMP, 0개/4096개 Blob, 처리 중 종료, x86/x64 및 깨끗한 PC 런타임 확인.
4. 결과 CSV, 상세 feature, ROI/극성/그룹 방향 UI는 후속 범위.

현재는 실행 가능한 EXE가 생성되거나 이미지 정확도가 확인된 상태가 아니다.
