//====================================================================================
// src/Core/Holes.h — 구멍 검출 코어 (배경 런렝스 + union-find)
//
// 근거: docs/02_DEFINITIONS.md
//   §11 area_holes         (구멍 픽셀 총합)
//   §12 connect_and_holes  (NumConnected, NumHoles)
//   §13 euler_number       (NumConnected - NumHoles)
//
// ⚠ 이 파일에서 가장 위험한 것은 **전경과 배경의 연결성이 서로 반대여야 한다**는 점이다.
//    디지털 위상에서 둘에 같은 연결성을 주면 Jordan 곡선 정리가 깨져
//    "닫힌 고리 안이 바깥과 통하는" 모순이 생기고, NumHoles 와 EulerNumber 가
//    도형에 따라 제멋대로 나온다.
//
//        전경 8-연결  <->  배경 4-연결      (본 라이브러리)
//        전경 4-연결  <->  배경 8-연결
//
//    전경은 core::LabelConnectedRuns 가 8-연결로 고정돼 있으므로(C-2),
//    배경은 4-연결이 논리적 귀결이다. 원문에 명시가 없어 미결(HC-1)로 남긴다.
//
// ⚠ 메모리 — 이 코어의 존재 이유이기도 하다
//    구멍을 찾겠다고 바운딩박스 크기의 배경 이미지를 만들면 안 된다.
//    01_ARCHITECTURE.md §9 (2026-07-21) 에서 이미 제거한 사고가 그대로 재발한다
//    (풀프레임을 가로지르는 가는 스크래치에서 dense 라벨 이미지는 1.3 GB 였다).
//    **배경도 런렝스로 표현한다.** 배경 런 수 <= 2 x 전경 런 수이므로 O(런 수)다.
//====================================================================================
#ifndef GLIM_HALCON_CORE_HOLES_H
#define GLIM_HALCON_CORE_HOLES_H

#include "Region.h"

//------------------------------------------------------------------------------------
// 미결 쟁점 스위치 (docs/02_DEFINITIONS.md §11.5)
//
// src/Core/Moments.h 와 같은 방식이다. HALCON 실측이 가능해지면
// docs/03_VERIFICATION.md §5C 의 판정 도형으로 확인하고 필요하면 뒤집는다.
// 이 헤더는 라이브러리 내부 전용이므로 매크로를 정의해도 사용처를 오염시키지 않는다.
//------------------------------------------------------------------------------------

// HC-1 : 배경의 연결성.
//        0 = 4-연결 (기본. 전경 8-연결의 쌍대. 위상적으로 무모순)
//        1 = 8-연결 (전경과 같은 연결성 — 위상적으로 모순이지만 HALCON 이 그렇다면 이쪽)
//        판정 도형: S10 대각 다이아몬드
//            . X .
//            X . X      -> 배경 4-연결이면 NumHoles = 1, EulerNumber = 0
//            . X .         배경 8-연결이면 NumHoles = 0, EulerNumber = 1
#ifndef GLIM_HALCON_BACKGROUND_CONNECTIVITY_8
#define GLIM_HALCON_BACKGROUND_CONNECTIVITY_8 0
#endif

namespace glim { namespace halcon { namespace core {

//------------------------------------------------------------------------------------
// 구멍 검출 결과 — 세 오퍼레이터가 **이 하나의 결과**를 나눠 쓴다.
//
//   area_holes        -> areaHoles
//   connect_and_holes -> numConnected, numHoles
//   euler_number      -> numConnected - numHoles
//
// euler_number 를 따로 구현하면 언젠가 connect_and_holes 와 어긋난다.
// 원문이 오일러 수를 "두 값의 차" 로 정의하므로, 어긋나는 순간 원문 정의 자체를 어기는 것이다.
//------------------------------------------------------------------------------------
struct HoleCounts
{
	long long numConnected;		// 전경 연결성분 개수 (8-연결, HC-2)
	long long numHoles;			// 유계(bounded) 배경 성분 개수
	long long areaHoles;		// 유계 배경 성분의 픽셀 총합

	HoleCounts()
		: numConnected(0), numHoles(0), areaHoles(0) {}
};

//------------------------------------------------------------------------------------
// 구멍을 검출한다. O(런 수) — 픽셀도, 바운딩박스도 순회하지 않는다.
//
// 매개변수
//   region   : 정규형 런렝스 리전
//   out (out): 진입 즉시 전 필드 0 으로 초기화된다
//
// 반환: 리전이 비었으면 false (out 은 전 필드 0). 그 외 true.
// 예외: 던지지 않는다 (내부에서 잡아 false + 전 필드 0 으로 축약한다).
//
// 정의 (docs/02_DEFINITIONS.md §11.3)
//   구멍 = 리전의 배경 중 **무한 원점과 이어지지 않은** 성분 전체.
//   섬(구멍 안의 전경)은 전경이므로 구멍 면적에 들어가지 않는다 (HC-3 잠정).
//------------------------------------------------------------------------------------
bool ComputeHoleCounts(const RegionData& region, HoleCounts& out);

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_HOLES_H
