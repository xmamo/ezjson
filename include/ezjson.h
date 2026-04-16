#ifndef EZJSON_H
#define EZJSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef enum Ezjson_Kind {
	EZJSON_OBJECT,
	EZJSON_ARRAY,
	EZJSON_STRING,
	EZJSON_NUMBER,
	EZJSON_BOOLEAN,
	EZJSON_NULL,
} Ezjson_Kind;

typedef struct Ezjson_Object {
	struct Ezjson_KeyValue* items;
	size_t length;
} Ezjson_Object;

typedef struct Ezjson_Array {
	struct Ezjson_Value* items;
	size_t length;
} Ezjson_Array;

typedef struct Ezjson_String {
	char* data;
	size_t length;
} Ezjson_String;

typedef struct Ezjson_Value {
	Ezjson_Kind kind;

	union {
		Ezjson_Object object;
		Ezjson_Array array;
		Ezjson_String string;
		double number;
		bool boolean;
	};
} Ezjson_Value;

typedef struct Ezjson_KeyValue {
	Ezjson_String key;
	Ezjson_Value value;
} Ezjson_KeyValue;

EZJSON_API bool Ezjson_ReadFile(FILE* file, Ezjson_Value* json);

EZJSON_API bool Ezjson_ReadMemory(const void* memory, size_t size, Ezjson_Value* json);

EZJSON_API bool Ezjson_Equals(const Ezjson_Value* left, const Ezjson_Value* right);

EZJSON_API Ezjson_Value* Ezjson_Lookup(const Ezjson_Value* json, const Ezjson_String* key);

EZJSON_API void Ezjson_Destroy(Ezjson_Value* json);

#ifdef __cplusplus
}
#endif

#endif
