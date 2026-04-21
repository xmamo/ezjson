#define _DEFAULT_SOURCE 1

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
#define StreamGet(stream) fgetc(stream)
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
	assert(stream != NULL);
	FileStream* fileStream = stream;
	return fgetc(fileStream->file);
}

static int MemoryStreamGet(void* stream) {
	assert(stream != NULL);
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
	assert(size != NULL && *size != 0);
	assert(capacity != NULL);

	if (cu <= 0x7F) {
		if (!GrowCharVec(data, size, capacity, 1, error)) return false;
		(*data)[*size - 2] = 0x00 | ((cu >> 0) & 0x7F);
	} else if (cu <= 0x7FF) {
		if (!GrowCharVec(data, size, capacity, 2, error)) return false;
		(*data)[*size - 3] = 0xC0 | ((cu >> 6) & 0xFF);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	} else if (cu <= 0xFFFF) {
		if (!GrowCharVec(data, size, capacity, 3, error)) return false;
		(*data)[*size - 4] = 0xE0 | ((cu >> 12) & 0x0F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	} else if (cu <= 0x1FFFFF) {
		if (!GrowCharVec(data, size, capacity, 4, error)) return false;
		(*data)[*size - 5] = 0xF0 | ((cu >> 18) & 0x07);
		(*data)[*size - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	} else if (cu <= 0x3FFFFFF) {
		if (!GrowCharVec(data, size, capacity, 5, error)) return false;
		(*data)[*size - 6] = 0xF8 | ((cu >> 24) & 0x03);
		(*data)[*size - 5] = 0x80 | ((cu >> 18) & 0x3F);
		(*data)[*size - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	} else if (cu <= 0x7FFFFFFF) {
		if (!GrowCharVec(data, size, capacity, 6, error)) return false;
		(*data)[*size - 7] = 0xFC | ((cu >> 30) & 0x01);
		(*data)[*size - 6] = 0x80 | ((cu >> 24) & 0x3F);
		(*data)[*size - 5] = 0x80 | ((cu >> 18) & 0x3F);
		(*data)[*size - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	} else {
		if (!GrowCharVec(data, size, capacity, 7, error)) return false;
		(*data)[*size - 8] = 0xFE | ((cu >> 31 >> 5) & 0x00);
		(*data)[*size - 7] = 0x80 | ((cu >> 30) & 0x3F);
		(*data)[*size - 6] = 0x80 | ((cu >> 24) & 0x3F);
		(*data)[*size - 5] = 0x80 | ((cu >> 18) & 0x3F);
		(*data)[*size - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*size - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*size - 2] = 0x80 | ((cu >> 0) & 0x3F);
	}

	return true;
}

static bool DestroyString(Ezjson_String* string) {
	assert(string != NULL);
	free(string->data);
	*string = (Ezjson_String){0};
	return true;
}

static bool DestroyValue(Ezjson_Value* value, size_t maxDepth, Ezjson_Error* error);

static bool DestroyArray(Ezjson_Array* array, size_t maxDepth, Ezjson_Error* error) {
	assert(array != NULL && (array->length != 0 || array->items == NULL));
	assert(maxDepth != 0);
	assert(error != NULL);

	for (size_t i = 0; i < array->length; ++i) {
		if (!DestroyValue(&array->items[i], maxDepth - 1, error))
			return false;
	}

	free(array->items);
	*array = (Ezjson_Array){0};
	return true;
}

static bool DestroyObject(Ezjson_Object* object, size_t maxDepth, Ezjson_Error* error) {
	assert(object != NULL && (object->length != 0 || object->items == NULL));
	assert(maxDepth != 0);
	assert(error != NULL);

	for (size_t i = 0; i < object->length; ++i) {
		Ezjson_KeyValue* item = &object->items[i];
		free(item->key.data);
		item->key = (Ezjson_String){0};

		if (!DestroyValue(&item->value, maxDepth - 1, error))
			return false;
	}

	free(object->items);
	*object = (Ezjson_Object){0};
	return true;
}

static bool DestroyValue(Ezjson_Value* value, size_t maxDepth, Ezjson_Error* error) {
	assert(value != NULL);
	assert(error != NULL);

	if (maxDepth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	if (value->kind == EZJSON_STRING) {
		return DestroyString(&value->string);
	} else if (value->kind == EZJSON_ARRAY) {
		return DestroyArray(&value->array, maxDepth, error);
	} else if (value->kind == EZJSON_OBJECT) {
		return DestroyObject(&value->object, maxDepth, error);
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
	if (!GrowCharVec(numeral, &size, &capacity, 2, error)) return false;
	(*numeral)[0] = (char)*c;

	// Data is now: [0-9-]

	if (*c == (unsigned char)'-') {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Failure;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?[0-9]

	if (*c != (unsigned char)'0') {
		do {
			if ((*c = StreamGet(stream)) == EOF || strchr("0123456789.eE", *c) == NULL) {
				// Data is now: -?[1-9][0-9]*
				goto Success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr(".eE", *c) == NULL);
	} else {
		if ((*c = StreamGet(stream)) == EOF || strchr(".eE", *c) == NULL) {
			// Data is now: -?0
			goto Success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?([1-9][0-9]*|0)[.eE]

	if (*c == (unsigned char)'.') {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Failure;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
		(*numeral)[size - 2] = (char)*c;

		do {
			if ((*c = StreamGet(stream)) == EOF || strchr("0123456789eE", *c) == NULL) {
				// Data is now: -?([1-9][0-9]*|0)\.[0-9]+
				goto Success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr("eE", *c) == NULL);
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE]

	if ((*c = StreamGet(stream)) == EOF || strchr("0123456789+-", *c) == NULL) {
		*error = EZJSON_SYNTAX_ERROR;
		goto Failure;
	}

	if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
	(*numeral)[size - 2] = (char)*c;

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][0-9+-]

	if (strchr("+-", *c) != NULL) {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Failure;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]

	while (true) {
		if ((*c = StreamGet(stream)) == EOF || strchr("0123456789", *c) == NULL) {
			// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]+
			goto Success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1, error)) goto Failure;
		(*numeral)[size - 2] = (char)*c;
	}

Success:
	(*numeral)[size - 1] = '\0';
	char* newNumeral = realloc(*numeral, sizeof(char) * size);
	if (newNumeral != NULL) *numeral = newNumeral;
	return true;

Failure:
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
		0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
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
	if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) return false;

	while (true) {
		*c = StreamGet(stream);

	Top:
		if (*c == EOF || *c <= 0x1F) {
			*error = EZJSON_SYNTAX_ERROR;
			goto Failure;
		}

		if (*c == (unsigned char)'"')
			break;

		if (*c != (unsigned char)'\\') {
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Failure;
			string->data[string->size - 2] = (char)*c;
			continue;
		}

		char v1 = '\0';
		char16_t cu1 = 0;
		if (!ReadEscape(stream, &v1, &cu1, c, error)) goto Failure;

		if (cu1 == 0) {
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Failure;
			string->data[string->size - 2] = v1;
			continue;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'\\' || cu1 < 0xD800 || cu1 > 0xDBFF) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Failure;
			goto Top;
		}

		char v2 = '\0';
		char16_t cu2 = 0;
		if (!ReadEscape(stream, &v2, &cu2, c, error)) goto Failure;

		if (cu2 == 0) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Failure;
			if (!GrowCharVec(&string->data, &string->size, &capacity, 1, error)) goto Failure;
			string->data[string->size - 2] = v2;
			continue;
		}

		if (cu2 < 0xDC00 || cu2 > 0xDFFF) {
			if (!AppendCu(&string->data, &string->size, &capacity, cu1, error)) goto Failure;
			if (!AppendCu(&string->data, &string->size, &capacity, cu2, error)) goto Failure;
			continue;
		}

		char32_t cp = 0x10000 + (((cu1 & UINT32_C(0x3FF)) << 10) | (cu2 & UINT32_C(0x3FF)));
		if (!AppendCu(&string->data, &string->size, &capacity, cp, error)) goto Failure;
	}

	string->data[string->size - 1] = '\0';
	char* newData = realloc(string->data, sizeof(char) * string->size);
	if (newData != NULL) string->data = newData;
	string->size -= 1;
	*c = StreamGet(stream);
	return true;

Failure:
	DestroyString(string);
	return false;
}

static bool ReadValue(
	Stream* stream,
	Ezjson_Value* value,
	int* c,
	size_t maxDepth,
	Ezjson_Error* error
);

static bool ReadArray(
	Stream* stream,
	Ezjson_Array* array,
	int* c,
	size_t maxDepth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(array != NULL && array->items == NULL && array->length == 0);
	assert(c != NULL && *c == '[');
	assert(error != NULL);

	if (maxDepth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);

	if (*c != (unsigned char)']') {
		size_t capacity = 0;

		while (true) {
			Ezjson_Value item = {0};
			if (!ReadValue(stream, &item, c, maxDepth - 1, error)) goto Failure;
			SkipWs(stream, c);

			if (*c == EOF) {
				*error = EZJSON_SYNTAX_ERROR;
				goto Failure;
			}

			if (!GrowValueVec(&array->items, &array->length, &capacity, 1, error)) goto Failure;
			array->items[array->length - 1] = item;

			if (*c == (unsigned char)']')
				break;

			if (*c != (unsigned char)',') {
				*error = EZJSON_SYNTAX_ERROR;
				goto Failure;
			}

			*c = StreamGet(stream);
			SkipWs(stream, c);
		}

		if (array->length != 0) {
			assert(array->length <= SIZE_MAX / sizeof(Ezjson_Value));
			Ezjson_Value* newItems = realloc(array->items, sizeof(Ezjson_Value) * array->length);
			if (newItems != NULL) array->items = newItems;
		}
	}

	*c = StreamGet(stream);
	return true;

Failure:
	free(array->items);
	*array = (Ezjson_Array){0};
	return false;
}

static bool ReadObject(
	Stream* stream,
	Ezjson_Object* object,
	int* c,
	size_t maxDepth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(object != NULL && object->items == NULL && object->length == 0);
	assert(c != NULL && *c == (unsigned char)'{');
	assert(error != NULL);

	if (maxDepth == 0) {
		*error = EZJSON_DEPTH_ERROR;
		return false;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);

	if (*c != (unsigned char)'}') {
		size_t capacity = 0;

		while (true) {
			if (*c != (unsigned char)'"') {
				*error = EZJSON_SYNTAX_ERROR;
				goto Failure;
			}

			if (!GrowKeyValueVec(&object->items, &object->length, &capacity, 1, error)) goto Failure;
			object->items[object->length - 1] = (Ezjson_KeyValue){0};

			if (!ReadString(stream, &object->items[object->length - 1].key, c, error)) goto Failure;
			SkipWs(stream, c);

			if (*c != (unsigned char)':') {
				*error = EZJSON_SYNTAX_ERROR;
				goto Failure;
			}

			*c = StreamGet(stream);
			SkipWs(stream, c);

			if (!ReadValue(stream, &object->items[object->length - 1].value, c, maxDepth - 1, error)) goto Failure;
			SkipWs(stream, c);

			if (*c == (unsigned char)'}')
				break;

			if (*c != (unsigned char)',') {
				*error = EZJSON_SYNTAX_ERROR;
				goto Failure;
			}

			*c = StreamGet(stream);
			SkipWs(stream, c);
		}

		if (object->length != 0) {
			assert(object->length <= SIZE_MAX / sizeof(Ezjson_KeyValue));
			Ezjson_KeyValue* newItems = realloc(object->items, sizeof(Ezjson_KeyValue) * object->length);
			if (newItems != NULL) object->items = newItems;
		}
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

Failure:
	DestroyObject(object, SIZE_MAX, &(Ezjson_Error){EZJSON_NO_ERROR});
	return false;
}

static bool ReadValue(
	Stream* stream,
	Ezjson_Value* value,
	int* c,
	size_t maxDepth,
	Ezjson_Error* error
) {
	assert(stream != NULL);
	assert(value != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));
	assert(error != NULL);

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
		return ReadArray(stream, &value->array, c, maxDepth, error);
	}

	if (*c == (unsigned char)'{') {
		*value = (Ezjson_Value){EZJSON_OBJECT, .object = (Ezjson_Object){0}};
		return ReadObject(stream, &value->object, c, maxDepth, error);
	}

	*error = EZJSON_SYNTAX_ERROR;
	return false;
}

static bool ReadStream(
	Stream* stream,
	Ezjson_Value* json,
	size_t maxDepth,
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

	SkipWs(stream, c);
	bool ok = ReadValue(stream, json, c, maxDepth, error);
	SkipWs(stream, c);
	return ok;
}

bool Ezjson_ReadFile(
	FILE* file,
	Ezjson_Value* json,
	size_t maxDepth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_ReadFile(file, json, maxDepth, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

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
	ok = ReadStream(file, json, maxDepth, &c, error);
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
	bool ok = ReadStream(&fileStream.super, json, maxDepth, &c, error);
	ungetc(c, file);
	_unlock_file(file);

	_free_locale(cLocale);
	return ok;
#else
	int c = 0;
	FileStream fileStream = {{FileStreamGet}, file};
	bool ok = ReadStream(&fileStream.super, json, maxDepth, &c, error);
	ungetc(c, file);
	return ok;
#endif
}

bool Ezjson_ReadMemory(
	const void* memory,
	size_t size,
	Ezjson_Value* json,
	size_t maxDepth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_ReadMemory(memory, size, json, maxDepth, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

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
		goto Cleanup2;
	}

	int c = 0;
	ok = ReadStream(file, json, maxDepth, &c, error);

	uselocale(oldLocale);
Cleanup2:
	freelocale(cLocale);
Cleanup1:
	fclose(file);
Cleanup0:
	return ok;
#elif defined(_WIN32)
	_locale_t cLocale = NULL;

	if ((cLocale = _wcreate_locale(LC_ALL, L"C")) == NULL) {
		*error = EZJSON_LOCALE_ERROR;
		return false;
	}

	MemoryStream memoryStream = {{MemoryStreamGet, cLocale}, memory, size};
	int c = 0;
	bool ok = ReadStream(&memoryStream.super, json, maxDepth, &c, error);

	_free_locale(cLocale);
	return ok;
#else
	MemoryStream memoryStream = {{MemoryStreamGet}, memory, size};
	int c = 0;
	return ReadStream(&memoryStream.super, json, maxDepth, &c, error);
#endif
}

static bool ValueEquals(
	const Ezjson_Value* left,
	const Ezjson_Value* right,
	size_t maxDepth,
	Ezjson_Error* error
) {
	assert(left != NULL);
	assert(right != NULL);
	assert(error != NULL);

	if (maxDepth == 0) {
		*error = EZJSON_DEPTH_ERROR;
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
			if (!ValueEquals(&left->array.items[i], &right->array.items[i], maxDepth - 1, error))
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

			if (!ValueEquals(&leftItem->value, &rightItem->value, maxDepth - 1, error))
				return false;
		}
	}

	return true;
}

bool Ezjson_Equals(
	const Ezjson_Value* left,
	const Ezjson_Value* right,
	size_t maxDepth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_Equals(left, right, maxDepth, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

	if (left == NULL || right == NULL)
		return left == right;

	return ValueEquals(left, right, maxDepth, error);
}

static Ezjson_Value* LookupValue(
	const Ezjson_Object* object,
	const char* key,
	size_t keySize,
	Ezjson_Error* error
) {
	assert(object != NULL);
	assert(key != NULL || keySize == 0);
	assert(error != NULL);

	for (size_t i = object->length; i > 0; --i) {
		Ezjson_KeyValue* item = &object->items[i - 1];

		if (item->key.size == keySize && memcmp(item->key.data, key, item->key.size) == 0)
			return &item->value;
	}

	*error = EZJSON_KEY_ERROR;
	return NULL;
}

Ezjson_Value* Ezjson_Lookup(
	const Ezjson_Value* json,
	const char* key,
	size_t keySize,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_Lookup(json, key, keySize, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

	if (json == NULL || (key == NULL && keySize != 0) || json->kind != EZJSON_OBJECT) {
		*error = EZJSON_ARGUMENT_ERROR;
		return NULL;
	}

	return LookupValue(&json->object, key, keySize, error);
}

Ezjson_Value* Ezjson_At(
	const Ezjson_Value* json,
	const char* jsonPointer,
	size_t jsonPointerSize,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_At(json, jsonPointer, jsonPointerSize, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

	if (json == NULL || (jsonPointer == NULL && jsonPointerSize != 0)) {
		*error = EZJSON_ARGUMENT_ERROR;
		return NULL;
	}

	if (jsonPointerSize == 0)
		return (Ezjson_Value*)json;

	for (size_t i = 0; i < jsonPointerSize; ++i) {
		if (jsonPointer[i] != '~') continue;
		if (i < jsonPointerSize - 1 && strchr("01", jsonPointer[i + 1]) != NULL) continue;
		*error = EZJSON_SYNTAX_ERROR;
		return NULL;
	}

	while (jsonPointerSize != 0) {
		jsonPointer += 1;
		jsonPointerSize -= 1;

		if (*jsonPointer != '/') {
			*error = EZJSON_SYNTAX_ERROR;
			return NULL;
		}

		size_t tokenSize = 0;
		while (tokenSize < jsonPointerSize && jsonPointer[tokenSize] != '/') tokenSize += 1;

		if (json->kind == EZJSON_ARRAY) {
			if (tokenSize == 0) {
				*error = EZJSON_KEY_ERROR;
				return NULL;
			}

			if (tokenSize == 1 && *jsonPointer == '-') {
				json = &json->array.items[json->array.length];
				goto Success;
			}

			if (tokenSize >= 2 && *jsonPointer == '0') {
				*error = EZJSON_KEY_ERROR;
				return NULL;
			}

			size_t index = 0;

			for (size_t ti = 0; ti < tokenSize; ++ti) {
				static const char key[] = "0123456789";
				static const size_t value[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
				const char* p = strchr(key, jsonPointer[ti]);

				if (p == NULL || index > SIZE_MAX / 10 || 10 * index > SIZE_MAX - value[key - p]) {
					*error = EZJSON_KEY_ERROR;
					return NULL;
				}

				index = 10 * index + value[key - p];
			}

			if (index >= json->array.length) {
				*error = EZJSON_KEY_ERROR;
				return NULL;
			}

			json = &json->array.items[index];
			goto Success;
		}

		if (json->kind == EZJSON_OBJECT) {
			for (size_t i = json->object.length; i > 0; --i) {
				const Ezjson_KeyValue* item = &json->object.items[i - 1];
				size_t ki = 0;

				for (size_t ti = 0; ti < tokenSize && ki < item->key.size;) {
					char c = jsonPointer[ti];

					if (strcmp(jsonPointer + ti, "~0") == 0) {
						c = '~';
						ti += 2;
					} else if (strcmp(jsonPointer + ti, "~1") == 0) {
						c = '/';
						ti += 2;
					} else {
						ti += 1;
					}

					if (c != item->key.data[ki++])
						break;
				}

				if (ki == item->key.size) {
					json = &item->value;
					goto Success;
				}
			}

			*error = EZJSON_KEY_ERROR;
			return NULL;
		}

		*error = EZJSON_KEY_ERROR;
		return NULL;

	Success:
		if (tokenSize >= jsonPointerSize) break;
		jsonPointer += tokenSize;
		jsonPointerSize -= tokenSize;
	}

	return (Ezjson_Value*)json;
}

bool Ezjson_Destroy(
	Ezjson_Value* json,
	size_t maxDepth,
	Ezjson_Error* error
) {
	if (error == NULL)
		return Ezjson_Destroy(json, maxDepth, &(Ezjson_Error){EZJSON_NO_ERROR});

	if (*error != EZJSON_NO_ERROR)
		return false;

	if (json == NULL) {
		*error = EZJSON_ARGUMENT_ERROR;
		return false;
	}

	return DestroyValue(json, maxDepth, error);
}
