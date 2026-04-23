cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(ezjson LANGUAGES C)

include(CTest)

find_package(Doxygen)

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
	add_executable(test-numbers)
	target_sources(test-numbers PRIVATE test/numbers.c)
	target_link_libraries(test-numbers PRIVATE ezjson)

	add_executable(test-strings)
	target_sources(test-strings PRIVATE test/strings.c)
	target_link_libraries(test-strings PRIVATE ezjson)

	add_executable(test-arrays)
	target_sources(test-arrays PRIVATE test/arrays.c)
	target_link_libraries(test-arrays PRIVATE ezjson)

	add_executable(test-examples)
	target_sources(test-examples PRIVATE test/examples.c)
	target_link_libraries(test-examples PRIVATE ezjson)

	add_executable(test-pointers)
	target_sources(test-pointers PRIVATE test/pointers.c)
	target_link_libraries(test-pointers PRIVATE ezjson)

	add_test(
		NAME test-numbers
		COMMAND test-numbers
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-strings
		COMMAND test-strings
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-arrays
		COMMAND test-arrays
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-examples
		COMMAND test-examples
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)

	add_test(
		NAME test-pointers
		COMMAND test-pointers
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
	)
endif()

if(Doxygen_FOUND)
	set(DOXYGEN_ALWAYS_DETAILED_SEC YES)
	set(DOXYGEN_STRIP_FROM_PATH include)
	set(DOXYGEN_STRIP_FROM_INC_PATH include)
	set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
	set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
	set(DOXYGEN_SHOW_INCLUDE_FILES NO)
	set(DOXYGEN_SORT_MEMBER_DOCS NO)
	set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
	set(DOXYGEN_WARN_IF_INCOMPLETE_DOC NO)
	set(DOXYGEN_EXCLUDE_SYMBOLS EZJSON_API)
	set(DOXYGEN_VERBATIM_HEADERS NO)
	set(DOXYGEN_ALPHABETICAL_INDEX NO)
	set(DOXYGEN_HTML_COLORSTYLE TOGGLE)
	set(DOXYGEN_ENUM_VALUES_PER_LINE 1)
	set(DOXYGEN_MACRO_EXPANSION YES)
	set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
	set(DOXYGEN_PREDEFINED DOXYGEN)
	set(DOXYGEN_EXPAND_AS_DEFINED EZJSON_API)
	set(DOXYGEN_DOT_IMAGE_FORMAT svg)
	doxygen_add_docs(ezjson-doc include ALL)
endif()

install(TARGETS ezjson EXPORT ezjson-exports FILE_SET HEADERS)
install(EXPORT ezjson-exports DESTINATION cmake FILE EzjsonTargets.cmake)
