#=====================================================================================
# GlimHalcon — MVTec HALCON 13 Regions/Features 재현 라이브러리
#
#   GlimHalcon      : DLL 기본 / static 선택 (외부 의존성 0)
#   GlimHalconTest  : 기준 도형 S1~S7 기댓값 대조 테스트 실행파일
#
# 빌드 예 (VS 2022 x64)
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   cmake --build build --config Release
#   build\Release\GlimHalconTest.exe
#=====================================================================================
cmake_minimum_required(VERSION 3.15)

project(GlimHalcon VERSION 0.0.3 LANGUAGES CXX)

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
	include/GlimImage.h
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
	src/Core/Holes.h
	src/Core/Holes.cpp
	src/Feature/FeatureContext.h
	src/Feature/FeatureContext.cpp
	src/Feature/RegionFeatures20.h
	src/Feature/RegionFeatures20.cpp
	src/Facade/Operators.cpp
	src/Facade/Inspection.cpp
	src/Facade/ImageAnalysis.cpp
	src/Facade/DefectGrouping.cpp
	src/Facade/RegionFeatures20.cpp
	src/Image/ImageResultInternal.h
	src/Image/ImageInternal.h
	src/Image/BmpReader.cpp
	src/Image/Segmentation.cpp
)

#-------------------------------------------------------------------------------------
# DLL (기본) / static lib 선택
#
#   SDK 배포를 위해 DLL 을 기본으로 한다. static 선택 시에도 사용처와 CRT 설정을 맞춘다.
#
#   DLL 로 빌드하면 GLIM_HALCON_EXPORTS 가 **이 타깃에만(PRIVATE)** 붙어
#   공개 심볼이 dllexport 된다. 사용처는 아무것도 정의하지 않는다
#   (이유는 include/GlimHalcon.h 상단 주석 참조).
#-------------------------------------------------------------------------------------
option(GLIM_HALCON_BUILD_SHARED "static lib 대신 DLL 로 빌드한다" ON)

if(GLIM_HALCON_BUILD_SHARED)
	add_library(GlimHalcon SHARED
		${GLIM_HALCON_PUBLIC_HEADERS}
		${GLIM_HALCON_SOURCES}
	)
	target_compile_definitions(GlimHalcon PRIVATE GLIM_HALCON_EXPORTS)

	# import lib 와 DLL 의 이름을 같게 유지한다 (GlimHalcon.lib / GlimHalcon.dll).
	set_target_properties(GlimHalcon PROPERTIES
		WINDOWS_EXPORT_ALL_SYMBOLS OFF		# dllexport 를 명시한 것만 내보낸다
	)
else()
	add_library(GlimHalcon STATIC
		${GLIM_HALCON_PUBLIC_HEADERS}
		${GLIM_HALCON_SOURCES}
	)
endif()

target_include_directories(GlimHalcon
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/Core
		${CMAKE_CURRENT_SOURCE_DIR}/src/Feature
		${CMAKE_CURRENT_SOURCE_DIR}/src/Image
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

add_executable(GlimImageTest test/ImageTest.cpp)
target_link_libraries(GlimImageTest PRIVATE GlimHalcon)
add_test(NAME GlimImageContract COMMAND GlimImageTest)
add_executable(GlimRegionFeatures20Test test/RegionFeatures20Test.cpp)
target_link_libraries(GlimRegionFeatures20Test PRIVATE GlimHalcon)
add_test(NAME GlimRegionFeatures20 COMMAND GlimRegionFeatures20Test)
add_executable(GlimDefectTest test/DefectTest.cpp)
target_link_libraries(GlimDefectTest PRIVATE GlimHalcon)
add_test(NAME GlimDefectGrouping COMMAND GlimDefectTest)
add_executable(GlimConcurrencyTest test/ConcurrencyTest.cpp)
find_package(Threads REQUIRED)
target_link_libraries(GlimConcurrencyTest PRIVATE GlimHalcon Threads::Threads)
add_test(NAME GlimConcurrentReads COMMAND GlimConcurrencyTest)

# Compile-only ABI checks for both header orders and caller packing modes.
if(MSVC)
	foreach(pack IN ITEMS 1 4 8)
		foreach(order IN ITEMS LEGACY_FIRST IMAGE_FIRST)
			set(abiTarget "GlimAbiPack${pack}_${order}")
			add_library(${abiTarget} OBJECT test/AbiPackingTest.cpp)
			target_include_directories(${abiTarget} PRIVATE include)
			target_compile_definitions(${abiTarget} PRIVATE ABI_TEST_PACK=${pack})
			if(order STREQUAL "IMAGE_FIRST")
				target_compile_definitions(${abiTarget} PRIVATE ABI_IMAGE_FIRST)
			endif()
		endforeach()
	endforeach()
endif()
add_executable(GlimContourFailureTest test/ContourFailureTest.cpp
	src/Core/Region.cpp src/Core/Contour.cpp)
target_include_directories(GlimContourFailureTest PRIVATE include src/Core)
add_test(NAME GlimContourFailure COMMAND GlimContourFailureTest)

if(MSVC AND GLIM_HALCON_BUILD_SHARED)
	set(consumerConventions Gd)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		list(APPEND consumerConventions Gz Gr)
	else()
		list(APPEND consumerConventions Gv)
	endif()
	foreach(convention IN LISTS consumerConventions)
		add_executable(GlimConsumer_${convention} test/DllConsumerTest.cpp)
		target_compile_options(GlimConsumer_${convention} PRIVATE /${convention})
		target_link_libraries(GlimConsumer_${convention} PRIVATE GlimHalcon)
		add_test(NAME GlimConsumer_${convention} COMMAND GlimConsumer_${convention})
	endforeach()
endif()

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
		06_image_blobs
		07_api_contract
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
	add_test(NAME GlimApiDocumentExample COMMAND 07_api_contract)
endif()

# Packaging is opt-in and never invoked automatically by this source change.
option(GLIM_HALCON_BUILD_MFC_DEMO "Build the optional Windows MFC SDK consumer" OFF)
if(GLIM_HALCON_BUILD_MFC_DEMO)
	add_subdirectory(apps/MfcDemo)
endif()
include(cmake/Packaging.cmake)
