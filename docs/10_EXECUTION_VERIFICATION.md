# 2026-11-05 가속 개발 실행 검증

사용자의 전체 승인 및 즉시 실행 요청에 따라 기존 빌드 보류를 해제하고 수행했다.

## 확인된 결과
- Windows 11, Visual Studio 2026 Community, MSVC 19.51.36260 / v145, SDK 10.0.26100.
- x64 Release DLL/예제/MFC 데모 빌드 성공. x64 CTest 9/9, x86 CTest 10/10 통과.
- 각 아키텍처 6개 packing/header-order 컴파일 검사 통과. x86 /Gd,/Gz,/Gr 및 x64 /Gd,/Gv 실제 DLL 링크/호출 통과.
- 8스레드 × 100회 공유 ImageResult의 그룹화·feature·마스크 조회 통과. 스레드 sanitizer 증명은 아님.
- 첫 MFC 빌드에서 afxdlgs.h 누락과 불필요한 AfxEnableControlContainer 호출 오류를 수정했다.
- C4121(의도된 packing probe)만 국소 억제. DefectTest fill의 int→byte 경고는 명시 변환으로 수정.
- 제공 Line JPG 10장을 로컬 BMP로 변환하여 AnalyzeBmpFile 실행 성공(Blob 12~21개). 원본 파일은 변경하지 않았다. 원본 BMP 실측이 아니다.
- MFC 실제 화면: 001213 이미지, Contrast10/MinArea12/Gap12/Across4에서 14 Blob, 12 그룹. 그룹4는 조각3개, 면적136px, bbox(58,47,13,35), 평균126.081, 표준편차13.951. 선택 마스크 표시 확인.
- x64 v145 NSIS EXE/ZIP 생성 성공. MFC EXE와 MFC/CRT app-local runtime 포함.
- ZIP 추출 폴더의 DLL로 GlimConsumer_Gd 실행 exit0. 추출본 MFC 실행 확인.
- Debug 구성 패키징이 의도대로 exit1로 거부됨.

## 증거 위치(로컬)
- build-verify-x64/Testing/Temporary/LastTest.log
- build-verify-x86/Testing/Temporary/LastTest.log
- build-verify-x64/build.log, build-verify-x86/build.log
- build-verify-x64/samples/*.bmp.csv
- build-verify-x64/dist/GlimHalconSDK-0.0.1-x64-v145.exe 및 .zip
- ../halcon-image-study/mfc-runtime-group.png

## 남은 검증
- VS2022/v143 Actions 빌드 및 배포 결과는 별도 확인. 로컬 v145 결과로 대체하지 않는다.
- 원본 BMP, 정상 이미지, 정답 마스크/그룹 라벨, 현장 합격 기준이 없어 검출 정확도·오병합률은 미판정.
- HALCON 실물 수치 대조, VS2015, 깨끗한 PC 설치/제거, 장시간/대형 이미지 P99 성능 미검증.
- 현 세션은 관리자 권한이 없어 관리자 설치기의 실제 설치/제거는 실행하지 않음. ZIP 로딩 검증과 구분한다.
- 자동 릴리스는 0.0.1 prerelease이며 생산 승인/1.0 완료가 아니다.

NSIS3.11 도구는 공식 SourceForge 게시 SHA256 c7d27f780ddb6cffb4730138cd1591e841f4b7edb155856901cdf5f214394fa1과 다운로드 ZIP을 대조한 후 사용했다.