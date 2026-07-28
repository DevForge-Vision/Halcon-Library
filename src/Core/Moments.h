//====================================================================================
// src/Core/Moments.h — 2차 중심 모멘트 코어 + 등가 타원
//
// 근거: docs/02_DEFINITIONS.md
//   §6  moments_region_2nd        (M11, M20, M02, Ia, Ib)   정규화 **없음**
//   §7  moments_region_2nd_invar  (M11, M20, M02)           **F^2** 로 정규화
//   §8  elliptic_axis             (Ra, Rb, Phi)             내부에서 **F** 로 정규화
//
// ⚠ 이 파일에서 가장 위험한 것은 **정규화 인자가 세 가지**라는 점이다.
//    하나라도 섞으면 2차 배치 5종이 통째로 틀린다. 그래서
//    **이 헤더는 비정규화 값만 만든다.** F / F^2 나눗셈은 각 오퍼레이터가 자기 자리에서 한다.
//    (elliptic_axis 만 예외로 여기서 F 정규화까지 한다 — Ra/Rb 가 그 정의의 일부이기 때문)
//
// ⚠ 픽셀 모델 (함정 #7)
//    픽셀은 면적 1의 사각형이 아니라 **픽셀 중심의 무한소 점**이다.
//    1픽셀 높이 가로 리전은 M20 = 0 → Ib = 0 → Rb = 0 이 된다. 정의이지 결함이 아니다.
//====================================================================================
#ifndef GLIM_HALCON_CORE_MOMENTS_H
#define GLIM_HALCON_CORE_MOMENTS_H

#include "Region.h"

//------------------------------------------------------------------------------------
// 미결 쟁점 스위치 (docs/02_DEFINITIONS.md §7.5 / §8.6 / §9.5 / §10.5)
//
// HALCON 실측이 가능해지면 docs/03_VERIFICATION.md §5B 의 판정 도형으로 확인하고
// 필요한 것만 뒤집는다. **기본값은 전부 "잠정 결론" 이다.**
// 이 헤더는 라이브러리 내부 전용이므로 매크로를 정의해도 사용처를 오염시키지 않는다.
//------------------------------------------------------------------------------------

// MI-1 : moments_region_2nd_invar 의 원점.
//        0 = 무게중심 (기본. §6 과 같은 원점)   /   1 = 바운딩박스 중심
//        판정 도형: S5 L자 (무게중심 (57.833, 41.167) vs bbox 중심 (49.5, 49.5))
#ifndef GLIM_HALCON_INVAR_ORIGIN_BBOX
#define GLIM_HALCON_INVAR_ORIGIN_BBOX 0
#endif

// EA-2 : elliptic_axis 의 Phi 가 정확히 -pi/2 로 떨어질 때.
//        1 = +pi/2 로 접는다 (기본. Assertion 은 -pi/2 < Phi 로 -pi/2 를 제외한다)
//        0 = 원문 수식 문자 그대로 -pi/2 를 둔다
//        판정 도형: S4T 세로선
#ifndef GLIM_HALCON_PHI_FOLD_HALF_PI
#define GLIM_HALCON_PHI_FOLD_HALF_PI 1
#endif

// EC-1 : Rb = 0 일 때 eccentricity 의 StructureFactor.
//        0 = 수식 그대로 (Anisometry=0, Bulkiness=0 → 0*0-1 = **-1**)  ← 기본
//        1 = "Rb 나눗셈에 의존하는 출력" 으로 보고 0 으로 덮는다
//        기본을 (a) 로 둔 이유: 틀렸을 때 -1 은 눈에 띄지만 0 은 빈 리전 규약에 묻혀 안 보인다
//        판정 도형: S4 가로선 / S6 단일픽셀
#ifndef GLIM_HALCON_STRUCTURE_FACTOR_ZERO_ON_RB0
#define GLIM_HALCON_STRUCTURE_FACTOR_ZERO_ON_RB0 0
#endif

// OR-1 : orientation_region 의 "회전 좌표계에서의 column 좌표" 부호.
//        proj = dc*cos(Phi) + (부호)*dr*sin(Phi)
//        -1 = 주축 단위벡터 (cos Phi, -sin Phi) 에 투영 (기본. Phi 정의에서 유도됨)
//        +1 = 반대 부호
//        판정 도형: S8 비대칭 아령
#ifndef GLIM_HALCON_ORIENTATION_PROJ_SIGN
#define GLIM_HALCON_ORIENTATION_PROJ_SIGN (-1)
#endif

// OR-2 : +pi 를 더한 뒤 [-pi, pi) 를 벗어난 값의 처리.
//        1 = 2pi 를 빼서 접는다 (기본. Assertion -pi <= Phi < pi 를 지키는 유일한 방법)
//        0 = 접지 않는다 (원문 문자 그대로)
#ifndef GLIM_HALCON_ORIENTATION_WRAP
#define GLIM_HALCON_ORIENTATION_WRAP 1
#endif

namespace glim { namespace halcon { namespace core {

//------------------------------------------------------------------------------------
// 2차 중심 모멘트 — moments_region_2nd 의 출력 5개 그대로.
//   **정규화 없음.** 순수 합이다 (docs/02_DEFINITIONS.md §6.3, 함정 #4/#6).
//
//   부호 규약: 원문은 (중심 - 픽셀) 이지만 2차 세 항은 (픽셀 - 중심) 과 값이 같다
//              (짝수 지수 / 음수 두 개 상쇄). 3차 홀수항에서만 실제로 갈린다.
//------------------------------------------------------------------------------------
struct CentralMoments2nd
{
	double m11;		// SUM (r0-r)(c0-c)
	double m20;		// SUM (r0-r)^2      행 의존
	double m02;		// SUM (c0-c)^2      열 의존
	double ia;		// h + sqrt(((m20-m02)/2)^2 + m11^2)     공분산 큰 고유값
	double ib;		// h - sqrt(...)                          공분산 작은 고유값

	CentralMoments2nd()
		: m11(0.0), m20(0.0), m02(0.0), ia(0.0), ib(0.0) {}
};

//------------------------------------------------------------------------------------
// elliptic_axis 의 출력 3개.
//------------------------------------------------------------------------------------
struct EllipseParams
{
	double ra;		// 주반지름  (Assertion: >= 0)
	double rb;		// 부반지름  (Assertion: 0 <= rb <= ra)
	double phi;		// x축(=column 축) 기준 라디안 (Assertion: -pi/2 < phi <= pi/2)

	EllipseParams()
		: ra(0.0), rb(0.0), phi(0.0) {}
};

//------------------------------------------------------------------------------------
// 2차 중심 모멘트를 계산한다. O(런 수) — 픽셀을 순회하지 않는다.
//
// 매개변수
//   region                 : 정규형 런렝스 리전
//   centerRow, centerColumn: 모멘트의 원점. **보통 무게중심**을 넣는다.
//                            무게중심이 아닌 값을 넣어도 그 원점 기준의 올바른 값이 나온다
//                            (일반식으로 구현했다. docs/02_DEFINITIONS.md §6.4 (2))
//   out (out)              : 진입 즉시 전 필드 0 으로 초기화된다
//
// 반환: 리전이 비었거나 면적이 0 이면 false (out 은 전 필드 0). 그 외 true.
// 예외: 던지지 않는다 (할당이 없다).
//------------------------------------------------------------------------------------
bool ComputeCentralMoments2nd(const RegionData& region,
							  double centerRow,
							  double centerColumn,
							  CentralMoments2nd& out);

//------------------------------------------------------------------------------------
// elliptic_axis 의 Ra / Rb / Phi 를 계산한다.
//
// 매개변수
//   moments : ComputeCentralMoments2nd 의 결과 (**비정규화 값**을 그대로 넣는다)
//   area    : 리전의 면적 F (픽셀 개수). 여기서 **F 로 나눈다** — EA-1 확정, F^2 아님
//   out (out): 진입 즉시 전 필드 0 으로 초기화된다
//
// 반환: area <= 0 이면 false (out 은 전 필드 0). 그 외 true.
// 예외: 던지지 않는다.
//------------------------------------------------------------------------------------
bool ComputeEllipseParams(const CentralMoments2nd& moments,
						  double area,
						  EllipseParams& out);

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_MOMENTS_H
