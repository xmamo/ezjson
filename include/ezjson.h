#ifndef EZJSON_H
#define EZJSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(EZJSON_EXPORTS)
#define EZJSON_API __declspec(dllexport)
#elif defined(_WIN32) && defined(_MSC_VER) && !defined(EZJSON_EXPORTS)
#define EZJSON_API __declspec(dllimport)
#elif defined(_WIN32) && defined(__GNUC__) && defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((dllexport))
#elif defined(_WIN32) && defined(__GNUC__) && !defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((dllimport))
#elif defined(__unix__) && defined(__GNUC__) && defined(EZJSON_EXPORTS)
#define EZJSON_API __attribute__((visibility("default")))
#else
#define EZJSON_API
#endif

typedef enum Ezjson_Kind {
	EZJSON_KIND_OBJECT,
	EZJSON_KIND_ARRAY,
	EZJSON_KIND_STRING,
	EZJSON_KIND_NUMBER,
	EZJSON_KIND_BOOL,
	EZJSON_KIND_NULL,
} Ezjson_Kind;

typedef struct Ezjson_Object Ezjson_Object;
typedef struct Ezjson_Array Ezjson_Array;
typedef struct Ezjson_String Ezjson_String;
typedef struct Ezjson_Value Ezjson_Value;
typedef struct Ezjson_KeyValue Ezjson_KeyValue;

struct Ezjson_Object {
	Ezjson_KeyValue* items;
	size_t length;
};

struct Ezjson_Array {
	Ezjson_Value* items;
	size_t length;
};

struct Ezjson_String {
	char* data;
	size_t length;
};

struct Ezjson_Value {
	Ezjson_Kind kind;

	union {
		Ezjson_Object object;
		Ezjson_Array array;
		Ezjson_String string;
		double number;
		bool boolean;
	};
};

struct Ezjson_KeyValue {
	Ezjson_String key;
	Ezjson_Value value;
};

EZJSON_API bool Ezjson_ReadFile(FILE* file, Ezjson_Value* json);

EZJSON_API bool Ezjson_ReadMemory(const void* memory, size_t size, Ezjson_Value* json);

EZJSON_API bool Ezjson_Equals(const Ezjson_Value* left, const Ezjson_Value* right);

EZJSON_API Ezjson_Value* Ezjson_Lookup(const Ezjson_Value* json, const Ezjson_String* key);

EZJSON_API void Ezjson_Destroy(Ezjson_Value* json);

#ifdef __cplusplus
}
#endif

#endif
