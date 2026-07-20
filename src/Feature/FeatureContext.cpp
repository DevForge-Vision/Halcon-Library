//====================================================================================
// src/Feature/FeatureContext.cpp — 중간산물 지연계산 캐시 구현
//====================================================================================
#include "FeatureContext.h"

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
	, m_hasMaxDistance(false)
	, m_maxDistance(0.0)
	, m_hasHullArea(false)
	, m_hullArea(0)
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
// circularity 의 max
//   "max is the maximum distance from the center to all contour pixels"
//   중심 = 무게중심 (CI-2 확정), 윤곽 = 외곽 윤곽 (CI-1 확정)
//------------------------------------------------------------------------------------
double FeatureContext::GetMaxContourDistance() const
{
	if (m_hasMaxDistance)
		return m_maxDistance;

	m_hasMaxDistance = true;
	m_maxDistance = 0.0;

	if (IsEmpty())
		return m_maxDistance;

	const double centerRow = GetCenterRow();
	const double centerCol = GetCenterColumn();

	const ContourSet& contours = GetContours();

	double maxSquared = 0.0;
	for (size_t i = 0; i < contours.m_contours.size(); ++i)
	{
		const std::vector<Point>& chain = contours.m_contours[i];
		for (size_t k = 0; k < chain.size(); ++k)
		{
			const double dr = static_cast<double>(chain[k].row) - centerRow;
			const double dc = static_cast<double>(chain[k].col) - centerCol;
			const double d2 = dr * dr + dc * dc;
			if (d2 > maxSquared)
				maxSquared = d2;
		}
	}

	m_maxDistance = std::sqrt(maxSquared);
	return m_maxDistance;
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

	std::vector<Point> points;
	GetContours().Flatten(points);		// CV-2: hull 입력은 외곽 contour 픽셀
	if (points.empty())
		return m_hullArea;

	std::vector<Point> hull;
	if (!ComputeConvexHull(points, hull))
		return m_hullArea;

	m_hullArea = RasterizeHullArea(hull);
	return m_hullArea;
}

}}}	// namespace glim::halcon::core
