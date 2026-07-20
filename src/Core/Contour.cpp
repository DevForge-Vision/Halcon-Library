//====================================================================================
// src/Core/Contour.cpp — Moore 이웃 경계추적 구현
//
// 절차
//   1) 런 단위 8-연결 성분 라벨링 (union-find, 두 포인터로 O(런 수))
//   2) 바운딩박스 + 1픽셀 패딩 라벨 이미지 생성
//   3) 성분마다 **최상단-최좌측 픽셀**에서 Moore 추적 시작
//      → 시작점이 성분의 최상단-최좌측이므로 추적되는 것은 항상 **외곽 윤곽**이다.
//        구멍 윤곽은 시작점이 될 수 없으므로 원리적으로 배제된다.
//        (docs/02_DEFINITIONS.md §2.4 "구멍의 윤곽은 절대 더하지 않는다")
//   4) 정지 조건: 시작점 p0 에 다시 도달했고 다음 점이 최초의 두 번째 점 p1 과 같을 때
//      (Jacob 정지조건. 얇은 구조에서 되돌아오는 경로도 올바르게 처리된다)
//====================================================================================
#include "Contour.h"

#include <cmath>
#include <cstddef>

namespace glim { namespace halcon { namespace core {

namespace {

//------------------------------------------------------------------------------------
// 8-이웃 방향 (시계 방향). row 는 아래로 증가한다.
//   0:E  1:SE  2:S  3:SW  4:W  5:NW  6:N  7:NE
//------------------------------------------------------------------------------------
const int kDirRow[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
const int kDirCol[8] = {  1,  1,  0, -1, -1, -1,  0,  1 };

int DirIndexOf(int dRow, int dCol)
{
	for (int i = 0; i < 8; ++i)
	{
		if (kDirRow[i] == dRow && kDirCol[i] == dCol)
			return i;
	}
	return -1;
}

//------------------------------------------------------------------------------------
// union-find
//------------------------------------------------------------------------------------
int FindRoot(std::vector<int>& parent, int x)
{
	while (parent[x] != x)
	{
		parent[x] = parent[parent[x]];	// 경로 압축
		x = parent[x];
	}
	return x;
}

void UnionSet(std::vector<int>& parent, int a, int b)
{
	const int ra = FindRoot(parent, a);
	const int rb = FindRoot(parent, b);
	if (ra != rb)
		parent[rb] = ra;
}

}	// anonymous namespace

//------------------------------------------------------------------------------------
void ContourSet::Flatten(std::vector<Point>& out) const
{
	out.clear();

	std::size_t total = 0;
	for (std::size_t i = 0; i < m_contours.size(); ++i)
		total += m_contours[i].size();

	out.reserve(total);
	for (std::size_t i = 0; i < m_contours.size(); ++i)
	{
		for (std::size_t k = 0; k < m_contours[i].size(); ++k)
			out.push_back(m_contours[i][k]);
	}
}

//------------------------------------------------------------------------------------
// 외곽 윤곽 추적
//------------------------------------------------------------------------------------
bool TraceOuterContours(const RegionData& region, ContourSet& out)
{
	out.m_contours.clear();

	try
	{
		const std::vector<Run>& runs = region.GetRuns();
		if (runs.empty())
			return false;

		const int runCount = static_cast<int>(runs.size());

		//----------------------------------------------------------------------------
		// 1) 런 단위 8-연결 성분 라벨링
		//----------------------------------------------------------------------------
		std::vector<int> parent(static_cast<std::size_t>(runCount));
		for (int i = 0; i < runCount; ++i)
			parent[static_cast<std::size_t>(i)] = i;

		int prevBegin = 0;
		int prevEnd = 0;			// [prevBegin, prevEnd) 가 이전 행의 런 구간
		int prevRow = 0;
		bool hasPrev = false;

		int i = 0;
		while (i < runCount)
		{
			const int curRow = runs[static_cast<std::size_t>(i)].row;
			int j = i;
			while (j < runCount && runs[static_cast<std::size_t>(j)].row == curRow)
				++j;

			// 이전 행이 바로 윗행이면 인접(겹침 또는 대각 접촉) 여부를 본다.
			if (hasPrev && prevRow == curRow - 1)
			{
				int m = prevBegin;
				for (int k = i; k < j; ++k)
				{
					const Run& cur = runs[static_cast<std::size_t>(k)];

					// 8-연결: [colBegin-1, colEnd+1] 구간이 겹치면 연결
					while (m < prevEnd && runs[static_cast<std::size_t>(m)].colEnd + 1 < cur.colBegin)
						++m;

					int t = m;
					while (t < prevEnd && runs[static_cast<std::size_t>(t)].colBegin <= cur.colEnd + 1)
					{
						UnionSet(parent, k, t);
						++t;
					}
				}
			}

			prevBegin = i;
			prevEnd = j;
			prevRow = curRow;
			hasPrev = true;
			i = j;
		}

		// 루트 → 순차 성분 ID
		std::vector<int> labelOfRun(static_cast<std::size_t>(runCount), -1);
		std::vector<int> firstRunOfLabel;			// 성분별 최초(스캔순) 런 인덱스
		std::vector<long long> pixelsOfLabel;		// 성분별 픽셀 수 (폭주 방지용)
		{
			std::vector<int> labelOfRoot(static_cast<std::size_t>(runCount), -1);
			for (int k = 0; k < runCount; ++k)
			{
				const int root = FindRoot(parent, k);
				if (labelOfRoot[static_cast<std::size_t>(root)] < 0)
				{
					labelOfRoot[static_cast<std::size_t>(root)] = static_cast<int>(firstRunOfLabel.size());
					firstRunOfLabel.push_back(k);
					pixelsOfLabel.push_back(0);
				}
				const int label = labelOfRoot[static_cast<std::size_t>(root)];
				labelOfRun[static_cast<std::size_t>(k)] = label;
				pixelsOfLabel[static_cast<std::size_t>(label)] +=
					static_cast<long long>(runs[static_cast<std::size_t>(k)].colEnd) - runs[static_cast<std::size_t>(k)].colBegin + 1;
			}
		}

		//----------------------------------------------------------------------------
		// 2) 라벨 이미지 (바운딩박스 + 1픽셀 패딩)
		//----------------------------------------------------------------------------
		int minRow = 0, maxRow = 0, minCol = 0, maxCol = 0;
		if (!region.GetBoundingBox(minRow, maxRow, minCol, maxCol))
			return false;

		const int imgWidth = (maxCol - minCol + 1) + 2;
		const int imgHeight = (maxRow - minRow + 1) + 2;
		if (imgWidth <= 0 || imgHeight <= 0)
			return false;

		std::vector<int> labelImage(static_cast<std::size_t>(imgWidth) * static_cast<std::size_t>(imgHeight), -1);

		// (row, col) → 라벨 이미지 인덱스
		const int offsetRow = 1 - minRow;
		const int offsetCol = 1 - minCol;

		for (int k = 0; k < runCount; ++k)
		{
			const Run& r = runs[static_cast<std::size_t>(k)];
			const int y = r.row + offsetRow;
			const std::size_t base = static_cast<std::size_t>(y) * static_cast<std::size_t>(imgWidth);
			const int label = labelOfRun[static_cast<std::size_t>(k)];

			for (int c = r.colBegin; c <= r.colEnd; ++c)
				labelImage[base + static_cast<std::size_t>(c + offsetCol)] = label;
		}

		//----------------------------------------------------------------------------
		// 3) 성분별 Moore 추적
		//----------------------------------------------------------------------------
		const int labelCount = static_cast<int>(firstRunOfLabel.size());
		out.m_contours.reserve(static_cast<std::size_t>(labelCount));

		for (int label = 0; label < labelCount; ++label)
		{
			const Run& startRun = runs[static_cast<std::size_t>(firstRunOfLabel[static_cast<std::size_t>(label)])];

			// 최상단-최좌측 픽셀 (런이 스캔 순으로 정렬돼 있으므로 이것이 곧 외곽 시작점이다)
			const Point start(startRun.row, startRun.colBegin);

			std::vector<Point> contour;

			Point p = start;
			Point back(start.row, start.col - 1);	// 시작 backtrack = 서쪽 (반드시 배경)
			Point second(0, 0);
			bool hasSecond = false;

			// 폭주 방지: 픽셀 수의 8배 + 여유
			const long long maxSteps = pixelsOfLabel[static_cast<std::size_t>(label)] * 8 + 16;
			long long stepGuard = 0;

			while (true)
			{
				if (++stepGuard > maxSteps)
					break;	// 방어: 정상 입력에서는 도달하지 않는다

				const int startDir = DirIndexOf(back.row - p.row, back.col - p.col);
				if (startDir < 0)
					break;	// 방어

				Point next(0, 0);
				Point newBack(0, 0);
				bool found = false;

				for (int step = 1; step <= 8; ++step)
				{
					const int d = (startDir + step) & 7;
					const int cr = p.row + kDirRow[d];
					const int cc = p.col + kDirCol[d];

					const int y = cr + offsetRow;
					const int x = cc + offsetCol;
					if (y < 0 || y >= imgHeight || x < 0 || x >= imgWidth)
						continue;	// 방어 (패딩 덕분에 정상적으로는 발생하지 않는다)

					if (labelImage[static_cast<std::size_t>(y) * static_cast<std::size_t>(imgWidth) + static_cast<std::size_t>(x)] == label)
					{
						next = Point(cr, cc);

						// 직전에 확인한 이웃(반드시 배경)이 새 backtrack 이 된다.
						const int pd = (d + 7) & 7;
						newBack = Point(p.row + kDirRow[pd], p.col + kDirCol[pd]);
						found = true;
						break;
					}
				}

				if (!found)
				{
					// 고립 픽셀: 체인 스텝이 없다 → ContLength = 0 (C-1 확정 결과)
					contour.clear();
					contour.push_back(start);
					break;
				}

				if (!hasSecond)
				{
					second = next;
					hasSecond = true;
				}
				else if (p.row == start.row && p.col == start.col &&
						 next.row == second.row && next.col == second.col)
				{
					break;	// 닫힘 (Jacob 정지조건)
				}

				contour.push_back(p);
				p = next;
				back = newBack;
			}

			if (!contour.empty())
				out.m_contours.push_back(contour);
		}

		return !out.m_contours.empty();
	}
	catch (...)
	{
		out.m_contours.clear();
		return false;
	}
}

//------------------------------------------------------------------------------------
// HALCON: contlength
//   ContLength = (축평행 스텝 수 × 1) + (대각 스텝 수 × √2)
//   체인은 닫힌 경로다 (마지막 점 → 첫 점 복귀 스텝 포함, 잔여 리스크 R-1).
//   단일 픽셀 체인(점 1개)은 스텝이 없으므로 0.0 이다.
//------------------------------------------------------------------------------------
double ComputeContourLength(const ContourSet& contours)
{
	static const double kSqrt2 = 1.4142135623730950488;

	double total = 0.0;

	for (std::size_t i = 0; i < contours.m_contours.size(); ++i)
	{
		const std::vector<Point>& chain = contours.m_contours[i];
		const std::size_t n = chain.size();
		if (n < 2)
			continue;	// 스텝 없음

		for (std::size_t k = 0; k < n; ++k)
		{
			const Point& a = chain[k];
			const Point& b = chain[(k + 1) % n];	// 마지막 → 첫 점 (닫힘)

			const int dRow = (a.row > b.row) ? (a.row - b.row) : (b.row - a.row);
			const int dCol = (a.col > b.col) ? (a.col - b.col) : (b.col - a.col);

			if (dRow == 1 && dCol == 1)
				total += kSqrt2;			// 대각 인접
			else
				total += static_cast<double>(dRow + dCol);	// 축평행 인접
		}
	}

	return total;
}

}}}	// namespace glim::halcon::core
