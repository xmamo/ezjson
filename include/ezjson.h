/// @file

#ifndef EZJSON_H
#define EZJSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#if defined(__unix__) && defined(__GNUC__) && defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((visibility("default")))
#elif defined(__unix__) && defined(__GNUC__) && !defined(EZJSON_EXPORTS)
#define EZJSON_API
#elif defined(_WIN32) && defined(_MSC_VER) && defined(EZJSON_EXPORTS)
#define EZJSON_API __declspec(dllexport)
#elif defined(_WIN32) && defined(_MSC_VER) && !defined(EZJSON_EXPORTS)
#define EZJSON_API __declspec(dllimport)
#elif defined(_WIN32) && defined(__GNUC__) && defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((dllexport))
#elif defined(_WIN32) && defined(__GNUC__) && !defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((dllimport))
#else
#define EZJSON_API
#endif

/// @brief String And Size macro. Expands to the provided string literal and its size in code units,
/// excluding the terminating `NUL`.
#define EZJSON_SAS(string) (string), ((sizeof(string) - sizeof(*(string))) / sizeof(*(string)))

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Ezjson_Error {
	EZJSON_NO_ERROR,
	EZJSON_ARGUMENT_ERROR,
	EZJSON_LOCALE_ERROR,
	EZJSON_MEMORY_ERROR,
	EZJSON_SYNTAX_ERROR,
	EZJSON_DEPTH_ERROR,
	EZJSON_KEY_ERROR,
} Ezjson_Error;

typedef enum Ezjson_Kind {
	EZJSON_NULL,
	EZJSON_BOOLEAN,
	EZJSON_NUMBER,
	EZJSON_STRING,
	EZJSON_ARRAY,
	EZJSON_OBJECT,
} Ezjson_Kind;

typedef struct Ezjson_String {
	char* data;
	size_t size;  ///< @brief The size in bytes of the string, excluding the terminating `NUL`.
} Ezjson_String;

typedef struct Ezjson_Array {
	struct Ezjson_Value* items;
	size_t length;
} Ezjson_Array;

typedef struct Ezjson_Object {
	struct Ezjson_KeyValue* items;
	size_t length;
} Ezjson_Object;

typedef struct Ezjson_Value {
	Ezjson_Kind kind;

#ifndef DOXYGEN
	union {
#endif
		bool boolean;
		double number;
		Ezjson_String string;
		Ezjson_Array array;
		Ezjson_Object object;
#ifndef DOXYGEN
	};
#endif
} Ezjson_Value;

typedef struct Ezjson_KeyValue {
	Ezjson_String key;
	Ezjson_Value value;
} Ezjson_KeyValue;

/// @brief Read a JSON value from a file stream.
///
/// @details
/// Reads from the current position of @p file up to the end of the first JSON value. Leading and
/// trailing whitespace is permitted. The caller remains responsible for the stream; on success, the
/// caller is responsible for destroying @p json when it is no longer needed.
///
/// The parser enforces a maximum nesting depth of @p maxDepth. A leaf (`null`, `false`, `true`,
/// number, or string) accounts for a depth of 1; a branch (array or object) accounts for a depth of
/// 1 plus the maximum nesting depth among its children.
///
/// @param json Pointer to a cleared JSON value that will receive the parsed result.
/// @param file File to read. Must not be `NULL`.
/// @param maxDepth Maximum allowed JSON nesting depth.
/// @param error Optional pointer to the error value to be set on failure.
///
/// @return `true` on success, `false` on failure.
///
/// @exception EZJSON_ARGUMENT_ERROR An invalid argument was provided.
/// @exception EZJSON_LOCALE_ERROR The C locale could not be set for parsing.
/// @exception EZJSON_MEMORY_ERROR Insufficient memory available.
/// @exception EZJSON_SYNTAX_ERROR The input is not valid JSON.
/// @exception EZJSON_DEPTH_ERROR JSON nesting exceeds @p maxDepth.
///
/// @par Example
/// ```C
/// FILE* file = fopen("example.json", "r");
/// Ezjson_Error error = EZJSON_NO_ERROR;
/// Ezjson_Value json = {0};
/// bool ok = Ezjson_ReadFile(&json, file, 64, &error);
/// ```
EZJSON_API bool Ezjson_ReadFile(
	Ezjson_Value* json,
	FILE* file,
	size_t maxDepth,
	Ezjson_Error* error
);

/// @brief Reads a JSON value from memory.
///
/// @details
/// Reads from @p memory up to the end of the first JSON value. Leading and trailing whitespace is
/// permitted. On success, the caller is responsible for destroying @p json when it is no longer
/// needed.
///
/// The parser enforces a maximum nesting depth of @p maxDepth. A leaf (`null`, `false`, `true`,
/// number, or string) accounts for a depth of 1; a branch (array or object) accounts for a depth of
/// 1 plus the maximum nesting depth among its children.
///
/// @param json Pointer to a cleared JSON value that will receive the parsed result.
/// @param memory Memory to read. Must not be `NULL`, unless @p size is 0.
/// @param size Size of the memory to read.
/// @param maxDepth Maximum allowed JSON nesting depth.
/// @param error Optional pointer to the error value to be set on failure.
///
/// @return `true` on success, `false` on failure.
///
/// @exception EZJSON_ARGUMENT_ERROR An invalid argument was provided.
/// @exception EZJSON_LOCALE_ERROR The C locale could not be set for parsing.
/// @exception EZJSON_MEMORY_ERROR Insufficient memory available.
/// @exception EZJSON_SYNTAX_ERROR The input is not valid JSON.
/// @exception EZJSON_DEPTH_ERROR JSON nesting exceeds @p maxDepth.
///
/// @par Example
/// ```C
/// const char* example = "[202, 254, 186, 190]";
/// Ezjson_Error error = EZJSON_NO_ERROR;
/// Ezjson_Value json = {0};
/// bool ok = Ezjson_ReadMemory(&json, example, strlen(example), 64, &error);
/// ```
EZJSON_API bool Ezjson_ReadMemory(
	Ezjson_Value* json,
	const void* memory,
	size_t size,
	size_t maxDepth,
	Ezjson_Error* error
);

/// @brief Determines whether two JSON values are equal.
///
/// @details
/// Equality is checked recursively. The procedure stops if JSON nesting exceeds @p maxDepth. A leaf
/// (`null`, `false`, `true`, number, or string) accounts for a depth of 1; a branch (array or
/// object) accounts for a depth of 1 plus the maximum nesting depth among its children.
///
/// Objects with different key order are considered different. For an order-independent comparison,
/// sort each object's key-value pairs before invoking this function.
///
/// @param maxDepth Maximum allowed JSON nesting depth.
/// @param error Optional pointer to the error value to be set on failure.
///
/// @return `true` if the JSON values are equal, `false` if they differ or an error occurred.
///
/// @exception EZJSON_DEPTH_ERROR JSON nesting exceeds @p maxDepth.
EZJSON_API bool Ezjson_Equals(
	const Ezjson_Value* left,
	const Ezjson_Value* right,
	size_t maxDepth,
	Ezjson_Error* error
);

/// @brief Looks up the value associated with a key in a given JSON object.
///
/// @param json Pointer to a value of kind `EZJSON_OBJECT`.
/// @param key The key to search for. Must not be `NULL`, unless @p keySize is 0.
/// @param keySize The size in bytes of the key, excluding the terminating `NUL`.
/// @param error Optional pointer to the error value to be set on failure.
///
/// @return A pointer to the value associated to the key, or `NULL` on failure.
///
/// @exception EZJSON_ARGUMENT_ERROR An invalid argument was provided.
/// @exception EZJSON_KEY_ERROR The object did not contain the key.
///
/// @par Example
/// ```
/// FILE* file = fopen("example.json", "r");
/// Ezjson_Error error = EZJSON_NO_ERROR;
/// Ezjson_Value json = {0};
/// Ezjson_ReadFile(&json, file, 64, &error);
/// bool ok = Ezjson_Lookup(&json, EZJSON_SAS("score"), &error);
/// ```
EZJSON_API Ezjson_Value* Ezjson_Lookup(
	const Ezjson_Value* json,
	const char* key,
	size_t keySize,
	Ezjson_Error* error
);

/// @brief Addresses the value identified by the given JSON Pointer.
///
/// @param json The reference root. Must not be `NULL`.
/// @param jsonPointer The JSON Pointer string. Must not be `NULL`, unless @p jsonPointerSize is 0.
/// @param jsonPointerSize The size in bytes of the Pointer string, excluding the terminating `NUL`.
/// @param error Optional pointer to the error value to be set on failure.
///
/// @return A pointer to the identified value, or `NULL` on failure.
///
/// @exception EZJSON_ARGUMENT_ERROR An invalid argument was provided.
/// @exception EZJSON_SYNTAX_ERROR The JSON Pointer syntax is invalid.
/// @exception EZJSON_KEY_ERROR The JSON Pointer could not be resolved.
///
/// @par Example
/// ```
/// const char example[] = "{\"foo\": [69, {\"bar\": 420}]}";
/// Ezjson_Error error = EZJSON_NO_ERROR;
/// Ezjson_Value json = {0};
/// Ezjson_ReadMemory(&json, EZJSON_SAS(example), 64, &error);
/// Ezjson_Value* bar = Ezjson_At(&json, EZJSON_SAS("/foo/1/bar"), &error);
/// ```
EZJSON_API Ezjson_Value* Ezjson_At(
	const Ezjson_Value* json,
	const char* jsonPointer,
	size_t jsonPointerSize,
	Ezjson_Error* error
);

EZJSON_API bool Ezjson_Destroy(
	Ezjson_Value* json,
	size_t maxDepth,
	Ezjson_Error* error
);

#ifdef __cplusplus
}
#endif

#endif
