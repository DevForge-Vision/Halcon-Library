//====================================================================================
// src/Core/ConvexHull.h — Andrew monotone chain + 스캔라인 래스터화
//
// 근거: docs/02_DEFINITIONS.md §5 (convexity)
//   - CV-1 확정: F_c 는 **hull 을 래스터화한 리전의 픽셀 개수**다.
//     폴리곤 기하 면적(신발끈/cv::contourArea)을 쓰면 정사각형에서 1.0203 이 나와
//     HALCON Assertion(Convexity <= 1) 을 위반한다.
//   - CV-2: hull 입력은 외곽 contour 픽셀 (결과 동일, 성능 선택)
//   - 래스터화 규칙: 경계 포함(inclusive). 각 행에서
//     ceil(xLeft - eps) .. floor(xRight + eps) 를 채운다 (eps = 1e-9)
//====================================================================================
#ifndef GLIM_HALCON_CORE_CONVEXHULL_H
#define GLIM_HALCON_CORE_CONVEXHULL_H

#include <vector>

#include "Contour.h"

namespace glim { namespace halcon { namespace core {

// 볼록 껍질 정점을 구한다 (Andrew monotone chain).
//   - 입력은 (row, col) 픽셀 좌표. 내부적으로 x = col, y = row 로 다룬다.
//   - 공선(collinear) 점은 제외되므로, 완전히 일직선인 입력은 정점 2개가 된다.
//   - 성공 시 true. 입력이 비면 false.
bool ComputeConvexHull(const std::vector<Point>& points, std::vector<Point>& hull);

// hull 폴리곤을 래스터화한 픽셀 개수 (경계 포함). 축퇴(점/선분) 입력도 처리한다.
long RasterizeHullArea(const std::vector<Point>& hull);

//------------------------------------------------------------------------------------
// HALCON: diameter_region — hull 정점 위의 최대 거리 점쌍 (회전 캘리퍼스)
//
// 근거: docs/02_DEFINITIONS.md §14
//   - 최대 거리 쌍은 **반드시 convex hull 의 정점 쌍**이다 (hull 밖에 점이 없다)
//   - 거리 비교는 **제곱거리를 long long 정수**로 한다. sqrt 는 마지막에 한 번뿐이다.
//     부동소수로 비교하면 동률 판정이 흔들려 DR-2 의 결정론이 깨진다
//   - DR-2 (동률 처리, 원문 미명시): 제곱거리가 **더 클 때만** 교체하고,
//     같으면 (Row1, Column1, Row2, Column2) 가 **사전순으로 더 작은** 쌍을 남긴다.
//     각 쌍은 point1 <= point2 (row 먼저, 같으면 col) 로 정규화한다.
//     이 규칙은 **순회 순서에 의존하지 않는다** — 전수 비교로 바꿔도 같은 답이 나온다
//
// 전제조건: hull 은 ComputeConvexHull 의 출력(공선 정점이 제거된 볼록 다각형)이어야 한다.
//
// 출력 (셋 다 진입 즉시 초기화된다)
//   point1, point2 : 최대 거리를 만드는 두 극점. 정점이 1개면 둘 다 그 점
//   diameter       : 두 점 사이의 유클리드 거리. 정점이 1개면 0.0 (DR-1: 픽셀 = 무한소 점)
//
// 반환: hull 이 비었으면 false (출력은 (0,0), (0,0), 0.0). 그 외 true.
// 예외: 던지지 않는다.
//------------------------------------------------------------------------------------
bool ComputeHullDiameter(const std::vector<Point>& hull,
						 Point& point1, Point& point2, double& diameter);

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_CONVEXHULL_H
