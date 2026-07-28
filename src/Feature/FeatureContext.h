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
#include "Moments.h"

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

	// 무게중심에서 가장 먼 외곽 윤곽 픽셀 (orientation_region 의 +pi 판정용).
	//   GetMaxContourDistance() 와 **같은 순회에서** 결정된다. 두 값이 어긋날 수 없다.
	//   동률이면 윤곽 순회 순서상 먼저 나온 점 (docs/02_DEFINITIONS.md §10.5 OR-3).
	//   윤곽이 비어 있으면 false 를 반환하고 point 는 (0,0) 이다.
	bool GetFarthestContourPoint(Point& point) const;

	// convex hull 을 래스터화한 픽셀 개수 (convexity 의 F_c)
	long GetConvexHullArea() const;

	//--------------------------------------------------------------------------------
	// 2차 모멘트 계열 (docs/02_DEFINITIONS.md §6~§10)
	//   리전당 **1회만** 계산되어 5종이 공유한다.
	//--------------------------------------------------------------------------------

	// moments_region_2nd 의 M11/M20/M02/Ia/Ib. **정규화 없음 (순수 합)**.
	//   원점 = 무게중심.
	const CentralMoments2nd& GetMoments2nd() const;

	// moments_region_2nd_invar 가 F^2 로 나눌 **분자**.
	//   MI-1 스위치가 기본값(0)이면 GetMoments2nd() 와 완전히 같은 값이다.
	//   1 이면 바운딩박스 중심을 원점으로 다시 계산한 값이다.
	const CentralMoments2nd& GetMoments2ndInvarSource() const;

	// elliptic_axis 의 Ra/Rb/Phi. 위 모멘트를 **F 로** 정규화해 만든다.
	const EllipseParams& GetEllipse() const;

private:
	void EnsureAreaCenter() const;
	void EnsureContours() const;
	void EnsureFarthestContour() const;

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

	// 최대거리와 최원점은 **하나의 순회 결과**다. 캐시 플래그도 하나만 둔다.
	mutable bool m_hasFarthest;
	mutable double m_maxDistance;
	mutable bool m_hasFarthestPoint;
	mutable Point m_farthestPoint;

	mutable bool m_hasHullArea;
	mutable long m_hullArea;

	mutable bool m_hasMoments;
	mutable CentralMoments2nd m_moments;

	mutable bool m_hasInvarSource;
	mutable CentralMoments2nd m_invarSource;

	mutable bool m_hasEllipse;
	mutable EllipseParams m_ellipse;

private:
	FeatureContext(const FeatureContext&);				// 복사 금지
	FeatureContext& operator=(const FeatureContext&);
};

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_FEATURE_CONTEXT_H
