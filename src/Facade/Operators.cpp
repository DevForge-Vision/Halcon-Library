//====================================================================================
// src/Facade/Operators.cpp — HALCON 오퍼레이터 5종
//
// 근거: docs/02_DEFINITIONS.md
//   §1 area_center / §2 contlength / §3 circularity / §4 compactness / §5 convexity
//
// 공통 규약
//   - 출력은 진입 즉시 0 으로 초기화한다.
//   - 모든 본문은 try-catch 로 감싼다. 예외는 공개 경계 밖으로 나가지 않는다.
//   - 빈 리전 / 0 나눗셈은 전부 0 (docs/02_DEFINITIONS.md §0.4).
//====================================================================================
#include "GlimHalcon.h"

#include "Region.h"
#include "FeatureContext.h"

#include <cmath>

namespace glim { namespace halcon {

namespace {

const double kPi = 3.14159265358979323846;

}	// anonymous namespace

//------------------------------------------------------------------------------------
// HALCON: area_center(Regions : : : Area, Row, Column)
//
//   Area = F                  (리전에 속한 픽셀 개수)
//   Row  = (1/F) * SUM(r_i)
//   Col  = (1/F) * SUM(c_i)
//
//   런렝스 누적으로 O(런 수). 열 좌표 합은 등차수열 합으로 상수시간 처리한다.
//------------------------------------------------------------------------------------
void AreaCenter(const Region& region, long& area, double& row, double& column)
{
	area = 0;
	row = 0.0;
	column = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;		// 빈 리전 규약

		core::FeatureContext context(data);
		area = context.GetArea();
		row = context.GetCenterRow();
		column = context.GetCenterColumn();
	}
	catch (...)
	{
		area = 0;
		row = 0.0;
		column = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: contlength(Regions : : : ContLength)     Assertion: ContLength >= 0
//
//   ContLength = (축평행 스텝 수 * 1) + (대각 스텝 수 * sqrt(2))
//
//   - 구멍(hole)의 윤곽은 포함하지 않는다 (원문 Attention).
//   - 픽셀 중심 체인(Freeman chain) 모델. 단일 픽셀 = 0.0 (C-1 확정).
//   - 연결성분이 여러 개면 각 성분의 외곽 윤곽 길이를 합산한다.
//------------------------------------------------------------------------------------
void ContLength(const Region& region, double& contLength)
{
	contLength = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);
		contLength = context.GetContourLength();

		if (!(contLength >= 0.0))	// NaN 방어. Assertion: ContLength >= 0
			contLength = 0.0;
	}
	catch (...)
	{
		contLength = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: circularity(Regions : : : Circularity)   Assertion: 0 <= C <= 1.0
//
//                F
//   C' = -------------------          max = 중심에서 모든 contour 픽셀까지의 최대 거리
//         max^2  *  pi
//
//   C  = min(1, C')                   ← 위로 1.0 에서 클리핑 (compactness 와 방향 반대!)
//
//   - 중심은 무게중심 (CI-2 확정), contour 는 외곽만 (CI-1 확정).
//   - max == 0 (단일 픽셀) 이면 0 나눗셈 → 0 을 반환한다.
//------------------------------------------------------------------------------------
void Circularity(const Region& region, double& circularity)
{
	circularity = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);

		const double area = static_cast<double>(context.GetArea());
		if (area <= 0.0)
			return;

		const double maxDistance = context.GetMaxContourDistance();
		if (maxDistance <= 0.0)
			return;		// 단일 픽셀 등 축퇴 → 0 나눗셈 방어

		const double denominator = maxDistance * maxDistance * kPi;
		if (denominator <= 0.0)
			return;

		double value = area / denominator;

		if (!(value == value))		// NaN 방어
			value = 0.0;
		if (value > 1.0)
			value = 1.0;			// min(1, C')
		if (value < 0.0)
			value = 0.0;

		circularity = value;
	}
	catch (...)
	{
		circularity = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: compactness(Regions : : : Compactness)   Assertion: C >= 1.0 || C == 0
//
//              L^2
//   C' = -------------------          L = contlength 결과
//         4  *  F  *  pi
//
//   C  = max(1, C')                   ← 아래로 1.0 에서 클리핑 (circularity 와 반대!)
//
//   - 원문 Description 에는 "clipped to 1.0" 라는 circularity 문장이 복붙돼 있으나
//     실제 수식과 Assertion 은 max 다. 수식/Assertion 을 따른다 (정의서 §4.3).
//   - 빈 리전은 0.
//------------------------------------------------------------------------------------
void Compactness(const Region& region, double& compactness)
{
	compactness = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);

		const double area = static_cast<double>(context.GetArea());
		if (area <= 0.0)
			return;		// F == 0 → 0

		const double length = context.GetContourLength();

		const double denominator = 4.0 * area * kPi;
		if (denominator <= 0.0)
			return;

		double value = (length * length) / denominator;

		if (!(value == value))		// NaN 방어
			value = 0.0;
		if (value < 1.0)
			value = 1.0;			// max(1, C')

		compactness = value;
	}
	catch (...)
	{
		compactness = 0.0;
	}
}

//------------------------------------------------------------------------------------
// HALCON: convexity(Regions : : : Convexity)       Assertion: C <= 1
//
//         F_o        (원본 리전의 면적 = 픽셀 개수)
//   C = -------
//         F_c        (convex hull 의 면적 = hull 을 래스터화한 픽셀 개수, CV-1 확정)
//
//   - 분자가 원본이다. 뒤집으면 항상 >= 1 이 되어 Assertion 을 위반한다.
//   - F_c 에 폴리곤 기하 면적(cv::contourArea)을 쓰면 정사각형에서 1.0203 이 나온다.
//------------------------------------------------------------------------------------
void Convexity(const Region& region, double& convexity)
{
	convexity = 0.0;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return;

		core::FeatureContext context(data);

		const double areaOriginal = static_cast<double>(context.GetArea());
		if (areaOriginal <= 0.0)
			return;

		const double areaHull = static_cast<double>(context.GetConvexHullArea());
		if (areaHull <= 0.0)
			return;		// 0 나눗셈 방어

		double value = areaOriginal / areaHull;

		if (!(value == value))		// NaN 방어
			value = 0.0;
		if (value < 0.0)
			value = 0.0;

		// 주의: 정의서 §5 에는 클리핑이 없다. Assertion(C <= 1) 은 서술일 뿐이므로
		// 여기서 1.0 으로 강제하지 않는다. 값이 1 을 넘으면 F_c 계산이 틀린 것이며
		// (예: 폴리곤 기하 면적 사용), 그 오류를 감추지 않기 위해 그대로 노출한다.
		convexity = value;
	}
	catch (...)
	{
		convexity = 0.0;
	}
}

}}	// namespace glim::halcon
