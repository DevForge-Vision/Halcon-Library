//====================================================================================
// src/Core/ConvexHull.cpp — Andrew monotone chain + 스캔라인 래스터화
//====================================================================================
#include "ConvexHull.h"

#include <cstddef>
#include <algorithm>
#include <cmath>

namespace glim { namespace halcon { namespace core {

namespace {

// 정렬 술어: x(=col) 오름차순 → y(=row) 오름차순
struct PointLess
{
	bool operator()(const Point& a, const Point& b) const
	{
		if (a.col != b.col)
			return a.col < b.col;
		return a.row < b.row;
	}
};

struct PointEqual
{
	bool operator()(const Point& a, const Point& b) const
	{
		return a.col == b.col && a.row == b.row;
	}
};

// 외적 (O→A) × (O→B). x = col, y = row.
// 좌표가 int 이므로 long long 으로 받아 오버플로를 막는다.
//
// ⚠ 전제: 좌표의 절댓값이 대략 2^30 이하여야 곱이 64비트를 넘지 않는다.
//   이미지 좌표(현실적으로 2^20 미만)에서는 문제가 없다. 아래 Dist2 / Area2 도 같은 전제다.
long long Cross(const Point& o, const Point& a, const Point& b)
{
	const long long ax = static_cast<long long>(a.col) - o.col;
	const long long ay = static_cast<long long>(a.row) - o.row;
	const long long bx = static_cast<long long>(b.col) - o.col;
	const long long by = static_cast<long long>(b.row) - o.row;
	return ax * by - ay * bx;
}

//------------------------------------------------------------------------------------
// diameter_region 보조 (docs/02_DEFINITIONS.md §14.5)
//------------------------------------------------------------------------------------

// 제곱거리. 정수로 계산해야 동률 판정(DR-2)이 흔들리지 않는다.
long long Dist2(const Point& a, const Point& b)
{
	const long long dr = static_cast<long long>(a.row) - b.row;
	const long long dc = static_cast<long long>(a.col) - b.col;
	return dr * dr + dc * dc;
}

// (row, col) 사전순
bool PointLessLex(const Point& a, const Point& b)
{
	if (a.row != b.row)
		return a.row < b.row;
	return a.col < b.col;
}

// (Row1, Column1, Row2, Column2) 사전순
bool PairLessLex(const Point& a1, const Point& a2, const Point& b1, const Point& b2)
{
	if (a1.row != b1.row) return a1.row < b1.row;
	if (a1.col != b1.col) return a1.col < b1.col;
	if (a2.row != b2.row) return a2.row < b2.row;
	return a2.col < b2.col;
}

//------------------------------------------------------------------------------------
// 후보 갱신 — DR-2 의 결정론이 여기 한 곳에 모여 있다.
//   더 크면 교체 / 같으면 사전순 최소 / 작으면 무시.
//   순회 순서에 의존하지 않으므로, 회전 캘리퍼스든 전수 비교든 같은 답이 나온다.
//------------------------------------------------------------------------------------
void ConsiderPair(const Point& a, const Point& b,
				  bool& has, long long& bestSquared, Point& best1, Point& best2)
{
	Point lo = a;
	Point hi = b;
	if (PointLessLex(b, a))
	{
		lo = b;
		hi = a;
	}

	const long long squared = Dist2(lo, hi);

	if (!has || squared > bestSquared)
	{
		has = true;
		bestSquared = squared;
		best1 = lo;
		best2 = hi;
		return;
	}

	if (squared < bestSquared)
		return;

	if (PairLessLex(lo, hi, best1, best2))
	{
		best1 = lo;
		best2 = hi;
	}
}

// 삼각형 넓이의 2배(절댓값). 볼록 다각형에서 한 변에 대한 정점의 거리에 비례한다.
// 절댓값을 쓰므로 hull 의 회전 방향(시계/반시계)에 의존하지 않는다.
long long Area2(const Point& edgeA, const Point& edgeB, const Point& p)
{
	const long long v = Cross(edgeA, edgeB, p);
	return (v < 0) ? -v : v;
}

// 전수 비교. 정점이 2개 이하일 때와, 캘리퍼스 방어가 걸렸을 때의 대비책이다.
void BruteForceDiameter(const std::vector<Point>& hull,
						bool& has, long long& bestSquared, Point& best1, Point& best2)
{
	const std::size_t n = hull.size();
	for (std::size_t a = 0; a + 1 < n; ++a)
	{
		for (std::size_t b = a + 1; b < n; ++b)
			ConsiderPair(hull[a], hull[b], has, bestSquared, best1, best2);
	}
}

}	// anonymous namespace

//------------------------------------------------------------------------------------
// Andrew monotone chain
//------------------------------------------------------------------------------------
bool ComputeConvexHull(const std::vector<Point>& points, std::vector<Point>& hull)
{
	hull.clear();

	try
	{
		if (points.empty())
			return false;

		std::vector<Point> pts(points);
		std::sort(pts.begin(), pts.end(), PointLess());
		pts.erase(std::unique(pts.begin(), pts.end(), PointEqual()), pts.end());

		const std::size_t n = pts.size();
		if (n <= 2)
		{
			hull = pts;			// 점 1개 또는 선분
			return true;
		}

		std::vector<Point> lower;
		lower.reserve(n + 1);
		for (std::size_t i = 0; i < n; ++i)
		{
			while (lower.size() >= 2 && Cross(lower[lower.size() - 2], lower[lower.size() - 1], pts[i]) <= 0)
				lower.pop_back();
			lower.push_back(pts[i]);
		}

		std::vector<Point> upper;
		upper.reserve(n + 1);
		for (std::size_t i = n; i > 0; --i)
		{
			const Point& p = pts[i - 1];
			while (upper.size() >= 2 && Cross(upper[upper.size() - 2], upper[upper.size() - 1], p) <= 0)
				upper.pop_back();
			upper.push_back(p);
		}

		// 양 끝 중복 제거 후 결합
		hull.reserve(lower.size() + upper.size());
		if (!lower.empty())
		{
			for (std::size_t i = 0; i + 1 < lower.size(); ++i)
				hull.push_back(lower[i]);
		}
		if (!upper.empty())
		{
			for (std::size_t i = 0; i + 1 < upper.size(); ++i)
				hull.push_back(upper[i]);
		}

		if (hull.empty())
			hull = pts;			// 방어 (전부 공선인 극단 케이스)

		return true;
	}
	catch (...)
	{
		hull.clear();
		return false;
	}
}

//------------------------------------------------------------------------------------
// 스캔라인 래스터화 → 픽셀 개수
//
//   각 정수 행 y 에 대해 폴리곤 변과의 교점에서 xLeft, xRight 를 구하고
//   ceil(xLeft - eps) .. floor(xRight + eps) 를 채운다 (경계 포함).
//   정사각형 hull 에서 원본 픽셀 수와 정확히 일치하도록 만드는 규칙이다.
//------------------------------------------------------------------------------------
long RasterizeHullArea(const std::vector<Point>& hull)
{
	try
	{
		const std::size_t n = hull.size();
		if (n == 0)
			return 0;
		if (n == 1)
			return 1;

		static const double kEps = 1e-9;

		int minRow = hull[0].row;
		int maxRow = hull[0].row;
		for (std::size_t i = 1; i < n; ++i)
		{
			if (hull[i].row < minRow) minRow = hull[i].row;
			if (hull[i].row > maxRow) maxRow = hull[i].row;
		}

		// 변의 개수: 정점이 2개면 선분 1개, 3개 이상이면 닫힌 폴리곤이므로 n 개
		const std::size_t edgeCount = (n == 2) ? 1 : n;

		long long total = 0;

		for (int y = minRow; y <= maxRow; ++y)
		{
			bool has = false;
			double xMin = 0.0;
			double xMax = 0.0;

			for (std::size_t e = 0; e < edgeCount; ++e)
			{
				const Point& p = hull[e];
				const Point& q = hull[(e + 1) % n];

				const int y1 = p.row;
				const int y2 = q.row;
				const int lo = (y1 < y2) ? y1 : y2;
				const int hi = (y1 < y2) ? y2 : y1;
				if (y < lo || y > hi)
					continue;

				if (y1 == y2)
				{
					// 수평 변: 양 끝점을 모두 반영
					const double xa = static_cast<double>(p.col);
					const double xb = static_cast<double>(q.col);
					if (!has) { xMin = xa; xMax = xa; has = true; }
					if (xa < xMin) xMin = xa;
					if (xa > xMax) xMax = xa;
					if (xb < xMin) xMin = xb;
					if (xb > xMax) xMax = xb;
				}
				else
				{
					const double t = static_cast<double>(y - y1) / static_cast<double>(y2 - y1);
					const double x = static_cast<double>(p.col) + t * (static_cast<double>(q.col) - static_cast<double>(p.col));
					if (!has) { xMin = x; xMax = x; has = true; }
					if (x < xMin) xMin = x;
					if (x > xMax) xMax = x;
				}
			}

			if (!has)
				continue;

			const double left = std::ceil(xMin - kEps);
			const double right = std::floor(xMax + kEps);
			if (right >= left)
				total += static_cast<long long>(right - left) + 1;
		}

		return static_cast<long>(total);
	}
	catch (...)
	{
		return 0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: diameter_region — 회전 캘리퍼스
//
//   볼록 다각형에서 최대 거리 쌍은 **대척점(antipodal) 쌍** 중에 있다.
//   변 하나를 고정하고 그 변에서 가장 먼 정점 j 를 따라가면, j 는 전체 순회 동안
//   한 방향으로만 이동하므로 총 전진량이 O(정점 수)다.
//
//   방어: 입력이 볼록이 아니면 j 가 무한히 돌 수 있다. 총 전진량에 상한을 두고,
//         걸리면 전수 비교로 되돌아간다 (정상 입력에서는 도달하지 않는다).
//------------------------------------------------------------------------------------
bool ComputeHullDiameter(const std::vector<Point>& hull,
						 Point& point1, Point& point2, double& diameter)
{
	point1 = Point();
	point2 = Point();
	diameter = 0.0;

	try
	{
		const std::size_t n = hull.size();
		if (n == 0)
			return false;

		if (n == 1)
		{
			// 단일 픽셀: 두 극점이 같은 점이므로 Diameter = 0 (DR-1)
			point1 = hull[0];
			point2 = hull[0];
			diameter = 0.0;
			return true;
		}

		bool has = false;
		long long bestSquared = 0;
		Point best1;
		Point best2;

		if (n == 2)
		{
			BruteForceDiameter(hull, has, bestSquared, best1, best2);
		}
		else
		{
			const long long maxAdvance = 2 * static_cast<long long>(n) + 4;
			long long advance = 0;
			bool overrun = false;

			std::size_t j = 1;
			for (std::size_t i = 0; i < n; ++i)
			{
				const std::size_t iNext = (i + 1) % n;

				while (Area2(hull[i], hull[iNext], hull[(j + 1) % n]) >
					   Area2(hull[i], hull[iNext], hull[j]))
				{
					j = (j + 1) % n;
					if (++advance > maxAdvance)
					{
						overrun = true;
						break;
					}
				}

				if (overrun)
					break;

				ConsiderPair(hull[i], hull[j], has, bestSquared, best1, best2);
				ConsiderPair(hull[iNext], hull[j], has, bestSquared, best1, best2);
			}

			if (overrun)
			{
				// 방어 경로. 결과가 부분적일 수 있으므로 처음부터 다시 센다.
				has = false;
				bestSquared = 0;
				BruteForceDiameter(hull, has, bestSquared, best1, best2);
			}
		}

		if (!has)
		{
			point1 = hull[0];
			point2 = hull[0];
			diameter = 0.0;
			return true;
		}

		point1 = best1;
		point2 = best2;
		diameter = std::sqrt(static_cast<double>(bestSquared));

		if (!(diameter >= 0.0))		// NaN 방어
			diameter = 0.0;

		return true;
	}
	catch (...)
	{
		point1 = Point();
		point2 = Point();
		diameter = 0.0;
		return false;
	}
}

}}}	// namespace glim::halcon::core
