//====================================================================================
// examples/03_from_runs.cpp — 런렝스 / 좌표 목록으로 Region 만들기
//
// 이 예제가 보여주는 것
//   1) 이미 런렝스(run-length) 형태로 blob 을 들고 있는 검사기에서
//      마스크 이미지를 다시 만들지 않고 바로 Region 을 만드는 법
//   2) 좌표 목록(픽셀 리스트)밖에 없을 때 런으로 변환하는 법
//   3) 정렬되지 않은 / 겹치는 / 인접한 런을 넣어도 내부에서 정규화된다는 점
//
// 외부 의존성 0. 표준 라이브러리만 쓴다.
//
// 빌드 (MSVC 개발자 명령 프롬프트, 저장소 루트에서)
//   cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
//      src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\03_from_runs.cpp ^
//      /Fe:fromruns.exe
//====================================================================================
#include "GlimHalcon.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

using namespace glim::halcon;

namespace {

void PrintFeatures(const char* name, const Region& region)
{
	long area = 0;
	double centerRow = 0.0;
	double centerColumn = 0.0;
	double contLength = 0.0;
	double circularity = 0.0;
	double compactness = 0.0;
	double convexity = 0.0;

	AreaCenter (region, area, centerRow, centerColumn);
	ContLength (region, contLength);
	Circularity(region, circularity);
	Compactness(region, compactness);
	Convexity  (region, convexity);

	std::printf("%-22s %8ld %10.4f %10.4f %12.4f %10.6f %10.6f %10.6f\n",
				name, area, centerRow, centerColumn,
				contLength, circularity, compactness, convexity);
}

void PrintHeader()
{
	std::printf("%-22s %8s %10s %10s %12s %10s %10s %10s\n",
				"Case", "Area", "Row", "Column", "ContLength", "Circular", "Compact", "Convex");
	std::printf("--------------------------------------------------------------------------------------------------\n");
}

//------------------------------------------------------------------------------------
// 1) 런렝스로 직접 만들기
//    colEnd 는 **끝 포함**이다. col 10..29 는 colBegin=10, colEnd=29 (개수 20).
//------------------------------------------------------------------------------------
Region MakeFromRuns()
{
	std::vector<int> rows;
	std::vector<int> colBegins;
	std::vector<int> colEnds;

	for (int row = 5; row <= 24; ++row)
	{
		rows.push_back(row);
		colBegins.push_back(10);
		colEnds.push_back(29);			// 끝 포함
	}

	return Region::FromRuns(&rows[0], &colBegins[0], &colEnds[0], static_cast<int>(rows.size()));
}

//------------------------------------------------------------------------------------
// 2) 정렬되지 않고 겹치고 인접한 런을 넣어도 결과는 같다.
//    RegionData::Assign 이 정렬 → 병합 → 정규형으로 만든다.
//------------------------------------------------------------------------------------
Region MakeFromMessyRuns()
{
	std::vector<int> rows;
	std::vector<int> colBegins;
	std::vector<int> colEnds;

	// 같은 정사각형을 조각내서, 역순으로, 겹치게, 인접하게 넣는다.
	for (int row = 24; row >= 5; --row)
	{
		rows.push_back(row); colBegins.push_back(20); colEnds.push_back(29);	// 뒷조각 먼저
		rows.push_back(row); colBegins.push_back(10); colEnds.push_back(22);	// 겹침 (20..22)
		rows.push_back(row); colBegins.push_back(15); colEnds.push_back(15);	// 완전 포함
	}

	// 잘못된 런(colEnd < colBegin) 은 내부에서 버려진다.
	rows.push_back(7); colBegins.push_back(50); colEnds.push_back(40);

	return Region::FromRuns(&rows[0], &colBegins[0], &colEnds[0], static_cast<int>(rows.size()));
}

//------------------------------------------------------------------------------------
// 3) 좌표 목록(픽셀 리스트) → 런 변환
//    검사기가 blob 픽셀을 (row, col) 목록으로 들고 있을 때 쓰는 어댑터다.
//------------------------------------------------------------------------------------
struct PixelPoint
{
	int row;
	int col;
};

bool PixelLess(const PixelPoint& a, const PixelPoint& b)
{
	if (a.row != b.row)
		return a.row < b.row;
	return a.col < b.col;
}

Region MakeFromPointList(std::vector<PixelPoint>& points)
{
	if (points.empty())
		return Region();

	std::sort(points.begin(), points.end(), PixelLess);

	std::vector<int> rows;
	std::vector<int> colBegins;
	std::vector<int> colEnds;

	std::size_t i = 0;
	while (i < points.size())
	{
		const int row = points[i].row;
		int begin = points[i].col;
		int end = points[i].col;

		std::size_t k = i + 1;
		while (k < points.size() && points[k].row == row && points[k].col <= end + 1)
		{
			if (points[k].col > end)
				end = points[k].col;
			++k;
		}

		rows.push_back(row);
		colBegins.push_back(begin);
		colEnds.push_back(end);
		i = k;
	}

	return Region::FromRuns(&rows[0], &colBegins[0], &colEnds[0], static_cast<int>(rows.size()));
}

}	// anonymous namespace

//====================================================================================
int main()
{
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 예제 03 - 런렝스 / 좌표 목록으로 Region 만들기\n");
	std::printf("====================================================================================\n\n");

	// 좌표 목록 케이스도 같은 정사각형으로 만든다.
	std::vector<PixelPoint> points;
	for (int row = 24; row >= 5; --row)			// 일부러 역순
	{
		for (int col = 29; col >= 10; --col)
		{
			PixelPoint p;
			p.row = row;
			p.col = col;
			points.push_back(p);
		}
	}

	PrintHeader();
	PrintFeatures("FromRuns (정렬됨)",   MakeFromRuns());
	PrintFeatures("FromRuns (뒤죽박죽)", MakeFromMessyRuns());
	PrintFeatures("좌표목록 -> 런",      MakeFromPointList(points));
	std::printf("\n");
	std::printf("세 줄의 값이 모두 같으면 정규화가 정상 동작한 것입니다.\n");

	return 0;
}
