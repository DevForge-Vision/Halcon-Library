# 2026-11-06 툴셋 선택 / API 문서 검증

## 변경

- Build-Sdk.ps1에서 generator/toolset/architecture를 선택한다. 기본 툴셋은 -T를 생략하며 실패 시 자동 대체하지 않는다.
- Actions 수동 실행에서 v142/v143 선택, MFC 포함 여부 선택. 태그의 v143+MFC 동작은 유지한다.
- API_REFERENCE.md에 이미지/그룹 14개 함수의 인자·출력·오류·수명, 구조체와 설정 범위, 저수준 Region 함수 목록을 정리했다.
- 07_api_contract.cpp를 CTest에 등록: 2 Blob→1그룹, 면적4, 중심(3.5,3), 평균200, 빈 간격 유지, source 해제 후 독립 결과 확인.

## 실제 검증

호스트는 VS2026이며 설치된 이전 툴셋을 -T로 선택했다. VS2017/2019 IDE 자체에서의 검증은 아니다.

| 조합 | 컴파일러 | 엔진 DLL/소비자/예제 | CTest |
|---|---|---|---|
| v141 / Win32 | MSVC 19.16.27054 | 성공 | 11/11 |
| v142 / x64 | MSVC 19.29.30159 | 성공 | 10/10 |

두 조합의 pack1/4/8 및 헤더 순서 컴파일 검사도 포함한다. 새 예제 검증으로 기존 결과보다 CTest가 1개 증가했다. 서로 다른 툴셋의 DLL과 소비자를 혼합한 검증은 아니다.

로그: build-v141-engine.log, build-v142-engine.log 및 각 build-sdk 폴더의 Testing/Temporary/LastTest.log (로컬, Git 제외).

v142/x64 엔진 전용 NSIS EXE와 ZIP 생성 성공. ZIP 내부에 API_REFERENCE.md, TOOLSET_GUIDE.md, 공개 헤더, 07_api_contract.cpp, v142 DLL이 포함됨을 확인했다. 로컬 개발 검증용 패키지이며 GitHub에 배포된 v0.0.1 자산과 동일한 소스라는 뜻은 아니다. 실제 설치/제거는 실행하지 않았다. 패키징 로그는 build-v142-package.log.

MFC 포함 빌드는 시도했으나 v141은 afxwin.h 부재(C1083), v142는 MFC 구성 요소 부재(MSB8041)로 실패했다. 엔진은 MFC 없이 빌드하여 위 테스트를 통과했다. 이는 엔진 DLL을 MFC 호출부에서 쓸 수 없다는 뜻이 아니라 해당 툴셋용 MFC 설치가 없다는 뜻이다. v143/v145 MFC의 이전 검증은 [별도 보고서](10_EXECUTION_VERIFICATION.md)에 있다.

## 미검증

- v140: 로컬 도구 부재. 선택 인자는 제공하되 VS2015 지원 검증 완료로 표시하지 않는다.
- 변경한 Actions 수동 v142 패키징은 아직 실행하지 않았다.
- 이번 변경을 기존 v0.0.1 Release 자산에 덮어쓰지 않았다. 문서와 예제는 이후 생성 패키지에 포함된다.
- 깨끗한 PC 설치, HALCON 실물 수치 비교, 현장 정확도는 별도 과제다.
