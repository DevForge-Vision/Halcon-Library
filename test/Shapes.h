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
//
// 2차 배치 판별 도형 (docs/03_VERIFICATION.md §2B)
//   S1b 정사각형 50x50  : row ∈ [0,49],  col ∈ [0,49]      — 스케일 불변성 (S1 과 짝)
//   S4T 세로선 100x1    : col = 50,      row ∈ [0,99]      — EA-2 판정
//   S8  비대칭 아령     : (작은 사각) row ∈ [0,9]   × col ∈ [0,9]
//                         ∪ (큰 사각) row ∈ [30,49] × col ∈ [30,49]
//                                                            — OR-1 / OR-3 판정
//
//   ⚠ S1~S7 은 **전부 대칭 도형**이라 orientation_region 의 +pi 분기를 판정할 수 없다
//     (최원 윤곽점이 동률이 되어 결과가 순회 순서에 좌우된다).
//     S8 은 두 사각형의 크기를 다르게 해 최원점이 (0,0) 하나로 유일해지도록 만든 도형이다.
//====================================================================================
#ifndef GLIM_HALCON_TEST_SHAPES_H
#define GLIM_HALCON_TEST_SHAPES_H

#include <cstddef>
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
		, m_data(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0)
	{
	}

	void Set(int row, int col)
	{
		if (row < 0 || row >= m_height || col < 0 || col >= m_width)
			return;
		m_data[static_cast<std::size_t>(row) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(col)] = 1;
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

//====================================================================================
// 2차 배치 판별 도형 (docs/03_VERIFICATION.md §2B)
//====================================================================================

//------------------------------------------------------------------------------------
// S1b : 정사각형 50x50 — S1 의 절반 크기.
//   목적: 스케일 불변성 판정.
//     moments_region_2nd       의 M20 : 520625  vs  S1 의 8332500  (약 16배 = s^4)
//     moments_region_2nd_invar 의 M20 : 0.0833  vs  S1 의 0.083325 (거의 동일)
//   닫힌 해: n x n 정사각형이면 M20_invar = (n^2 - 1) / (12·n^2)
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS1bSquare50()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 49; ++r)
	{
		for (int c = 0; c <= 49; ++c)
			mask.Set(r, c);
	}
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S4T : 세로선 100x1 (col = 50) — S4 의 전치.
//   목적: EA-2 판정 (Phi 가 정확히 -pi/2 로 떨어지는 유일한 도형).
//     M20 = 83325, M02 = 0 → atan2(+0, 음수) = +pi → Phi = -pi/2 → 접으면 +pi/2
//   덤으로 Rb = 0 이 가로/세로 양쪽에서 나오는지 확인한다.
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS4TVerticalLine()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 99; ++r)
		mask.Set(r, 50);
	return mask.ToRegion();
}

//------------------------------------------------------------------------------------
// S8 : 비대칭 아령 — 작은 사각(좌상) + 큰 사각(우하) 을 대각으로 배치.
//   목적: orientation_region 의 +pi 분기(OR-1) 판정.
//
//   설계 의도 (docs/03_VERIFICATION.md §2B)
//     - 크기가 다르므로 비대칭 → 최원 윤곽점이 (0,0) 하나로 **유일** (OR-3 회피)
//     - 대각 배치라 M20 == M02, M11 > 0 → Phi = -pi/4 가 정확히 떨어진다
//     - 좌표를 골라 Ra = 41, Ia = 210125, Ib = 14125 가 **정확한 정수**로 나온다
//     - 8-연결 성분이 2개 → 모멘트가 성분 분리와 무관한 전체 합인지 함께 확인된다
//
//   기댓값: F=500, 중심 (32.5, 32.5), M11=98000, M20=M02=112125,
//           Ra=41, Rb=sqrt(113), Phi=-pi/4, orientation=3pi/4
//------------------------------------------------------------------------------------
inline glim::halcon::Region MakeS8AsymmetricDumbbell()
{
	Mask mask(kCanvas, kCanvas);
	for (int r = 0; r <= 9; ++r)			// 작은 사각 10x10 (좌상)
	{
		for (int c = 0; c <= 9; ++c)
			mask.Set(r, c);
	}
	for (int r = 30; r <= 49; ++r)			// 큰 사각 20x20 (우하)
	{
		for (int c = 30; c <= 49; ++c)
			mask.Set(r, c);
	}
	return mask.ToRegion();
}

}	// namespace shapes

#endif	// GLIM_HALCON_TEST_SHAPES_H
