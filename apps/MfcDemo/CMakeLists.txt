if(NOT MSVC OR NOT WIN32 OR NOT GLIM_HALCON_BUILD_SHARED)
	message(FATAL_ERROR "MFC demo requires Windows MSVC and shared GlimHalcon")
endif()
enable_language(RC)
set(CMAKE_MFC_FLAG 2)
# Older Windows SDK rc.exe cannot read UTF-8 BOM reliably. Keep repository
# sources BOM-encoded, but generate BOM-free ASCII resource inputs for RC.
# These two files contain only ASCII; fail instead of silently corrupting text.
string(ASCII 239 187 191 resourceBom)
foreach(resourceFile IN ITEMS Demo.rc resource.h)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${resourceFile}")
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${resourceFile}" resourceText)
	string(REPLACE "${resourceBom}" "" resourceText "${resourceText}")
	if(resourceText MATCHES "[^\t\r\n -~]")
		message(FATAL_ERROR "RC input must be ASCII: ${resourceFile}")
	endif()
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${resourceFile}" "${resourceText}")
endforeach()
add_executable(GlimMfcDemo WIN32 Demo.cpp "${CMAKE_CURRENT_BINARY_DIR}/Demo.rc" resource.h AnalysisService.cpp AnalysisService.h)
target_compile_definitions(GlimMfcDemo PRIVATE _AFXDLL _MBCS _WIN32_WINNT=0x0A00)
target_compile_options(GlimMfcDemo PRIVATE /UUNICODE /U_UNICODE)
target_link_libraries(GlimMfcDemo PRIVATE GlimHalcon comctl32)
add_custom_command(TARGET GlimMfcDemo POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:GlimHalcon> $<TARGET_FILE_DIR:GlimMfcDemo>)

