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
	PUBLIC FILE_SET HEADERS BASE_DIRS include FILES include/ezjson.h
	PRIVATE source/ezjson.c
)

if(BUILD_TESTING)
	add_executable(test-examples)
	target_sources(test-examples PRIVATE test/examples.c)
	target_link_libraries(test-examples PRIVATE ezjson)

	add_executable(test-strings)
	target_sources(test-strings PRIVATE test/strings.c)
	target_link_libraries(test-strings PRIVATE ezjson)

	add_executable(test-numbers)
	target_sources(test-numbers PRIVATE test/numbers.c)
	target_link_libraries(test-numbers PRIVATE ezjson)

	add_test(
		NAME test-examples
		COMMAND test-examples
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-strings
		COMMAND test-strings
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-numbers
		COMMAND test-numbers
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)
endif()

install(TARGETS ezjson EXPORT ezjson-exports FILE_SET HEADERS)
install(EXPORT ezjson-exports DESTINATION cmake FILE EzjsonTargets.cmake)
