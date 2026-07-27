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

//------------------------------------------------------------------------------------
// 런 단위 8-연결 성분 라벨링 (union-find, 두 포인터로 O(런 수))
//
// 원래 TraceOuterContours() 안에만 있던 코드를 내부 함수로 승격시킨 것이다.
// SelectLargestBlob 이 같은 라벨링을 필요로 하는데, 로직을 두 벌로 두면
// "윤곽이 잡는 성분" 과 "최대 blob 으로 뽑히는 성분" 이 언젠가 어긋난다.
// 그래서 **양쪽이 이 함수 하나만 호출한다.**
//
// 전제조건: runs 는 RegionData 의 정규형이어야 한다
//           (row 오름차순 → colBegin 오름차순, 같은 행에서 겹침/인접 없음).
//
// 출력 (셋 다 진입 시 clear 된다)
//   labelOfRun      : 크기 = runs.size(). runs[i] 가 속한 성분 ID (0-based)
//   firstRunOfLabel : 크기 = 성분 개수. 성분마다 **스캔 순으로 가장 먼저 나오는** 런 인덱스.
//                     라벨 번호는 이 값의 오름차순으로 부여된다 (라벨 0 = 최상단-최좌측 성분)
//   pixelsOfLabel   : 크기 = 성분 개수. 성분의 픽셀 개수
//
// 반환: 성분이 하나 이상이면 true. runs 가 비었거나 실패하면 false (출력은 전부 빈 상태).
//------------------------------------------------------------------------------------
bool LabelConnectedRuns(const std::vector<Run>& runs,
						std::vector<int>& labelOfRun,
						std::vector<int>& firstRunOfLabel,
						std::vector<long long>& pixelsOfLabel);

// 외곽 윤곽 추적. 성공 시 true. (빈 리전이면 false, out 은 비워진다)
bool TraceOuterContours(const RegionData& region, ContourSet& out);

// HALCON: contlength — 축평행 스텝 1, 대각 스텝 √2 의 합 (닫힌 경로)
double ComputeContourLength(const ContourSet& contours);

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_CONTOUR_H
