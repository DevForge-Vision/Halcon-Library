# GlimHalcon SDK 툴셋 및 DLL 연동 가이드

문서 기준: 2026-11-06, 공개 이미지 API v1 / SDK 0.0.1 소스 계열.

## 지원 방식

엔진은 C++14이며 v143 전용 코드를 요구하지 않는다. 원하는 툴셋으로 **SDK와 소비자를 함께 빌드**하는 것이 기본 지원 경로다. x86 프로그램에는 x86 DLL/LIB, x64에는 x64 DLL/LIB를 연결한다.

| 툴셋 | 대응 Visual Studio | 선택 방법 |
|---|---|---|
| v140 | 2015 | 설치된 도구를 지정, 실행 검증 대기 |
| v141 | 2017 | 설치된 도구를 지정 |
| v142 | 2019 | 로컬 도구 지정 / Actions 수동 선택 |
| v143 | 2022 | 로컬 도구 지정 / 기본 Release |
| v145 | 2026 | 로컬 도구 지정 |
| default | 선택한 VS의 기본값 | `-T`를 생략 |

선택 가능과 실제 설치·빌드 성공은 다르다. 도구가 없으면 오류를 반환하며 다른 툴셋으로 자동 대체하지 않는다. MFC 데모는 해당 툴셋용 MFC/ATL 구성 요소도 필요하다. 엔진만 사용하면 MFC가 필요하지 않다. VS2015 자체 generator가 필요하면 이를 지원하는 CMake 3.x를 사용한다.

## 빌드 스크립트

저장소 루트에서 PowerShell로 실행한다. CMake가 PATH에 없으면 `-CMake '절대경로/cmake.exe'`를 지정한다.

```powershell
# VS2022에 설치한 VS2019 도구로 DLL 및 테스트 생성
./scripts/Build-Sdk.ps1 -Toolset v142 -Architecture x64

# VS2026 기본 툴셋 + MFC + 설치 EXE/ZIP
./scripts/Build-Sdk.ps1 -Generator 'Visual Studio 18 2026' -MfcDemo -Package

# VS2022에서 v140을 쓰려면 v140 도구/Windows SDK를 먼저 설치
./scripts/Build-Sdk.ps1 -Toolset v140 -Architecture Win32 -ConfigureOnly
```

`ConfigureOnly`는 프로젝트 생성까지만 수행한다. 기본 실행은 Release 빌드와 CTest를 수행하며, 테스트 실패 시 패키징하지 않는다. `Package`는 NSIS가 필요하고 실행파일을 설치하지 않는다. 결과는 `build-sdk-<generator>-<toolset>-<architecture>/dist`에 생긴다. 서로 다른 툴셋은 빌드 폴더를 공유하지 않는다.

직접 CMake를 써도 동일하다:

```powershell
cmake -S . -B build-v142-x64 -G 'Visual Studio 17 2022' -A x64 -T v142
cmake --build build-v142-x64 --config Release
ctest --test-dir build-v142-x64 -C Release --output-on-failure
```

Actions → Windows SDK Release → Run workflow에서 v142/v143 선택 가능. 수동 실행은 아티팩트만 생성한다. MFC 데모는 기본 제외이며 해당 툴셋용 MFC가 runner에 설치된 경우에만 mfc_demo를 켠다. 태그 릴리스는 기존 v143 정책을 유지한다. 이미 배포한 0.0.1 파일을 덮어쓰지 않는다.

## MFC 프로젝트 연결

1. `include`를 추가 포함 디렉터리에 지정하고 `#include "GlimImage.h"`를 사용한다.
2. 해당 아키텍처·툴셋·Release의 `lib/.../Release`를 라이브러리 디렉터리에 지정한다.
3. 추가 종속성에 `GlimHalcon.lib`를 넣는다. 소비자에 `GLIM_HALCON_EXPORTS`를 정의하지 않는다.
4. `bin/.../Release`의 DLL과 동봉 런타임을 EXE 옆에 배치한다. Debug/Release DLL을 혼합하지 않는다.
5. 문자 집합은 멀티바이트. `AnalyzeBmpFile` 경로는 Windows ANSI 코드 페이지이며 UTF-8 경로 API가 아니다.

## 같은 DLL을 다른 툴셋에서 쓸 수 있는가?

현재 공개 경계는 **C++ ABI**이며 C ABI나 모든 컴파일러용 공통 바이너리가 아니다. DLL이 할당한 핸들은 반드시 DLL의 Release 함수로 해제한다. STL/MFC/OpenCV 객체를 공개 함수에 전달하지 않는다.

Microsoft는 VS2015 이후 도구 간 바이너리 호환성을 제공하지만, 링크에 사용하는 도구는 입력 바이너리를 만든 도구 중 가장 최신 버전 이상이어야 한다. `/GL`·`/LTCG`에는 더 엄격한 제한이 있고 런타임도 충분히 최신이어야 한다. 따라서 v143 배포본을 v140 프로젝트에 그냥 연결하는 경로를 지원 보장으로 삼지 않는다. v140 프로젝트에는 v140으로 만든 SDK를 사용하는 방식으로 검증한다.

[Microsoft 공식 바이너리 호환성 문서](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017)

MinGW/GCC와 MSVC 사이의 C++ DLL 교환은 지원 대상이 아니다. Linux 소스 빌드 결과는 Windows DLL 호환성의 증거가 아니다. 새 검증 기록은 [실행 보고서](11_TOOLSET_VERIFICATION.md)를 참조한다.
