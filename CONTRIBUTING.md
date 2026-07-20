# 기여 안내

> 이 문서는 **입구**입니다. 상세 규칙은 [`CLAUDE.md`](CLAUDE.md) 에 있고, 여기서 다시 쓰지 않습니다.
> 작업을 시작하기 전에 [`CLAUDE.md`](CLAUDE.md) 를 끝까지 읽어 주세요.

---

## 1. 단 하나의 원칙

> ### 정확도 > 개수
> **값이 틀린 40종보다, 값이 맞는 5종이 낫습니다.**

구현한 오퍼레이터 개수는 성과가 아닙니다. HALCON 실측값과 일치하는 것만 성과입니다.
"일단 다 만들고 나중에 맞추자" 는 이 저장소에서 금지입니다.

---

## 2. 순서 — 정의 → 검증 → 구현

```
docs/02_DEFINITIONS.md 에 정의를 쓴다        (원문 인용 · 수식 · 예외 · 미결 쟁점)
        ↓
docs/03_VERIFICATION.md 에 기댓값을 손계산한다  (기준 도형 S1~S7, 계산 과정 포함)
        ↓
(가능하면) HALCON 실측과 대조한다
        ↓
그 다음에야 코드를 짠다
        ↓
단위테스트가 기댓값과 일치하는지 확인한다
        ↓
docs/00_OVERVIEW.md §6 현황표를 갱신한다
```

**정의서에 없는 오퍼레이터를 먼저 구현하지 마세요.** 정의가 틀리면 코드는 전부 폐기입니다.

작업 중 자주 밟는 지뢰 세 가지만 미리 알려 둡니다. 나머지는 [`CLAUDE.md`](CLAUDE.md) §3 에 있습니다.

| 지뢰 | 요점 |
|---|---|
| 곡선 도형 기댓값 | 디지털 원의 체인 길이는 `2πR` 보다 **+5.5%** 큽니다. 연속 기하 공식으로 기댓값을 잡지 마세요 ([`docs/04_IMPLEMENTATION_NOTES.md`](docs/04_IMPLEMENTATION_NOTES.md) §3) |
| 소스 인코딩 | **UTF-8 BOM 필수.** BOM 없는 한글 주석은 VS 가 CP949 로 오독해 다음 줄을 삼킵니다 |
| 공개 헤더 | 외부 타입(`cv::Mat` 등)을 노출하지 마세요. PIMPL 로 숨깁니다 |

---

## 3. 빌드와 테스트

CMake 로 `GlimHalcon`(static lib) 과 `GlimHalconTest`(테스트 실행파일) 을 만듭니다.

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

테스트가 전부 통과하면 `GlimHalconTest` 의 **exit code 가 0** 입니다. 이것이 통과 기준입니다.

### 의존성이 0 이므로 컴파일러 단독 빌드도 됩니다

Phase 1 은 OpenCV 를 포함해 **어떤 외부 라이브러리도 쓰지 않습니다.**
CMake 없이 `cl.exe` 만으로도 그대로 빌드됩니다.

```bat
cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature /I test ^
   src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp test\TestMain.cpp ^
   /Fe:glimtest.exe
```

CI([`.github/workflows/ci.yml`](.github/workflows/ci.yml)) 는 이 사실을 Windows(MSVC) 와 Linux(g++)
양쪽에서 **패키지 설치 단계 없이** 빌드·테스트해 증명합니다.
CI 에 설치 단계를 추가해야 한다면, 그것은 의존성이 생겼다는 뜻이므로 먼저 이슈로 논의해 주세요.

> 참고: 저장소 소유자는 Visual Studio 에서 직접 빌드합니다.
> AI 에이전트로 작업하는 경우 **빌드 명령을 실행하지 마세요** ([`CLAUDE.md`](CLAUDE.md) §4.2).

---

## 4. 이슈와 PR

| 상황 | 사용할 것 |
|---|---|
| 값이 HALCON 과 다르다 | 이슈 → **값 불일치 리포트** (가장 중요) |
| 오퍼레이터를 추가하고 싶다 | 이슈 → **오퍼레이터 추가 요청** |
| 그 밖 (크래시 · 빌드 · 문서) | blank issue |

PR 은 [`.github/pull_request_template.md`](.github/pull_request_template.md) 의 **정확도 게이트 체크리스트**를
전부 만족해야 병합됩니다. 해당 없는 항목은 `N/A` 와 사유를 적어 주세요.

커밋은 **1기능 1커밋**, 컨벤셔널 커밋(`feat:` `fix:` `docs:` `test:` `refactor:` `chore:`) 을 씁니다.
메시지는 한국어로 써도 됩니다. 예: `feat: circularity 구현 (S1~S7 기댓값 검증 통과)`

---

전체 지침은 [`CLAUDE.md`](CLAUDE.md), 문서 지도는 [`README.md`](README.md) 를 참고하세요.
