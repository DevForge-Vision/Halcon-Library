//====================================================================================
// src/Core/Contour.h — Moore 이웃 경계추적 (외곽 윤곽만)
//
// 근거: docs/02_DEFINITIONS.md §2 (contlength)
//   - "The contour of holes is not calculated."  → 구멍 윤곽은 절대 추적하지 않는다
//   - 거리 가중치: 축평행 = 1, 대각 = √2
//   - C-1 확정: **픽셀 중심 체인**(Freeman chain). 단일 픽셀 → 길이 0
//   - C-2: 8-이웃 고정
//   - R-1: 체인은 **닫힌 경로**로 본다 (마지막 점 → 첫 점 복귀 1스텝 포함)
//====================================================================================
#ifndef GLIM_HALCON_CORE_CONTOUR_H
#define GLIM_HALCON_CORE_CONTOUR_H

#include <vector>

#include "Region.h"

namespace glim { namespace halcon { namespace core {

// HALCON 관례 (row, col)
struct Point
{
	int row;
	int col;

	Point() : row(0), col(0) {}
	Point(int r, int c) : row(r), col(c) {}
};

//------------------------------------------------------------------------------------
// ContourSet — 연결성분별 외곽 윤곽 픽셀 체인
//   각 체인은 닫힌 경로로 간주한다(마지막 점과 첫 점이 이웃).
//------------------------------------------------------------------------------------
class ContourSet
{
public:
	std::vector< std::vector<Point> > m_contours;

	bool IsEmpty() const { return m_contours.empty(); }

	// 모든 성분의 윤곽 픽셀을 하나의 배열로 모은다.
	void Flatten(std::vector<Point>& out) const;
};

// 외곽 윤곽 추적. 성공 시 true. (빈 리전이면 false, out 은 비워진다)
bool TraceOuterContours(const RegionData& region, ContourSet& out);

// HALCON: contlength — 축평행 스텝 1, 대각 스텝 √2 의 합 (닫힌 경로)
double ComputeContourLength(const ContourSet& contours);

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_CONTOUR_H
