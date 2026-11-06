# v0.0.2 설치·제거 검증

검증일: 2026-11-06 (America/Phoenix).

[Actions 실행 결과](https://github.com/DevForge-Vision/Halcon-Library/actions/runs/42271592615)
에서 v140/v141/v142/v143 × x64/x86 총 8개 작업이 모두 성공했다.
검증 스크립트 커밋: `1865bfd`. 기존 v0.0.2 Release 자산을 다운로드하여 검사했으며
DLL을 다시 빌드하거나 공개 자산을 교체하지 않았다.

## 확인한 항목

- EXE와 ZIP의 SHA256을 게시된 manifest와 대조.
- GUID별 임시 경로 아래 `SDK With Spaces` 폴더에 무인 설치.
- 프로그램 추가/제거 등록과 UninstallString 경로 확인.
- ZIP의 모든 파일이 설치 위치에 존재하고 SHA256이 동일한지 확인.
- v141~v143의 MFC 데모 6개가 설치 위치에서 WaitForInputIdle에 도달.
- v140 설치본 2개에는 MFC 데모가 없음을 확인.
- 제거 프로그램 종료코드 0, 설치 폴더와 프로그램 추가/제거 등록 제거 확인.
- 각 조합의 `installer-result.json`을 Actions artifact에 저장.

## 자동화

`scripts/Test-Installer.ps1`은 일회용 GitHub Actions Windows runner 전용이다.
기존 설치 등록이 있으면 중단하며, 관리자 권한을 요청하거나 개발 PC에서 설치하지 않는다.
설치 및 제거 프로세스에 120초 제한을 두고 실행한 프로세스만 관리한다.

`Published Installer Verification` workflow는 기존 릴리스 태그로 재실행할 수 있다.
소스 빌드가 없다. `Windows SDK Release`에도 같은 검사를 패키지 게시 전 단계로 연결했다.
추가한 릴리스 단계 자체는 다음 릴리스에서 실행되며, 이번 증거는 위의 별도 workflow 결과다.

무인 설치 `/S /D=...`와 제거 `_?=...`는
[NSIS 공식 명령행 규약](https://nsis.sourceforge.io/Docs/Chapter3.html)을 따른다.
제거 프로그램을 설치 폴더 밖에 복사하고 실행하여 실제 제거가 끝날 때까지 기다린다.

## 남은 범위

이 환경은 개발 도구와 런타임이 미리 설치된 Windows Server 2022 runner다.
**Visual Studio/VC 런타임이 없는 깨끗한 PC의 의존성 검증으로 간주하지 않는다.**
설치 마법사 화면, 기본 Program Files 경로, 사용자 변경 파일 보존, 다른 버전과 공존,
업그레이드, 표준 사용자/UAC, Windows 10/11 실물 환경은 별도 검증해야 한다.

MFC는 시작 확인이며 실제 BMP 열기·선택 마스크·재분석 UI 전체 검증이 아니다.
HALCON 실물 대조, 원본 BMP/정답 기반 검출 정확도, 장시간 P99 성능 역시 미완료다.
로컬 VS 추가 MFC 설치는 이전 관리자 실행 차단 상태를 유지하며 재시도하지 않았다.
