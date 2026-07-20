#=====================================================================================
# GlimHalcon — MVTec HALCON 13 Regions/Features 재현 라이브러리
#
#   GlimHalcon      : static lib (Phase 1 — 외부 의존성 0)
#   GlimHalconTest  : 기준 도형 S1~S7 기댓값 대조 테스트 실행파일
#
# 빌드 예 (VS 2022 x64)
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release
#   build\Release\GlimHalconTest.exe
#=====================================================================================
cmake_minimum_required(VERSION 3.5)

project(GlimHalcon CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	# 사용처가 MFC 멀티바이트다. 유니코드 매크로를 켜지 않는다.
	add_definitions(-D_MBCS -D_CRT_SECURE_NO_WARNINGS)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra)
endif()

#-------------------------------------------------------------------------------------
# 라이브러리
#-------------------------------------------------------------------------------------
set(GLIM_HALCON_PUBLIC_HEADERS
	include/GlimHalcon.h
)

set(GLIM_HALCON_SOURCES
	src/Core/Region.h
	src/Core/Region.cpp
	src/Core/Contour.h
	src/Core/Contour.cpp
	src/Core/ConvexHull.h
	src/Core/ConvexHull.cpp
	src/Feature/FeatureContext.h
	src/Feature/FeatureContext.cpp
	src/Facade/Operators.cpp
)

add_library(GlimHalcon STATIC
	${GLIM_HALCON_PUBLIC_HEADERS}
	${GLIM_HALCON_SOURCES}
)

target_include_directories(GlimHalcon
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/Core
		${CMAKE_CURRENT_SOURCE_DIR}/src/Feature
)

source_group("include" FILES ${GLIM_HALCON_PUBLIC_HEADERS})
source_group("src" FILES ${GLIM_HALCON_SOURCES})

#-------------------------------------------------------------------------------------
# 테스트
#-------------------------------------------------------------------------------------
add_executable(GlimHalconTest
	test/Shapes.h
	test/TestMain.cpp
)

target_link_libraries(GlimHalconTest PRIVATE GlimHalcon)

target_include_directories(GlimHalconTest
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/test
)

enable_testing()
add_test(NAME GlimHalconShapes COMMAND GlimHalconTest)
