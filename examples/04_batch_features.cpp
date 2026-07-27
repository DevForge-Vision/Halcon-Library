//====================================================================================
// examples/04_batch_features.cpp — 배치 API ComputeFeatures 사용법과 교차 검증
//
// 이 예제가 보여주는 것
//   1) ComputeFeatures 로 5종 값을 한 번에 뽑는 법
//   2) 배치 결과가 개별 오퍼레이터 5종 호출 결과와 **비트 단위로 같은지** 대조
//   3) 잘못된 stride(stride < width) 를 넘겼을 때의 동작 — 빈 리전
//
// 값이 하나라도 다르면 exit code 1 을 돌려준다 (회귀 감시용).
//
// 외부 의존성 0. 표준 라이브러리만 쓴다.
//
// 빌드 (MSVC 개발자 명령 프롬프트, 저장소 루트에서)
//   cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
//      src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\04_batch_features.cpp ^
//      /Fe:batch.exe
//====================================================================================
#include "GlimHalcon.h"

#include <cstddef>
#include <cstdio>
#include <vector>

using namespace glim::halcon;

namespace {

int gFailCount = 0;

//------------------------------------------------------------------------------------
// 마스크 버퍼 (examples/02 와 같은 형태)
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

const int kCanvas = 40;

Region MakeRectangle()
{
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

Region MakeRing()
{
	// 도넛: 바깥 R=15, 안쪽 R=7. 구멍 윤곽이 ContLength 에 섞이지 않는지 확인용.
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int row = 0; row < kCanvas; ++row)
	{
		for (int col = 0; col < kCanvas; ++col)
		{
			const int dr = row - 20;
			const int dc = col - 20;
			const int d2 = dr * dr + dc * dc;
			if (d2 <= 15 * 15 && d2 > 7 * 7)
				buffer.Set(row, col, 255);
		}
	}
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

Region MakeSinglePixel()
{
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	buffer.Set(20, 20, 255);
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

Region MakeThinLine()
{
	// 1 x 30 가로선 (1픽셀 두께 축퇴 케이스)
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int col = 5; col <= 34; ++col)
		buffer.Set(20, col, 255);
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

Region MakeTwoBlobs()
{
	// 떨어진 두 덩어리 — 연결성분이 2개인 경우
	MaskBuffer buffer(kCanvas, kCanvas, kCanvas);
	for (int row = 2; row <= 6; ++row)
	{
		for (int col = 2; col <= 6; ++col)
			buffer.Set(row, col, 255);
	}
	for (int row = 30; row <= 37; ++row)
	{
		for (int col = 28; col <= 35; ++col)
			buffer.Set(row, col, 255);
	}
	return Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
}

//------------------------------------------------------------------------------------
// 배치 vs 개별 — 완전 일치를 요구한다 (허용 오차 없음. 비트가 같아야 한다)
//------------------------------------------------------------------------------------
void CheckSame(const char* name, const Region& region)
{
	// 개별 경로
	long area = 0;
	double row = 0.0;
	double column = 0.0;
	double contLength = 0.0;
	double circularity = 0.0;
	double compactness = 0.0;
	double convexity = 0.0;

	AreaCenter (region, area, row, column);
	ContLength (region, contLength);
	Circularity(region, circularity);
	Compactness(region, compactness);
	Convexity  (region, convexity);

	// 배치 경로
	RegionFeatures f;
	ComputeFeatures(region, f);

	const bool same =
		(f.area == area) &&
		(f.row == row) &&
		(f.column == column) &&
		(f.contLength == contLength) &&
		(f.circularity == circularity) &&
		(f.compactness == compactness) &&
		(f.convexity == convexity);

	std::printf("%-16s %8ld %10.4f %10.4f %12.6f %11.9f %11.9f %11.9f   %s\n",
				name, f.area, f.row, f.column,
				f.contLength, f.circularity, f.compactness, f.convexity,
				same ? "MATCH" : "*** MISMATCH ***");

	if (!same)
	{
		++gFailCount;
		std::printf("   individual : %ld %.17g %.17g %.17g %.17g %.17g %.17g\n",
					area, row, column, contLength, circularity, compactness, convexity);
		std::printf("   batch      : %ld %.17g %.17g %.17g %.17g %.17g %.17g\n",
					f.area, f.row, f.column, f.contLength, f.circularity, f.compactness, f.convexity);
	}
}

void PrintHeader()
{
	std::printf("%-16s %8s %10s %10s %12s %11s %11s %11s   %s\n",
				"Shape", "Area", "Row", "Column", "ContLength", "Circular", "Compact", "Convex", "batch==each");
	std::printf("---------------------------------------------------------------------------------------------------------------\n");
}

//------------------------------------------------------------------------------------
// stride 검증 — 0 < stride < width 는 빈 리전이어야 한다.
//------------------------------------------------------------------------------------
void CheckStrideGuard()
{
	const int width = 40;
	const int height = 40;
	std::vector<unsigned char> raw(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 255);

	// (1) 정상: stride == width
	Region ok = Region::FromMask(&raw[0], width, height, width);

	// (2) 정상: stride <= 0 → width 로 간주 (기존 규약)
	Region zeroStride = Region::FromMask(&raw[0], width, height, 0);

	// (3) 비정상: stride < width → 빈 리전 (읽기를 시도하지 않는다)
	Region bad = Region::FromMask(&raw[0], width, height, width - 8);

	std::printf("stride = width (%d)      : IsEmpty=%s   (기대: false)\n", width, ok.IsEmpty() ? "true" : "false");
	std::printf("stride = 0               : IsEmpty=%s   (기대: false, width 로 간주)\n", zeroStride.IsEmpty() ? "true" : "false");
	std::printf("stride = %d (< width)    : IsEmpty=%s   (기대: true, 버퍼 침범 방어)\n", width - 8, bad.IsEmpty() ? "true" : "false");

	if (ok.IsEmpty() || zeroStride.IsEmpty() || !bad.IsEmpty())
	{
		++gFailCount;
		std::printf("*** stride guard FAILED ***\n");
	}

	// stride <= 0 이 width 와 같은 결과를 내는지도 확인한다 (기존 규약 회귀 감시)
	RegionFeatures a;
	RegionFeatures b;
	ComputeFeatures(ok, a);
	ComputeFeatures(zeroStride, b);
	if (a.area != b.area || a.row != b.row || a.column != b.column || a.contLength != b.contLength)
	{
		++gFailCount;
		std::printf("*** stride<=0 규약 회귀: width 로 간주되지 않았다 ***\n");
	}
}

}	// anonymous namespace

//====================================================================================
int main()
{
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 예제 04 - 배치 API(ComputeFeatures) 와 개별 오퍼레이터 교차 검증\n");
	std::printf("====================================================================================\n\n");

	std::printf("[배치 결과 = 개별 호출 결과] --------------------------------------------------------\n");
	PrintHeader();
	CheckSame("Rectangle 20x20", MakeRectangle());
	CheckSame("Disk R=10",       MakeDisk());
	CheckSame("L-Shape",         MakeLShape());
	CheckSame("Ring 15/7",       MakeRing());
	CheckSame("SinglePixel",     MakeSinglePixel());
	CheckSame("ThinLine 1x30",   MakeThinLine());
	CheckSame("TwoBlobs",        MakeTwoBlobs());
	CheckSame("Empty",           Region());
	std::printf("\n");

	std::printf("[stride 검증] ----------------------------------------------------------------------\n");
	CheckStrideGuard();
	std::printf("\n");

	std::printf("[쓰는 법] --------------------------------------------------------------------------\n");
	std::printf("  RegionFeatures f;\n");
	std::printf("  ComputeFeatures(region, f);\n");
	std::printf("  // f.area / f.row / f.column / f.contLength / f.circularity / f.compactness / f.convexity\n");
	std::printf("  개별 5회 호출은 윤곽 추적 4회 + hull 1회, ComputeFeatures 는 윤곽 추적 1회 + hull 1회.\n\n");

	if (gFailCount != 0)
	{
		std::printf("FAIL %d\n", gFailCount);
		return 1;
	}

	std::printf("ALL OK\n");
	return 0;
}
