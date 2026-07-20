//====================================================================================
// src/Feature/FeatureContext.h — 리전당 중간산물 지연계산 캐시
//
// 근거: docs/01_ARCHITECTURE.md §5
//   area/center · contour · convexHull 은 여러 특징값이 공유한다.
//   **처음 요청될 때 1회만** 계산하고 이후 재사용한다.
//   전역 캐시를 두지 않는다(스레드 안전). 캐시 수명은 인스턴스 수명에 묶인다.
//====================================================================================
#ifndef GLIM_HALCON_FEATURE_CONTEXT_H
#define GLIM_HALCON_FEATURE_CONTEXT_H

#include <vector>

#include "Region.h"
#include "Contour.h"
#include "ConvexHull.h"

namespace glim { namespace halcon { namespace core {

class FeatureContext
{
public:
	// region 이 0 이거나 빈 리전이면 모든 값은 0 으로 축약된다.
	explicit FeatureContext(const RegionData* region);

	bool IsEmpty() const;

	// area_center
	long GetArea() const;
	double GetCenterRow() const;
	double GetCenterColumn() const;

	// 외곽 윤곽 (구멍 제외)
	const ContourSet& GetContours() const;

	// contlength
	double GetContourLength() const;

	// 중심에서 윤곽 픽셀까지의 최대 거리 (circularity 의 max)
	double GetMaxContourDistance() const;

	// convex hull 을 래스터화한 픽셀 개수 (convexity 의 F_c)
	long GetConvexHullArea() const;

private:
	void EnsureAreaCenter() const;
	void EnsureContours() const;

	const RegionData* m_pRegion;

	// --- 캐시 (지연계산) ---
	mutable bool m_hasAreaCenter;
	mutable long m_area;
	mutable double m_centerRow;
	mutable double m_centerColumn;

	mutable bool m_hasContours;
	mutable ContourSet m_contours;

	mutable bool m_hasContourLength;
	mutable double m_contourLength;

	mutable bool m_hasMaxDistance;
	mutable double m_maxDistance;

	mutable bool m_hasHullArea;
	mutable long m_hullArea;

private:
	FeatureContext(const FeatureContext&);				// 복사 금지
	FeatureContext& operator=(const FeatureContext&);
};

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_FEATURE_CONTEXT_H
