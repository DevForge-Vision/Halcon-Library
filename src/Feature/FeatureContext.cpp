//====================================================================================
// src/Feature/FeatureContext.cpp — 중간산물 지연계산 캐시 구현
//====================================================================================
#include "FeatureContext.h"

#include <cstddef>
#include <cmath>

namespace glim { namespace halcon { namespace core {

FeatureContext::FeatureContext(const RegionData* region)
	: m_pRegion(region)
	, m_hasAreaCenter(false)
	, m_area(0)
	, m_centerRow(0.0)
	, m_centerColumn(0.0)
	, m_hasContours(false)
	, m_hasContourLength(false)
	, m_contourLength(0.0)
	, m_hasFarthest(false)
	, m_maxDistance(0.0)
	, m_hasFarthestPoint(false)
	, m_farthestPoint()
	, m_hasHull(false)
	, m_hull()
	, m_hasHullArea(false)
	, m_hullArea(0)
	, m_hasDiameter(false)
	, m_hasDiameterPoints(false)
	, m_diameterPoint1()
	, m_diameterPoint2()
	, m_diameter(0.0)
	, m_hasHoleCounts(false)
	, m_holeCounts()
	, m_hasMoments(false)
	, m_moments()
	, m_hasInvarSource(false)
	, m_invarSource()
	, m_hasEllipse(false)
	, m_ellipse()
{
}

bool FeatureContext::IsEmpty() const
{
	return (m_pRegion == 0) || m_pRegion->IsEmpty();
}

//------------------------------------------------------------------------------------
void FeatureContext::EnsureAreaCenter() const
{
	if (m_hasAreaCenter)
		return;

	m_hasAreaCenter = true;
	m_area = 0;
	m_centerRow = 0.0;
	m_centerColumn = 0.0;

	if (IsEmpty())
		return;

	m_area = m_pRegion->GetArea();
	if (!m_pRegion->GetCenter(m_centerRow, m_centerColumn))
	{
		m_centerRow = 0.0;
		m_centerColumn = 0.0;
	}
}

void FeatureContext::EnsureContours() const
{
	if (m_hasContours)
		return;

	m_hasContours = true;
	m_contours.m_contours.clear();

	if (IsEmpty())
		return;

	TraceOuterContours(*m_pRegion, m_contours);
}

//------------------------------------------------------------------------------------
long FeatureContext::GetArea() const
{
	EnsureAreaCenter();
	return m_area;
}

double FeatureContext::GetCenterRow() const
{
	EnsureAreaCenter();
	return m_centerRow;
}

double FeatureContext::GetCenterColumn() const
{
	EnsureAreaCenter();
	return m_centerColumn;
}

const ContourSet& FeatureContext::GetContours() const
{
	EnsureContours();
	return m_contours;
}

double FeatureContext::GetContourLength() const
{
	if (m_hasContourLength)
		return m_contourLength;

	m_hasContourLength = true;
	m_contourLength = ComputeContourLength(GetContours());
	return m_contourLength;
}

//------------------------------------------------------------------------------------
// 무게중심에서 가장 먼 외곽 윤곽 픽셀 — 거리와 점을 **한 번의 순회**로 함께 구한다.
//
//   circularity 의 max        : "maximum distance from the center to all contour pixels"
//   orientation_region 의 기준점: "the point on the contour with maximal distance to the
//                                 center of gravity"
//
// 두 오퍼레이터가 **같은 점**을 봐야 한다. 순회를 두 벌로 두면 언젠가 어긋나므로 하나로 묶었다.
//   중심 = 무게중심 (CI-2 확정), 윤곽 = 외곽 윤곽 (CI-1 확정)
//
// 동률 처리: 엄격 부등호(>)로 갱신하므로 **순회 순서상 먼저 나온 점**이 남는다.
//            원문에 규정이 없다 (docs/02_DEFINITIONS.md §10.5 OR-3).
//------------------------------------------------------------------------------------
void FeatureContext::EnsureFarthestContour() const
{
	if (m_hasFarthest)
		return;

	m_hasFarthest = true;
	m_maxDistance = 0.0;
	m_hasFarthestPoint = false;
	m_farthestPoint = Point();

	if (IsEmpty())
		return;

	const double centerRow = GetCenterRow();
	const double centerCol = GetCenterColumn();

	const ContourSet& contours = GetContours();

	// -1 에서 시작한다. 0 에서 시작하면 **단일 픽셀 리전**(거리 0)에서 점이 잡히지 않는다.
	// 최대 거리값 자체는 아래에서 음수를 0 으로 접으므로 기존 동작과 완전히 같다.
	double maxSquared = -1.0;

	for (std::size_t i = 0; i < contours.m_contours.size(); ++i)
	{
		const std::vector<Point>& chain = contours.m_contours[i];
		for (std::size_t k = 0; k < chain.size(); ++k)
		{
			const double dr = static_cast<double>(chain[k].row) - centerRow;
			const double dc = static_cast<double>(chain[k].col) - centerCol;
			const double d2 = dr * dr + dc * dc;
			if (d2 > maxSquared)
			{
				maxSquared = d2;
				m_farthestPoint = chain[k];
				m_hasFarthestPoint = true;
			}
		}
	}

	m_maxDistance = (maxSquared > 0.0) ? std::sqrt(maxSquared) : 0.0;
}

double FeatureContext::GetMaxContourDistance() const
{
	EnsureFarthestContour();
	return m_maxDistance;
}

bool FeatureContext::GetFarthestContourPoint(Point& point) const
{
	EnsureFarthestContour();
	point = m_farthestPoint;
	return m_hasFarthestPoint;
}

//------------------------------------------------------------------------------------
// convex hull 정점 — convexity 의 F_c 와 diameter_region 이 **같은 hull** 을 본다.
//
// 3차 배치 전에는 hull 을 GetConvexHullArea() 안에서 만들고 버렸다. diameter_region 이
// 같은 hull 을 필요로 하는데, 각자 계산하게 두면 "면적을 잰 hull" 과 "지름을 잰 hull" 이
// 어긋날 여지가 생긴다. GetMaxContourDistance / GetFarthestContourPoint 를 한 순회로
// 묶은 것과 같은 판단이다 (01_ARCHITECTURE.md §9, 2026-07-28).
//
// 계산 입력(외곽 contour 픽셀)과 알고리즘이 완전히 같으므로 **convexity 값은 변하지 않는다.**
//------------------------------------------------------------------------------------
void FeatureContext::EnsureConvexHull() const
{
	if (m_hasHull)
		return;

	m_hasHull = true;
	m_hull.clear();

	if (IsEmpty())
		return;

	std::vector<Point> points;
	GetContours().Flatten(points);		// CV-2: hull 입력은 외곽 contour 픽셀
	if (points.empty())
		return;

	if (!ComputeConvexHull(points, m_hull))
		m_hull.clear();
}

const std::vector<Point>& FeatureContext::GetConvexHullVertices() const
{
	EnsureConvexHull();
	return m_hull;
}

//------------------------------------------------------------------------------------
// convexity 의 F_c — hull 을 래스터화한 픽셀 개수 (CV-1 확정)
//------------------------------------------------------------------------------------
long FeatureContext::GetConvexHullArea() const
{
	if (m_hasHullArea)
		return m_hullArea;

	m_hasHullArea = true;
	m_hullArea = 0;

	if (IsEmpty())
		return m_hullArea;

	const std::vector<Point>& hull = GetConvexHullVertices();
	if (hull.empty())
		return m_hullArea;

	m_hullArea = RasterizeHullArea(hull);
	return m_hullArea;
}

//------------------------------------------------------------------------------------
// diameter_region — hull 위의 최대 거리 점쌍 (docs/02_DEFINITIONS.md §14)
//------------------------------------------------------------------------------------
void FeatureContext::EnsureDiameter() const
{
	if (m_hasDiameter)
		return;

	m_hasDiameter = true;
	m_hasDiameterPoints = false;
	m_diameterPoint1 = Point();
	m_diameterPoint2 = Point();
	m_diameter = 0.0;

	if (IsEmpty())
		return;

	const std::vector<Point>& hull = GetConvexHullVertices();
	if (hull.empty())
		return;

	m_hasDiameterPoints =
		ComputeHullDiameter(hull, m_diameterPoint1, m_diameterPoint2, m_diameter);

	if (!m_hasDiameterPoints)
	{
		m_diameterPoint1 = Point();
		m_diameterPoint2 = Point();
		m_diameter = 0.0;
	}
}

bool FeatureContext::GetDiameter(Point& point1, Point& point2, double& diameter) const
{
	EnsureDiameter();
	point1 = m_diameterPoint1;
	point2 = m_diameterPoint2;
	diameter = m_diameter;
	return m_hasDiameterPoints;
}

//------------------------------------------------------------------------------------
// 구멍 검출 — area_holes / connect_and_holes / euler_number 세 종의 공유 지점.
//
// ★ 리전당 **1회만** 계산된다. 그래서 EulerNumber = NumConnected - NumHoles 가
//   구조적으로 항상 성립한다 (docs/03_VERIFICATION.md T-18 I-8).
//------------------------------------------------------------------------------------
const HoleCounts& FeatureContext::GetHoleCounts() const
{
	if (m_hasHoleCounts)
		return m_holeCounts;

	m_hasHoleCounts = true;
	m_holeCounts = HoleCounts();		// 전 필드 0

	if (IsEmpty())
		return m_holeCounts;

	if (!ComputeHoleCounts(*m_pRegion, m_holeCounts))
		m_holeCounts = HoleCounts();

	return m_holeCounts;
}

//------------------------------------------------------------------------------------
// 2차 모멘트 계열 (docs/02_DEFINITIONS.md §6~§10)
//
// ★ 여기가 2차 배치 5종의 공유 지점이다.
//   moments_region_2nd / _invar / elliptic_axis / eccentricity / orientation_region
//   다섯 종이 이 캐시 하나를 본다. 리전당 모멘트 누적은 **1회**다.
//------------------------------------------------------------------------------------
const CentralMoments2nd& FeatureContext::GetMoments2nd() const
{
	if (m_hasMoments)
		return m_moments;

	m_hasMoments = true;
	m_moments = CentralMoments2nd();		// 전 필드 0

	if (IsEmpty())
		return m_moments;

	// 원점 = 무게중심 (원문 "center of gravity")
	if (!ComputeCentralMoments2nd(*m_pRegion, GetCenterRow(), GetCenterColumn(), m_moments))
		m_moments = CentralMoments2nd();

	return m_moments;
}

//------------------------------------------------------------------------------------
// moments_region_2nd_invar 의 분자.
//
// MI-1 (docs/02_DEFINITIONS.md §7.5)
//   원문이 이 페이지에서만 "center of gravity" 가 아니라 그냥 "the center" 라고 쓴다.
//   기본 해석은 무게중심이며, 그 경우 §6 의 결과와 **완전히 같다**(다시 계산하지 않는다).
//------------------------------------------------------------------------------------
const CentralMoments2nd& FeatureContext::GetMoments2ndInvarSource() const
{
	if (m_hasInvarSource)
		return m_invarSource;

	m_hasInvarSource = true;
	m_invarSource = CentralMoments2nd();

	if (IsEmpty())
		return m_invarSource;

#if GLIM_HALCON_INVAR_ORIGIN_BBOX
	// 대안 해석: 원점 = 바운딩박스 중심. 별도 누적이 필요하다.
	int minRow = 0;
	int maxRow = 0;
	int minCol = 0;
	int maxCol = 0;
	if (!m_pRegion->GetBoundingBox(minRow, maxRow, minCol, maxCol))
		return m_invarSource;

	const double originRow = 0.5 * (static_cast<double>(minRow) + static_cast<double>(maxRow));
	const double originCol = 0.5 * (static_cast<double>(minCol) + static_cast<double>(maxCol));

	if (!ComputeCentralMoments2nd(*m_pRegion, originRow, originCol, m_invarSource))
		m_invarSource = CentralMoments2nd();
#else
	// 기본 해석: 무게중심 원점 → §6 결과를 그대로 쓴다.
	m_invarSource = GetMoments2nd();
#endif

	return m_invarSource;
}

//------------------------------------------------------------------------------------
// elliptic_axis — 위 모멘트를 **F 로** 정규화해 만든다 (EA-1 확정. F^2 아님).
//------------------------------------------------------------------------------------
const EllipseParams& FeatureContext::GetEllipse() const
{
	if (m_hasEllipse)
		return m_ellipse;

	m_hasEllipse = true;
	m_ellipse = EllipseParams();			// 전 필드 0

	if (IsEmpty())
		return m_ellipse;

	if (!ComputeEllipseParams(GetMoments2nd(), static_cast<double>(GetArea()), m_ellipse))
		m_ellipse = EllipseParams();

	return m_ellipse;
}

}}}	// namespace glim::halcon::core
