#ifdef __unix__
#define _DEFAULT_SOURCE
#endif

#include "ezjson.h"

#include <assert.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#ifdef __unix__
typedef FILE Stream;
#define StreamGet fgetc
#else
typedef struct Stream {
	int (*get)(void* stream);
#ifdef _WIN32
	_locale_t locale;
#endif
} Stream;

typedef struct FileStream {
	Stream super;
	FILE* file;
} FileStream;

typedef struct MemoryStream {
	Stream super;
	const unsigned char* data;
	size_t size;
} MemoryStream;

static int StreamGet(Stream* stream) {
	assert(stream != NULL && stream->get != NULL);
	return stream->get(stream);
}

static int FileStreamGet(void* stream) {
	FileStream* fileStream = stream;
	return fgetc(fileStream->file);
}

static int MemoryStreamGet(void* stream) {
	MemoryStream* memoryStream = stream;

	if (memoryStream->size != 0) {
		memoryStream->data += 1;
		memoryStream->size -= 1;
		return memoryStream->data[-1];
	} else {
		return EOF;
	}
}
#endif

static bool GrowVec(
	void** data,
	size_t size,
	size_t* length,
	size_t* capacity,
	size_t delta,
	Ezjson_Error* error
) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);
	assert(error != NULL);

	if (*length > SIZE_MAX - delta) {
		*error = EZJSON_MEMORY_ERROR;
		return false;
	}

	size_t newLength = *length + delta;
	size_t newCapacity = *capacity;
	void* newData = *data;

	while (newLength > newCapacity) {
		if (newCapacity > SIZE_MAX / 2) {
			*error = EZJSON_MEMORY_ERROR;
			return false;
		}

		newCapacity = newCapacity != 0 ? 2 * newCapacity : 1;
	}

	if (newCapacity > *capacity) {
		if (newCapacity > SIZE_MAX / size) {
			*error = EZJSON_MEMORY_ERROR;
			return false;
		}

		if ((newData = realloc(*data, size * newCapacity)) == NULL) {
			*error = EZJSON_MEMORY_ERROR;
			return false;
		}

		*data = newData;
		*capacity = newCapacity;
	}

	*length = newLength;
	return true;
}

static bool GrowCharVec(
	char** data,
	size_t* size,
	size_t* capacity,
	size_t delta,
	Ezjson_Error* error
) {
	assert(data != NULL);
	assert(size != NULL);
	assert(capacity != NULL);
	assert(error != NULL);

	void* p = *data;
	bool ok = GrowVec(&p, sizeof(char), size, capacity, delta, error);
	*data = p;
	return ok;
}

static bool GrowValueVec(
	Ezjson_Value** data,
	size_t* length,
	size_t* capacity,
	size_t delta,
	Ezjson_Error* error
) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);
	assert(error != NULL);

	void* p = *data;
	bool ok = GrowVec(&p, sizeof(Ezjson_Value), length, capacity, delta, error);
	*data = p;
	return ok;
}

static bool GrowKeyValueVec(
	Ezjson_KeyValue** data,
	size_t* length,
	size_t* capacity,
	size_t delta,
	Ezjson_Error* error
) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);
	assert(error != NULL);

	void* p = *data;
	bool ok = GrowVec(&p, sizeof(Ezjson_KeyValue), length, capacity, delta, error);
	*data = p;
	return ok;
}

static bool AppendCu(
	char** data,
	size_t* size,
	size_t* capacity,
	char32_t cu,
	Ezjson_Error* error
) {
	assert(data != NULL);
	assert(size != NULL);
	assert(capacity != NULL);
	assert(cu <= 0x1FFFFF);

	if (cu <= 0x7F) {
		if (!GrowCharVec(data, size, capacity, 1, error)) return false;
		(*data)[*size - 2] = 0x00 | ((cu >> 0) & 0x7F);
		return true;
	}

	if (cu <= 0x7FF) {
		if (!GrowCharVec(data, size, capacity, 2, error)) return false;
		(*data)[*size - 3] = 0xC0 | ((cu >> 6) & 0xFF);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	if (cu <= 0xFFFF) {
		if (!GrowCharVec(data, size, capacity, 3, error)) return false;
		(*data)[*size - 4] = 0xE0 | ((cu >> 12) & 0x0F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	if (cu <= 0x1FFFFF) {
		if (!GrowCharVec(data, size, capacity, 4, error)) return false;
		(*data)[*size - 5] = 0xF0 | ((cu >> 18) & 0x07);
		(*data)[*size - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	return false;
}

static bool DestroyString(Ezjson_String* string) {
	assert(string != NULL);
	free(string->data);
	*string = (Ezjson_String){0};
	return true;
}

static bool Ezjson_DestroyValue(Ezjson_Value* json, size_t depth, Ezjson_Error* error);

static bool DestroyArray(Ezjson_Array* array, size_t depth, Ezjson_Error* error) {
	assert(array != NULL && (array->length != 0 || array->items == NULL));
	assert(depth != 0);
	assert(error != NULL);

	for (size_t i = 0; i < array->length; ++i) {
		if (!Ezjson_DestroyValue(&array->items[i], depth - 1, error))
			return false;
	}

	free(array->items);
	*array = (Ezjson_Array){0};
	return true;
}

static bool DestroyObject(Ezjson_Object* object, size_t depth, Ezjson_Error* error) {
	assert(object != NULL && (object->length != 0 || object->items == NULL));
	assert(depth != 0);
	assert(error != NULL);

	for (size_t i = 0; i < object->length; ++i) {
		Ezjson_KeyValue* item = &object->items[i];
		free(item->key.data);
		item->key = (Ezjson_String){0};

		if (!Ezjson_DestroyValue(&item->value, depth - 1, error))
			return false;
	}

	free(object->items);
	*object = (Ezjson_Object){0};
	return true;
}

static bool Ezjson_DestroyValue(Ezjson_Value* value, size_t depth, Ezjson_Error* error) {
	assert(value != NULL);
	assert(error != NULL);

	if (depth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	if (value->kind == EZJSON_STRING) {
		return DestroyString(&value->string);
	} else if (value->kind == EZJSON_ARRAY) {
		return DestroyArray(&value->array, depth, error);
	} else if (value->kind == EZJSON_OBJECT) {
		return DestroyObject(&value->object, depth, error);
	} else {
		return true;
	}
}

static void SkipWs(Stream* stream, int* c) {
	assert(stream != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));

	while (*c != EOF && strchr(" \t\n\r", *c) != NULL) {
		*c = StreamGet(stream);
	}
}

static bool ReadNumeral(Stream* stream, char** numeral, int* c, Ezjson_Error* error) {
	assert(stream != NULL);
	assert(numeral != NULL && *numeral == NULL);
	assert(c != NULL && *c != EOF && strchr("0123456789-", *c) != NULL);
	assert(error != NULL);

	size_t size = 0;
	size_t capacity = 0;
	if (!GrowCharVec(numeral, &size, &capacity, 2, error)) return error;
	(*numeral)[0] = (char)*c;

	// Data is now: [0-9-]

	if (*c == (unsigned char)'-') {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?[0-9]

	if (*c != (unsigned char)'0') {
		do {
			if ((*c = StreamGet(stream)) == EOF || strchr("0123456789.eE", *c) == NULL) {
				// Data is now: -?[1-9][0-9]*
				goto Success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr(".eE", *c) == NULL);
	} else {
		if ((*c = StreamGet(stream)) == EOF || strchr(".eE", *c) == NULL) {
			// Data is now: -?0
			goto Success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?([1-9][0-9]*|0)[.eE]

	if (*c == (unsigned char)'.') {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
		(*numeral)[size - 2] = (char)*c;

		do {
			if ((*c = StreamGet(stream)) == EOF || strchr("0123456789eE", *c) == NULL) {
				// Data is now: -?([1-9][0-9]*|0)\.[0-9]+
				goto Success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr("eE", *c) == NULL);
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE]

	if ((*c = StreamGet(stream)) == EOF || strchr("0123456789+-", *c) == NULL) {
		*error = EZJSON_SYNTAX_ERROR;
		goto Error;
	}

	if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
	(*numeral)[size - 2] = (char)*c;

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][0-9+-]

	if (strchr("+-", *c) != NULL) {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]

	while (true) {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]+
			goto Success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Error;
		(*numeral)[size - 2] = (char)*c;
	}

Success:
	(*numeral)[size - 1] = '\0';
	char* newNumeral = realloc(*numeral, sizeof(char) * size);
	if (newNumeral != NULL) *numeral = newNumeral;

	SkipWs(stream, c);
	return true;

Error:
	free(*numeral);
	*numeral = NULL;
	return false;
}

static bool ReadEscape(Stream* stream, char* v, char16_t* cu, int* c, Ezjson_Error* error) {
	assert(stream != NULL);
	assert(v != NULL && *v == '\0');
	assert(cu != NULL && *cu == 0);
	assert(c != NULL && *c == (unsigned char)'\\');
	assert(error != NULL);

	static const char escapeKey[] =
		"\"\\/bfnrt";

	static const char escapeValue[] =
		"\"\\/\b\f\n\r\t";

	static const char hexKey[] =
		"0123456789"
		"abcdef"
		"ABCDEF";

	static const unsigned short hexValue[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	};

	if ((*c = StreamGet(stream)) == EOF) {
		*error = EZJSON_SYNTAX_ERROR;
		return false;
	}

	const char* p = strchr(escapeKey, *c);

	if (p != NULL) {
		*v = escapeValue[p - escapeKey];
		return true;
	}

	if (*c != (unsigned char)'u') {
		*error = EZJSON_SYNTAX_ERROR;
		return false;
	}

	for (int i = 0; i < 4; ++i) {
		if ((*c = StreamGet(stream)) == EOF || (p = strchr(hexKey, *c)) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		*cu = (*cu << 4) | hexValue[p - hexKey];
	}

	return true;
}

static bool ReadString(Stream* stream, Ezjson_String* string, int* c, Ezjson_Error* error) {
	assert(stream != NULL);
	assert(string != NULL && string->data == NULL && string->size == 0);
	assert(c != NULL && *c == (unsigned char)'"');
	assert(error != NULL);

	size_t capacity = 0;
	if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) return error;

	while (true) {
		*c = StreamGet(stream);

	Top:
		if (*c == EOF || *c <= 0x1F) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (*c == (unsigned char)'"')
			break;

		if (*c != (unsigned char)'\\') {
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Error;
			string->data[string->size - 2] = (char)*c;
			continue;
		}

		char v1 = '\0';
		char16_t cu1 = 0;
		if (!ReadEscape(stream, &v1, &cu1, c, error)) goto Error;

		if (cu1 == 0) {
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Error;
			string->data[string->size - 2] = v1;
			continue;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'\\' || cu1 < 0xD800 || cu1 > 0xDBFF) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Error;
			goto Top;
		}

		char v2 = '\0';
		char16_t cu2 = 0;
		if (!ReadEscape(stream, &v2, &cu2, c, error)) goto Error;

		if (cu2 == 0) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Error;
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Error;
			string->data[string->size - 2] = v2;
			continue;
		}

		if (cu2 < 0xDC00 || cu2 > 0xDFFF) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Error;
			if (!AppendCu(&string->data, &string->size, &capacity, cu2, error)) goto Error;
			continue;
		}

		char32_t cp = 0x10000 + (((cu1 & UINT32_C(0x3FF)) << 10) | (cu2 & UINT32_C(0x3FF)));
		if (!AppendCu(&string->data, &string->size, &capacity, cp, error)) goto Error;
	}

	string->data[string->size - 1] = '\0';
	char* newData = realloc(string->data, sizeof(char) * string->size);
	if (newData != NULL) string->data = newData;
	string->size -= 1;

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

Error:
	DestroyString(string);
	return false;
}

static bool ReadValue(
	Stream* stream,
	Ezjson_Value* value,
	int* c,
	size_t depth,
	Ezjson_Error* error
);

static bool ReadArray(
	Stream* stream,
	Ezjson_Array* array,
	int* c,
	size_t depth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(array != NULL && array->items == NULL && array->length == 0);
	assert(c != NULL && *c == '[');
	assert(error != NULL);

	if (depth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	if (*c == (unsigned char)']') return true;

	size_t capacity = 0;

	while (true) {
		Ezjson_Value item = {0};
		if (!ReadValue(stream, &item, c, depth - 1, error)) goto Error;

		if (*c == EOF) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (!GrowValueVec(&array->items, &array->length, &capacity, 1, error)) goto Error;
		array->items[array->length - 1] = item;

		if (*c == (unsigned char)']')
			break;

		if (*c != (unsigned char)',') {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		*c = StreamGet(stream);
	}

	if (array->length != 0) {
		assert(array->length <= SIZE_MAX / sizeof(Ezjson_Value));
		Ezjson_Value* newItems = realloc(array->items, sizeof(Ezjson_Value) * array->length);
		if (newItems != NULL) array->items = newItems;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

Error:
	free(array->items);
	*array = (Ezjson_Array){0};
	return false;
}

static bool ReadObject(
	Stream* stream,
	Ezjson_Object* object,
	int* c,
	size_t depth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(object != NULL && object->items == NULL && object->length == 0);
	assert(c != NULL && *c == (unsigned char)'{');
	assert(error != NULL);

	if (depth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	if (*c == (unsigned char)'}') return true;

	size_t capacity = 0;

	while (true) {
		if (*c != (unsigned char)'"') {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		if (!GrowKeyValueVec(&object->items, &object->length, &capacity, 1, error)) goto Error;
		object->items[object->length - 1] = (Ezjson_KeyValue){0};

		if (!ReadString(stream, &object->items[object->length - 1].key, c, error))
			goto Error;

		if (*c != (unsigned char)':') {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		*c = StreamGet(stream);
		if (!ReadValue(stream, &object->items[object->length - 1].value, c, depth - 1, error)) goto Error;

		if (*c == (unsigned char)'}')
			break;

		if (*c != (unsigned char)',') {
			*error = EZJSON_SYNTAX_ERROR;
			goto Error;
		}

		*c = StreamGet(stream);
		SkipWs(stream, c);
	}

	if (object->length != 0) {
		assert(object->length <= SIZE_MAX / sizeof(Ezjson_KeyValue));
		Ezjson_KeyValue* newItems = realloc(object->items, sizeof(Ezjson_KeyValue) * object->length);
		if (newItems != NULL) object->items = newItems;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

Error:
	DestroyObject(object, SIZE_MAX, &(Ezjson_Error){EZJSON_SUCCESS});
	return false;
}

static bool ReadValue(
	Stream* stream,
	Ezjson_Value* value,
	int* c,
	size_t depth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(value != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));
	assert(error != NULL);

	SkipWs(stream, c);

	if (*c == (unsigned char)'n') {
		value->kind = EZJSON_NULL;

		if ((*c = StreamGet(stream)) != (unsigned char)'u') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'l') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'l') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'f') {
		*value = (Ezjson_Value){EZJSON_BOOLEAN, .boolean = false};

		if ((*c = StreamGet(stream)) != (unsigned char)'a') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'l') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'s') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'e') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'t') {
		*value = (Ezjson_Value){EZJSON_BOOLEAN, .boolean = true};

		if ((*c = StreamGet(stream)) != (unsigned char)'r') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'u') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'e') {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c != EOF && strchr("0123456789-", *c) != NULL) {
		*value = (Ezjson_Value){EZJSON_NUMBER, .number = 0.0};

		char* numeral = NULL;
		if (!ReadNumeral(stream, &numeral, c, error)) return false;

		char* end = numeral;
#ifdef _WIN32
		value->number = _strtod_l(numeral, &end, stream->locale);
#else
		value->number = strtod(numeral, &end);
#endif
		bool ok = end > numeral && *end == '\0';
		free(numeral);

		if (ok) {
			return true;
		} else {
			*error = EZJSON_SYNTAX_ERROR;
			return false;
		}
	}

	if (*c == (unsigned char)'"') {
		*value = (Ezjson_Value){EZJSON_STRING, .string = (Ezjson_String){0}};
		return ReadString(stream, &value->string, c, error);
	}

	if (*c == (unsigned char)'[') {
		*value = (Ezjson_Value){EZJSON_ARRAY, .array = (Ezjson_Array){0}};
		return ReadArray(stream, &value->array, c, depth, error);
	}

	if (*c == (unsigned char)'{') {
		*value = (Ezjson_Value){EZJSON_OBJECT, .object = (Ezjson_Object){0}};
		return ReadObject(stream, &value->object, c, depth, error);
	}

	*error = EZJSON_SYNTAX_ERROR;
	return false;
}

static bool Ezjson_Read(
	Stream* stream,
	Ezjson_Value* json,
	size_t depth,
	int* c,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(json != NULL);
	assert(c != NULL && *c == 0);
	assert(error != NULL);

	if ((*c = StreamGet(stream)) == 0xEF) {  // Skip BOM
		if ((*c = StreamGet(stream)) != 0xBB) return false;
		if ((*c = StreamGet(stream)) != 0xBF) return false;
		*c = StreamGet(stream);
	}

	return ReadValue(stream, json, c, depth, error);
}

bool Ezjson_ReadFile(
	FILE* file,
	Ezjson_Value* json,
	size_t depth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_ReadFile(file, json, depth, &(Ezjson_Error){EZJSON_SUCCESS});

	if (file == NULL || json == NULL) {
		*error = EZJSON_ARGUMENT_ERROR;
		return false;
	}

#if defined(__unix__)
	bool ok = false;
	locale_t cLocale = (locale_t)0;
	locale_t oldLocale = (locale_t)0;

	if ((cLocale = newlocale(LC_ALL_MASK, "C", (locale_t)0)) == (locale_t)0) {
		*error = EZJSON_LOCALE_ERROR;
		goto Cleanup0;
	}

	if ((oldLocale = uselocale(cLocale)) == (locale_t)0) {
		*error = EZJSON_LOCALE_ERROR;
		goto Cleanup1;
	}

	flockfile(file);
	int c = 0;
	ok = Ezjson_Read(file, json, depth, &c, error);
	ungetc(c, file);
	funlockfile(file);

	uselocale(oldLocale);
Cleanup1:
	freelocale(cLocale);
Cleanup0:
	return ok;
#elif defined(_WIN32)
	_locale_t cLocale = NULL;

	if ((cLocale = _wcreate_locale(LC_ALL, L"C")) == NULL) {
		*error = EZJSON_LOCALE_ERROR;
		return false;
	}

	_lock_file(file);
	int c = 0;
	FileStream fileStream = {{FileStreamGet, cLocale}, file};
	bool ok = Ezjson_Read(&fileStream.super, json, depth, &c, error);
	ungetc(c, file);
	_unlock_file(file);

	_free_locale(cLocale);
	return ok;
#else
	int c = 0;
	FileStream fileStream = {{FileStreamGet}, file};
	bool ok = Ezjson_Read(&fileStream.super, json, depth, &c, error);
	ungetc(c, file);
	return ok;
#endif
}

bool Ezjson_ReadMemory(
	const void* memory,
	size_t size,
	Ezjson_Value* json,
	size_t depth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_ReadMemory(memory, size, json, depth, &(Ezjson_Error){EZJSON_SUCCESS});

	if ((memory == NULL && size != 0) || json == NULL) {
		*error = EZJSON_ARGUMENT_ERROR;
		return false;
	}

#if defined(__unix__)
	bool ok = false;
	FILE* file = NULL;
	locale_t cLocale = (locale_t)0;
	locale_t oldLocale = (locale_t)0;

	if ((file = fmemopen((void*)memory, size, "r")) == NULL) {
		*error = EZJSON_MEMORY_ERROR;
		goto Cleanup0;
	}

	if ((cLocale = newlocale(LC_ALL_MASK, "C", (locale_t)0)) == (locale_t)0) {
		*error = EZJSON_LOCALE_ERROR;
		goto Cleanup1;
	}

	if ((oldLocale = uselocale(cLocale)) == (locale_t)0) {
		*error = EZJSON_LOCALE_ERROR;
		goto Cleanup1;
	}

	int c = 0;
	ok = Ezjson_Read(file, json, depth, &c, error);

	uselocale(oldLocale);
Cleanup2:
	freelocale(cLocale);
Cleanup1:
	fclose(file);
Cleanup0:
	return ok;
#elif defined(_WIN32)
	_locale_t cLocale = NULL;

	if ((cLocale =_wcreate_locale(LC_ALL, L"C")) == NULL) {
		*error = EZJSON_LOCALE_ERROR;
		return false;
	}

	MemoryStream memoryStream = {{MemoryStreamGet, cLocale}, memory, size};
	int c = 0;
	bool ok = Ezjson_Read(&memoryStream.super, json, depth, &c, error);

	_free_locale(cLocale);
	return ok;
#else
	MemoryStream memoryStream = {{MemoryStreamGet}, memory, size};
	int c = 0;
	return Ezjson_Read(&memoryStream.super, json, depth, &c, error);
#endif
}

bool Ezjson_Equals(
	const Ezjson_Value* left,
	const Ezjson_Value* right,
	size_t depth,
	Ezjson_Error* error
) {
	if (left == NULL || right == NULL) {
		if (error != NULL) *error = EZJSON_ARGUMENT_ERROR;
		return left == right;
	}

	if (depth == 0) {
		if (error != NULL) *error = EZJSON_DEPTH_ERROR;
		return false;
	}

	if (left->kind != right->kind)
		return false;

	if (left->kind == EZJSON_BOOLEAN) {
		if (left->boolean != right->boolean)
			return false;
	}

	if (left->kind == EZJSON_NUMBER) {
		if (left->number != right->number)
			return false;
	}

	if (left->kind == EZJSON_STRING) {
		if (left->string.size != right->string.size)
			return false;

		if (memcmp(left->string.data, right->string.data, left->string.size) != 0)
			return false;
	}

	if (left->kind == EZJSON_ARRAY) {
		if (left->array.length != right->array.length)
			return false;

		for (size_t i = 0; i < left->array.length; ++i) {
			if (!Ezjson_Equals(&left->array.items[i], &right->array.items[i], depth - 1, error))
				return false;
		}
	}

	if (left->kind == EZJSON_OBJECT) {
		if (left->object.length != right->object.length)
			return false;

		for (size_t i = 0; i < left->object.length; ++i) {
			const Ezjson_KeyValue* leftItem = &left->object.items[i];
			const Ezjson_KeyValue* rightItem = &right->object.items[i];

			if (leftItem->key.size != rightItem->key.size)
				return false;

			if (memcmp(leftItem->key.data, rightItem->key.data, leftItem->key.size) != 0)
				return false;

			if (!Ezjson_Equals(&leftItem->value, &rightItem->value, depth - 1, error))
				return false;
		}
	}

	return true;
}

Ezjson_Value* Ezjson_Lookup(const Ezjson_Value* json, const Ezjson_String* key, Ezjson_Error* error) {
	if (error == NULL)
		return Ezjson_Lookup(json, key, &(Ezjson_Error){EZJSON_SUCCESS});
	
	if (json == NULL || key == NULL) {
		*error = EZJSON_ARGUMENT_ERROR;
		return NULL;
	}

	if (json->kind == EZJSON_OBJECT) {
		for (size_t i = json->object.length; i > 0; --i) {
			Ezjson_KeyValue* item = &json->object.items[i - 1];

			if (item->key.size == key->size && memcmp(item->key.data, key->data, item->key.size) == 0)
				return &item->value;
		}
	}

	*error = EZJSON_KEY_ERROR;
	return NULL;
}

bool Ezjson_Destroy(Ezjson_Value* json, size_t depth, Ezjson_Error* error) {
	if (error == NULL)
		return Ezjson_Destroy(json, depth, &(Ezjson_Error){EZJSON_SUCCESS});

	if (json == NULL) {
		*error = EZJSON_ARGUMENT_ERROR;
		return true;
	}

	return Ezjson_DestroyValue(json, depth, error);
}
