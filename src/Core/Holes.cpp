//====================================================================================
// src/Core/Holes.cpp — 구멍 검출 구현 (배경 런렝스 + union-find)
//
// 절차 (docs/02_DEFINITIONS.md §11.4)
//   1) 전경 8-연결 성분 라벨링   ← 기존 core::LabelConnectedRuns 를 **그대로** 쓴다
//      (contlength / SelectLargestBlob 과 같은 함수. 성분 수가 어긋날 여지를 없앤다)
//   2) 바운딩박스를 좌우로 1칸 넓히고, **전경 런이 있는 행만** 배경 런으로 만든다
//      배경 런 = [colLo, colHi] 에서 전경 런을 뺀 여집합.
//      정규형(같은 행의 런이 인접하지 않음) 덕분에 배경 런은 항상 "전경 런 수 + 1" 개다.
//      -> 배경 런 총수 <= 2 x 전경 런 수.  메모리 O(런 수). 바운딩박스 크기와 무관하다
//   3) union-find 에 "바깥(outer)" 가상 노드를 하나 두고
//        - colLo / colHi 를 포함하는 런        -> outer (좌우 여백 열은 전 행이 배경이다)
//        - 위 또는 아래 행에 전경이 없으면      -> outer (그 행은 통째로 배경이라 여백 열과 이어진다)
//        - 바로 윗행이 있으면 두 포인터로 겹치는 배경 런끼리 합친다
//   4) outer 와 루트가 다른 배경 런들이 구멍이다
//
// ⭐ 행 방향으로 박스를 넓히지 않는 이유
//    좌우 여백 열 colLo / colHi 는 어느 행에서나 배경이므로 그 자체가 연결된 "바깥" 이다.
//    위/아래로 벗어난 배경은 전부 이 여백 열과 이어지므로 따로 만들 필요가 없다.
//    3번의 두 번째 규칙이 그것을 대신한다. 덕분에 **빈 행을 자료구조로 만들지 않는다** —
//    전경 픽셀이 두 개뿐이면서 프레임 양 끝에 있는 리전에서도 배경 런은 4개뿐이다.
//====================================================================================
#include "Holes.h"

#include "Contour.h"

#include <cstddef>
#include <vector>

namespace glim { namespace halcon { namespace core {

namespace {

//------------------------------------------------------------------------------------
// union-find
//
// Contour.cpp 에도 같은 것이 있으나 그쪽은 익명 네임스페이스에 갇혀 있다.
// **자료구조**이지 값을 정하는 규칙이 아니므로, 두 벌이 있어도 값이 갈릴 위험이 없다.
// (값을 정하는 규칙인 성분 라벨링 자체는 LabelConnectedRuns 하나만 쓴다.)
//------------------------------------------------------------------------------------
int FindRoot(std::vector<int>& parent, int x)
{
	while (parent[static_cast<std::size_t>(x)] != x)
	{
		parent[static_cast<std::size_t>(x)] =
			parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];	// 경로 압축
		x = parent[static_cast<std::size_t>(x)];
	}
	return x;
}

void UnionSet(std::vector<int>& parent, int a, int b)
{
	const int ra = FindRoot(parent, a);
	const int rb = FindRoot(parent, b);
	if (ra != rb)
		parent[static_cast<std::size_t>(rb)] = ra;
}

//------------------------------------------------------------------------------------
// 배경 런.
//   열 좌표를 long long 으로 두는 이유: colLo = minCol - 1 / colHi = maxCol + 1 이
//   int 극단값에서 넘칠 수 있다. 픽셀 수 합산도 여기서 바로 long long 으로 받는다.
//------------------------------------------------------------------------------------
struct BackgroundRun
{
	long long colBegin;
	long long colEnd;
};

// HC-1 스위치. 4-연결이면 구간이 실제로 겹쳐야 하고, 8-연결이면 1칸 여유를 준다.
#if GLIM_HALCON_BACKGROUND_CONNECTIVITY_8
const long long kBackgroundSlack = 1;
#else
const long long kBackgroundSlack = 0;
#endif

}	// anonymous namespace

//------------------------------------------------------------------------------------
bool ComputeHoleCounts(const RegionData& region, HoleCounts& out)
{
	out = HoleCounts();

	try
	{
		const std::vector<Run>& runs = region.GetRuns();
		if (runs.empty())
			return false;		// 빈 리전 규약 (docs/02_DEFINITIONS.md §0.4)

		//----------------------------------------------------------------------------
		// 1) 전경 8-연결 성분 개수
		//    필요한 것은 개수뿐이지만 **일부러** 기존 함수를 그대로 호출한다.
		//    라벨링을 다시 짜면 contlength 가 세는 성분 수와 언젠가 어긋난다 (HC-2).
		//----------------------------------------------------------------------------
		std::vector<int> labelOfRun;
		std::vector<int> firstRunOfLabel;
		std::vector<long long> pixelsOfLabel;

		if (!LabelConnectedRuns(runs, labelOfRun, firstRunOfLabel, pixelsOfLabel))
			return false;

		out.numConnected = static_cast<long long>(firstRunOfLabel.size());

		//----------------------------------------------------------------------------
		// 2) 배경 런 생성 (전경 런이 있는 행만)
		//----------------------------------------------------------------------------
		// 행 범위(minRow/maxRow)는 받기만 하고 쓰지 않는다.
		// 위/아래 여백 행을 자료구조로 만들지 않기 때문이다 (파일 상단 ⭐ 주석 참조).
		// 필요한 것은 좌우 여백 열뿐이다.
		int minRow = 0;
		int maxRow = 0;
		int minCol = 0;
		int maxCol = 0;
		if (!region.GetBoundingBox(minRow, maxRow, minCol, maxCol))
			return false;

		const long long colLo = static_cast<long long>(minCol) - 1;
		const long long colHi = static_cast<long long>(maxCol) + 1;

		std::vector<BackgroundRun> bgRuns;
		std::vector<int> rowValues;				// 배경 런이 존재하는 행 (= 전경 런이 있는 행)
		std::vector<std::size_t> rowBegin;		// rowValues[t] 행의 런은 [rowBegin[t], rowBegin[t+1])

		bgRuns.reserve(runs.size() * 2 + 2);
		rowValues.reserve(runs.size());
		rowBegin.reserve(runs.size() + 1);

		const std::size_t runCount = runs.size();
		std::size_t i = 0;
		while (i < runCount)
		{
			const int row = runs[i].row;
			std::size_t j = i;
			while (j < runCount && runs[j].row == row)
				++j;

			rowValues.push_back(row);
			rowBegin.push_back(bgRuns.size());

			long long prev = colLo;
			for (std::size_t k = i; k < j; ++k)
			{
				const long long gapEnd = static_cast<long long>(runs[k].colBegin) - 1;
				if (gapEnd >= prev)
				{
					BackgroundRun bg;
					bg.colBegin = prev;
					bg.colEnd = gapEnd;
					bgRuns.push_back(bg);
				}
				prev = static_cast<long long>(runs[k].colEnd) + 1;
			}

			// 마지막 런은 항상 존재한다 (prev <= maxCol + 1 == colHi).
			BackgroundRun tail;
			tail.colBegin = prev;
			tail.colEnd = colHi;
			bgRuns.push_back(tail);

			i = j;
		}
		rowBegin.push_back(bgRuns.size());

		if (bgRuns.empty())
			return true;		// 이론상 도달하지 않는다 (행마다 최소 1개)

		//----------------------------------------------------------------------------
		// 3) union-find + 바깥 가상 노드
		//----------------------------------------------------------------------------
		const std::size_t bgCount = bgRuns.size();
		const int outerNode = static_cast<int>(bgCount);

		std::vector<int> parent(bgCount + 1);
		for (std::size_t p = 0; p <= bgCount; ++p)
			parent[p] = static_cast<int>(p);

		const std::size_t rowCount = rowValues.size();
		for (std::size_t t = 0; t < rowCount; ++t)
		{
			const std::size_t begin = rowBegin[t];
			const std::size_t end = rowBegin[t + 1];

			// (a) 좌우 여백 열에 닿는 런은 바깥이다.
			for (std::size_t u = begin; u < end; ++u)
			{
				if (bgRuns[u].colBegin == colLo || bgRuns[u].colEnd == colHi)
					UnionSet(parent, static_cast<int>(u), outerNode);
			}

			// (b) 위/아래에 전경이 없는 행이 있으면 그 행은 통째로 배경이다 -> 바깥과 이어진다.
			//     int 언더플로/오버플로를 피하려고 long long 으로 비교한다.
			const bool upMissing = (t == 0) ||
				(static_cast<long long>(rowValues[t - 1]) + 1 != static_cast<long long>(rowValues[t]));
			const bool downMissing = (t + 1 == rowCount) ||
				(static_cast<long long>(rowValues[t]) + 1 != static_cast<long long>(rowValues[t + 1]));

			if (upMissing || downMissing)
			{
				for (std::size_t u = begin; u < end; ++u)
					UnionSet(parent, static_cast<int>(u), outerNode);
			}

			// (c) 바로 윗행이 있으면 겹치는 배경 런끼리 합친다 (두 포인터, O(런 수)).
			if (!upMissing)
			{
				const std::size_t prevBegin = rowBegin[t - 1];
				const std::size_t prevEnd = rowBegin[t];

				std::size_t m = prevBegin;
				for (std::size_t u = begin; u < end; ++u)
				{
					const BackgroundRun& cur = bgRuns[u];

					while (m < prevEnd && bgRuns[m].colEnd + kBackgroundSlack < cur.colBegin)
						++m;

					std::size_t w = m;
					while (w < prevEnd && bgRuns[w].colBegin <= cur.colEnd + kBackgroundSlack)
					{
						UnionSet(parent, static_cast<int>(u), static_cast<int>(w));
						++w;
					}
				}
			}
		}

		//----------------------------------------------------------------------------
		// 4) 바깥과 루트가 다른 런 = 구멍
		//----------------------------------------------------------------------------
		const int outerRoot = FindRoot(parent, outerNode);

		// 루트 인덱스 -> 이미 셌는가. 루트는 [0, bgCount] 범위의 인덱스이므로 배열로 충분하다
		// (map 을 쓰면 O(n log n) 이 된다).
		std::vector<unsigned char> counted(bgCount + 1, static_cast<unsigned char>(0));

		for (std::size_t u = 0; u < bgCount; ++u)
		{
			const int root = FindRoot(parent, static_cast<int>(u));
			if (root == outerRoot)
				continue;

			out.areaHoles += bgRuns[u].colEnd - bgRuns[u].colBegin + 1;

			if (counted[static_cast<std::size_t>(root)] == 0)
			{
				counted[static_cast<std::size_t>(root)] = 1;
				++out.numHoles;
			}
		}

		return true;
	}
	catch (...)
	{
		out = HoleCounts();
		return false;
	}
}

}}}	// namespace glim::halcon::core
