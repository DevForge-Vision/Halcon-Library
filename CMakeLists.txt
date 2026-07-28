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
	src/Core/Moments.h
	src/Core/Moments.cpp
	src/Feature/FeatureContext.h
	src/Feature/FeatureContext.cpp
	src/Facade/Operators.cpp
	src/Facade/Inspection.cpp
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

#-------------------------------------------------------------------------------------
# 예제 (docs/05_USER_GUIDE.md 에서 참조)
#   외부 의존성 0. 공개 헤더 하나만 include 한다.
#   끄려면: cmake -S . -B build -DGLIM_HALCON_BUILD_EXAMPLES=OFF
#-------------------------------------------------------------------------------------
option(GLIM_HALCON_BUILD_EXAMPLES "예제 실행파일을 빌드한다" ON)

if(GLIM_HALCON_BUILD_EXAMPLES)
	set(GLIM_HALCON_EXAMPLES
		01_quickstart
		02_from_binary_mask
		03_from_runs
		04_batch_features
		05_largest_blob
	)

	foreach(example ${GLIM_HALCON_EXAMPLES})
		add_executable(${example} examples/${example}.cpp)
		target_link_libraries(${example} PRIVATE GlimHalcon)
		set_target_properties(${example} PROPERTIES FOLDER "examples")
	endforeach()

	# 04 는 단순 예제가 아니라 **회귀 감시**다.
	# 배치 API 결과가 개별 오퍼레이터 결과와 어긋나면 exit code 1 을 돌려준다.
	add_test(NAME GlimHalconBatchFeatures COMMAND 04_batch_features)

	# 05 도 회귀 감시다.
	# 원샷 API 결과가 FromMask+SelectLargestBlob+ComputeFeatures 수동 조합과 어긋나거나,
	# 최대 blob 선택·입력 검증이 기대와 다르면 exit code 1 을 돌려준다.
	add_test(NAME GlimHalconLargestBlob COMMAND 05_largest_blob)
endif()
