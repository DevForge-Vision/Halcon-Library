# GitHub Actions 버전 배포
현재 버전의 정본은 CMakeLists.txt의 project VERSION(0.0.1)입니다.

## 흐름
소스 커밋 → v0.0.1 태그 push → windows-2022 / v143 x64·Win32 빌드 → CTest → CPack NSIS/ZIP → SHA256 → GitHub Pre-release.
수동 workflow_dispatch는 선택 ref를 빌드하여 Actions artifact만 보관하며 Release는 게시하지 않습니다.
태그 버전과 CMake 버전이 다르거나 릴리스 노트가 없으면 실패합니다.
두 아키텍처가 모두 성공한 경우에만 Release를 생성합니다. 같은 버전 자산을 덮어쓰지 않습니다.
실패 시 원인을 수정하고 새로운 버전으로 배포하는 것을 원칙으로 합니다.

## 다음 배포
1. CMakeLists.txt 버전과 README 예시 경로를 변경합니다.
2. docs/releases/<버전>.md에 변경점/설치법/알려진 문제를 기록합니다.
3. 필요한 소스 전부를 커밋합니다. 현재 신규 DLL/이미지/그룹화 파일들은 미커밋이므로 누락하지 않아야 합니다.
4. 빌드 실행이 허용되었을 때 소유자가 해당 커밋에 v<버전> 태그를 만들어 push합니다.
5. Actions 성공 후 저장소 Releases에서 설치 EXE 또는 ZIP을 다운로드합니다.

이번 변경에서는 빌드/워크플로 실행/태그/커밋/push/Release 게시를 하지 않았습니다.
NSIS는 runner에서 탐색하고 없으면 Chocolatey의 nsis 3.10을 설치합니다. 라이브러리 실행에 NSIS는 필요하지 않습니다.
패키지는 v143 전용이며 v140 호환성을 주장하지 않습니다. 설치/제거 실행 검증은 별도입니다.
CI 기존 main push 빌드도 유지되어 있으므로 소스 push 자체가 빌드를 시작할 수 있습니다.

## 진행 기록 규칙
작업 시작/완료/일정 변경을 https://github.com/orgs/DevForge-Vision/projects/1 에 반영합니다.
소스 준비, CI 빌드/테스트 성공, 설치 검증, 실제 영상 검증을 구분합니다.
현재: 0.0.1 자동 배포 소스 준비, 실행 및 설치파일 생성 대기.

참고: https://cli.github.com/manual/gh_release_create
