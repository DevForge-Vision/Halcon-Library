//====================================================================================
// examples/01_quickstart.cpp — 5분 시작하기
//
// 이 예제가 보여주는 것
//   1) 공개 헤더는 include/GlimHalcon.h 하나뿐이다.
//   2) 이진 마스크 버퍼(unsigned char*) 를 Region 으로 만든다.
//   3) 오퍼레이터를 호출해 특징값을 받는다. 출력은 참조 인자다.
//
// 외부 의존성 0. 표준 라이브러리만 쓴다.
//
// 빌드 (MSVC 개발자 명령 프롬프트, 저장소 루트에서)
//   cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
//      src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\01_quickstart.cpp ^
//      /Fe:quickstart.exe
//====================================================================================
#include "GlimHalcon.h"

#include <cstdio>
#include <vector>

using namespace glim::halcon;

int main()
{
	//--------------------------------------------------------------------------------
	// 1) 이진 마스크를 만든다.
	//    좌표계는 HALCON 관례다. row = y (아래로 증가), col = x (오른쪽으로 증가).
	//    좌상단이 (0, 0) 이고 0-based 다.
	//    버퍼는 행 우선(row-major) 이고, 0 이 아닌 값이면 전부 전경으로 본다.
	//--------------------------------------------------------------------------------
	const int width = 8;
	const int height = 6;

	std::vector<unsigned char> mask(static_cast<size_t>(width) * height, 0);

	// row 1..4, col 2..5 를 채운다 → 4행 x 4열 = 16 픽셀 정사각형
	for (int row = 1; row <= 4; ++row)
	{
		for (int col = 2; col <= 5; ++col)
			mask[static_cast<size_t>(row) * width + col] = 255;
	}

	//--------------------------------------------------------------------------------
	// 2) Region 생성. stride 는 한 행의 바이트 수다 (0 이하이면 width 로 간주).
	//--------------------------------------------------------------------------------
	Region region = Region::FromMask(&mask[0], width, height, width);

	if (region.IsEmpty())
	{
		std::printf("빈 리전입니다.\n");
		return 1;
	}

	//--------------------------------------------------------------------------------
	// 3) 특징값 호출. 출력은 참조 인자로 받는다 (HALCON 시그니처와 1:1 대응).
	//--------------------------------------------------------------------------------
	long area = 0;
	double centerRow = 0.0;
	double centerColumn = 0.0;
	AreaCenter(region, area, centerRow, centerColumn);

	double contLength = 0.0;
	ContLength(region, contLength);

	double circularity = 0.0;
	Circularity(region, circularity);

	std::printf("Area        = %ld\n", area);
	std::printf("Row         = %.6f\n", centerRow);
	std::printf("Column      = %.6f\n", centerColumn);
	std::printf("ContLength  = %.6f\n", contLength);
	std::printf("Circularity = %.6f\n", circularity);

	return 0;
}
