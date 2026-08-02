//====================================================================================
// test/TestMain.cpp — 기준 도형 기댓값 대조 테스트
//
// 근거: docs/03_VERIFICATION.md
//   §1  허용 오차
//   §2  기준 도형 S1~S7        / §2B 2차 배치 판별 도형 S1b · S4T · S8
//   §3  1차 배치 손계산 기댓값 (T-1 ~ T-6)
//   §3B 2차 배치 손계산 기댓값 (T-7 ~ T-13)
//   §5  결정적 판정 P-1 ~ P-3  / §5B 판정 P-4 ~ P-9 + 미결 4건
//
// 종료 코드
//   0 : 모든 HARD 검사 통과
//   1 : 하나라도 실패
//
// NOTE 로 분류된 항목은 "정의서 §2 수식대로 계산하면 문서의 손계산 기댓값과
// 어긋나는 것이 정상"인 지점이다. 실패로 세지 않되 반드시 출력한다.
//====================================================================================
#include "GlimHalcon.h"
#include "Shapes.h"

#include <cstddef>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

using namespace glim::halcon;

namespace {

const double kPi = 3.14159265358979323846;

int g_passCount = 0;
int g_failCount = 0;
std::vector<std::string> g_failMessages;
std::vector<std::string> g_noteMessages;

//------------------------------------------------------------------------------------
// 검사 헬퍼
//------------------------------------------------------------------------------------
void Report(bool ok, const char* item, double got, double expect, const char* how)
{
	std::printf("  [%s] %-34s got=%18.9f  expect=%18.9f  (%s)\n",
				ok ? "PASS" : "FAIL", item, got, expect, how);

	if (ok)
	{
		++g_passCount;
	}
	else
	{
		++g_failCount;

		char buffer[512];
		std::sprintf(buffer, "%s : got=%.9f expect=%.9f (%s)", item, got, expect, how);
		g_failMessages.push_back(std::string(buffer));
	}
}

// 완전 일치 (정수 / 이산값) — 부동소수 표현 오차만 허용
void CheckExact(const char* item, double got, double expect)
{
	Report(std::fabs(got - expect) <= 1e-9, item, got, expect, "exact / 1e-9");
}

// 절대 오차
void CheckAbs(const char* item, double got, double expect, double tolerance)
{
	char how[64];
	std::sprintf(how, "abs <= %g", tolerance);
	Report(std::fabs(got - expect) <= tolerance, item, got, expect, how);
}

// 상대 오차 (곡선 도형: ±2%)
void CheckRelative(const char* item, double got, double expect, double relative)
{
	const double base = (std::fabs(expect) > 0.0) ? std::fabs(expect) : 1.0;
	char how[64];
	std::sprintf(how, "rel <= %.1f%%", relative * 100.0);
	Report(std::fabs(got - expect) <= base * relative, item, got, expect, how);
}

// 두 후보값 중 하나이면 통과 (대칭 도형의 orientation_region — OR-3)
//   원문에 동률 규정이 없어 어느 쪽도 오답이 아니다. 구현 내부 순회 순서에 테스트를 묶지 않는다.
void CheckOneOf(const char* item, double got, double candidateA, double candidateB, double tolerance)
{
	const bool ok = (std::fabs(got - candidateA) <= tolerance)
				 || (std::fabs(got - candidateB) <= tolerance);

	char how[128];
	std::sprintf(how, "%.9f 또는 %.9f (OR-3 동률)", candidateA, candidateB);

	std::printf("  [%s] %-34s got=%18.9f  %s\n", ok ? "PASS" : "FAIL", item, got, how);

	if (ok)
	{
		++g_passCount;
	}
	else
	{
		++g_failCount;

		char buffer[512];
		std::sprintf(buffer, "%s : got=%.9f (%s)", item, got, how);
		g_failMessages.push_back(std::string(buffer));
	}
}

// 조건 검사 (판별 시험용)
void CheckCondition(bool ok, const char* item, double got, const char* how)
{
	std::printf("  [%s] %-34s got=%18.9f  %s\n", ok ? "PASS" : "FAIL", item, got, how);

	if (ok)
	{
		++g_passCount;
	}
	else
	{
		++g_failCount;

		char buffer[512];
		std::sprintf(buffer, "%s : got=%.9f (%s)", item, got, how);
		g_failMessages.push_back(std::string(buffer));
	}
}

// 정의서 기댓값과 어긋나는 지점 기록 (실패로 세지 않는다)
void Note(const char* item, double got, double docExpect, const char* reason)
{
	std::printf("  [NOTE] %-34s got=%18.9f  doc=%18.9f  %s\n", item, got, docExpect, reason);

	char buffer[512];
	std::sprintf(buffer, "%s : got=%.6f, docs=%.6f -> %s", item, got, docExpect, reason);
	g_noteMessages.push_back(std::string(buffer));
}

//------------------------------------------------------------------------------------
// 한 도형의 전 특징값
//------------------------------------------------------------------------------------
struct Measured
{
	const char* name;
	long area;
	double row;
	double column;
	double contLength;
	double circularity;
	double compactness;
	double convexity;
};

Measured Measure(const char* name, const Region& region)
{
	Measured m;
	m.name = name;
	m.area = 0;
	m.row = 0.0;
	m.column = 0.0;
	m.contLength = 0.0;
	m.circularity = 0.0;
	m.compactness = 0.0;
	m.convexity = 0.0;

	AreaCenter(region, m.area, m.row, m.column);
	ContLength(region, m.contLength);
	Circularity(region, m.circularity);
	Compactness(region, m.compactness);
	Convexity(region, m.convexity);
	return m;
}

//------------------------------------------------------------------------------------
// 2차 배치 — 한 도형의 모멘트 계열 15값
//   개별 오퍼레이터 5종을 각각 호출해 채운다 (배치 API 와의 일치는 T-13 에서 따로 검사).
//------------------------------------------------------------------------------------
struct MeasuredMoments
{
	const char* name;
	long area;
	double centerRow;
	double centerColumn;

	double m11, m20, m02, ia, ib;
	double m11Invar, m20Invar, m02Invar;
	double ra, rb, phi;
	double anisometry, bulkiness, structureFactor;
	double orientation;
};

MeasuredMoments MeasureMoments(const char* name, const Region& region)
{
	MeasuredMoments m;
	m.name = name;
	m.area = 0;
	m.centerRow = 0.0;
	m.centerColumn = 0.0;
	m.m11 = 0.0; m.m20 = 0.0; m.m02 = 0.0; m.ia = 0.0; m.ib = 0.0;
	m.m11Invar = 0.0; m.m20Invar = 0.0; m.m02Invar = 0.0;
	m.ra = 0.0; m.rb = 0.0; m.phi = 0.0;
	m.anisometry = 0.0; m.bulkiness = 0.0; m.structureFactor = 0.0;
	m.orientation = 0.0;

	AreaCenter(region, m.area, m.centerRow, m.centerColumn);
	MomentsRegion2nd(region, m.m11, m.m20, m.m02, m.ia, m.ib);
	MomentsRegion2ndInvar(region, m.m11Invar, m.m20Invar, m.m02Invar);
	EllipticAxis(region, m.ra, m.rb, m.phi);
	Eccentricity(region, m.anisometry, m.bulkiness, m.structureFactor);
	OrientationRegion(region, m.orientation);
	return m;
}

//------------------------------------------------------------------------------------
// T-12. 정의 연쇄 항등식 — 모든 도형에서 항상 성립해야 한다
//   (docs/03_VERIFICATION.md §3B T-12)
//   기댓값 표와 별개로, 한 오퍼레이터만 고치고 나머지를 안 고치는 회귀를 잡는다.
//------------------------------------------------------------------------------------
void CheckMomentIdentities(const MeasuredMoments& m)
{
	char item[160];
	const double area = static_cast<double>(m.area);

	// I-3 : 고유값 순서와 반정치성
	std::sprintf(item, "%s I-3 Ia>=Ib>=0", m.name);
	CheckCondition(m.ia >= m.ib && m.ib >= 0.0, item, m.ib, "Ia >= Ib >= 0");

	std::sprintf(item, "%s I-3 Ra>=Rb>=0", m.name);
	CheckCondition(m.ra >= m.rb && m.rb >= 0.0, item, m.rb, "Ra >= Rb >= 0 (Assertion)");

	// I-6 : orientation_region 범위 (Assertion)
	std::sprintf(item, "%s I-6 -pi<=Ori<pi", m.name);
	CheckCondition(m.orientation >= -kPi && m.orientation < kPi, item, m.orientation,
				   "-pi <= Phi < pi (Assertion)");

	// elliptic_axis 의 Phi 범위 (Assertion). EA-2 기본값이면 -pi/2 는 제외된다.
	std::sprintf(item, "%s -pi/2<Phi<=pi/2", m.name);
	CheckCondition(m.phi > -kPi * 0.5 && m.phi <= kPi * 0.5, item, m.phi,
				   "-pi/2 < Phi <= pi/2 (Assertion, EA-2)");

	if (area <= 0.0)
		return;			// 빈 리전은 여기까지 (아래 항등식은 F > 0 을 전제한다)

	// I-1 : moments_region_2nd_invar · F^2 == moments_region_2nd
	const double areaSquared = area * area;
	const double scale20 = (std::fabs(m.m20) > 1.0) ? std::fabs(m.m20) : 1.0;
	const double scale02 = (std::fabs(m.m02) > 1.0) ? std::fabs(m.m02) : 1.0;
	const double scale11 = (std::fabs(m.m11) > 1.0) ? std::fabs(m.m11) : 1.0;

	std::sprintf(item, "%s I-1 M20i*F^2==M20", m.name);
	CheckCondition(std::fabs(m.m20Invar * areaSquared - m.m20) <= scale20 * 1e-12,
				   item, m.m20Invar * areaSquared, "F^2 정규화 연쇄");

	std::sprintf(item, "%s I-1 M02i*F^2==M02", m.name);
	CheckCondition(std::fabs(m.m02Invar * areaSquared - m.m02) <= scale02 * 1e-12,
				   item, m.m02Invar * areaSquared, "F^2 정규화 연쇄");

	std::sprintf(item, "%s I-1 M11i*F^2==M11", m.name);
	CheckCondition(std::fabs(m.m11Invar * areaSquared - m.m11) <= scale11 * 1e-12,
				   item, m.m11Invar * areaSquared, "F^2 정규화 연쇄");

	// I-2 : Ra == 2*sqrt(Ia/F)  — elliptic_axis 의 F 정규화(EA-1)를 잡는 유일한 지표
	const double raFromIa = 2.0 * std::sqrt(m.ia / area);
	const double rbFromIb = 2.0 * std::sqrt(m.ib / area);
	const double scaleRa = (m.ra > 1.0) ? m.ra : 1.0;
	const double scaleRb = (m.rb > 1.0) ? m.rb : 1.0;

	std::sprintf(item, "%s I-2 Ra==2sqrt(Ia/F)", m.name);
	CheckCondition(std::fabs(m.ra - raFromIa) <= scaleRa * 1e-12, item, raFromIa,
				   "EA-1 (F 정규화) 연쇄");

	std::sprintf(item, "%s I-2 Rb==2sqrt(Ib/F)", m.name);
	CheckCondition(std::fabs(m.rb - rbFromIb) <= scaleRb * 1e-12, item, rbFromIb,
				   "EA-1 (F 정규화) 연쇄");

	// I-4 / I-5 : eccentricity 내부 정합 (Rb > 0 일 때만 유효)
	if (m.rb > 0.0)
	{
		const double anisoExpect = m.ra / m.rb;
		std::sprintf(item, "%s I-4 Aniso==Ra/Rb", m.name);
		CheckCondition(std::fabs(m.anisometry - anisoExpect) <= anisoExpect * 1e-12,
					   item, m.anisometry, "Anisometry = Ra/Rb");

		const double sfExpect = kPi * m.ra * m.ra / area - 1.0;
		const double scaleSf = (std::fabs(sfExpect) > 1.0) ? std::fabs(sfExpect) : 1.0;
		std::sprintf(item, "%s I-5 SF==pi*Ra^2/A-1", m.name);
		CheckCondition(std::fabs(m.structureFactor - sfExpect) <= scaleSf * 1e-12,
					   item, m.structureFactor, "StructureFactor 항등식");
	}
	else
	{
		// 함정 #12 : Rb == 0 이면 Anisometry 는 반드시 0 이다 (Assertion 을 어기는 것이 정답)
		std::sprintf(item, "%s #12 Rb=0 -> Aniso=0", m.name);
		CheckExact(item, m.anisometry, 0.0);
	}

	// I-6 : orientation ∈ { Phi, wrap(Phi+pi) }
	double wrapped = m.phi + kPi;
	if (wrapped >= kPi)
		wrapped -= 2.0 * kPi;
	std::sprintf(item, "%s I-6 Ori in {Phi,Phi+pi}", m.name);
	CheckCondition(std::fabs(m.orientation - m.phi) <= 1e-9
				|| std::fabs(m.orientation - wrapped) <= 1e-9,
				   item, m.orientation, "orientation = Phi 또는 Phi+pi");
}

//------------------------------------------------------------------------------------
// T-13. 배치 API 가 개별 오퍼레이터와 **비트 단위로 같은지**
//   값 계산 헬퍼를 한 벌만 두었으므로 반드시 같아야 한다. 다르면 계산식이 복제된 것이다.
//------------------------------------------------------------------------------------
void CheckBatchMatchesIndividual(const char* name, const Region& region, const MeasuredMoments& m)
{
	RegionMoments batch;
	ComputeMoments(region, batch);

	RegionFeatures combinedFeatures;
	RegionMoments combinedMoments;
	ComputeFeaturesAndMoments(region, combinedFeatures, combinedMoments);

	RegionFeatures soloFeatures;
	ComputeFeatures(region, soloFeatures);

	const bool momentsMatch =
		batch.m11 == m.m11 && batch.m20 == m.m20 && batch.m02 == m.m02 &&
		batch.ia == m.ia && batch.ib == m.ib &&
		batch.m11Invar == m.m11Invar && batch.m20Invar == m.m20Invar &&
		batch.m02Invar == m.m02Invar &&
		batch.ra == m.ra && batch.rb == m.rb && batch.phi == m.phi &&
		batch.anisometry == m.anisometry && batch.bulkiness == m.bulkiness &&
		batch.structureFactor == m.structureFactor &&
		batch.orientation == m.orientation;

	const bool combinedMatch =
		combinedMoments.m11 == batch.m11 && combinedMoments.m20 == batch.m20 &&
		combinedMoments.m02 == batch.m02 && combinedMoments.ia == batch.ia &&
		combinedMoments.ib == batch.ib &&
		combinedMoments.m11Invar == batch.m11Invar &&
		combinedMoments.m20Invar == batch.m20Invar &&
		combinedMoments.m02Invar == batch.m02Invar &&
		combinedMoments.ra == batch.ra && combinedMoments.rb == batch.rb &&
		combinedMoments.phi == batch.phi &&
		combinedMoments.anisometry == batch.anisometry &&
		combinedMoments.bulkiness == batch.bulkiness &&
		combinedMoments.structureFactor == batch.structureFactor &&
		combinedMoments.orientation == batch.orientation;

	const bool featuresMatch =
		combinedFeatures.area == soloFeatures.area &&
		combinedFeatures.row == soloFeatures.row &&
		combinedFeatures.column == soloFeatures.column &&
		combinedFeatures.contLength == soloFeatures.contLength &&
		combinedFeatures.circularity == soloFeatures.circularity &&
		combinedFeatures.compactness == soloFeatures.compactness &&
		combinedFeatures.convexity == soloFeatures.convexity;

	char item[160];

	std::sprintf(item, "%s ComputeMoments==개별", name);
	CheckCondition(momentsMatch, item, batch.m20, "15 값 비트 단위 일치");

	std::sprintf(item, "%s ComputeF&M==ComputeM", name);
	CheckCondition(combinedMatch, item, combinedMoments.m20, "15 값 비트 단위 일치");

	std::sprintf(item, "%s ComputeF&M==ComputeF", name);
	CheckCondition(featuresMatch, item, combinedFeatures.circularity, "7 값 비트 단위 일치");
}

//------------------------------------------------------------------------------------
// 3차 배치 — 한 도형의 구멍/지름 9값 (+ 교차검증용 Area / Convexity)
//   개별 오퍼레이터 4종을 각각 호출해 채운다 (배치 API 와의 일치는 T-18 I-15 에서 검사).
//------------------------------------------------------------------------------------
struct MeasuredTopology
{
	const char* name;
	long area;
	double convexity;

	long areaHoles;
	long numConnected;
	long numHoles;
	long eulerNumber;
	long row1, column1, row2, column2;
	double diameter;
};

MeasuredTopology MeasureTopology(const char* name, const Region& region)
{
	MeasuredTopology m;
	m.name = name;
	m.area = 0;
	m.convexity = 0.0;
	m.areaHoles = 0;
	m.numConnected = 0;
	m.numHoles = 0;
	m.eulerNumber = 0;
	m.row1 = 0; m.column1 = 0; m.row2 = 0; m.column2 = 0;
	m.diameter = 0.0;

	double centerRow = 0.0;
	double centerColumn = 0.0;

	AreaCenter(region, m.area, centerRow, centerColumn);
	Convexity(region, m.convexity);

	AreaHoles(region, m.areaHoles);
	ConnectAndHoles(region, m.numConnected, m.numHoles);
	EulerNumber(region, m.eulerNumber);
	DiameterRegion(region, m.row1, m.column1, m.row2, m.column2, m.diameter);
	return m;
}

// 극점 좌표 정확값 대조 (최대쌍이 **유일한** 도형에서만 쓴다 — DR-2)
void CheckDiameterPoints(const MeasuredTopology& m, long r1, long c1, long r2, long c2)
{
	char item[160];
	char how[160];

	const bool ok = (m.row1 == r1) && (m.column1 == c1) && (m.row2 == r2) && (m.column2 == c2);

	std::sprintf(item, "%s 극점 (%ld,%ld)-(%ld,%ld)", m.name, m.row1, m.column1, m.row2, m.column2);
	std::sprintf(how, "기댓값 (%ld,%ld)-(%ld,%ld)", r1, c1, r2, c2);

	CheckCondition(ok, item, m.diameter, how);
}

//------------------------------------------------------------------------------------
// T-18. 3차 배치 정의 연쇄 항등식 — 모든 도형에서 항상 성립해야 한다
//   (docs/03_VERIFICATION.md §3C T-18)
//------------------------------------------------------------------------------------
void CheckTopologyIdentities(const MeasuredTopology& m)
{
	char item[160];

	// I-8 : EulerNumber == NumConnected - NumHoles
	//       ⭐ 이 배치에서 가장 중요하다. 깨지면 코어가 두 벌이라는 뜻이다.
	std::sprintf(item, "%s I-8 Euler==NC-NH", m.name);
	CheckCondition(m.eulerNumber == m.numConnected - m.numHoles,
				   item, static_cast<double>(m.eulerNumber), "euler_number 는 두 값의 차여야 한다");

	// I-9 : NumHoles == 0  <=>  AreaHoles == 0
	std::sprintf(item, "%s I-9 NH==0<=>AH==0", m.name);
	CheckCondition((m.numHoles == 0) == (m.areaHoles == 0),
				   item, static_cast<double>(m.areaHoles), "개수와 면적이 같은 성분 집합에서 나오는가");

	// I-10 : 구멍 하나는 최소 1픽셀
	std::sprintf(item, "%s I-10 AH>=NH", m.name);
	CheckCondition(m.areaHoles >= m.numHoles,
				   item, static_cast<double>(m.areaHoles), "AreaHoles >= NumHoles");

	// I-11 : Area > 0  <=>  NumConnected > 0
	std::sprintf(item, "%s I-11 A>0<=>NC>0", m.name);
	CheckCondition((m.area > 0) == (m.numConnected > 0),
				   item, static_cast<double>(m.numConnected), "빈 리전 규약");

	// I-12 : 돌려준 두 점이 실제로 그 거리를 만드는가
	//        ⭐ 동률 도형(DR-2)에서 극점을 검사하는 유일한 수단이다.
	{
		const double dr = static_cast<double>(m.row1 - m.row2);
		const double dc = static_cast<double>(m.column1 - m.column2);
		const double distance = std::sqrt(dr * dr + dc * dc);

		std::sprintf(item, "%s I-12 |P1-P2|==Diameter", m.name);
		CheckCondition(std::fabs(distance - m.diameter) <= 1e-9,
					   item, distance, "돌려준 점쌍이 실제로 Diameter 를 만드는가");
	}

	// I-13 : 구멍이 있으면 볼록도가 1 미만이다 (1차 배치와의 교차 검증)
	//        구멍 픽셀은 hull 안에 반드시 들어가므로 hull 면적 > 리전 면적이다.
	if (m.numHoles > 0)
	{
		std::sprintf(item, "%s I-13 NH>0 -> Convex<1", m.name);
		CheckCondition(m.convexity < 1.0, item, m.convexity, "구멍이 있으면 Convexity < 1");
	}

	// I-14 : Area <= 1  <=>  Diameter == 0   (DR-1 픽셀 모델)
	std::sprintf(item, "%s I-14 A<=1<=>D==0", m.name);
	CheckCondition((m.area <= 1) == (m.diameter == 0.0),
				   item, m.diameter, "픽셀 = 무한소 점. 2픽셀 이상이면 Diameter >= 1");
}

//------------------------------------------------------------------------------------
// T-18 I-15. 배치 API 가 개별 오퍼레이터와 **비트 단위로 같은지**
//------------------------------------------------------------------------------------
void CheckTopologyBatch(const Region& region, const MeasuredTopology& m)
{
	RegionTopology batch;
	ComputeTopology(region, batch);

	RegionFeatures allFeatures;
	RegionMoments allMoments;
	RegionTopology allTopology;
	ComputeFeaturesMomentsAndTopology(region, allFeatures, allMoments, allTopology);

	RegionFeatures soloFeatures;
	ComputeFeatures(region, soloFeatures);

	RegionMoments soloMoments;
	ComputeMoments(region, soloMoments);

	const bool topologyMatch =
		batch.areaHoles == m.areaHoles &&
		batch.numConnected == m.numConnected &&
		batch.numHoles == m.numHoles &&
		batch.eulerNumber == m.eulerNumber &&
		batch.diameterRow1 == m.row1 && batch.diameterColumn1 == m.column1 &&
		batch.diameterRow2 == m.row2 && batch.diameterColumn2 == m.column2 &&
		batch.diameter == m.diameter;

	const bool combinedMatch =
		allTopology.areaHoles == batch.areaHoles &&
		allTopology.numConnected == batch.numConnected &&
		allTopology.numHoles == batch.numHoles &&
		allTopology.eulerNumber == batch.eulerNumber &&
		allTopology.diameterRow1 == batch.diameterRow1 &&
		allTopology.diameterColumn1 == batch.diameterColumn1 &&
		allTopology.diameterRow2 == batch.diameterRow2 &&
		allTopology.diameterColumn2 == batch.diameterColumn2 &&
		allTopology.diameter == batch.diameter;

	// 3배치 묶음이 기존 배치 API 의 값을 **하나도 바꾸지 않았는지** 함께 본다 (회귀 감시)
	const bool featuresMatch =
		allFeatures.area == soloFeatures.area &&
		allFeatures.row == soloFeatures.row &&
		allFeatures.column == soloFeatures.column &&
		allFeatures.contLength == soloFeatures.contLength &&
		allFeatures.circularity == soloFeatures.circularity &&
		allFeatures.compactness == soloFeatures.compactness &&
		allFeatures.convexity == soloFeatures.convexity;

	const bool momentsMatch =
		allMoments.m11 == soloMoments.m11 && allMoments.m20 == soloMoments.m20 &&
		allMoments.m02 == soloMoments.m02 && allMoments.ia == soloMoments.ia &&
		allMoments.ib == soloMoments.ib &&
		allMoments.m11Invar == soloMoments.m11Invar &&
		allMoments.m20Invar == soloMoments.m20Invar &&
		allMoments.m02Invar == soloMoments.m02Invar &&
		allMoments.ra == soloMoments.ra && allMoments.rb == soloMoments.rb &&
		allMoments.phi == soloMoments.phi &&
		allMoments.anisometry == soloMoments.anisometry &&
		allMoments.bulkiness == soloMoments.bulkiness &&
		allMoments.structureFactor == soloMoments.structureFactor &&
		allMoments.orientation == soloMoments.orientation;

	char item[160];

	std::sprintf(item, "%s ComputeTopology==개별", m.name);
	CheckCondition(topologyMatch, item, static_cast<double>(batch.areaHoles), "9 값 비트 단위 일치");

	std::sprintf(item, "%s ComputeFMT==ComputeT", m.name);
	CheckCondition(combinedMatch, item, static_cast<double>(allTopology.areaHoles), "9 값 비트 단위 일치");

	std::sprintf(item, "%s ComputeFMT==ComputeF", m.name);
	CheckCondition(featuresMatch, item, allFeatures.convexity, "7 값 비트 단위 일치 (1차 회귀)");

	std::sprintf(item, "%s ComputeFMT==ComputeM", m.name);
	CheckCondition(momentsMatch, item, allMoments.m20, "15 값 비트 단위 일치 (2차 회귀)");
}

//------------------------------------------------------------------------------------
// HALCON Assertion 재현 확인 (함정 #1 클리핑 방향, 함정 #2 분자·분모 방향)
//------------------------------------------------------------------------------------
void CheckAssertions(const Measured& m)
{
	char item[128];

	std::sprintf(item, "%s assert ContLength>=0", m.name);
	CheckCondition(m.contLength >= 0.0, item, m.contLength, "ContLength >= 0");

	std::sprintf(item, "%s assert 0<=Circ<=1", m.name);
	CheckCondition(m.circularity >= 0.0 && m.circularity <= 1.0, item, m.circularity, "0 <= C <= 1");

	std::sprintf(item, "%s assert Comp>=1||==0", m.name);
	CheckCondition(m.compactness == 0.0 || m.compactness >= 1.0, item, m.compactness, "C >= 1 or C == 0");

	std::sprintf(item, "%s assert Convex<=1", m.name);
	CheckCondition(m.convexity <= 1.0, item, m.convexity, "C <= 1 (CV-1: F_c 는 픽셀 개수)");
}

}	// anonymous namespace

//====================================================================================
int main()
{
	const double kSqrt2 = std::sqrt(2.0);

	Region s1 = shapes::MakeS1Square();
	Region s2 = shapes::MakeS2Disk();
	Region s3 = shapes::MakeS3Ring();
	Region s4 = shapes::MakeS4Line();
	Region s5 = shapes::MakeS5LShape();
	Region s6 = shapes::MakeS6SinglePixel();
	Region s7 = shapes::MakeS7Empty();

	Measured m1 = Measure("S1", s1);
	Measured m2 = Measure("S2", s2);
	Measured m3 = Measure("S3", s3);
	Measured m4 = Measure("S4", s4);
	Measured m5 = Measure("S5", s5);
	Measured m6 = Measure("S6", s6);
	Measured m7 = Measure("S7", s7);

	const Measured all[7] = { m1, m2, m3, m4, m5, m6, m7 };

	//--------------------------------------------------------------------------------
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon Phase 1 - 기준 도형 S1~S7 측정값\n");
	std::printf(" (docs/03_VERIFICATION.md)\n");
	std::printf("====================================================================================\n");
	std::printf("%-4s %8s %12s %12s %14s %12s %12s %12s\n",
				"ID", "Area", "Row", "Column", "ContLength", "Circular", "Compact", "Convex");
	std::printf("------------------------------------------------------------------------------------------------\n");
	for (int i = 0; i < 7; ++i)
	{
		std::printf("%-4s %8ld %12.6f %12.6f %14.6f %12.7f %12.7f %12.7f\n",
					all[i].name, all[i].area, all[i].row, all[i].column,
					all[i].contLength, all[i].circularity, all[i].compactness, all[i].convexity);
	}
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-1. area_center   (docs/03_VERIFICATION.md §3 T-1)
	//--------------------------------------------------------------------------------
	std::printf("[T-1] area_center -----------------------------------------------------------------\n");
	CheckExact("S1 Area",        static_cast<double>(m1.area), 10000.0);
	CheckExact("S1 Row",         m1.row,    49.5);
	CheckExact("S1 Column",      m1.column, 49.5);

	CheckExact("S4 Area",        static_cast<double>(m4.area), 100.0);
	CheckExact("S4 Row",         m4.row,    50.0);
	CheckExact("S4 Column",      m4.column, 49.5);

	CheckExact("S5 Area",        static_cast<double>(m5.area), 7500.0);
	CheckAbs  ("S5 Row",         m5.row,    (5000.0 * 49.5 + 2500.0 * 74.5) / 7500.0, 1e-9);
	CheckAbs  ("S5 Column",      m5.column, (5000.0 * 24.5 + 2500.0 * 74.5) / 7500.0, 1e-9);

	CheckExact("S6 Area",        static_cast<double>(m6.area), 1.0);
	CheckExact("S6 Row",         m6.row,    50.0);
	CheckExact("S6 Column",      m6.column, 50.0);

	CheckExact("S7 Area",        static_cast<double>(m7.area), 0.0);
	CheckExact("S7 Row",         m7.row,    0.0);
	CheckExact("S7 Column",      m7.column, 0.0);

	// 대칭 도형의 무게중심은 정확히 중심이어야 한다.
	CheckExact("S2 Row(대칭)",   m2.row,    50.0);
	CheckExact("S2 Column(대칭)",m2.column, 50.0);
	CheckExact("S3 Row(대칭)",   m3.row,    50.0);
	CheckExact("S3 Column(대칭)",m3.column, 50.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-2. contlength   (§3 T-2, 결정적 판정 P-1 / P-3)
	//--------------------------------------------------------------------------------
	std::printf("[T-2] contlength ------------------------------------------------------------------\n");
	// P-1: 픽셀 중심 체인 → S1 = 396 (경계둘레 모델이면 400)
	CheckExact("S1 ContLength (P-1)", m1.contLength, 396.0);
	CheckExact("S4 ContLength (P-1)", m4.contLength, 198.0);
	// P-1 의 가장 명확한 판별: 단일 픽셀은 스텝이 없으므로 0 (경계둘레 모델이면 4)
	CheckExact("S6 ContLength (P-1)", m6.contLength, 0.0);
	CheckExact("S7 ContLength",       m7.contLength, 0.0);

	// S5 L자: 축평행 394 스텝 + 오목 코너의 대각 1스텝
	//   상단 49 + 좌팔 우변 49 + (대각 1) + 하팔 상변 49 + 우변 49 + 하변 99 + 좌변 99
	CheckExact("S5 ContLength", m5.contLength, 394.0 + kSqrt2);

	// P-3: 구멍 윤곽 제외. 포함해 버리면 링에서 약 471 이 된다.
	CheckCondition(m3.contLength < 400.0, "S3 ContLength 구멍제외 (P-3)", m3.contLength,
				   "< 400 이어야 함 (구멍 포함시 약 471)");
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-3. circularity   (§3 T-3)
	//--------------------------------------------------------------------------------
	std::printf("[T-3] circularity -----------------------------------------------------------------\n");
	// S1: max = sqrt(49.5^2 + 49.5^2) → max^2 = 4900.5
	CheckAbs("S1 Circularity", m1.circularity, 10000.0 / (4900.5 * kPi), 1e-9);
	// S4: max = 49.5
	CheckAbs("S4 Circularity", m4.circularity, 100.0 / (49.5 * 49.5 * kPi), 1e-9);
	CheckExact("S6 Circularity", m6.circularity, 0.0);		// max == 0 → 0 나눗셈 방어
	CheckExact("S7 Circularity", m7.circularity, 0.0);
	// 곡선 도형: ±2%
	CheckRelative("S2 Circularity", m2.circularity, 1.0,  0.02);
	CheckRelative("S3 Circularity", m3.circularity, 0.75, 0.02);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-4. compactness   (§3 T-4) — contlength 에 연쇄 의존
	//--------------------------------------------------------------------------------
	std::printf("[T-4] compactness -----------------------------------------------------------------\n");
	CheckAbs("S1 Compactness", m1.compactness, (396.0 * 396.0) / (4.0 * 10000.0 * kPi), 1e-9);
	CheckAbs("S4 Compactness", m4.compactness, (198.0 * 198.0) / (4.0 * 100.0 * kPi), 1e-9);
	// 단일 픽셀: L=0, F=1 → C'=0 → max(1,0) = 1.0  (정의서 §4.3 수식 그대로)
	CheckExact("S6 Compactness", m6.compactness, 1.0);
	CheckExact("S7 Compactness", m7.compactness, 0.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-5. convexity   (§3 T-5, 결정적 판정 P-2)
	//--------------------------------------------------------------------------------
	std::printf("[T-5] convexity -------------------------------------------------------------------\n");
	// P-2: F_c = hull 래스터화 픽셀 개수 → 정사각형에서 정확히 1.0
	//      폴리곤 기하 면적(99x99=9801)을 쓰면 1.0203 이 되어 여기서 즉시 실패한다.
	CheckExact("S1 Convexity (P-2)", m1.convexity, 1.0);
	CheckExact("S4 Convexity",       m4.convexity, 1.0);		// 축퇴 hull = 자기 자신
	CheckExact("S6 Convexity",       m6.convexity, 1.0);
	CheckExact("S7 Convexity",       m7.convexity, 0.0);
	CheckRelative("S2 Convexity", m2.convexity, 1.0,   0.02);
	CheckRelative("S3 Convexity", m3.convexity, 0.75,  0.02);
	CheckRelative("S5 Convexity", m5.convexity, 0.872, 0.02);	// 래스터화 해석 기준
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// HALCON Assertion 재현
	//--------------------------------------------------------------------------------
	std::printf("[T-6] HALCON Assertion ------------------------------------------------------------\n");
	for (int i = 0; i < 7; ++i)
		CheckAssertions(all[i]);
	std::printf("\n");

	//================================================================================
	// 2차 배치 — 2차 모멘트 계열 5종
	//   moments_region_2nd / moments_region_2nd_invar / elliptic_axis /
	//   eccentricity / orientation_region
	//   (docs/02_DEFINITIONS.md §6~§10, docs/03_VERIFICATION.md §3B)
	//================================================================================
	Region s1b = shapes::MakeS1bSquare50();			// 스케일 불변성 판별
	Region s4t = shapes::MakeS4TVerticalLine();		// EA-2 판별
	Region s8  = shapes::MakeS8AsymmetricDumbbell();// OR-1 / OR-3 판별

	// 1차 7값도 새 도형 3종에 대해 Assertion 을 확인한다 (회귀 감시 확장)
	const Measured extra[3] = { Measure("S1b", s1b), Measure("S4T", s4t), Measure("S8", s8) };

	MeasuredMoments q1  = MeasureMoments("S1",  s1);
	MeasuredMoments q2  = MeasureMoments("S2",  s2);
	MeasuredMoments q3  = MeasureMoments("S3",  s3);
	MeasuredMoments q4  = MeasureMoments("S4",  s4);
	MeasuredMoments q5  = MeasureMoments("S5",  s5);
	MeasuredMoments q6  = MeasureMoments("S6",  s6);
	MeasuredMoments q7  = MeasureMoments("S7",  s7);
	MeasuredMoments q1b = MeasureMoments("S1b", s1b);
	MeasuredMoments q4t = MeasureMoments("S4T", s4t);
	MeasuredMoments q8  = MeasureMoments("S8",  s8);

	const MeasuredMoments allMoments[10] = { q1, q2, q3, q4, q5, q6, q7, q1b, q4t, q8 };
	const Region* allRegions[10] = { &s1, &s2, &s3, &s4, &s5, &s6, &s7, &s1b, &s4t, &s8 };

	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 2차 배치 - 2차 모멘트 계열 측정값\n");
	std::printf("====================================================================================\n");
	std::printf("%-4s %8s %16s %16s %16s %12s %12s\n",
				"ID", "Area", "M11", "M20", "M02", "Ia", "Ib");
	for (int i = 0; i < 10; ++i)
	{
		std::printf("%-4s %8ld %16.4f %16.4f %16.4f %12.1f %12.1f\n",
					allMoments[i].name, allMoments[i].area,
					allMoments[i].m11, allMoments[i].m20, allMoments[i].m02,
					allMoments[i].ia, allMoments[i].ib);
	}
	std::printf("\n");
	std::printf("%-4s %14s %14s %14s %10s %10s %10s %10s %10s\n",
				"ID", "M11invar", "M20invar", "M02invar", "Ra", "Rb", "Phi", "Aniso", "Orient");
	for (int i = 0; i < 10; ++i)
	{
		std::printf("%-4s %14.9f %14.9f %14.9f %10.6f %10.6f %10.6f %10.6f %10.6f\n",
					allMoments[i].name,
					allMoments[i].m11Invar, allMoments[i].m20Invar, allMoments[i].m02Invar,
					allMoments[i].ra, allMoments[i].rb, allMoments[i].phi,
					allMoments[i].anisometry, allMoments[i].orientation);
	}
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-7. moments_region_2nd   (§3B T-7)
	//   직사각형 H x W :  M20 = W·H(H²−1)/12 ,  M02 = H·W(W²−1)/12 ,  M11 = 0
	//--------------------------------------------------------------------------------
	std::printf("[T-7] moments_region_2nd (정규화 없음) --------------------------------------------\n");
	CheckExact("S1 M11",  q1.m11, 0.0);
	CheckExact("S1 M20",  q1.m20, 8332500.0);		// 100 · 100·9999/12
	CheckExact("S1 M02",  q1.m02, 8332500.0);
	CheckExact("S1 Ia",   q1.ia,  8332500.0);
	CheckExact("S1 Ib",   q1.ib,  8332500.0);

	CheckExact("S1b M20", q1b.m20, 520625.0);		//  50 ·  50·2499/12
	CheckExact("S1b M02", q1b.m02, 520625.0);
	CheckExact("S1b M11", q1b.m11, 0.0);

	// 함정 #7 : 픽셀은 무한소 점이다. 두께 1 을 더하면 M20 = 100/12 ≈ 8.33 이 나온다
	CheckExact("S4 M20 (함정#7)",  q4.m20, 0.0);
	CheckExact("S4 M02",           q4.m02, 83325.0);
	CheckExact("S4 Ia",            q4.ia,  83325.0);
	CheckExact("S4 Ib (함정#7)",   q4.ib,  0.0);

	CheckExact("S4T M20",          q4t.m20, 83325.0);
	CheckExact("S4T M02 (함정#7)", q4t.m02, 0.0);
	CheckExact("S4T Ib (함정#7)",  q4t.ib,  0.0);

	CheckAbs("S5 M11", q5.m11, 18750000.0 / 9.0, 1e-6);
	CheckAbs("S5 M20", q5.m20, 51556875.0 / 9.0, 1e-6);
	CheckAbs("S5 M02", q5.m02, 51556875.0 / 9.0, 1e-6);
	CheckAbs("S5 Ia",  q5.ia,  70306875.0 / 9.0, 1e-6);		// (51556875 + 18750000)/9
	CheckAbs("S5 Ib",  q5.ib,  32806875.0 / 9.0, 1e-6);		// (51556875 − 18750000)/9

	CheckExact("S6 M20", q6.m20, 0.0);
	CheckExact("S6 Ia",  q6.ia,  0.0);
	CheckExact("S7 M20", q7.m20, 0.0);
	CheckExact("S7 Ia",  q7.ia,  0.0);

	CheckExact("S8 M11", q8.m11, 98000.0);
	CheckExact("S8 M20", q8.m20, 112125.0);
	CheckExact("S8 M02", q8.m02, 112125.0);
	CheckExact("S8 Ia",  q8.ia,  210125.0);
	CheckExact("S8 Ib",  q8.ib,  14125.0);

	// 8중 대칭 도형은 구조 등식이 **정확히** 성립해야 한다 (행/열 누적 중 한쪽이 틀리면 깨진다)
	CheckExact("S2 M11 (대칭)",     q2.m11, 0.0);
	CheckExact("S2 M20-M02 (대칭)", q2.m20 - q2.m02, 0.0);
	CheckExact("S3 M11 (대칭)",     q3.m11, 0.0);
	CheckExact("S3 M20-M02 (대칭)", q3.m20 - q3.m02, 0.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-8. moments_region_2nd_invar   (§3B T-8) ⚠ 정규화 대조군
	//   n x n 정사각형 : M20_invar = (n²−1)/(12·n²)
	//--------------------------------------------------------------------------------
	std::printf("[T-8] moments_region_2nd_invar (F^2 정규화) ---------------------------------------\n");
	CheckAbs("S1 M20invar",  q1.m20Invar,  9999.0 / 120000.0, 1e-12);	// 0.083325
	CheckAbs("S1 M02invar",  q1.m02Invar,  9999.0 / 120000.0, 1e-12);
	CheckExact("S1 M11invar", q1.m11Invar, 0.0);

	CheckAbs("S1b M20invar", q1b.m20Invar, 2499.0 / 30000.0, 1e-12);	// 0.0833

	CheckExact("S4 M20invar",  q4.m20Invar, 0.0);
	CheckAbs("S4 M02invar",    q4.m02Invar, 8.3325, 1e-12);
	CheckAbs("S4T M20invar",   q4t.m20Invar, 8.3325, 1e-12);
	CheckExact("S4T M02invar", q4t.m02Invar, 0.0);

	// MI-1 판정 : 원점이 무게중심이면 0.10184074…, 바운딩박스 중심이면 다른 값이 나온다
	CheckAbs("S5 M20invar (MI-1)", q5.m20Invar, (51556875.0 / 9.0) / (7500.0 * 7500.0), 1e-12);
	CheckAbs("S5 M11invar (MI-1)", q5.m11Invar, 1.0 / 27.0, 1e-12);

	CheckAbs("S8 M20invar", q8.m20Invar, 0.4485, 1e-12);
	CheckAbs("S8 M11invar", q8.m11Invar, 0.392,  1e-12);

	CheckExact("S7 M20invar (0나눗셈)", q7.m20Invar, 0.0);

	// ⭐ 결정적 판정 — 함정 #6 (정규화 유무). S1 ↔ S1b 쌍이 시험이다
	CheckCondition(std::fabs(q1.m20Invar - q1b.m20Invar) < 1e-4,
				   "invar 스케일 불변 (S1 vs S1b)", q1.m20Invar - q1b.m20Invar,
				   "|차| < 1e-4 이어야 한다 (F^2 정규화)");
	CheckCondition(q1b.m20 > 0.0 && (q1.m20 / q1b.m20) > 15.0,
				   "2nd 는 불변 아님 (함정#6)", q1.m20 / q1b.m20,
				   "> 15 이어야 한다 (약 16배 = s^4). 1 이면 정규화가 잘못 들어간 것");
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-9. elliptic_axis   (§3B T-9)
	//   n x n 정사각형 : Ra = Rb = sqrt((n²−1)/3)
	//--------------------------------------------------------------------------------
	std::printf("[T-9] elliptic_axis ---------------------------------------------------------------\n");
	CheckAbs("S1 Ra (EA-1)",  q1.ra, std::sqrt(3333.0), 1e-9);	// F 정규화가 아니면 5773 또는 0.577
	CheckAbs("S1 Rb",         q1.rb, std::sqrt(3333.0), 1e-9);
	CheckExact("S1 Phi",      q1.phi, 0.0);

	CheckAbs("S1b Ra", q1b.ra, std::sqrt(833.0), 1e-9);
	CheckAbs("S1b Rb", q1b.rb, std::sqrt(833.0), 1e-9);

	CheckAbs("S4 Ra",             q4.ra, std::sqrt(3333.0), 1e-9);
	CheckExact("S4 Rb (함정#7)",  q4.rb, 0.0);
	CheckExact("S4 Phi",          q4.phi, 0.0);

	CheckAbs("S4T Ra",            q4t.ra, std::sqrt(3333.0), 1e-9);
	CheckExact("S4T Rb (함정#7)", q4t.rb, 0.0);
	// ⭐ EA-2 판정 : 접으면 +pi/2, 원문 문자 그대로면 −pi/2
	CheckAbs("S4T Phi (EA-2)",    q4t.phi, kPi * 0.5, 1e-9);

	CheckAbs("S5 Ra", q5.ra, std::sqrt(37497.0 / 9.0), 1e-9);
	CheckAbs("S5 Rb", q5.rb, std::sqrt(17497.0 / 9.0), 1e-9);
	// ⭐ 함정 #11 판정 : 통념식이면 +pi/4 가 나온다
	CheckAbs("S5 Phi (함정#11)", q5.phi, -kPi * 0.25, 1e-9);

	CheckExact("S8 Ra",           q8.ra, 41.0);				// sqrt(6724)/2 — 정확한 정수
	CheckAbs("S8 Rb",             q8.rb, std::sqrt(113.0), 1e-9);
	CheckAbs("S8 Phi (함정#11)",  q8.phi, -kPi * 0.25, 1e-9);

	CheckExact("S6 Ra", q6.ra, 0.0);
	CheckExact("S6 Rb", q6.rb, 0.0);
	CheckExact("S7 Ra", q7.ra, 0.0);

	// 곡선 도형 : 연속 기하 근사값과 ±2% 로만 비교한다 (04_IMPLEMENTATION_NOTES.md §3 규칙)
	CheckRelative("S2 Ra", q2.ra, 50.0, 0.02);
	CheckRelative("S3 Ra", q3.ra, 2.0 * std::sqrt(781.25), 0.02);
	CheckExact("S2 Ra-Rb (대칭)", q2.ra - q2.rb, 0.0);
	CheckExact("S3 Ra-Rb (대칭)", q3.ra - q3.rb, 0.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-10. eccentricity   (§3B T-10)
	//--------------------------------------------------------------------------------
	std::printf("[T-10] eccentricity ---------------------------------------------------------------\n");
	CheckExact("S1 Anisometry", q1.anisometry, 1.0);
	CheckAbs("S1 Bulkiness",       q1.bulkiness,       kPi * 3333.0 / 10000.0, 1e-9);
	CheckAbs("S1 StructureFactor", q1.structureFactor, kPi * 3333.0 / 10000.0 - 1.0, 1e-9);

	CheckExact("S1b Anisometry", q1b.anisometry, 1.0);
	CheckAbs("S1b Bulkiness",    q1b.bulkiness, kPi * 833.0 / 2500.0, 1e-9);

	CheckExact("S2 Anisometry (대칭)", q2.anisometry, 1.0);
	CheckExact("S3 Anisometry (대칭)", q3.anisometry, 1.0);
	// Bulkiness 는 Ra 의 제곱을 쓰므로 곡선 도형의 이산화 편차가 배로 전파된다 → ±3%
	CheckRelative("S2 Bulkiness", q2.bulkiness, 1.0, 0.03);
	CheckRelative("S3 Bulkiness", q3.bulkiness, 5.0 / 3.0, 0.03);
	CheckAbs("S2 StructureFactor", q2.structureFactor, 0.0, 0.05);
	CheckRelative("S3 StructureFactor", q3.structureFactor, 2.0 / 3.0, 0.05);

	// ⭐ 함정 #12 판정 : 1.0 이 나오면 Assertion 을 지키려고 손댄 것이다
	CheckExact("S4 Anisometry (함정#12)",  q4.anisometry,  0.0);
	CheckExact("S4T Anisometry (함정#12)", q4t.anisometry, 0.0);
	CheckExact("S6 Anisometry (함정#12)",  q6.anisometry,  0.0);
	CheckExact("S4 Bulkiness",             q4.bulkiness,   0.0);

	// ⭐ EC-1 판정 : 수식 그대로면 −1, "함께 0 으로" 해석이면 0
	CheckExact("S4 StructureFactor (EC-1)", q4.structureFactor,  -1.0);
	CheckExact("S6 StructureFactor (EC-1)", q6.structureFactor,  -1.0);
	// 빈 리전은 −1 이 아니라 0 이어야 한다 (축퇴 리전과 구분된다)
	CheckExact("S7 StructureFactor (빈리전)", q7.structureFactor, 0.0);
	CheckExact("S7 Anisometry",               q7.anisometry,      0.0);

	CheckAbs("S5 Anisometry",       q5.anisometry, std::sqrt(37497.0 / 17497.0), 1e-9);
	// Ra·Rb = sqrt(Ra²·Rb²) = sqrt((37497/9)·(17497/9)) — 측정값을 쓰지 않고 손계산으로 만든다
	CheckAbs("S5 Bulkiness",        q5.bulkiness,
			 kPi * std::sqrt((37497.0 / 9.0) * (17497.0 / 9.0)) / 7500.0, 1e-9);
	CheckAbs("S5 StructureFactor",  q5.structureFactor, kPi * (37497.0 / 9.0) / 7500.0 - 1.0, 1e-9);

	CheckAbs("S8 Anisometry",       q8.anisometry, 41.0 / std::sqrt(113.0), 1e-9);
	CheckAbs("S8 Bulkiness",        q8.bulkiness,  kPi * 41.0 * std::sqrt(113.0) / 500.0, 1e-9);
	CheckAbs("S8 StructureFactor",  q8.structureFactor, kPi * 1681.0 / 500.0 - 1.0, 1e-9);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-11. orientation_region   (§3B T-11)
	//   ⭐ 정확값 판정은 S8 하나가 담당한다. 나머지는 전부 대칭 도형이라 동률이다 (OR-3)
	//--------------------------------------------------------------------------------
	std::printf("[T-11] orientation_region ---------------------------------------------------------\n");
	// ⭐ OR-1 판정 : 투영 부호가 반대면 −pi/4 가 나온다
	CheckAbs("S8 Orientation (OR-1)", q8.orientation, 0.75 * kPi, 1e-9);

	CheckExact("S6 Orientation", q6.orientation, 0.0);	// 최원점 = 자기 자신 → proj = 0
	CheckExact("S7 Orientation", q7.orientation, 0.0);

	// OR-3 : 대칭 도형은 { Phi, wrap(Phi+pi) } 소속만 검사한다
	CheckOneOf("S1 Orientation (OR-3)",  q1.orientation,  0.0, -kPi, 1e-9);
	CheckOneOf("S1b Orientation (OR-3)", q1b.orientation, 0.0, -kPi, 1e-9);
	CheckOneOf("S2 Orientation (OR-3)",  q2.orientation,  0.0, -kPi, 1e-9);
	CheckOneOf("S3 Orientation (OR-3)",  q3.orientation,  0.0, -kPi, 1e-9);
	CheckOneOf("S4 Orientation (OR-3)",  q4.orientation,  0.0, -kPi, 1e-9);
	CheckOneOf("S4T Orientation (OR-3)", q4t.orientation, kPi * 0.5, -kPi * 0.5, 1e-9);
	CheckOneOf("S5 Orientation (OR-3)",  q5.orientation,  -kPi * 0.25, 0.75 * kPi, 1e-9);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-12. 정의 연쇄 항등식 (도형 무관)
	//--------------------------------------------------------------------------------
	std::printf("[T-12] 정의 연쇄 항등식 -----------------------------------------------------------\n");
	for (int i = 0; i < 10; ++i)
		CheckMomentIdentities(allMoments[i]);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-13. 배치 API == 개별 오퍼레이터 (비트 단위)
	//--------------------------------------------------------------------------------
	std::printf("[T-13] 배치 API 일치 --------------------------------------------------------------\n");
	for (int i = 0; i < 10; ++i)
		CheckBatchMatchesIndividual(allMoments[i].name, *allRegions[i], allMoments[i]);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-6b. 판별 도형 3종에 대한 1차 배치 Assertion (회귀 감시 확장)
	//--------------------------------------------------------------------------------
	std::printf("[T-6b] 판별 도형 3종의 1차 Assertion ----------------------------------------------\n");
	for (int i = 0; i < 3; ++i)
		CheckAssertions(extra[i]);
	std::printf("\n");

	//================================================================================
	// 3차 배치 — 구멍 검출 계열 4종
	//   area_holes / connect_and_holes / euler_number / diameter_region
	//   (docs/02_DEFINITIONS.md §11~§14, docs/03_VERIFICATION.md §3C)
	//================================================================================
	Region s9  = shapes::MakeS9Checker2x2();		// HC-2 판별 (전경 연결성)
	Region s10 = shapes::MakeS10Diamond();			// HC-1 판별 (배경 연결성) ★
	Region s11 = shapes::MakeS11TwoHoles();			// 구멍 합산 / Euler 음수
	Region s12 = shapes::MakeS12IslandInHole();		// HC-3 판별 (구멍 면적 집계 단위) ★

	MeasuredTopology t1  = MeasureTopology("S1",  s1);
	MeasuredTopology t2  = MeasureTopology("S2",  s2);
	MeasuredTopology t3  = MeasureTopology("S3",  s3);
	MeasuredTopology t4  = MeasureTopology("S4",  s4);
	MeasuredTopology t5  = MeasureTopology("S5",  s5);
	MeasuredTopology t6  = MeasureTopology("S6",  s6);
	MeasuredTopology t7  = MeasureTopology("S7",  s7);
	MeasuredTopology t1b = MeasureTopology("S1b", s1b);
	MeasuredTopology t4t = MeasureTopology("S4T", s4t);
	MeasuredTopology t8  = MeasureTopology("S8",  s8);
	MeasuredTopology t9  = MeasureTopology("S9",  s9);
	MeasuredTopology t10 = MeasureTopology("S10", s10);
	MeasuredTopology t11 = MeasureTopology("S11", s11);
	MeasuredTopology t12 = MeasureTopology("S12", s12);

	const MeasuredTopology allTopology[14] = {
		t1, t2, t3, t4, t5, t6, t7, t1b, t4t, t8, t9, t10, t11, t12
	};
	const Region* topologyRegions[14] = {
		&s1, &s2, &s3, &s4, &s5, &s6, &s7, &s1b, &s4t, &s8, &s9, &s10, &s11, &s12
	};

	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 3차 배치 - 구멍 검출 계열 측정값\n");
	std::printf("====================================================================================\n");
	std::printf("%-4s %8s %10s %6s %6s %7s %6s %6s %6s %6s %14s\n",
				"ID", "Area", "AreaHoles", "NConn", "NHole", "Euler",
				"Row1", "Col1", "Row2", "Col2", "Diameter");
	std::printf("------------------------------------------------------------------------------------------------\n");
	for (int i = 0; i < 14; ++i)
	{
		std::printf("%-4s %8ld %10ld %6ld %6ld %7ld %6ld %6ld %6ld %6ld %14.9f\n",
					allTopology[i].name, allTopology[i].area, allTopology[i].areaHoles,
					allTopology[i].numConnected, allTopology[i].numHoles, allTopology[i].eulerNumber,
					allTopology[i].row1, allTopology[i].column1,
					allTopology[i].row2, allTopology[i].column2,
					allTopology[i].diameter);
	}
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-14. area_holes   (§3C T-14)
	//--------------------------------------------------------------------------------
	std::printf("[T-14] area_holes -----------------------------------------------------------------\n");
	CheckExact("S1 AreaHoles",  static_cast<double>(t1.areaHoles),  0.0);
	CheckExact("S1b AreaHoles", static_cast<double>(t1b.areaHoles), 0.0);
	CheckExact("S2 AreaHoles",  static_cast<double>(t2.areaHoles),  0.0);
	CheckExact("S4 AreaHoles",  static_cast<double>(t4.areaHoles),  0.0);
	CheckExact("S4T AreaHoles", static_cast<double>(t4t.areaHoles), 0.0);
	// L자는 **오목**하지만 구멍은 없다. 볼록도와 구멍을 혼동한 구현이면 여기서 걸린다.
	CheckExact("S5 AreaHoles (오목≠구멍)", static_cast<double>(t5.areaHoles), 0.0);
	CheckExact("S6 AreaHoles",  static_cast<double>(t6.areaHoles),  0.0);
	CheckExact("S7 AreaHoles",  static_cast<double>(t7.areaHoles),  0.0);
	CheckExact("S8 AreaHoles",  static_cast<double>(t8.areaHoles),  0.0);
	CheckExact("S9 AreaHoles",  static_cast<double>(t9.areaHoles),  0.0);

	// S3 링: 안쪽 디지털 원판 R=25 의 픽셀 수. 연속 pi*25^2 = 1963.5 가 **아니다**
	CheckExact("S3 AreaHoles (이산 정확값)", static_cast<double>(t3.areaHoles), 1961.0);
	// 교차검증: |S2| - |S3| 가 그대로 구멍 면적이어야 한다
	CheckCondition(t3.areaHoles == t2.area - t3.area,
				   "S3 AreaHoles == |S2|-|S3|", static_cast<double>(t2.area - t3.area),
				   "디지털 원판 차이와 일치해야 한다");

	CheckExact("S11 AreaHoles (3x3 두 개)", static_cast<double>(t11.areaHoles), 18.0);

	// ⭐ HC-1 판정 : 배경 8-연결이면 0 이 된다
	CheckExact("S10 AreaHoles (HC-1)", static_cast<double>(t10.areaHoles), 1.0);
	// ⭐ HC-3 판정 : 성분별 합 해석이면 49 가 된다 (섬 9px 이 끼어든다)
	CheckExact("S12 AreaHoles (HC-3)", static_cast<double>(t12.areaHoles), 40.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-15. connect_and_holes   (§3C T-15)
	//--------------------------------------------------------------------------------
	std::printf("[T-15] connect_and_holes ----------------------------------------------------------\n");
	CheckExact("S1 NumConnected",  static_cast<double>(t1.numConnected), 1.0);
	CheckExact("S1 NumHoles",      static_cast<double>(t1.numHoles),     0.0);
	CheckExact("S3 NumConnected",  static_cast<double>(t3.numConnected), 1.0);
	CheckExact("S3 NumHoles",      static_cast<double>(t3.numHoles),     1.0);
	CheckExact("S5 NumConnected",  static_cast<double>(t5.numConnected), 1.0);
	CheckExact("S6 NumConnected",  static_cast<double>(t6.numConnected), 1.0);
	CheckExact("S7 NumConnected (빈리전)", static_cast<double>(t7.numConnected), 0.0);
	CheckExact("S7 NumHoles (빈리전)",     static_cast<double>(t7.numHoles),     0.0);
	// 성분 2개 — 두 사각형이 20행 떨어져 있어 8-연결로도 닿지 않는다
	CheckExact("S8 NumConnected",  static_cast<double>(t8.numConnected), 2.0);
	CheckExact("S11 NumHoles",     static_cast<double>(t11.numHoles),    2.0);
	CheckExact("S12 NumConnected (테두리+섬)", static_cast<double>(t12.numConnected), 2.0);
	CheckExact("S12 NumHoles",     static_cast<double>(t12.numHoles),    1.0);

	// ⭐ HC-2 판정 : 전경 4-연결이면 2 가 된다 (contlength 가 세는 성분 수와 어긋난다)
	CheckExact("S9 NumConnected (HC-2)", static_cast<double>(t9.numConnected), 1.0);
	// ⭐ HC-1 판정 : 배경 8-연결이면 0 이 된다
	CheckExact("S10 NumConnected", static_cast<double>(t10.numConnected), 1.0);
	CheckExact("S10 NumHoles (HC-1)", static_cast<double>(t10.numHoles), 1.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-16. euler_number   (§3C T-16)
	//--------------------------------------------------------------------------------
	std::printf("[T-16] euler_number ---------------------------------------------------------------\n");
	CheckExact("S1 EulerNumber",  static_cast<double>(t1.eulerNumber),  1.0);
	CheckExact("S2 EulerNumber",  static_cast<double>(t2.eulerNumber),  1.0);
	CheckExact("S3 EulerNumber (링)", static_cast<double>(t3.eulerNumber), 0.0);
	CheckExact("S4 EulerNumber",  static_cast<double>(t4.eulerNumber),  1.0);
	CheckExact("S6 EulerNumber",  static_cast<double>(t6.eulerNumber),  1.0);
	CheckExact("S7 EulerNumber",  static_cast<double>(t7.eulerNumber),  0.0);
	CheckExact("S8 EulerNumber (성분2)", static_cast<double>(t8.eulerNumber), 2.0);
	CheckExact("S9 EulerNumber",  static_cast<double>(t9.eulerNumber),  1.0);
	// ⭐ 음수. 부호 없는 정수로 다루면 여기서 즉시 드러난다
	CheckExact("S11 EulerNumber (음수)", static_cast<double>(t11.eulerNumber), -1.0);
	// 2 - 1 = 1. S1 과 값은 같지만 경로가 전혀 다르다
	CheckExact("S12 EulerNumber (2-1)",  static_cast<double>(t12.eulerNumber), 1.0);
	// ⭐ HC-1 판정 : 배경 8-연결이면 1 이 된다
	CheckExact("S10 EulerNumber (HC-1)", static_cast<double>(t10.eulerNumber), 0.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-17. diameter_region   (§3C T-17)
	//   ⭐ Diameter 값은 언제나 HARD. 극점 좌표는 최대쌍이 **유일한** 도형에서만 HARD (DR-2)
	//--------------------------------------------------------------------------------
	std::printf("[T-17] diameter_region ------------------------------------------------------------\n");
	CheckAbs("S1 Diameter",  t1.diameter,  99.0 * kSqrt2, 1e-9);
	CheckAbs("S1b Diameter", t1b.diameter, 49.0 * kSqrt2, 1e-9);
	// 디지털 원판: 격자점이 반지름 위에 정확히 놓여 이산 지름이 정확히 100 이다 (§3C T-17 증명)
	CheckExact("S2 Diameter (정확값)", t2.diameter, 100.0);
	CheckExact("S3 Diameter (외곽만)", t3.diameter, 100.0);
	CheckExact("S4 Diameter",  t4.diameter,  99.0);
	CheckExact("S4T Diameter", t4t.diameter, 99.0);
	CheckAbs("S5 Diameter",  t5.diameter,  99.0 * kSqrt2, 1e-9);
	CheckAbs("S8 Diameter",  t8.diameter,  49.0 * kSqrt2, 1e-9);
	CheckAbs("S9 Diameter",  t9.diameter,  kSqrt2, 1e-9);
	CheckExact("S10 Diameter", t10.diameter, 2.0);
	CheckAbs("S11 Diameter", t11.diameter, std::sqrt(260.0), 1e-9);
	CheckAbs("S12 Diameter", t12.diameter, 10.0 * kSqrt2, 1e-9);

	// ⭐ DR-1 판정 : 픽셀을 변 1의 사각형으로 보면 1.41421 이 나온다
	CheckExact("S6 Diameter (DR-1)", t6.diameter, 0.0);
	CheckExact("S7 Diameter (빈리전)", t7.diameter, 0.0);

	// 극점 좌표 — 최대쌍이 유일한 도형만
	CheckDiameterPoints(t4,  50, 0, 50, 99);
	CheckDiameterPoints(t4t, 0, 50, 99, 50);
	CheckDiameterPoints(t5,  0, 0, 99, 99);
	CheckDiameterPoints(t8,  0, 0, 49, 49);
	CheckDiameterPoints(t9,  0, 0, 1, 1);
	CheckDiameterPoints(t6,  50, 50, 50, 50);		// 단일 픽셀: 네 좌표가 모두 그 픽셀
	CheckDiameterPoints(t7,  0, 0, 0, 0);			// 빈 리전: 원문이 명시한 동작
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-18. 3차 배치 정의 연쇄 항등식 + 배치 API 일치
	//--------------------------------------------------------------------------------
	std::printf("[T-18] 3차 정의 연쇄 항등식 -------------------------------------------------------\n");
	for (int i = 0; i < 14; ++i)
		CheckTopologyIdentities(allTopology[i]);
	std::printf("\n");

	std::printf("[T-18 I-15] 배치 API 일치 ---------------------------------------------------------\n");
	for (int i = 0; i < 14; ++i)
		CheckTopologyBatch(*topologyRegions[i], allTopology[i]);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-6c. 3차 판별 도형 4종에 대한 1차 배치 Assertion (회귀 감시 확장)
	//--------------------------------------------------------------------------------
	std::printf("[T-6c] 3차 판별 도형 4종의 1차 Assertion ------------------------------------------\n");
	{
		const Measured extra3[4] = {
			Measure("S9", s9), Measure("S10", s10), Measure("S11", s11), Measure("S12", s12)
		};
		for (int i = 0; i < 4; ++i)
			CheckAssertions(extra3[i]);
	}
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// 정의서 손계산 기댓값과 어긋나는 지점 (실패 아님 / 문서 갱신 필요)
	//--------------------------------------------------------------------------------
	std::printf("[NOTE] 정의서 기댓값과의 차이 -----------------------------------------------------\n");
	Note("S3 ContLength", m3.contLength, 314.159265,
		 "문서는 연속 원주 2*pi*R. 체인코드(축1/대각sqrt2)는 디지털 원에서 약 +5.5% 편향이 원리적으로 존재");
	Note("S2 ContLength", m2.contLength, 314.159265,
		 "동일 원인 (문서 표에는 없음, 참고용)");
	Note("S2 Compactness", m2.compactness, 1.0,
		 "L 편향이 제곱으로 전파. L=2*pi*R 이면 1.0016 이 된다");
	Note("S2 Circularity", m2.circularity, 1.0,
		 "문서는 min 클리핑이 걸린다고 봤으나 디지털 원판 면적은 7845 라 클리핑 미발동 (허용오차 내)");
	std::printf("\n");

	//--------------------------------------------------------------------------------
	std::printf("====================================================================================\n");
	std::printf(" 결과 : PASS %d / FAIL %d   (NOTE %d 건)\n",
				g_passCount, g_failCount, static_cast<int>(g_noteMessages.size()));
	std::printf("====================================================================================\n");

	if (!g_failMessages.empty())
	{
		std::printf("\n실패 목록\n");
		for (std::size_t i = 0; i < g_failMessages.size(); ++i)
			std::printf("  - %s\n", g_failMessages[i].c_str());
	}

	if (!g_noteMessages.empty())
	{
		std::printf("\n문서 갱신 필요 목록 (docs/03_VERIFICATION.md)\n");
		for (std::size_t i = 0; i < g_noteMessages.size(); ++i)
			std::printf("  - %s\n", g_noteMessages[i].c_str());
	}

	return (g_failCount == 0) ? 0 : 1;
}
