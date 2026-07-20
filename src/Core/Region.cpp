//====================================================================================
// src/Core/Region.cpp — 런렝스 리전 구현 + 공개 Region 의 PIMPL
//====================================================================================
#include "Region.h"

#include <cstddef>
#include <algorithm>
#include <new>

namespace glim { namespace halcon {

namespace core {

namespace {

// 정렬 술어: row 오름차순 → colBegin 오름차순
struct RunLess
{
	bool operator()(const Run& a, const Run& b) const
	{
		if (a.row != b.row)
			return a.row < b.row;
		return a.colBegin < b.colBegin;
	}
};

}	// anonymous namespace

//------------------------------------------------------------------------------------
// 정규형으로 변환하여 보관한다.
//   - 잘못된 런(colEnd < colBegin) 제거
//   - 정렬
//   - 같은 행에서 겹치거나 인접한 런 병합
//------------------------------------------------------------------------------------
void RegionData::Assign(std::vector<Run>& runs)
{
	m_runs.clear();

	// 유효한 런만 추린다.
	std::vector<Run> valid;
	valid.reserve(runs.size());
	for (std::size_t i = 0; i < runs.size(); ++i)
	{
		if (runs[i].colEnd >= runs[i].colBegin)
			valid.push_back(runs[i]);
	}
	if (valid.empty())
		return;

	std::sort(valid.begin(), valid.end(), RunLess());

	m_runs.reserve(valid.size());
	m_runs.push_back(valid[0]);
	for (std::size_t i = 1; i < valid.size(); ++i)
	{
		Run& last = m_runs.back();
		const Run& cur = valid[i];

		// 같은 행이고, 겹치거나 인접(끝+1 == 시작)하면 병합한다.
		if (cur.row == last.row && cur.colBegin <= last.colEnd + 1)
		{
			if (cur.colEnd > last.colEnd)
				last.colEnd = cur.colEnd;
		}
		else
		{
			m_runs.push_back(cur);
		}
	}
}

//------------------------------------------------------------------------------------
// HALCON: area_center — Area
//   Area = F = 리전에 속한 픽셀 개수
//   런 하나당 (colEnd - colBegin + 1) 을 더한다. O(런 수).
//------------------------------------------------------------------------------------
long RegionData::GetArea() const
{
	long long total = 0;
	for (std::size_t i = 0; i < m_runs.size(); ++i)
		total += static_cast<long long>(m_runs[i].colEnd) - m_runs[i].colBegin + 1;

	return static_cast<long>(total);
}

//------------------------------------------------------------------------------------
// HALCON: area_center — Row, Column
//   Row = (1/F)·Σ r_i ,  Col = (1/F)·Σ c_i
//
//   런 하나에서
//       count = colEnd - colBegin + 1
//       Σ row  = row * count
//       Σ col  = (colBegin + colEnd) * count / 2        ← 등차수열 합, 상수시간
//   픽셀을 하나씩 순회하지 않는다 (docs/02_DEFINITIONS.md §1.4).
//------------------------------------------------------------------------------------
bool RegionData::GetCenter(double& row, double& column) const
{
	row = 0.0;
	column = 0.0;

	if (m_runs.empty())
		return false;

	double totalCount = 0.0;
	double sumRow = 0.0;
	double sumCol = 0.0;

	for (std::size_t i = 0; i < m_runs.size(); ++i)
	{
		const Run& r = m_runs[i];
		const double count = static_cast<double>(r.colEnd - r.colBegin + 1);

		totalCount += count;
		sumRow += static_cast<double>(r.row) * count;
		sumCol += (static_cast<double>(r.colBegin) + static_cast<double>(r.colEnd)) * count * 0.5;
	}

	if (totalCount <= 0.0)
		return false;

	row = sumRow / totalCount;
	column = sumCol / totalCount;
	return true;
}

bool RegionData::GetBoundingBox(int& minRow, int& maxRow, int& minCol, int& maxCol) const
{
	minRow = 0; maxRow = 0; minCol = 0; maxCol = 0;
	if (m_runs.empty())
		return false;

	// 정규형이므로 첫 런의 row 가 최소, 마지막 런의 row 가 최대다.
	minRow = m_runs.front().row;
	maxRow = m_runs.back().row;
	minCol = m_runs.front().colBegin;
	maxCol = m_runs.front().colEnd;

	for (std::size_t i = 1; i < m_runs.size(); ++i)
	{
		if (m_runs[i].colBegin < minCol) minCol = m_runs[i].colBegin;
		if (m_runs[i].colEnd   > maxCol) maxCol = m_runs[i].colEnd;
	}
	return true;
}

}	// namespace core

//====================================================================================
// 공개 Region 의 PIMPL
//====================================================================================
struct Region::Impl
{
	std::shared_ptr<const core::RegionData> m_data;	// 널이면 빈 리전
};

Region::Region()
	: m_pImpl(0)
{
	// 생성자에서도 예외를 밖으로 내보내지 않는다. 실패하면 빈 리전으로 동작한다.
	m_pImpl = new (std::nothrow) Impl();
}

Region::~Region()
{
	delete m_pImpl;
	m_pImpl = 0;
}

Region::Region(const Region& other)
	: m_pImpl(0)
{
	m_pImpl = new (std::nothrow) Impl();
	if (m_pImpl != 0 && other.m_pImpl != 0)
		m_pImpl->m_data = other.m_pImpl->m_data;	// shared_ptr 복사 = O(1)
}

Region& Region::operator=(const Region& other)
{
	if (this == &other)
		return *this;

	if (m_pImpl == 0)
		m_pImpl = new (std::nothrow) Impl();

	if (m_pImpl != 0)
	{
		if (other.m_pImpl != 0)
			m_pImpl->m_data = other.m_pImpl->m_data;
		else
			m_pImpl->m_data.reset();
	}
	return *this;
}

bool Region::IsEmpty() const
{
	if (m_pImpl == 0 || !m_pImpl->m_data)
		return true;
	return m_pImpl->m_data->IsEmpty();
}

//------------------------------------------------------------------------------------
// 마스크 → 리전. 행마다 연속 전경 구간을 런으로 만든다.
//------------------------------------------------------------------------------------
Region Region::FromMask(const unsigned char* data, int width, int height, int stride)
{
	Region region;

	try
	{
		if (data == 0 || width <= 0 || height <= 0)
			return region;

		const int lineStride = (stride > 0) ? stride : width;

		std::vector<Run> runs;
		for (int y = 0; y < height; ++y)
		{
			const unsigned char* line = data + static_cast<std::size_t>(y) * static_cast<std::size_t>(lineStride);

			int x = 0;
			while (x < width)
			{
				if (line[x] != 0)
				{
					const int begin = x;
					while (x < width && line[x] != 0)
						++x;

					Run run;
					run.row = y;
					run.colBegin = begin;
					run.colEnd = x - 1;		// 끝 포함
					runs.push_back(run);
				}
				else
				{
					++x;
				}
			}
		}

		std::shared_ptr<core::RegionData> data2(new core::RegionData());
		data2->Assign(runs);

		if (region.m_pImpl != 0)
			region.m_pImpl->m_data = data2;
	}
	catch (...)
	{
		// 실패 시 빈 리전 (예외를 공개 경계 밖으로 던지지 않는다)
		if (region.m_pImpl != 0)
			region.m_pImpl->m_data.reset();
	}

	return region;
}

//------------------------------------------------------------------------------------
// 런 배열 → 리전. 정렬/병합은 Assign 이 수행한다.
//------------------------------------------------------------------------------------
Region Region::FromRuns(const int* rows, const int* colBegins, const int* colEnds, int count)
{
	Region region;

	try
	{
		if (rows == 0 || colBegins == 0 || colEnds == 0 || count <= 0)
			return region;

		std::vector<Run> runs;
		runs.reserve(static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
		{
			Run run;
			run.row = rows[i];
			run.colBegin = colBegins[i];
			run.colEnd = colEnds[i];
			runs.push_back(run);
		}

		std::shared_ptr<core::RegionData> data(new core::RegionData());
		data->Assign(runs);

		if (region.m_pImpl != 0)
			region.m_pImpl->m_data = data;
	}
	catch (...)
	{
		if (region.m_pImpl != 0)
			region.m_pImpl->m_data.reset();
	}

	return region;
}

//------------------------------------------------------------------------------------
// RegionInternal
//------------------------------------------------------------------------------------
const core::RegionData* RegionInternal::GetData(const Region& region)
{
	if (region.m_pImpl == 0)
		return 0;
	return region.m_pImpl->m_data.get();
}

Region RegionInternal::Create(const std::shared_ptr<const core::RegionData>& data)
{
	Region region;
	if (region.m_pImpl != 0)
		region.m_pImpl->m_data = data;
	return region;
}

}}	// namespace glim::halcon
