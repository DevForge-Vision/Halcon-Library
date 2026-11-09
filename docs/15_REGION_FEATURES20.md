# Blob / 그룹의 region_features 1~20 조회

2026-11-09: 로컬 v142/x64 DLL 빌드 및 회귀 11/11 통과. MFC는 Actions에서 검증한다. HALCON 실측은 미실시.
계산은 Feature 계층, 핸들/인자 검증과 예외 변환은 Facade 계층에 분리했다.
배포된 v0.0.2 DLL에는 이 API가 없다. 새 헤더/MFC 소스와 구버전 DLL을 섞으면 안 된다.

```cpp
RegionFeatures20 values = {};
ImageStatus status = GetBlobRegionFeatures20(result, blobIndex, &values, sizeof(values));
if (status == ImageStatus::Ok)
{
	double area = values.values[static_cast<int>(RegionFeature20::Area)];
	double radius = values.values[static_cast<int>(RegionFeature20::InnerRadius)];
	bool provisional = (values.provisionalMask & (1u << static_cast<int>(RegionFeature20::InnerRadius))) != 0;
}
// 그룹도 동일한 반환형: GetDefectRegionFeatures20(groupResult, groupIndex, &values, sizeof(values));
```

입력은 기존 AnalyzeImage/AnalyzeBmpFile, GroupBlobDefects가 반환한 살아 있는 핸들과
0부터 시작하는 Blob/그룹 인덱스다. 배열은 호출자가 소유하며 추가 해제가 필요 없다.
기존 BlobFeatures/DefectFeatures의 크기나 필드는 변경하지 않았다.

| 순번 / 배열 인덱스 | 이름 | 의미 / 단위 |
|---|---|---|
| 1 / 0 | anisometry | 타원 장축/단축 비 |
| 2 / 1 | area | 전경 픽셀 수 |
| 3 / 2 | area_holes | 구멍 픽셀 수 |
| 4 / 3 | bulkiness | 기존 eccentricity 정의의 bulkiness |
| 5 / 4 | circularity | 원형도 |
| 6 / 5 | column | 면적 중심 X, 전역 이미지 좌표 |
| 7 / 6 | column1 | bbox 최소 X, 포함 |
| 8 / 7 | column2 | bbox 최대 X, 포함 |
| 9 / 8 | compactness | 조밀도 |
| 10 / 9 | connect_num | 8연결 전경 성분 수 |
| 11 / 10 | contlength | 외곽 윤곽 길이, 픽셀 단위 |
| 12 / 11 | convexity | 전경 면적 / convex hull 면적 |
| 13 / 12 | dist_deviation* | 경계→면적 중심 거리의 모집단 표준편차, px |
| 14 / 13 | dist_mean* | 경계→면적 중심 거리의 평균, px |
| 15 / 14 | euler_number | 성분 수 - 구멍 수 |
| 16 / 15 | height | bbox 포함 높이, px |
| 17 / 16 | holes_num | 구멍 수 |
| 18 / 17 | inner_height* | 내접 축평행 사각형 포함 높이, px |
| 19 / 18 | inner_radius* | 내접 이산 원 반지름, px |
| 20 / 19 | inner_width* | 내접 축평행 사각형 포함 폭, px |

`*`는 반환값이 있지만 정의에 미결 사항이 있는 항목이다. `provisionalMask`의
해당 인덱스 비트가 1이다. 나머지 항목도 HALCON 실측 동등성을 보증하지 않는다.
기본 정책과 대안 매크로는 [정의서](02_DEFINITIONS.md)의 RF20-D1/R1/C1에 기록했다.
컴파일 시 적용된 대안은 policyFlags의 1/2/4 비트로 식별한다.

그룹 특징은 원래 조각의 합집합에서 계산한다. 간격은 채우지 않으며
그룹의 내접 사각형/원은 전체 bbox 또는 조각별 값의 합이 아니다.

MFC에서는 기존 9열 뒤에 20열을 추가했다. 오른쪽으로 가로 스크롤하여 확인한다.
추가 계산은 기존 작업 스레드에서 수행한다. 확장 조회가 실패하면 해당 행의
20개 셀에 `N/A (status N)`을 표시하며 기존 결과와 마스크는 유지한다.

호출 실패: null/잘못된 인덱스는 InvalidArgument, bytes 불일치는 VersionMismatch,
패딩 bbox 4,194,304픽셀 초과는 ResourceLimit, 메모리 부족은 OutOfMemory다.
bytes 불일치는 출력 메모리를 건드리지 않는다. 나머지 실패는 올바른 출력 포인터에
메타데이터와 0값을 남긴다. 실패의 0을 실제 측정치로 쓰지 말 것.
추가 조회는 O(bbox 면적)의 임시 메모리/시간을 사용하므로 호출자는 결과를 캐시한다.

검증 코드는 `test/RegionFeatures20Test.cpp`, CTest 이름은 `GlimRegionFeatures20`이다.
단일 픽셀, 3x3, 링, 그룹 수명, ROI 전역 좌표, 15개 기존값 매핑,
잘못된 인자와 ABI 크기를 대상으로 작성했다. 실행 결과는 아직 없다.
