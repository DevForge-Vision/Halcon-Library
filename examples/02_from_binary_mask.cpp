//====================================================================================
// examples/02_from_binary_mask.cpp — 이진 마스크 버퍼로 5종 특징값 전부 뽑기
//
// 이 예제가 보여주는 것
//   1) 임의 크기의 이진 마스크 버퍼를 Region 으로 만드는 법
//   2) 5종 오퍼레이터를 한 번에 호출해 표로 출력하는 법
//   3) stride(패딩이 있는 버퍼) 를 그대로 넘기는 법
//   4) 큰 이미지에서 ROI(부분 영역)만 잘라 넘기는 법 — 좌표 오프셋 주의
//
// 외부 의존성 0. 표준 라이브러리만 쓴다.
//
// 빌드 (MSVC 개발자 명령 프롬프트, 저장소 루트에서)
//   cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
//      src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\02_from_binary_mask.cpp ^
//      /Fe:frommask.exe
//====================================================================================
#include "GlimHalcon.h"

#include <cstddef>
#include <cstdio>
#include <vector>

using namespace glim::halcon;

namespace {

//------------------------------------------------------------------------------------
// 검사기에서 흔히 쓰는 형태의 마스크 버퍼 래퍼.
// 실제 코드에서는 카메라 버퍼나 이진화 결과 버퍼가 이 자리를 대신한다.
//------------------------------------------------------------------------------------
class MaskBuffer
{
public:
	MaskBuffer(int width, int height, int stride)
		: m_width(width)
		, m_height(height)
		, m_stride((stride > 0) ? stride : width)
		, m_data(static_cast<std::size_t>((stride > 0) ? stride : width) * static_cast<std::size_t>(height), 0)
	{
	}

	void Set(int row, int col, unsigned char value)
	{
		if (row < 0 || row >= m_height || col < 0 || col >= m_width)
			return;
		m_data[static_cast<std::size_t>(row) * static_cast<std::size_t>(m_stride) + static_cast<std::size_t>(col)] = value;
	}

	const unsigned char* GetData() const { return &m_data[0]; }
	int GetWidth()  const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetStride() const { return m_stride; }

private:
	int m_width;
	int m_height;
	int m_stride;
	std::vector<unsigned char> m_data;
};

//------------------------------------------------------------------------------------
// 5종 특징값을 한 줄로 출력한다.
//   오퍼레이터는 예외를 밖으로 던지지 않는다. 빈 리전이면 전부 0 이 나온다.
//------------------------------------------------------------------------------------
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

	std::printf("%-16s %8ld %10.4f %10.4f %12.4f %10.6f %10.6f %10.6f\n",
				name, area, centerRow, centerColumn,
				contLength, circularity, compactness, convexity);
}

void PrintHeader()
{
	std::printf("%-16s %8s %10s %10s %12s %10s %10s %10s\n",
				"Shape", "Area", "Row", "Column", "ContLength", "Circular", "Compact", "Convex");
	std::printf("--------------------------------------------------------------------------------------------\n");
}

//------------------------------------------------------------------------------------
// 도형 생성기 (모두 40x40 캔버스)
//------------------------------------------------------------------------------------
const int kCanvas = 40;

Region MakeRectangle()
{
	// row 5..24, col 10..29  → 20 x 20
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int row = 5; row <= 24; ++row)
	{
		for (int col = 10; col <= 29; ++col)
			buffer.Set(row, col, 255);
	}
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

Region MakeDisk()
{
	// 중심 (20, 20), 반지름 10
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int row = 0; row < kCanvas; ++row)
	{
		for (int col = 0; col < kCanvas; ++col)
		{
			const int dr = row - 20;
			const int dc = col - 20;
			if (dr * dr + dc * dc <= 10 * 10)
				buffer.Set(row, col, 255);
		}
	}
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

Region MakeLShape()
{
	// 세로팔 row 5..24 x col 5..14, 가로팔 row 15..24 x col 5..24
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int row = 5; row <= 24; ++row)
	{
		for (int col = 5; col <= 14; ++col)
			buffer.Set(row, col, 255);
	}
	for (int row = 15; row <= 24; ++row)
	{
		for (int col = 5; col <= 24; ++col)
			buffer.Set(row, col, 255);
	}
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

//------------------------------------------------------------------------------------
// stride 가 width 보다 큰(행 끝에 패딩이 있는) 버퍼도 그대로 넘길 수 있다.
// 패딩 영역에 쓰레기 값이 있어도 width 까지만 읽으므로 결과에 영향이 없다.
//------------------------------------------------------------------------------------
Region MakePaddedRectangle()
{
	const int width = 40;
	const int height = 40;
	const int stride = 48;			// 8바이트 패딩

	MaskBuffer buffer(width, height, stride);

	// 패딩 영역을 일부러 오염시킨다. Set 은 width 범위를 넘으면 무시하므로 직접 채운다.
	std::vector<unsigned char> raw(static_cast<std::size_t>(stride) * static_cast<std::size_t>(height), 0);
	for (int row = 5; row <= 24; ++row)
	{
		for (int col = 10; col <= 29; ++col)
			raw[static_cast<std::size_t>(row) * static_cast<std::size_t>(stride) + static_cast<std::size_t>(col)] = 255;

		for (int col = width; col < stride; ++col)
			raw[static_cast<std::size_t>(row) * static_cast<std::size_t>(stride) + static_cast<std::size_t>(col)] = 0xAB;	// 쓰레기
	}

	return Region::FromMask(&raw[0], width, height, stride);
}

//------------------------------------------------------------------------------------
// 큰 이미지에서 ROI 만 넘기는 경우.
//   시작 픽셀 포인터를 옮기고 stride 는 원본 그대로 둔다.
//   ⚠ 결과 좌표는 **ROI 로컬 좌표**다. 전역 좌표가 필요하면 ROI 원점을 더해야 한다.
//------------------------------------------------------------------------------------
void RunRoiSample()
{
	const int imageWidth = 200;
	const int imageHeight = 100;

	std::vector<unsigned char> image(static_cast<std::size_t>(imageWidth) * static_cast<std::size_t>(imageHeight), 0);

	// 전역 좌표 row 30..49, col 120..139 에 정사각형을 하나 그린다.
	for (int row = 30; row <= 49; ++row)
	{
		for (int col = 120; col <= 139; ++col)
			image[static_cast<std::size_t>(row) * static_cast<std::size_t>(imageWidth) + static_cast<std::size_t>(col)] = 255;
	}

	// ROI: 원점 (row 20, col 100), 크기 60 x 60
	const int roiRow = 20;
	const int roiCol = 100;
	const int roiWidth = 60;
	const int roiHeight = 60;

	const unsigned char* roiStart = &image[0]
		+ static_cast<std::size_t>(roiRow) * static_cast<std::size_t>(imageWidth)
		+ static_cast<std::size_t>(roiCol);

	Region roiRegion = Region::FromMask(roiStart, roiWidth, roiHeight, imageWidth);

	long area = 0;
	double localRow = 0.0;
	double localColumn = 0.0;
	AreaCenter(roiRegion, area, localRow, localColumn);

	std::printf("ROI 로컬 좌표   : Area=%ld  Row=%.4f  Column=%.4f\n", area, localRow, localColumn);
	std::printf("전역 좌표 환산  : Row=%.4f  Column=%.4f  (ROI 원점 %d, %d 를 더함)\n",
				localRow + roiRow, localColumn + roiCol, roiRow, roiCol);
}

}	// anonymous namespace

//====================================================================================
int main()
{
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 예제 02 - 이진 마스크 버퍼로 5종 특징값 뽑기\n");
	std::printf("====================================================================================\n\n");

	PrintHeader();
	PrintFeatures("Rectangle 20x20", MakeRectangle());
	PrintFeatures("Disk R=10",       MakeDisk());
	PrintFeatures("L-Shape",         MakeLShape());
	PrintFeatures("Padded(st=48)",   MakePaddedRectangle());
	PrintFeatures("Empty",           Region());
	std::printf("\n");

	std::printf("[ROI 로 잘라 넘기기] ----------------------------------------------------------------\n");
	RunRoiSample();
	std::printf("\n");

	std::printf("주의: Disk 의 ContLength 는 연속 원주 2*pi*R = %.4f 보다 약 5.5%% 큽니다.\n", 2.0 * 3.14159265358979323846 * 10.0);
	std::printf("      디지털 원의 8연결 체인이 갖는 원리적 성질입니다 (docs/04_IMPLEMENTATION_NOTES.md 3장).\n");

	return 0;
}
