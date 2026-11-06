# GitHub Actions 버전 배포

버전 정본은 CMakeLists.txt의 project VERSION이다. 0.0.2부터 v140/v141/v142/v143 × x64/Win32의 8개 조합을 검증한다.

## 검증 및 배포 순서

1. Microsoft Visual Studio Installer로 선택 툴셋과 MFC 구성 요소를 설치한다.
2. VS2022 generator에서 명시한 -T 툴셋, Windows SDK 10.0.19041.0으로 DLL/예제/MFC를 빌드한다.
3. 전체 CTest를 실행한다. pack/header-order 컴파일 검사도 빌드에 포함한다.
4. MFC/CRT를 동봉한 NSIS EXE와 ZIP, 조합별 SHA256 목록을 생성한다.
5. ZIP을 별도 폴더에 풀어 DLL 소비자와 API 계약 예제를 실행한다. MFC가 입력 대기 상태에 도달하는지도 확인한다.
6. 태그 실행에서 모든 조합이 성공하면 자산24개(EXE8+ZIP8+SHA256목록8)를 새 prerelease에 게시한다.

main push와 수동 실행은 Actions 아티팩트만 생성한다. 수동 실행에서는 단일 툴셋 또는 all을 선택할 수 있다. 태그 이름과 CMake 버전 불일치, 누락된 릴리스 노트, 실패한 회귀검증/패키지 검증은 배포를 차단한다. 기존 Release 자산을 덮어쓰지 않는다.

## 새 버전

- CMake 버전과 docs/releases/<version>.md를 변경한다.
- main 검증을 확인하고 해당 소스에 v<version> 태그를 push한다.
- 태그 실행은 같은 검증을 다시 수행하고 성공한 산출물만 게시한다.
- Projects에 빌드/회귀/MFC 시작/실제 이미지/설치 검증을 구분해서 기록한다.

MFC 시작 검사는 UI의 모든 동작을 검증하지 않는다. 개발 도구가 있는 runner에서 ZIP 실행 성공은 깨끗한 PC의 설치/제거 증명이 아니다.

[진행 현황](https://github.com/orgs/DevForge-Vision/projects/1)