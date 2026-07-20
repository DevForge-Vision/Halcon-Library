//====================================================================================
// src/Core/Region.h — 런렝스 리전의 내부 표현
//
// 근거: docs/01_ARCHITECTURE.md §4 (런렝스 자료구조 / 정규형 불변식)
//       docs/02_DEFINITIONS.md §1.4 (area_center 의 O(런 수) 누적)
//
// 이 헤더는 라이브러리 내부 전용이다. 사용처는 include/GlimHalcon.h 만 본다.
//====================================================================================
#ifndef GLIM_HALCON_CORE_REGION_H
#define GLIM_HALCON_CORE_REGION_H

#include <vector>
#include <memory>

#include "GlimHalcon.h"

namespace glim { namespace halcon {

//------------------------------------------------------------------------------------
// 런 = 한 행에서 연속된 전경 픽셀 구간. colEnd 는 **끝 포함**이다.
//------------------------------------------------------------------------------------
struct Run
{
	int row;
	int colBegin;
	int colEnd;
};

namespace core {

//------------------------------------------------------------------------------------
// RegionData — 불변 런렝스 컨테이너
//
// 정규형 불변식 (생성 시 보장, 이후 변경 금지)
//   1) row 오름차순, 같은 행 안에서 colBegin 오름차순
//   2) 같은 행의 런끼리 겹치거나 인접(colEnd + 1 == 다음 colBegin)하지 않는다
//------------------------------------------------------------------------------------
class RegionData
{
public:
	RegionData() {}

	const std::vector<Run>& GetRuns() const { return m_runs; }
	bool IsEmpty() const { return m_runs.empty(); }

	// 정규화하여 채운다. 생성 시점에만 호출한다.
	void Assign(std::vector<Run>& runs);

	// area_center — O(런 수). 픽셀 단위 순회를 하지 않는다.
	// HALCON: Area = F, Row = (1/F)·Σr_i, Col = (1/F)·Σc_i
	long GetArea() const;
	bool GetCenter(double& row, double& column) const;

	// 바운딩 박스. 빈 리전이면 false.
	bool GetBoundingBox(int& minRow, int& maxRow, int& minCol, int& maxCol) const;

private:
	std::vector<Run> m_runs;
};

}	// namespace core

//------------------------------------------------------------------------------------
// RegionInternal — 공개 Region 의 내부 데이터 접근자 (friend)
//------------------------------------------------------------------------------------
struct RegionInternal
{
	// 내부 데이터 포인터. 실패/빈 리전이면 0 을 반환할 수 있다.
	static const core::RegionData* GetData(const Region& region);

	// 내부 데이터로부터 공개 Region 을 만든다.
	static Region Create(const std::shared_ptr<const core::RegionData>& data);
};

}}	// namespace glim::halcon

#endif	// GLIM_HALCON_CORE_REGION_H
