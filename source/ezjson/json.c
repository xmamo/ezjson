#if defined(__unix__)
	#define _DEFAULT_SOURCE
#endif

#include "ezjson/json.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#if defined(__unix__)
	typedef FILE Stream;
	#define StreamGet fgetc
#else
	typedef struct Stream {
		int (*get)(void* stream);
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

static bool GrowVec(void** data, size_t size, size_t* length, size_t* capacity, size_t delta) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);

	if (*length > SIZE_MAX - delta) return false;
	size_t newLength = *length + delta;
	size_t newCapacity = *capacity;

	while (newLength > newCapacity) {
		if (newCapacity > SIZE_MAX / 2) return false;
		newCapacity = newCapacity != 0 ? 2 * newCapacity : 1;
	}

	if (newCapacity > *capacity) {
		if (newCapacity > SIZE_MAX / size) return false;
		void* newData = realloc(*data, size * newCapacity);
		if (newData == NULL) return false;
		*data = newData;
		*capacity = newCapacity;
	}

	*length = newLength;
	return true;
}

static bool GrowCharVec(char** data, size_t* length, size_t* capacity, size_t delta) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);

	void* p = *data;
	bool r = GrowVec(&p, sizeof(char), length, capacity, delta);
	*data = p;
	return r;
}

static bool GrowValueVec(Ezjson_Value** data, size_t* length, size_t* capacity, size_t delta) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);

	void* p = *data;
	bool r = GrowVec(&p, sizeof(Ezjson_Value), length, capacity, delta);
	*data = p;
	return r;
}

static bool GrowKeyValueVec(Ezjson_KeyValue** data, size_t* length, size_t* capacity, size_t delta) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);

	void* p = *data;
	bool r = GrowVec(&p, sizeof(Ezjson_KeyValue), length, capacity, delta);
	*data = p;
	return r;
}

static bool AppendCU(char** data, size_t* length, size_t* capacity, char32_t cu) {
	assert(data != NULL);
	assert(length != NULL);
	assert(capacity != NULL);
	assert(cu <= 0x10FFFF);

	if (cu <= 0x7F) {
		if (!GrowCharVec(data, length, capacity, 1)) return false;
		(*data)[*length - 2] = 0x00 | ((cu >> 0) & 0x7F);
		return true;
	}

	if (cu <= 0x7FF) {
		if (!GrowCharVec(data, length, capacity, 2)) return false;
		(*data)[*length - 3] = 0xC0 | ((cu >> 6) & 0xFF);
		(*data)[*length - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	if (cu <= 0xFFFF) {
		if (!GrowCharVec(data, length, capacity, 3)) return false;
		(*data)[*length - 4] = 0xE0 | ((cu >> 12) & 0x0F);
		(*data)[*length - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*length - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	if (cu <= 0x1FFFFF) {
		if (!GrowCharVec(data, length, capacity, 4)) return false;
		(*data)[*length - 5] = 0xF0 | ((cu >> 18) & 0x07);
		(*data)[*length - 4] = 0x80 | ((cu >> 12) & 0x3F);
		(*data)[*length - 3] = 0x80 | ((cu >> 6) & 0x3F);
		(*data)[*length - 2] = 0x80 | ((cu >> 0) & 0x3F);
		return true;
	}

	return false;
}

static void SkipWs(Stream* stream, int* c) {
	assert(stream != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));

	while (*c != EOF && strchr(" \t\n\r", *c) != NULL) {
		*c = StreamGet(stream);
	}
}

static bool ReadEscape(Stream* stream, char* v, char16_t* cu, int* c) {
	assert(stream != NULL);
	assert(v != NULL && *v == '\0');
	assert(cu != NULL && *cu == 0);
	assert(c != NULL && *c == (unsigned char)'\\');

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

	if ((*c = StreamGet(stream)) == EOF) return false;
	const char* p = strchr(escapeKey, *c);

	if (p != NULL) {
		*v = escapeValue[p - escapeKey];
		return true;
	}

	if (*c != (unsigned char)'u')
		return false;

	for (int i = 0; i < 4; ++i) {
		if ((*c = StreamGet(stream)) == EOF) return false;
		p = strchr(hexKey, *c);
		if (p == NULL) return false;
		*cu = (*cu << 4) | hexValue[p - hexKey];
	}

	return true;
}

static bool ReadString(Stream* stream, Ezjson_String* string, int* c) {
	assert(stream != NULL);
	assert(string != NULL && string->data == NULL && string->length == 0);
	assert(c != NULL && *c == (unsigned char)'"');

	string->data = malloc(sizeof(char));
	if (string->data == NULL) return false;
	string->length = 1;
	size_t capacity = 1;

	while (true) {
		*c = StreamGet(stream);

	top:
		if (*c == EOF || *c < (unsigned char)' ')
			goto error;

		if (*c == (unsigned char)'"')
			break;

		if (*c != (unsigned char)'\\') {
			if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
			string->data[string->length - 2] = (char)*c;
			continue;
		}

		char v1 = '\0';
		char16_t cu1 = 0;
		if (!ReadEscape(stream, &v1, &cu1, c)) goto error;

		if (cu1 == 0) {
			if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
			string->data[string->length - 2] = v1;
			continue;
		}

		if ((*c = StreamGet(stream)) != (unsigned char)'\\' || cu1 < 0xD800 || cu1 > 0xDBFF) {
			if (!AppendCU(&string->data, &string->length, &capacity, cu1)) goto error;
			goto top;
		}

		char v2 = '\0';
		char16_t cu2 = 0;
		if (!ReadEscape(stream, &v2, &cu2, c)) return false;

		if (cu2 == 0) {
			if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
			string->data[string->length - 2] = v2;
			continue;
		}

		if (cu2 < 0xDC00 || cu2 > 0xDFFF) {
			if (!AppendCU(&string->data, &string->length, &capacity, cu2)) goto error;
			continue;
		}

		char32_t cp = 0x10000 + (((cu1 & UINT32_C(0x3FF)) << 10) | (cu2 & UINT32_C(0x3FF)));
		if (!AppendCU(&string->data, &string->length, &capacity, cp)) return false;
	}

	string->data[string->length - 1] = '\0';
	char* newData = realloc(string->data, sizeof(char) * string->length);
	if (newData != NULL) string->data = newData;
	string->length -= 1;

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

error:
	free(string->data);
	*string = (Ezjson_String){NULL};
	return false;
}

static bool ReadNumeral(Stream* stream, char** numeral, int* c) {
	assert(stream != NULL);
	assert(numeral != NULL && *numeral == NULL);
	assert(c != NULL && *c != EOF && strchr("0123456789-", *c) != NULL);

	*numeral = malloc(sizeof(char) * 2);
	if (*numeral == NULL) return false;
	(*numeral)[0] = (char)*c;
	size_t size = 2;
	size_t capacity = 2;

	// Data is now: [0-9-]

	if (*c == (unsigned char)'-') {
		*c = StreamGet(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", *c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?[0-9]

	if (*c != (unsigned char)'0') {
		do {
			*c = StreamGet(stream);

			if (*c == EOF || strchr("0123456789.eE", *c) == NULL) {
				// Data is now: -?[1-9][0-9]*
				goto success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr(".eE", *c) == NULL);
	} else {
		*c = StreamGet(stream);

		if (*c == EOF || strchr(".eE", *c) == NULL) {
			// Data is now: -?0
			goto success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?([1-9][0-9]*|0)[.eE]

	if (*c == (unsigned char)'.') {
		*c = StreamGet(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", *c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;

		do {
			*c = StreamGet(stream);

			if (*c == EOF || strchr("0123456789eE", *c) == NULL) {
				// Data is now: -?([1-9][0-9]*|0)\.[0-9]+
				goto success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
			(*numeral)[size - 2] = (char)*c;
		} while (strchr("eE", *c) == NULL);
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE]

	*c = StreamGet(stream);
	if (*c == EOF) goto error;
	if (strchr("0123456789+-", *c) == NULL) goto error;

	if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
	(*numeral)[size - 2] = (char)*c;

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][0-9+-]

	if (strchr("+-", *c) != NULL) {
		*c = StreamGet(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", *c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]

	do {
		*c = StreamGet(stream);

		if (*c == EOF || strchr("0123456789", *c) == NULL) {
			// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]+
			goto success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	} while (true);

success:
	(*numeral)[size - 1] = '\0';
	char* newNumeral = realloc(*numeral, sizeof(char) * size);
	if (newNumeral != NULL) *numeral = newNumeral;

	SkipWs(stream, c);
	return true;

error:
	free(*numeral);
	*numeral = NULL;
	return false;
}

static bool ReadValue(Stream* stream, Ezjson_Value* value, int* c);

static bool ReadObject(Stream* stream, Ezjson_Object* object, int* c) {
	assert(stream != NULL);
	assert(object != NULL && object->items == NULL && object->length == 0);
	assert(c != NULL && *c == (unsigned char)'{');

	size_t capacity = 0;

	while (true) {
		*c = StreamGet(stream);
		SkipWs(stream, c);
		if (*c != (unsigned char)'"') goto error1;

		if (!GrowKeyValueVec(&object->items, &object->length, &capacity, 1)) goto error1;
		object->items[object->length - 1] = (Ezjson_KeyValue){0};

		if (!ReadString(stream, &object->items[object->length - 1].key, c)) goto error1;
		if (*c != (unsigned char)':') goto error1;

		*c = StreamGet(stream);
		if (!ReadValue(stream, &object->items[object->length - 1].value, c)) goto error2;
		if (*c == (unsigned char)',') continue;
		if (*c == (unsigned char)'}') break;

	error2:
		free(object->items[object->length - 1].key.data);
		object->items[object->length - 1].key = (Ezjson_String){0};
	error1:
		free(object->items);
		*object = (Ezjson_Object){0};
		return false;
	}

	if (object->length != 0) {
		assert(object->length <= SIZE_MAX / sizeof(Ezjson_KeyValue));
		Ezjson_KeyValue* newItems = realloc(object->items, sizeof(Ezjson_KeyValue) * object->length);
		if (newItems != NULL) object->items = newItems;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;
}

static bool ReadArray(Stream* stream, Ezjson_Array* array, int* c) {
	assert(stream != NULL);
	assert(array != NULL && array->items == NULL && array->length == 0);
	assert(c != NULL && *c == '[');

	size_t capacity = 0;

	while (true) {
		*c = StreamGet(stream);
		Ezjson_Value item = {0};
		if (!ReadValue(stream, &item, c)) goto error;
		if (*c == EOF) goto error;

		if (!GrowValueVec(&array->items, &array->length, &capacity, 1)) goto error;
		array->items[array->length - 1] = item;

		if (*c == (unsigned char)']') break;
		if (*c != (unsigned char)',') goto error;
	}

	if (array->length != 0) {
		assert(array->length <= SIZE_MAX / sizeof(Ezjson_Value));
		Ezjson_Value* newItems = realloc(array->items, sizeof(Ezjson_Value) * array->length);
		if (newItems != NULL) array->items = newItems;
	}

	*c = StreamGet(stream);
	SkipWs(stream, c);
	return true;

error:
	free(array->items);
	*array = (Ezjson_Array){0};
	return false;
}

static bool ReadValue(Stream* stream, Ezjson_Value* value, int* c) {
	assert(stream != NULL);
	assert(value != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));

	SkipWs(stream, c);

	if (*c == (unsigned char)'{') {
		*value = (Ezjson_Value){EZJSON_KIND_OBJECT, .object = (Ezjson_Object){0}};
		return ReadObject(stream, &value->object, c);
	}

	if (*c == (unsigned char)'[') {
		*value = (Ezjson_Value){EZJSON_KIND_ARRAY, .array = (Ezjson_Array){0}};
		return ReadArray(stream, &value->array, c);
	}

	if (*c == (unsigned char)'"') {
		*value = (Ezjson_Value){EZJSON_KIND_STRING, .string = (Ezjson_String){0}};
		return ReadString(stream, &value->string, c);
	}

	if (*c != EOF && strchr("0123456789-", *c) != NULL) {
		*value = (Ezjson_Value){EZJSON_KIND_NUMBER, .number = 0.0};

		char* numeral = NULL;
		if (!ReadNumeral(stream, &numeral, c)) return false;

		char* end = numeral;
		value->number = strtod(numeral, &end);
		bool ok = *end == '\0';
		free(numeral);
		return ok;
	}

	if (*c == (unsigned char)'t') {
		value->kind = EZJSON_KIND_TRUE;

		if ((*c = StreamGet(stream)) != (unsigned char)'r') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'u') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'e') return false;

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'f') {
		value->kind = EZJSON_KIND_FALSE;

		if ((*c = StreamGet(stream)) != (unsigned char)'a') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'l') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'s') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'e') return false;

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'n') {
		value->kind = EZJSON_KIND_NULL;

		if ((*c = StreamGet(stream)) != (unsigned char)'u') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'l') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'l') return false;

		*c = StreamGet(stream);
		SkipWs(stream, c);
		return true;
	}

	return false;
}

static bool Ezjson_Read(Stream* stream, Ezjson_Value* json, int* c) {
	assert(stream != NULL);
	assert(json != NULL);
	assert(c != NULL && *c == '\0');

	*c = StreamGet(stream);

	if (*c == (unsigned char)'\xEF') {  // Skip BOM
		if ((*c = StreamGet(stream)) != (unsigned char)'\xBB') return false;
		if ((*c = StreamGet(stream)) != (unsigned char)'\xBF') return false;
		*c = StreamGet(stream);
	}

	return ReadValue(stream, json, c);
}

bool Ezjson_FileRead(FILE* file, Ezjson_Value* json) {
	assert(file != NULL);
	assert(json != NULL);

#if defined(__unix__)
	flockfile(file);
	int c = '\0';
	bool r = Ezjson_Read(file, json, &c);
	ungetc(c, file);
	funlockfile(file);
	return r;
#elif defined(_WIN32)
	_lock_file(file);
	FileStream stream = {{FileStreamGet}, file};
	int c = '\0';
	bool r = Ezjson_Read(&stream.super, json, &c);
	ungetc(c, file);
	_unlock_file(file);
	return r;
#else
	FileStream stream = {{FileStreamGet}, file};
	int c = '\0';
	bool r = Ezjson_Read(&stream.super, json, &c);
	ungetc(c, file);
	return r;
#endif
}

bool Ezjson_MemoryRead(const void* memory, size_t size, Ezjson_Value* json) {
	assert(memory != NULL || size == 0);
	assert(json != NULL);

#if defined(__unix__)
	FILE* file = fmemopen((void*)memory, size, "r");
	if (file == NULL) return false;
	int c = '\0';
	bool r = Ezjson_Read(file, json, &c);
	fclose(file);
	return r;
#else
	MemoryStream stream = {{MemoryStreamGet}, memory, size};
	int c = '\0';
	return Ezjson_Read(&stream.super, json, &c);
#endif
}

bool Ezjson_Equal(const Ezjson_Value* left, const Ezjson_Value* right) {
	assert(left != NULL);
	assert(right != NULL);

	if (left->kind != right->kind)
		return false;

	if (left->kind == EZJSON_KIND_OBJECT) {
		if (left->object.length != right->object.length)
			return false;

		for (size_t i = 0; i < left->object.length; ++i) {
			const Ezjson_KeyValue* leftItem = &left->object.items[i];
			const Ezjson_KeyValue* rightItem = &right->object.items[i];

			if (leftItem->key.length != rightItem->key.length)
				return false;

			if (memcmp(leftItem->key.data, rightItem->key.data, leftItem->key.length) != 0)
				return false;

			if (!Ezjson_Equal(&leftItem->value, &rightItem->value))
				return false;
		}
	}

	if (left->kind == EZJSON_KIND_ARRAY) {
		if (left->array.length != right->array.length)
			return false;

		for (size_t i = 0; i < left->array.length; ++i) {
			if (!Ezjson_Equal(&left->array.items[i], &right->array.items[i]))
				return false;
		}
	}

	if (left->kind == EZJSON_KIND_STRING) {
		if (left->string.length != right->string.length)
			return false;

		if (memcmp(left->string.data, right->string.data, left->string.length) != 0)
			return false;
	}

	if (left->kind == EZJSON_KIND_NUMBER) {
		if (left->number != right->number)
			return false;
	}

	return true;
}

void Ezjson_Destroy(Ezjson_Value* json) {
	assert(json != NULL);

	if (json->kind == EZJSON_KIND_OBJECT) {
		for (size_t i = 0; i < json->object.length; ++i) {
			Ezjson_KeyValue* item = &json->object.items[i];
			free(item->key.data);
			Ezjson_Destroy(&item->value);
		}

		free(json->object.items);
		json->object.items = NULL;
		json->object.length = 0;
		return;
	}

	if (json->kind == EZJSON_KIND_ARRAY) {
		for (size_t i = 0; i < json->array.length; ++i) {
			Ezjson_Destroy(&json->array.items[i]);
		}

		free(json->array.items);
		json->array.items = NULL;
		json->array.length = 0;
		return;
	}

	if (json->kind == EZJSON_KIND_STRING) {
		free(json->string.data);
		json->string.data = NULL;
		json->string.length = 0;
		return;
	}
}
