//====================================================================================
// test/Shapes.h — 기준 도형 S1~S7 생성기
//
// 근거: docs/03_VERIFICATION.md §2
//   좌표는 HALCON 관례 (row, col), 좌상단이 (0, 0).
//
//   S1 정사각형 100x100 : row ∈ [0,99],  col ∈ [0,99]
//   S2 원판 R=50        : 중심 (50,50),  (r-50)² + (c-50)² <= 50²
//   S3 링(도넛)         : 중심 (50,50),  25² < (r-50)²+(c-50)² <= 50²
//   S4 가로선 1x100     : row = 50,      col ∈ [0,99]
//   S5 L자              : (좌팔) row ∈ [0,99] × col ∈ [0,49]
//                         ∪ (하팔) row ∈ [50,99] × col ∈ [50,99]
//   S6 단일 픽셀        : (50, 50)
//   S7 빈 리전          : 픽셀 없음
//====================================================================================
#ifndef GLIM_HALCON_TEST_SHAPES_H
#define GLIM_HALCON_TEST_SHAPES_H

#include <vector>

#include "GlimHalcon.h"

namespace shapes {

// 캔버스 크기: 모든 도형이 들어가도록 넉넉하게 잡는다 (row/col 0..120)
const int kCanvas = 121;

//------------------------------------------------------------------------------------
// 마스크 버퍼 헬퍼
//------------------------------------------------------------------------------------
class Mask
{
public:
	Mask(int width, int height)
		: m_width(width)
		, m_height(height)
		, m_data(static_cast<size_t>(width) * static_cast<size_t>(height), 0)
	{
	}

	void Set(int row, int col)
	{
		if (row < 0 || row >= m_height || col < 0 || col >= m_width)
			return;
		m_data[static_cast<size_t>(row) * static_cast<size_t>(m_width) + static_cast<size_t>(col)] = 1;
	}

	glim::halcon::Region ToRegion() const
	{
		return glim::halcon::Region::FromMask(&m_data[0], m_width, m_height, m_width);
	}

private:
	int m_width;
	int m_height;
	std::vector<unsigned char> m_data;
};

//------------------------------------------------------------------------------------
// S1 : 정사각형 100x100
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS1Square()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 99; ++r)
	{
		for (int c = 0; c <= 99; ++c)
			mask.Set(r, c);
	}
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S2 : 원판 R=50, 중심 (50,50)
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS2Disk()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 100; ++r)
	{
		for (int c = 0; c <= 100; ++c)
		{
			const int dr = r - 50;
			const int dc = c - 50;
			if (dr * dr + dc * dc <= 50 * 50)
				mask.Set(r, c);
		}
	}
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S3 : 링 — 외경 50, 내경 25
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS3Ring()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 100; ++r)
	{
		for (int c = 0; c <= 100; ++c)
		{
			const int dr = r - 50;
			const int dc = c - 50;
			const int d2 = dr * dr + dc * dc;
			if (d2 > 25 * 25 && d2 <= 50 * 50)
				mask.Set(r, c);
		}
	}
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S4 : 가로선 1x100 (row = 50)
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS4Line()
{
	Mask mask(kCanvas, kCanvas);
	for (int c = 0; c <= 99; ++c)
		mask.Set(50, c);
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S5 : L자
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS5LShape()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 99; ++r)			// 좌팔
	{
		for (int c = 0; c <= 49; ++c)
			mask.Set(r, c);
	}
	for (int r = 50; r <= 99; ++r)			// 하팔
	{
		for (int c = 50; c <= 99; ++c)
			mask.Set(r, c);
	}
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S6 : 단일 픽셀 (50, 50)
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS6SinglePixel()
{
	Mask mask(kCanvas, kCanvas);
	mask.Set(50, 50);
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S7 : 빈 리전
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS7Empty()
{
	return glim::halcon::Region();
}

}	// namespace shapes

#endif	// GLIM_HALCON_TEST_SHAPES_H
