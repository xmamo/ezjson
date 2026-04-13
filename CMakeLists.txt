cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(ezjson LANGUAGES C)

include(CTest)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_NO_BUILTIN_CHRPATH OFF)

add_library(ezjson SHARED)
set_target_properties(ezjson PROPERTIES DEFINE_SYMBOL EZJSON_EXPORTS)
target_compile_features(ezjson PUBLIC c_std_11)

target_sources(ezjson
	PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
		include/ezjson/api.h
		include/ezjson/json.h

	PRIVATE
		source/ezjson/json.c
)

if(BUILD_TESTING)
	add_executable(test-example1)
	target_sources(test-example1 PRIVATE test/example1.c)
	target_link_libraries(test-example1 PRIVATE ezjson)

	add_executable(test-example2)
	target_sources(test-example2 PRIVATE test/example2.c)
	target_link_libraries(test-example2 PRIVATE ezjson)

	add_executable(test-strings)
	target_sources(test-strings PRIVATE test/strings.c)
	target_link_libraries(test-strings PRIVATE ezjson)

	add_test(
		NAME test-example1
		COMMAND test-example1
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-example2
		COMMAND test-example2
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-strings
		COMMAND test-strings
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)
endif()
