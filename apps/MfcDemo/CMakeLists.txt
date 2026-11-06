if(NOT MSVC OR NOT WIN32 OR NOT GLIM_HALCON_BUILD_SHARED)
	message(FATAL_ERROR "MFC demo requires Windows MSVC and shared GlimHalcon")
endif()
enable_language(RC)
set(CMAKE_MFC_FLAG 2)
add_executable(GlimMfcDemo WIN32 Demo.cpp Demo.rc resource.h AnalysisService.cpp AnalysisService.h)
target_compile_definitions(GlimMfcDemo PRIVATE _AFXDLL _MBCS _WIN32_WINNT=0x0A00)
target_compile_options(GlimMfcDemo PRIVATE /UUNICODE /U_UNICODE)
target_link_libraries(GlimMfcDemo PRIVATE GlimHalcon comctl32)
add_custom_command(TARGET GlimMfcDemo POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:GlimHalcon> $<TARGET_FILE_DIR:GlimMfcDemo>)

