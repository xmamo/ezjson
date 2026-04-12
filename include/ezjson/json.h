#ifndef EZJSON_JSON_H
#define EZJSON_JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <ezjson/api.h>

EZJSON_BEGIN_DECLS

typedef enum Ezjson_Kind {
	EZJSON_KIND_OBJECT,
	EZJSON_KIND_ARRAY,
	EZJSON_KIND_STRING,
	EZJSON_KIND_NUMBER,
	EZJSON_KIND_TRUE,
	EZJSON_KIND_FALSE,
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
	};
};

struct Ezjson_KeyValue {
	Ezjson_String key;
	Ezjson_Value value;
};

EZJSON_API bool Ezjson_Read(FILE* stream, Ezjson_Value* json);

EZJSON_API bool Ezjson_Equal(const Ezjson_Value* left, const Ezjson_Value* right);

EZJSON_API void Ezjson_Destroy(Ezjson_Value* json);

EZJSON_END_DECLS

#endif
