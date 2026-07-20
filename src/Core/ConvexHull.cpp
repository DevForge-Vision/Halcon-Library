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
long long Cross(const Point& o, const Point& a, const Point& b)
{
	const long long ax = static_cast<long long>(a.col) - o.col;
	const long long ay = static_cast<long long>(a.row) - o.row;
	const long long bx = static_cast<long long>(b.col) - o.col;
	const long long by = static_cast<long long>(b.row) - o.row;
	return ax * by - ay * bx;
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

}}}	// namespace glim::halcon::core
