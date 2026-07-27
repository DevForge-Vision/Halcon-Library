//====================================================================================
// src/Core/Contour.cpp — Moore 이웃 경계추적 구현
//
// 절차
//   1) 런 단위 8-연결 성분 라벨링 (union-find, 두 포인터로 O(런 수))
//   2) 런 조회 인덱스 생성 (행 목록 + 행별 런 구간). 라벨 이미지를 만들지 않는다
//      → 메모리는 O(런 수). 바운딩박스가 아무리 커도 폭발하지 않는다
//        (풀프레임을 가로지르는 가는 스크래치에서 라벨 이미지는 1 GB 를 넘었다)
//      → 멤버십 판정은 O(1) 이 아니라 O(log n) 이 된다. 의도한 교환이다
//   3) 성분마다 **최상단-최좌측 픽셀**에서 Moore 추적 시작
//      → 시작점이 성분의 최상단-최좌측이므로 추적되는 것은 항상 **외곽 윤곽**이다.
//        구멍 윤곽은 시작점이 될 수 없으므로 원리적으로 배제된다.
//        (docs/02_DEFINITIONS.md §2.4 "구멍의 윤곽은 절대 더하지 않는다")
//   4) 정지 조건: 시작점 p0 에 다시 도달했고 다음 점이 최초의 두 번째 점 p1 과 같을 때
//      (Jacob 정지조건. 얇은 구조에서 되돌아오는 경로도 올바르게 처리된다)
//====================================================================================
#include "Contour.h"

#include <algorithm>
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

//------------------------------------------------------------------------------------
// RunIndex — (row, col) 이 어느 런에 속하는지 이진탐색으로 판정하는 조회 인덱스
//
// 왜 라벨 이미지를 쓰지 않는가
//   라벨 이미지는 바운딩박스 크기다. 픽셀 수가 10개여도 프레임 양 끝에 하나씩 있으면
//   (53580 x 6144) x 4바이트 = 약 1.3 GB 가 잡힌다. 리전의 실제 크기와 무관하게 폭발한다.
//
// 자료구조
//   m_rowValues : 리전에 실제로 존재하는 row 값만 오름차순으로 (희소하므로 dense 배열 금지)
//   m_rowBegin  : m_rowValues[i] 행의 런이 runs[ m_rowBegin[i], m_rowBegin[i+1] ) 에 있다
//   런은 이미 정규형(row 오름차순, 같은 row 안에서 colBegin 오름차순, 겹침/인접 없음)이므로
//   같은 행의 런은 연속 구간에 모여 있다. 한 번 훑으면 인덱스가 만들어진다. O(런 수).
//
// 판정
//   row 이진탐색 → 행 구간 안에서 colBegin 이진탐색 → colEnd 범위 확인.  O(log n)
//   범위 밖 좌표는 자연히 "런에 없음 = 배경" 이 된다 (예전의 1픽셀 패딩 역할을 대신한다).
//------------------------------------------------------------------------------------
class RunIndex
{
public:
	RunIndex() : m_pRuns(0) {}

	// runs 는 정규형이어야 하고, 이 객체보다 오래 살아 있어야 한다.
	bool Build(const std::vector<Run>& runs)
	{
		m_pRuns = &runs;
		m_rowValues.clear();
		m_rowBegin.clear();

		if (runs.empty())
			return false;

		m_rowValues.reserve(runs.size());
		m_rowBegin.reserve(runs.size() + 1);

		for (std::size_t i = 0; i < runs.size(); ++i)
		{
			if (m_rowValues.empty() || m_rowValues.back() != runs[i].row)
			{
				m_rowValues.push_back(runs[i].row);
				m_rowBegin.push_back(i);
			}
		}
		m_rowBegin.push_back(runs.size());
		return true;
	}

	// (row, col) 을 담고 있는 런의 인덱스. 배경이면 -1.
	int FindRunIndex(int row, int col) const
	{
		if (m_pRuns == 0 || m_rowValues.empty())
			return -1;

		// 1) row 이진탐색
		const std::vector<int>::const_iterator it =
			std::lower_bound(m_rowValues.begin(), m_rowValues.end(), row);
		if (it == m_rowValues.end() || *it != row)
			return -1;		// 그 행에는 런이 하나도 없다 = 배경

		const std::size_t rowIndex = static_cast<std::size_t>(it - m_rowValues.begin());
		const std::size_t begin = m_rowBegin[rowIndex];
		const std::size_t end = m_rowBegin[rowIndex + 1];
		if (begin >= end)
			return -1;

		// 2) 행 구간 안에서 colBegin <= col 인 마지막 런을 찾는다.
		//    (겹치지 않으므로 후보는 최대 하나다)
		const std::vector<Run>& runs = *m_pRuns;
		std::size_t low = begin;
		std::size_t high = end;			// [low, high) 에서 colBegin > col 인 첫 런을 찾는다
		while (low < high)
		{
			const std::size_t mid = low + (high - low) / 2;
			if (runs[mid].colBegin <= col)
				low = mid + 1;
			else
				high = mid;
		}

		if (low == begin)
			return -1;		// colBegin <= col 인 런이 없다

		const std::size_t candidate = low - 1;
		if (col > runs[candidate].colEnd)
			return -1;		// 런 사이의 빈 구간

		return static_cast<int>(candidate);
	}

private:
	const std::vector<Run>* m_pRuns;
	std::vector<int> m_rowValues;			// 오름차순 유일 row 목록
	std::vector<std::size_t> m_rowBegin;	// 크기 = m_rowValues.size() + 1

private:
	RunIndex(const RunIndex&);				// 복사 금지 (m_pRuns 수명 혼동 방지)
	RunIndex& operator=(const RunIndex&);
};

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
// 런 단위 8-연결 성분 라벨링 (선언과 계약은 Contour.h 참조)
//
// TraceOuterContours() 안에 인라인으로 있던 것을 그대로 함수로 승격시켰다.
// 알고리즘·라벨 부여 순서는 **한 줄도 바뀌지 않았다.**
//------------------------------------------------------------------------------------
bool LabelConnectedRuns(const std::vector<Run>& runs,
						std::vector<int>& labelOfRun,
						std::vector<int>& firstRunOfLabel,
						std::vector<long long>& pixelsOfLabel)
{
	labelOfRun.clear();
	firstRunOfLabel.clear();
	pixelsOfLabel.clear();

	try
	{
		if (runs.empty())
			return false;

		const int runCount = static_cast<int>(runs.size());

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
		labelOfRun.assign(static_cast<std::size_t>(runCount), -1);

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

		return !firstRunOfLabel.empty();
	}
	catch (...)
	{
		labelOfRun.clear();
		firstRunOfLabel.clear();
		pixelsOfLabel.clear();
		return false;
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

		//----------------------------------------------------------------------------
		// 1) 런 단위 8-연결 성분 라벨링 (SelectLargestBlob 과 **같은 함수**를 쓴다)
		//----------------------------------------------------------------------------
		std::vector<int> labelOfRun;
		std::vector<int> firstRunOfLabel;			// 성분별 최초(스캔순) 런 인덱스
		std::vector<long long> pixelsOfLabel;		// 성분별 픽셀 수 (폭주 방지용)

		if (!LabelConnectedRuns(runs, labelOfRun, firstRunOfLabel, pixelsOfLabel))
			return false;

		//----------------------------------------------------------------------------
		// 2) 런 조회 인덱스 (라벨 이미지 대체)
		//    메모리는 O(런 수)다. 바운딩박스 크기와 무관하다.
		//----------------------------------------------------------------------------
		RunIndex runIndex;
		if (!runIndex.Build(runs))
			return false;

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

					// 런 이진탐색으로 멤버십 판정. 리전 밖 좌표는 자연히 배경(-1)이 된다.
					const int hitRun = runIndex.FindRunIndex(cr, cc);
					if (hitRun >= 0 && labelOfRun[static_cast<std::size_t>(hitRun)] == label)
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
