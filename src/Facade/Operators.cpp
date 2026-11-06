//====================================================================================
// src/Facade/Operators.cpp — HALCON 오퍼레이터 5종
//
// 근거: docs/02_DEFINITIONS.md
//   §1 area_center / §2 contlength / §3 circularity / §4 compactness / §5 convexity
//
// 공통 규약
//   - 출력은 진입 즉시 0 으로 초기화한다.
//   - 모든 본문은 try-catch 로 감싼다. 예외는 공개 경계 밖으로 나가지 않는다.
//   - 빈 리전 / 0 나눗셈은 전부 0 (docs/02_DEFINITIONS.md §0.4).
//====================================================================================
#include "GlimHalcon.h"
#include "ImageInternal.h"

#include "Region.h"
#include "FeatureContext.h"
#include "Moments.h"

#include <cmath>

namespace glim { namespace halcon {

namespace {

const double kPi = 3.14159265358979323846;

//------------------------------------------------------------------------------------
// 값 계산 본체 — FeatureContext 하나를 받아 값 하나를 만든다.
//
// 개별 오퍼레이터와 ComputeFeatures(배치) 가 **똑같이 이 함수들을 호출한다.**
// 계산식을 두 벌로 복제하면 언젠가 반드시 어긋나므로, 여기 한 곳에만 둔다.
// 각 수식의 근거와 클리핑 방향은 아래 각 오퍼레이터의 주석 블록에 있다.
//------------------------------------------------------------------------------------
void ComputeAreaCenterValue(const core::FeatureContext& context, long& area, double& row, double& column)
{
	area = context.GetArea();
	row = context.GetCenterRow();
	column = context.GetCenterColumn();
}

double ComputeContLengthValue(const core::FeatureContext& context)
{
	double value = context.GetContourLength();

	if (!(value >= 0.0))		// NaN 방어. Assertion: ContLength >= 0
		value = 0.0;

	return value;
}

double ComputeCircularityValue(const core::FeatureContext& context)
{
	const double area = static_cast<double>(context.GetArea());
	if (area <= 0.0)
		return 0.0;

	const double maxDistance = context.GetMaxContourDistance();
	if (maxDistance <= 0.0)
		return 0.0;		// 단일 픽셀 등 축퇴 → 0 나눗셈 방어

	const double denominator = maxDistance * maxDistance * kPi;
	if (denominator <= 0.0)
		return 0.0;

	double value = area / denominator;

	if (!(value == value))		// NaN 방어
		value = 0.0;
	if (value > 1.0)
		value = 1.0;			// min(1, C')
	if (value < 0.0)
		value = 0.0;

	return value;
}

double ComputeCompactnessValue(const core::FeatureContext& context)
{
	const double area = static_cast<double>(context.GetArea());
	if (area <= 0.0)
		return 0.0;		// F == 0 → 0

	const double length = context.GetContourLength();

	const double denominator = 4.0 * area * kPi;
	if (denominator <= 0.0)
		return 0.0;

	double value = (length * length) / denominator;

	if (!(value == value))		// NaN 방어
		value = 0.0;
	if (value < 1.0)
		value = 1.0;			// max(1, C')

	return value;
}

double ComputeConvexityValue(const core::FeatureContext& context)
{
	const double areaOriginal = static_cast<double>(context.GetArea());
	if (areaOriginal <= 0.0)
		return 0.0;

	const double areaHull = static_cast<double>(context.GetConvexHullArea());
	if (areaHull <= 0.0)
		return 0.0;		// 0 나눗셈 방어

	double value = areaOriginal / areaHull;

	if (!(value == value))		// NaN 방어
		value = 0.0;
	if (value < 0.0)
		value = 0.0;

	// 주의: 정의서 §5 에는 클리핑이 없다. 1.0 으로 강제하지 않는다 (아래 Convexity 주석 참조).
	return value;
}

//------------------------------------------------------------------------------------
// 2차 모멘트 계열 값 계산 본체 (docs/02_DEFINITIONS.md §6~§10)
//
// 위 5종과 같은 규칙이다. 개별 오퍼레이터와 배치 API 가 **똑같이 이 함수들을 호출한다.**
//------------------------------------------------------------------------------------
void ComputeMomentsRegion2ndValue(const core::FeatureContext& context,
								  double& m11, double& m20, double& m02,
								  double& ia, double& ib)
{
	const core::CentralMoments2nd& moments = context.GetMoments2nd();

	// 정규화하지 않는다 (함정 #4/#6). 여기서 면적으로 나누면 계열 전체가 오염된다.
	m11 = moments.m11;
	m20 = moments.m20;
	m02 = moments.m02;
	ia = moments.ia;
	ib = moments.ib;
}

void ComputeMomentsRegion2ndInvarValue(const core::FeatureContext& context,
									   double& m11, double& m20, double& m02)
{
	m11 = 0.0;
	m20 = 0.0;
	m02 = 0.0;

	const double area = static_cast<double>(context.GetArea());
	if (area <= 0.0)
		return;					// 0 나눗셈 방어 (빈 리전)

	// 정규화 지수는 **고정 2**다. i, j 와 무관하다 (원문 수식 (1/F^2)·SUM).
	const double areaSquared = area * area;
	if (!(areaSquared > 0.0))
		return;					// 오버플로/NaN 방어

	const core::CentralMoments2nd& moments = context.GetMoments2ndInvarSource();

	m11 = moments.m11 / areaSquared;
	m20 = moments.m20 / areaSquared;
	m02 = moments.m02 / areaSquared;
}

void ComputeEllipticAxisValue(const core::FeatureContext& context,
							  double& ra, double& rb, double& phi)
{
	const core::EllipseParams& ellipse = context.GetEllipse();

	ra = ellipse.ra;
	rb = ellipse.rb;
	phi = ellipse.phi;
}

void ComputeEccentricityValue(const core::FeatureContext& context,
							  double& anisometry, double& bulkiness, double& structureFactor)
{
	anisometry = 0.0;
	bulkiness = 0.0;
	structureFactor = 0.0;

	const double area = static_cast<double>(context.GetArea());
	if (area <= 0.0)
		return;					// 빈 리전 규약 — StructureFactor 도 0 이다 (-1 이 아니다)

	const core::EllipseParams& ellipse = context.GetEllipse();

	// 함정 #12: Rb == 0 이면 Anisometry 는 **0** 이다.
	// 문서상 Assertion 은 >= 1.0 이지만 원문 Attention 이 0 이라고 직접 명시한다.
	// 여기서 1.0 을 넣으면 1픽셀 두께 스크래치가 "완전한 원" 으로 둔갑한다.
	if (ellipse.rb > 0.0)
		anisometry = ellipse.ra / ellipse.rb;

	// Rb == 0 이면 자연히 0 이다. 별도 분기가 필요 없다.
	bulkiness = kPi * ellipse.ra * ellipse.rb / area;

	structureFactor = anisometry * bulkiness - 1.0;

#if GLIM_HALCON_STRUCTURE_FACTOR_ZERO_ON_RB0
	// EC-1 해석 (b): "Rb 나눗셈에 의존하는 출력" 으로 보고 함께 0 으로 덮는다.
	if (!(ellipse.rb > 0.0))
		structureFactor = 0.0;
#endif

	if (!(anisometry == anisometry))		// NaN 방어
		anisometry = 0.0;
	if (!(bulkiness == bulkiness))
		bulkiness = 0.0;
	if (!(structureFactor == structureFactor))
		structureFactor = 0.0;
}

double ComputeOrientationRegionValue(const core::FeatureContext& context)
{
	const core::EllipseParams& ellipse = context.GetEllipse();

	double phi = ellipse.phi;
	if (!(phi == phi))						// NaN 방어
		return 0.0;

	core::Point farthest;
	if (!context.GetFarthestContourPoint(farthest))
		return phi;							// 윤곽이 없다 (이론상 발생하지 않는다)

	const double dr = static_cast<double>(farthest.row) - context.GetCenterRow();
	const double dc = static_cast<double>(farthest.col) - context.GetCenterColumn();

	// OR-1: "회전 좌표계에서의 column 좌표" = 주축 방향 성분.
	//   주축 단위벡터는 (col, row) 기준 (cos Phi, -sin Phi) 이므로 (§8.3)
	//       proj = dc·cos(Phi) - dr·sin(Phi)
	//   무게중심 자신의 회전 column 좌표는 0 이므로 "중심보다 작다" 는 곧 proj < 0 이다.
	const double projSign = static_cast<double>(GLIM_HALCON_ORIENTATION_PROJ_SIGN);
	const double proj = dc * std::cos(phi) + projSign * dr * std::sin(phi);

	if (proj < 0.0)
		phi += kPi;

#if GLIM_HALCON_ORIENTATION_WRAP
	// OR-2: Assertion 은 -pi <= Phi < pi 다. Phi0 in (-pi/2, pi/2] 에 pi 를 더하면
	//       (pi/2, 3pi/2] 가 되므로 한 번의 보정으로 충분하다. 반복문을 쓰지 않는다.
	if (phi >= kPi)
		phi -= 2.0 * kPi;
	if (phi < -kPi)
		phi += 2.0 * kPi;
#endif

	// -0.0 을 +0.0 으로 정규화한다. (x + 0.0 은 x == -0.0 일 때만 값이 바뀐다)
	return phi + 0.0;
}

//------------------------------------------------------------------------------------
// 구멍 검출 계열 값 계산 본체 (docs/02_DEFINITIONS.md §11~§14)
//
// ★ 세 오퍼레이터(AreaHoles / ConnectAndHoles / EulerNumber)가 **같은 캐시**를 본다.
//   FeatureContext::GetHoleCounts() 는 리전당 1회만 계산되므로
//   EulerNumber == NumConnected - NumHoles 가 구조적으로 항상 성립한다.
//------------------------------------------------------------------------------------
long ComputeAreaHolesValue(const core::FeatureContext& context)
{
	const core::HoleCounts& counts = context.GetHoleCounts();

	if (counts.areaHoles <= 0)
		return 0;

	return static_cast<long>(counts.areaHoles);
}

void ComputeConnectAndHolesValue(const core::FeatureContext& context,
								 long& numConnected, long& numHoles)
{
	const core::HoleCounts& counts = context.GetHoleCounts();

	numConnected = (counts.numConnected > 0) ? static_cast<long>(counts.numConnected) : 0;
	numHoles = (counts.numHoles > 0) ? static_cast<long>(counts.numHoles) : 0;
}

long ComputeEulerNumberValue(const core::FeatureContext& context)
{
	// 원문 정의가 "두 값의 차" 이므로 **반드시** 위 헬퍼를 경유한다.
	// 여기서 GetHoleCounts() 를 직접 빼면 언젠가 ConnectAndHoles 와 어긋난다.
	long numConnected = 0;
	long numHoles = 0;
	ComputeConnectAndHolesValue(context, numConnected, numHoles);

	return numConnected - numHoles;		// 음수 가능
}

void ComputeDiameterRegionValue(const core::FeatureContext& context,
								long& row1, long& column1, long& row2, long& column2,
								double& diameter)
{
	row1 = 0;
	column1 = 0;
	row2 = 0;
	column2 = 0;
	diameter = 0.0;

	core::Point point1;
	core::Point point2;
	double distance = 0.0;

	if (!context.GetDiameter(point1, point2, distance))
		return;			// 빈 리전 / 윤곽 없음 — 원문이 명시한 "전부 0" 동작

	row1 = static_cast<long>(point1.row);
	column1 = static_cast<long>(point1.col);
	row2 = static_cast<long>(point2.row);
	column2 = static_cast<long>(point2.col);

	if (!(distance >= 0.0))		// NaN 방어
		distance = 0.0;

	diameter = distance;
}

//------------------------------------------------------------------------------------
// RegionMoments 초기화 — 진입/실패 경로가 같은 함수를 쓰게 해 필드 누락을 막는다.
//------------------------------------------------------------------------------------
void ClearRegionMoments(RegionMoments& moments)
{
	moments.m11 = 0.0;
	moments.m20 = 0.0;
	moments.m02 = 0.0;
	moments.ia = 0.0;
	moments.ib = 0.0;
	moments.m11Invar = 0.0;
	moments.m20Invar = 0.0;
	moments.m02Invar = 0.0;
	moments.ra = 0.0;
	moments.rb = 0.0;
	moments.phi = 0.0;
	moments.anisometry = 0.0;
	moments.bulkiness = 0.0;
	moments.structureFactor = 0.0;
	moments.orientation = 0.0;
}

void ClearRegionFeatures(RegionFeatures& features)
{
	features.area = 0;
	features.row = 0.0;
	features.column = 0.0;
	features.contLength = 0.0;
	features.circularity = 0.0;
	features.compactness = 0.0;
	features.convexity = 0.0;
}

void ClearRegionTopology(RegionTopology& topology)
{
	topology.areaHoles = 0;
	topology.numConnected = 0;
	topology.numHoles = 0;
	topology.eulerNumber = 0;
	topology.diameterRow1 = 0;
	topology.diameterColumn1 = 0;
	topology.diameterRow2 = 0;
	topology.diameterColumn2 = 0;
	topology.diameter = 0.0;
}

//------------------------------------------------------------------------------------
// 컨텍스트 하나에서 7값 / 15값 / 9값을 채운다. 배치 API 네 개가 이것만 호출한다.
//------------------------------------------------------------------------------------
void FillFeatures(const core::FeatureContext& context, RegionFeatures& features)
{
	ComputeAreaCenterValue(context, features.area, features.row, features.column);
	features.contLength = ComputeContLengthValue(context);
	features.circularity = ComputeCircularityValue(context);
	features.compactness = ComputeCompactnessValue(context);
	features.convexity = ComputeConvexityValue(context);
}

void FillMoments(const core::FeatureContext& context, RegionMoments& moments)
{
	ComputeMomentsRegion2ndValue(context,
								 moments.m11, moments.m20, moments.m02,
								 moments.ia, moments.ib);

	ComputeMomentsRegion2ndInvarValue(context,
									  moments.m11Invar, moments.m20Invar, moments.m02Invar);

	ComputeEllipticAxisValue(context, moments.ra, moments.rb, moments.phi);

	ComputeEccentricityValue(context,
							 moments.anisometry, moments.bulkiness, moments.structureFactor);

	moments.orientation = ComputeOrientationRegionValue(context);
}

void FillTopology(const core::FeatureContext& context, RegionTopology& topology)
{
	topology.areaHoles = ComputeAreaHolesValue(context);

	ComputeConnectAndHolesValue(context, topology.numConnected, topology.numHoles);

	topology.eulerNumber = ComputeEulerNumberValue(context);

	ComputeDiameterRegionValue(context,
							   topology.diameterRow1, topology.diameterColumn1,
							   topology.diameterRow2, topology.diameterColumn2,
							   topology.diameter);
}

}	// anonymous namespace

//------------------------------------------------------------------------------------
// HALCON: area_center(Regions : : : Area, Row, Column)
//
//   Area = F                  (리전에 속한 픽셀 개수)
//   Row  = (1/F) * SUM(r_i)
//   Col  = (1/F) * SUM(c_i)
//
//   런렝스 누적으로 O(런 수). 열 좌표 합은 등차수열 합으로 상수시간 처리한다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL AreaCenter(const Region& region, long& area, double& row, double& column)
{
	area = 0;
	row = 0.0;
	column = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약

		core::FeatureContext context(data);
		ComputeAreaCenterValue(context, area, row, column);
	}
	catch (...)
	{
		area = 0;
		row = 0.0;
		column = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: contlength(Regions : : : ContLength)     Assertion: ContLength >= 0
//
//   ContLength = (축평행 스텝 수 * 1) + (대각 스텝 수 * sqrt(2))
//
//   - 구멍(hole)의 윤곽은 포함하지 않는다 (원문 Attention).
//   - 픽셀 중심 체인(Freeman chain) 모델. 단일 픽셀 = 0.0 (C-1 확정).
//   - 연결성분이 여러 개면 각 성분의 외곽 윤곽 길이를 합산한다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ContLength(const Region& region, double& contLength)
{
	contLength = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		contLength = ComputeContLengthValue(context);
	}
	catch (...)
	{
		contLength = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: circularity(Regions : : : Circularity)   Assertion: 0 <= C <= 1.0
//
//                F
//   C' = -------------------          max = 중심에서 모든 contour 픽셀까지의 최대 거리
//         max^2  *  pi
//
//   C  = min(1, C')                   ← 위로 1.0 에서 클리핑 (compactness 와 방향 반대!)
//
//   - 중심은 무게중심 (CI-2 확정), contour 는 외곽만 (CI-1 확정).
//   - max == 0 (단일 픽셀) 이면 0 나눗셈 → 0 을 반환한다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL Circularity(const Region& region, double& circularity)
{
	circularity = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		circularity = ComputeCircularityValue(context);
	}
	catch (...)
	{
		circularity = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: compactness(Regions : : : Compactness)   Assertion: C >= 1.0 || C == 0
//
//              L^2
//   C' = -------------------          L = contlength 결과
//         4  *  F  *  pi
//
//   C  = max(1, C')                   ← 아래로 1.0 에서 클리핑 (circularity 와 반대!)
//
//   - 원문 Description 에는 "clipped to 1.0" 라는 circularity 문장이 복붙돼 있으나
//     실제 수식과 Assertion 은 max 다. 수식/Assertion 을 따른다 (정의서 §4.3).
//   - 빈 리전은 0.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL Compactness(const Region& region, double& compactness)
{
	compactness = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		compactness = ComputeCompactnessValue(context);
	}
	catch (...)
	{
		compactness = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: convexity(Regions : : : Convexity)       Assertion: C <= 1
//
//         F_o        (원본 리전의 면적 = 픽셀 개수)
//   C = -------
//         F_c        (convex hull 의 면적 = hull 을 래스터화한 픽셀 개수, CV-1 확정)
//
//   - 분자가 원본이다. 뒤집으면 항상 >= 1 이 되어 Assertion 을 위반한다.
//   - F_c 에 폴리곤 기하 면적(cv::contourArea)을 쓰면 정사각형에서 1.0203 이 나온다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL Convexity(const Region& region, double& convexity)
{
	convexity = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		// 주의: 정의서 §5 에는 클리핑이 없다. Assertion(C <= 1) 은 서술일 뿐이므로
		// 여기서 1.0 으로 강제하지 않는다. 값이 1 을 넘으면 F_c 계산이 틀린 것이며
		// (예: 폴리곤 기하 면적 사용), 그 오류를 감추지 않기 위해 그대로 노출한다.
		core::FeatureContext context(data);
		convexity = ComputeConvexityValue(context);
	}
	catch (...)
	{
		convexity = 0.0;
	}
}

//------------------------------------------------------------------------------------
// 배치 계산 — 개별 오퍼레이터 5종을 한 번에 뽑는다.
//
// 존재 이유는 오직 하나, **중간산물 중복 계산 제거**다.
//   개별 호출 : ContLength / Circularity / Compactness / Convexity 가 각자
//               FeatureContext 를 새로 만들므로 윤곽 추적이 4회, hull 이 1회 일어난다.
//   배치 호출 : FeatureContext 를 **하나만** 만들어 5종 헬퍼에 넘긴다.
//               윤곽 추적 1회 + hull 1회로 끝난다.
//
// 값은 개별 오퍼레이터와 **완전히 동일하다.** 계산식이 같은 헬퍼 하나로 공유되기 때문이며,
// 두 경로가 어긋날 여지 자체를 만들지 않았다.
//
// 계약
//   - 진입 즉시 모든 필드를 0 으로 초기화한다.
//   - 예외를 밖으로 던지지 않는다. 실패 시 전 필드가 0 이다 (빈 리전과 구분되지 않는다).
//   - 스레드 안전: 전역/정적 가변 상태가 없다. 서로 다른 스레드가 같은 Region 을
//     동시에 읽는 것은 안전하다 (Region 내부 데이터는 불변).
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ComputeFeatures(const Region& region, RegionFeatures& features)
{
	ClearRegionFeatures(features);

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약 — 전 필드 0

		// ★ 이 라인이 배치 API 의 전부다. 컨텍스트 하나를 5종이 공유한다.
		core::FeatureContext context(data);

		FillFeatures(context, features);
	}
	catch (...)
	{
		ClearRegionFeatures(features);
	}
}

//====================================================================================
// 2차 배치 — 2차 모멘트 계열 5종
//
// 근거: docs/02_DEFINITIONS.md §6~§10 / docs/03_VERIFICATION.md §3B
//
// 다섯 종 전부 FeatureContext 의 **같은 모멘트 캐시**를 본다.
// 개별로 호출하면 리전당 컨텍스트가 새로 생겨 모멘트 누적이 반복되므로,
// 둘 이상이 필요하면 ComputeMoments / ComputeFeaturesAndMoments 를 쓴다.
//====================================================================================

//------------------------------------------------------------------------------------
// HALCON: moments_region_2nd(Regions : : : M11, M20, M02, Ia, Ib)
//
//   M_ij = SUM (r0 - r)^i · (c0 - c)^j        (r0, c0 = 무게중심)
//   h    = (M20 + M02)/2
//   Ia   = h + sqrt(h^2 - M20·M02 + M11^2)    ← 구현은 항등 변형을 쓴다 (정의서 §6.4 (3))
//   Ib   = h - sqrt(...)
//
//   ⚠ **정규화 없음.** 면적으로 나누면 _invar 와 elliptic_axis 가 전부 오염된다 (함정 #4/#6).
//   ⚠ 부호 규약 (r0 - r) 은 2차에서 (r - r0) 과 값이 같다. 3차 홀수항에서만 갈린다 (함정 #5).
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL MomentsRegion2nd(const Region& region,
					  double& m11, double& m20, double& m02, double& ia, double& ib)
{
	m11 = 0.0;
	m20 = 0.0;
	m02 = 0.0;
	ia = 0.0;
	ib = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeMomentsRegion2ndValue(context, m11, m20, m02, ia, ib);
	}
	catch (...)
	{
		m11 = 0.0;
		m20 = 0.0;
		m02 = 0.0;
		ia = 0.0;
		ib = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: moments_region_2nd_invar(Regions : : : M11, M20, M02)
//
//   M_ij = (1 / F^2) · SUM (r0 - r)^i · (c0 - c)^j
//
//   - 정규화 지수는 **고정 2**다. i, j 와 무관하다.
//   - moments_region_2nd 와의 차이는 오직 이 F^2 나눗셈뿐이다.
//     둘을 같은 도형에 나란히 걸면 정규화 오류가 값으로 드러난다 (검증 T-8).
//   - 빈 리전이면 0 나눗셈을 피해 전부 0.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL MomentsRegion2ndInvar(const Region& region, double& m11, double& m20, double& m02)
{
	m11 = 0.0;
	m20 = 0.0;
	m02 = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeMomentsRegion2ndInvarValue(context, m11, m20, m02);
	}
	catch (...)
	{
		m11 = 0.0;
		m20 = 0.0;
		m02 = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: elliptic_axis(Regions : : : Ra, Rb, Phi)
//   Assertion: Ra >= 0 / 0 <= Rb <= Ra / -pi/2 < Phi <= pi/2
//
//   M20n = M20/F, M02n = M02/F, M11n = M11/F      ← **F** 다. F^2 이 아니다 (EA-1)
//   D    = sqrt((M20n - M02n)^2 + 4·M11n^2)
//   Ra   = sqrt(8·(M20n + M02n + D)) / 2
//   Rb   = sqrt(8·(M20n + M02n - D)) / 2
//   Phi  = -0.5 · atan2(2·M11n, M02n - M20n)      ← 함정 #11. 통념과 부호가 반대다
//
//   ⚠ Phi 는 정규화 인자에 둔감하다(분자·분모가 같이 스케일된다).
//     정규화 오류는 Ra/Rb 로만 잡힌다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL EllipticAxis(const Region& region, double& ra, double& rb, double& phi)
{
	ra = 0.0;
	rb = 0.0;
	phi = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeEllipticAxisValue(context, ra, rb, phi);
	}
	catch (...)
	{
		ra = 0.0;
		rb = 0.0;
		phi = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: eccentricity(Regions : : : Anisometry, Bulkiness, StructureFactor)
//
//   Anisometry      = Ra / Rb                     (Assertion 상 >= 1.0)
//   Bulkiness       = pi · Ra · Rb / A
//   StructureFactor = Anisometry · Bulkiness - 1
//
//   ⚠ 함정 #12 — Rb == 0 이면 Anisometry 는 **0** 이다.
//     원문 Attention 이 명시한 동작이며, 문서상 Assertion(>= 1.0)과 모순된다.
//     Assertion 쪽을 따라 1.0 을 넣으면 1픽셀 두께 스크래치가 원으로 판정된다.
//     여기서는 **실제 동작(0)** 을 재현한다 (정의서 §0.4 / §9.3).
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL Eccentricity(const Region& region,
				  double& anisometry, double& bulkiness, double& structureFactor)
{
	anisometry = 0.0;
	bulkiness = 0.0;
	structureFactor = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeEccentricityValue(context, anisometry, bulkiness, structureFactor);
	}
	catch (...)
	{
		anisometry = 0.0;
		bulkiness = 0.0;
		structureFactor = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: orientation_region(Regions : : : Phi)     Assertion: -pi <= Phi < pi
//
//   1. elliptic_axis 의 Phi 로 시작한다 (축만 정해지고 방향은 미정)
//   2. 무게중심에서 가장 먼 외곽 윤곽 픽셀을 찾는다
//   3. 회전 좌표계에서 그 점의 column 좌표가 중심보다 작으면 pi 를 더한다
//   4. [-pi, pi) 로 접는다 (OR-2)
//
//   ⚠ 대칭 도형은 최원점이 동률이라 결과가 Phi 또는 Phi±pi 중 하나로 갈린다.
//     원문에 규정이 없으므로 어느 쪽도 오답이 아니다 (OR-3).
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL OrientationRegion(const Region& region, double& phi)
{
	phi = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		phi = ComputeOrientationRegionValue(context);
	}
	catch (...)
	{
		phi = 0.0;
	}
}

//------------------------------------------------------------------------------------
// 배치 계산 — 2차 모멘트 계열 5종을 한 번에.
//
// 값은 개별 오퍼레이터와 **완전히 동일**하다. 같은 헬퍼를 호출하기 때문이며,
// 계산식을 두 벌로 복제하지 않았으므로 두 경로가 어긋날 여지가 구조적으로 없다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ComputeMoments(const Region& region, RegionMoments& moments)
{
	ClearRegionMoments(moments);

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약 — 전 필드 0

		core::FeatureContext context(data);
		FillMoments(context, moments);
	}
	catch (...)
	{
		ClearRegionMoments(moments);
	}
}

//------------------------------------------------------------------------------------
// 1차 7값 + 2차 15값을 컨텍스트 **하나**로 계산한다.
//
//   ComputeFeatures + ComputeMoments 를 따로 호출 : 윤곽 추적 2회 + hull 1회 + 모멘트 2회
//   ComputeFeaturesAndMoments                    : 윤곽 추적 1회 + hull 1회 + 모멘트 1회
//
// 값은 어느 경로로 불러도 동일하다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ComputeFeaturesAndMoments(const Region& region,
							   RegionFeatures& features,
							   RegionMoments& moments)
{
	ClearRegionFeatures(features);
	ClearRegionMoments(moments);

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);

		FillFeatures(context, features);
		FillMoments(context, moments);
	}
	catch (...)
	{
		ClearRegionFeatures(features);
		ClearRegionMoments(moments);
	}
}

//====================================================================================
// 3차 배치 — 구멍 검출 계열 4종
//
// 근거: docs/02_DEFINITIONS.md §11~§14 / docs/03_VERIFICATION.md §3C
//
// 앞의 셋은 FeatureContext 의 **같은 구멍 캐시**를 본다. 개별로 호출하면 리전당
// 컨텍스트가 새로 생겨 구멍 검출이 반복되므로, 둘 이상이 필요하면
// ComputeTopology / ComputeFeaturesMomentsAndTopology 를 쓴다.
//
// ⚠ 연결성: 전경 8-연결 <-> 배경 4-연결.
//   디지털 위상에서 둘은 서로 반대여야 한다. 같게 잡으면 NumHoles 와 EulerNumber 가
//   대각으로만 닫힌 도형에서 모순된 값을 낸다 (미결 HC-1, src/Core/Holes.h 스위치).
//====================================================================================

//------------------------------------------------------------------------------------
// HALCON: area_holes(Regions : : : Area)
//
//   Area = |유계 배경 성분의 픽셀 전체|
//
//   - 구멍 = 리전이 둘러싼 배경. 구멍 안의 전경 덩어리(섬)는 전경이므로 세지 않는다 (HC-3).
//   - "오목함" 과 "구멍" 은 다른 개념이다. L자 도형은 Convexity < 1 이지만 Area = 0 이다.
//   - 빈 리전은 0.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL AreaHoles(const Region& region, long& area)
{
	area = 0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약

		core::FeatureContext context(data);
		area = ComputeAreaHolesValue(context);
	}
	catch (...)
	{
		area = 0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: connect_and_holes(Regions : : : NumConnected, NumHoles)
//
//   NumConnected = 전경 연결성분 개수 (8-연결)
//   NumHoles     = 유계 배경 성분 개수 (배경 4-연결)
//
//   - NumConnected 는 core::LabelConnectedRuns 의 결과를 그대로 쓴다.
//     ContLength(성분별 외곽 윤곽 합) · SelectLargestBlob 과 **같은 함수**다.
//     여기만 4-연결로 바꾸면 같은 리전에서 성분 수가 서로 어긋난다 (HC-2).
//   - 빈 리전은 둘 다 0.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ConnectAndHoles(const Region& region, long& numConnected, long& numHoles)
{
	numConnected = 0;
	numHoles = 0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeConnectAndHolesValue(context, numConnected, numHoles);
	}
	catch (...)
	{
		numConnected = 0;
		numHoles = 0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: euler_number(Regions : : : EulerNumber)
//
//   EulerNumber = NumConnected - NumHoles
//
//   ⚠ 원문이 이 값을 connect_and_holes 두 출력의 **차** 로 정의한다.
//     그러므로 구현을 두 벌로 두면 원문 정의 자체를 어기게 된다.
//     여기서는 ComputeConnectAndHolesValue 를 그대로 호출해 빼기만 한다.
//   - 구멍이 성분보다 많으면 음수다 (구멍 2개 · 성분 1개 -> -1).
//   - 빈 리전은 0 - 0 = 0.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL EulerNumber(const Region& region, long& eulerNumber)
{
	eulerNumber = 0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		eulerNumber = ComputeEulerNumberValue(context);
	}
	catch (...)
	{
		eulerNumber = 0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: diameter_region(Regions : : : Row1, Column1, Row2, Column2, Diameter)
//
//   Diameter = max over 경계점 쌍 (p, q) of |p - q|
//
//   - 최대 거리 쌍은 반드시 convex hull 의 정점 쌍이다 -> hull 위에서만 찾는다.
//     hull 은 Convexity 가 쓰던 것을 그대로 공유한다 (재계산 없음).
//   - 픽셀은 무한소 점이다 (함정 #7 / DR-1). 단일 픽셀이면 Diameter = 0.
//   - 동률일 때의 규정이 원문에 없다 (DR-2). 사전순 최소 쌍을 돌려준다.
//   - 빈 리전은 네 좌표와 Diameter 가 전부 0 이다. **원문이 명시한 동작**이며
//     "혼동을 줄 수 있다" 는 경고까지 붙어 있다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL DiameterRegion(const Region& region,
					long& row1, long& column1, long& row2, long& column2,
					double& diameter)
{
	row1 = 0;
	column1 = 0;
	row2 = 0;
	column2 = 0;
	diameter = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		ComputeDiameterRegionValue(context, row1, column1, row2, column2, diameter);
	}
	catch (...)
	{
		row1 = 0;
		column1 = 0;
		row2 = 0;
		column2 = 0;
		diameter = 0.0;
	}
}

//------------------------------------------------------------------------------------
// 배치 계산 — 구멍 검출 계열 4종을 한 번에.
//
// 값은 개별 오퍼레이터와 **완전히 동일**하다. 같은 헬퍼를 호출하기 때문이며,
// 계산식을 두 벌로 복제하지 않았으므로 두 경로가 어긋날 여지가 구조적으로 없다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ComputeTopology(const Region& region, RegionTopology& topology)
{
	ClearRegionTopology(topology);

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약 — 전 필드 0

		core::FeatureContext context(data);
		FillTopology(context, topology);
	}
	catch (...)
	{
		ClearRegionTopology(topology);
	}
}

//------------------------------------------------------------------------------------
// 1차 7값 + 2차 15값 + 3차 9값을 컨텍스트 **하나**로 계산한다.
//
//   따로 호출     : 윤곽 추적 2회 + hull 2회 + 모멘트 2회 + 구멍 검출 1회
//   이 함수 1회   : 윤곽 추적 1회 + hull 1회 + 모멘트 1회 + 구멍 검출 1회
//
// 값은 어느 경로로 불러도 동일하다.
//------------------------------------------------------------------------------------
void GLIM_HALCON_CALL ComputeFeaturesMomentsAndTopology(const Region& region,
									   RegionFeatures& features,
									   RegionMoments& moments,
									   RegionTopology& topology)
{
	ClearRegionFeatures(features);
	ClearRegionMoments(moments);
	ClearRegionTopology(topology);

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);

		FillFeatures(context, features);
		FillMoments(context, moments);
		FillTopology(context, topology);
	}
	catch (...)
	{
		ClearRegionFeatures(features);
		ClearRegionMoments(moments);
		ClearRegionTopology(topology);
	}
}

namespace image {
// Internal checked entry: reuse the same formulas without the legacy void API's
// exception-to-zero conversion. Exceptions are caught at the new DLL boundary.
bool ComputeChecked(const core::RegionData& data, BlobFeatures& output, int expectedComponents)
{
	if (data.IsEmpty()) return false;
	core::FeatureContext context(&data);
	if (context.GetContours().IsEmpty() || context.GetConvexHullVertices().empty() ||
		context.GetConvexHullArea() <= 0) return false;
	FillFeatures(context, output.shape);
	FillMoments(context, output.moments);
	FillTopology(context, output.topology);
	return output.shape.area == data.GetArea() && output.shape.area > 0 &&
		output.topology.numConnected == expectedComponents;
}
}

}}	// namespace glim::halcon
