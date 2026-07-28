//====================================================================================
// src/Core/Moments.cpp — 2차 중심 모멘트 코어 구현
//
// 근거: docs/02_DEFINITIONS.md §6.4 (구현 지침) / §8.5
//====================================================================================
#include "Moments.h"

#include <cstddef>
#include <cmath>

namespace glim { namespace halcon { namespace core {

namespace {

// 이 두 상수는 EA-2 접기 분기에서만 쓴다.
// 스위치를 0 으로 내렸을 때 g++ 의 -Wunused-variable 이 새로 뜨지 않도록
// 정의 자체를 같은 조건 안에 둔다 (경고 0 을 어느 설정에서도 유지한다).
#if GLIM_HALCON_PHI_FOLD_HALF_PI
const double kPi = 3.14159265358979323846;

// kHalfPi 는 kPi 의 정확히 절반이다 (2 로 나누는 것은 부동소수에서 오차가 없다).
// 그래서 atan2 가 돌려준 pi 의 -0.5 배와 **비트 단위로** 같아진다 — `<=` 비교가 정확히 성립한다.
const double kHalfPi = 1.57079632679489661923;
#endif

}	// anonymous namespace

//------------------------------------------------------------------------------------
// 2차 중심 모멘트 — O(런 수)
//
// 런 (row, colBegin..colEnd), n = colEnd - colBegin + 1, a = colBegin 일 때
//     SUM c   = n·a + n(n-1)/2                        등차수열 합
//     SUM c^2 = n·a^2 + a·n(n-1) + (n-1)n(2n-1)/6     제곱합 (Faulhaber)
// 로 런 하나를 상수시간에 처리한다. 픽셀 순회는 하지 않는다.
//
// ⚠ 정수 원점 시프트 (docs/02_DEFINITIONS.md §6.4 (2))
//    원시 모멘트를 그대로 모아 SUM c^2 - (SUM c)^2/F 로 빼면 자리수가 통째로 날아간다.
//    (53580 열 이미지의 작은 blob: SUM c^2 ~ 3e11 인데 결과는 1e3 규모)
//    그래서 무게중심에 가장 가까운 **격자점**으로 좌표를 먼저 옮기고,
//    남은 소수부 (fr, fc) 만 마지막에 보정한다.
//
//        dr = r - rs,  dc = c - cs,  fr = r0 - rs,  fc = c0 - cs   (|fr|,|fc| <= 0.5)
//        M20 = SUM dr^2 - 2·fr·SUM dr + fr^2·F
//        M02 = SUM dc^2 - 2·fc·SUM dc + fc^2·F
//        M11 = SUM dr·dc - fr·SUM dc - fc·SUM dr + fr·fc·F
//
//    (r0, c0) 가 정확한 무게중심이면 SUM dr = fr·F 라서 위 식은
//    M20 = SUM dr^2 - fr^2·F 로 축약되지만, **축약형을 쓰지 않는다.**
//    축약형은 "넘겨받은 중심이 진짜 무게중심" 이라는 전제에 의존하고,
//    그 전제가 깨지면 조용히 틀린 값을 낸다. 비용 차이는 누적 3개뿐이다.
//------------------------------------------------------------------------------------
bool ComputeCentralMoments2nd(const RegionData& region,
							  double centerRow,
							  double centerColumn,
							  CentralMoments2nd& out)
{
	out.m11 = 0.0;
	out.m20 = 0.0;
	out.m02 = 0.0;
	out.ia = 0.0;
	out.ib = 0.0;

	const std::vector<Run>& runs = region.GetRuns();
	if (runs.empty())
		return false;

	// NaN 이 들어오면 floor 도 NaN 이 되어 전 결과가 오염된다. 진입에서 막는다.
	if (!(centerRow == centerRow) || !(centerColumn == centerColumn))
		return false;

	const double shiftRow = std::floor(centerRow + 0.5);
	const double shiftCol = std::floor(centerColumn + 0.5);
	const double fr = centerRow - shiftRow;			// |fr| <= 0.5
	const double fc = centerColumn - shiftCol;		// |fc| <= 0.5

	double area = 0.0;
	double sumDr = 0.0;
	double sumDc = 0.0;
	double sumDr2 = 0.0;
	double sumDc2 = 0.0;
	double sumDrDc = 0.0;

	for (std::size_t i = 0; i < runs.size(); ++i)
	{
		const Run& run = runs[i];

		// 정규형이면 colEnd >= colBegin 이 보장되지만 방어한다.
		const double n = static_cast<double>(run.colEnd) - static_cast<double>(run.colBegin) + 1.0;
		if (n <= 0.0)
			continue;

		const double dr = static_cast<double>(run.row) - shiftRow;
		const double a = static_cast<double>(run.colBegin) - shiftCol;

		const double runSumDc = n * a + n * (n - 1.0) * 0.5;
		const double runSumDc2 = n * a * a
							   + a * n * (n - 1.0)
							   + (n - 1.0) * n * (2.0 * n - 1.0) / 6.0;

		area += n;
		sumDr += dr * n;
		sumDc += runSumDc;
		sumDr2 += dr * dr * n;
		sumDc2 += runSumDc2;
		sumDrDc += dr * runSumDc;
	}

	if (!(area > 0.0))
		return false;

	out.m20 = sumDr2 - 2.0 * fr * sumDr + fr * fr * area;
	out.m02 = sumDc2 - 2.0 * fc * sumDc + fc * fc * area;
	out.m11 = sumDrDc - fr * sumDc - fc * sumDr + fr * fc * area;

	// M20, M02 는 제곱합이므로 수학적으로 >= 0 이다. 음수는 부동소수 오차뿐이다.
	if (!(out.m20 >= 0.0))
		out.m20 = 0.0;
	if (!(out.m02 >= 0.0))
		out.m02 = 0.0;
	if (!(out.m11 == out.m11))
		out.m11 = 0.0;

	//--------------------------------------------------------------------------------
	// Ia / Ib — 공분산 행렬 [[M20, M11], [M11, M02]] 의 고유값
	//
	// 원문 형태 : Ia = h + sqrt(h^2 - M20·M02 + M11^2),  h = (M20+M02)/2
	// 항등 변형 : h^2 - M20·M02 + M11^2 == ((M20-M02)/2)^2 + M11^2
	//
	// **항등 변형으로 구현한다** (docs/02_DEFINITIONS.md §6.4 (3)).
	//   - 원문 형태는 M20 ~= M02 일 때 큰 두 수의 차라 자리수가 날아가고,
	//     대면적(M ~ 3e17)에서는 h^2 ~ 1e35 로 유효자리가 전멸한다
	//   - 원문 형태는 부동소수 오차로 **음수**가 되어 sqrt 가 NaN 을 낼 수 있다
	//   - 항등 변형은 두 제곱의 합이라 원리적으로 >= 0 이다
	// 값은 대수적으로 완전히 같다.
	//--------------------------------------------------------------------------------
	const double h = 0.5 * (out.m20 + out.m02);
	const double halfDiff = 0.5 * (out.m20 - out.m02);
	const double root = std::sqrt(halfDiff * halfDiff + out.m11 * out.m11);

	out.ia = h + root;
	out.ib = h - root;

	// 공분산 행렬은 반정치(positive semi-definite)이므로 Ib >= 0 이 보장된다.
	// 음수는 부동소수 오차뿐이다.
	if (!(out.ib >= 0.0))
		out.ib = 0.0;
	if (!(out.ia >= out.ib))
		out.ia = out.ib;

	return true;
}

//------------------------------------------------------------------------------------
// elliptic_axis (docs/02_DEFINITIONS.md §8.3)
//
//   M20n = M20/F      M02n = M02/F      M11n = M11/F        ← EA-1 확정: F 다. F^2 아니다
//   D    = sqrt((M20n - M02n)^2 + 4·M11n^2)
//   Ra   = sqrt(8·(M20n + M02n + D)) / 2
//   Rb   = sqrt(8·(M20n + M02n - D)) / 2
//   Phi  = -0.5 · atan2(2·M11n, M02n - M20n)                ← 함정 #11. 통념과 부호가 반대다
//------------------------------------------------------------------------------------
bool ComputeEllipseParams(const CentralMoments2nd& moments,
						  double area,
						  EllipseParams& out)
{
	out.ra = 0.0;
	out.rb = 0.0;
	out.phi = 0.0;

	if (!(area > 0.0))
		return false;		// 빈 리전 규약

	const double m20n = moments.m20 / area;
	const double m02n = moments.m02 / area;
	const double m11n = moments.m11 / area;

	const double diff = m20n - m02n;
	const double disc = std::sqrt(diff * diff + 4.0 * m11n * m11n);

	double innerA = 8.0 * (m20n + m02n + disc);
	double innerB = 8.0 * (m20n + m02n - disc);

	// innerB 는 수학적으로 >= 0 이지만 축퇴 도형(직선/단일점)에서 -1e-13 같은 값이 나온다.
	// 그대로 sqrt 하면 NaN 이다. (NaN > 0 은 거짓이므로 NaN 도 여기서 걸린다)
	if (!(innerA > 0.0))
		innerA = 0.0;
	if (!(innerB > 0.0))
		innerB = 0.0;

	out.ra = std::sqrt(innerA) * 0.5;
	out.rb = std::sqrt(innerB) * 0.5;

	// Assertion: Rb <= Ra. 부동소수 오차 방어.
	if (out.rb > out.ra)
		out.rb = out.ra;

	double phi = -0.5 * std::atan2(2.0 * m11n, m02n - m20n);

#if GLIM_HALCON_PHI_FOLD_HALF_PI
	// EA-2 : 세로 1픽셀 리전은 atan2(+0, 음수) = +pi 라 Phi = -pi/2 가 된다.
	//        그런데 Assertion 은 -pi/2 < Phi 로 -pi/2 를 **제외**한다.
	//        -pi/2 와 +pi/2 는 같은 축이므로 반개구간의 대표값 +pi/2 로 접는다.
	if (phi <= -kHalfPi)
		phi += kPi;
#endif

	if (!(phi == phi))
		phi = 0.0;			// NaN 방어

	// -0.0 을 +0.0 으로 정규화한다. (x + 0.0 은 x == -0.0 일 때만 값이 바뀐다)
	out.phi = phi + 0.0;
	return true;
}

}}}	// namespace glim::halcon::core
