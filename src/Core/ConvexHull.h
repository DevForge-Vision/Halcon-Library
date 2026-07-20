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

}}}	// namespace glim::halcon::core

#endif	// GLIM_HALCON_CORE_CONVEXHULL_H
