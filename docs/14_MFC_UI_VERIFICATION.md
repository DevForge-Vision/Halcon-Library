# v0.0.2 MFC 실제 이미지·Blob·그룹화 확인

검증일: 2026-11-06 (America/Phoenix). 공개 v0.0.2 x64/v142 ZIP의
`GlimMfcDemo.exe`를 Windows에서 직접 실행하고 파일 대화상자, Analyze,
Blobs/Grouped defects 전환 및 결과 행 선택을 조작했다. 새 빌드는 수행하지 않았다.
프로세스의 GlimHalcon.dll 로드 경로가 동일 패키지 폴더임을 확인했다.

## 실제 흠집 샘플

입력: `Defect_MAFXM121C1_001213_Cam1_lane9_02_103_Line_NG.bmp`.
이는 사용자가 제공한 JPG에서 이전에 변환한 128×128 BMP이며 카메라 원본 BMP는 아니다.
Contrast=10, Min area=12, Gap=12, Across=4. Bright/LocalMean, 수직 그룹 축.

| 확인 항목 | 실제 결과 |
|---|---|
| Blob 목록 | 14개, 면적·bbox·밝기 평균/표준편차 표시 |
| 그룹 목록 | 12개 |
| 중앙 그룹 4 | Parts=3, Area=136, X=58, Y=47, Width=13, Height=35 |
| 밝기 통계 | Mean=126.081, StdDev=13.951 |
| 선택 마스크 | 중앙 3조각 표시, 틈은 검정색으로 유지 |
| 오류 후 재열기·재분석 | 같은 14 Blob/12그룹 결과로 복구 |

같은 DLL 옆에 기존 `06_image_blobs.exe` 소비자를 복사해 실행했다(exit 0).
Blob 4/5/6의 면적은 23/36/77이며 합은 136이다.
각 Blob의 평균과 모분산으로 면적 가중 통계를 독립적으로 다시 계산하면
평균 126.08088235294117, 표준편차 13.95109902220372로 화면의 세 자리 표시와 일치한다.
소비자는 기존 로컬 v145 빌드 바이너리이고 DLL은 공개 v142 패키지다.
기존 `07_api_contract.exe`도 같은 DLL에서 exit 0:
`Blobs=2 Defects=1 Members=2 Area=4 Row=3.5 Column=3.0 Mean=200.0`.

## 합성 정답 및 오류 시나리오

64×64, 배경 밝기 20. 밝기 200인 3×10 사각형을
(30,10), (30,25), (30,40), (50,10)에 둔다. 좌표는 좌상단 (x,y).
앞의 세 조각이 한 흠집이고 오른쪽 조각은 독립 결함이다.

| 시나리오 | 화면에서 확인한 결과 |
|---|---|
| Gap=12, Across=4 | 4 Blob → 2그룹 |
| 합쳐진 그룹 | Parts=3, Area=90, bbox=(30,10,3,40), Mean=200, StdDev=0 |
| 독립 조각 | Parts=1, Area=30, bbox=(50,10,3,10) |
| 그룹 마스크 | 3조각만 흰색, 두 틈과 옆 조각은 검정색 |
| Gap=0 후 재분석 | 4 Blob → 4그룹, 각 면적 30, 기존 선택 마스크 초기화 |
| 균일 배경 BMP | 0 Blob/0그룹, 목록과 마스크 비어 있음 |
| Min area=1.5 | Invalid parameters 메시지, 분석 거부 |
| 손상 BMP | Cannot load BMP preview 메시지, 기존 이미지/목록/마스크 제거 |
| 이후 정상 BMP 열기 | 분석 및 그룹 목록 정상 복구 |

시험 BMP는 `python scripts/Create-MfcFixtures.py build-mfc-fixtures`로 재생성한다.
생성기는 기존 파일 덮어쓰기를 거부한다. 초기 UI 시험은 동일 픽셀의 Pillow 생성 BMP로
수행했고, 저장한 생성기의 출력은 디코딩한 RGB 픽셀이 동일함을 별도로 확인했다.

## 판정과 한계

이번 시나리오에서 기능 수정이 필요한 결함은 발견하지 못했다. 실행 코드 변경 및
재배포는 하지 않았으며, 결과 문서와 재현용 fixture 생성기를 추가했다.
MFC 목록의 X/Y는 무게중심이 아니라 bbox 좌상단이다. 전체 shape/moments/topology는
DLL에서 제공하지만 MFC 목록에는 일부 필드만 표시한다.

기본 Min area=1에서는 작은 배경 성분도 많이 남을 수 있다. 12는 이 샘플의 재현 조건이며
생산 합격 임계값으로 확정한 값이 아니다. 파라미터 변경 후 Analyze를 눌러야 새 결과가 나온다.
정답 라벨 기반 정확도, 전체 10장 MFC 조작, x86 UI, 대형 이미지/장시간 처리,
HALCON 일치성, 깨끗한 PC는 이번 검증 범위에 포함하지 않는다.

## 바이너리 및 로컬 증거

- MFC SHA256: `bc3b0ebd69193e927cf8e1dc7aa4b50cf8e40e54699f7bd9504f27b9c526d642`
- DLL SHA256: `5a8323f05fa96d246bf26ed51615685bf228a579710c568132802a5683d2376d`
- `../halcon-image-study/v002-mfc-blobs.png`
- `../halcon-image-study/v002-mfc-group.png`
- `../halcon-image-study/v002-mfc-synthetic.png`
- `../halcon-image-study/v002-mfc-gap0.png`
- `../halcon-image-study/v002-mfc-blank.png`
- `../halcon-image-study/v002-mfc-invalid-param.png`
- `../halcon-image-study/v002-mfc-invalid-bmp.png`
- `build-release-download-002/001213-dll.csv`, `group-reference.json`

위 경로는 저장소 루트 기준 로컬 증거이며 공개 저장소에는 이미지 원본을 업로드하지 않았다.
