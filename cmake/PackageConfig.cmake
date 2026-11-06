# Prevent a Debug/RelWithDebInfo SDK being packaged with Release-only CRT files.
if(NOT CPACK_BUILD_CONFIG STREQUAL "Release")
	message(FATAL_ERROR "SDK packages require cpack -C Release")
endif()
