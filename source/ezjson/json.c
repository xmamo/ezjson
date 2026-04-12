#ifdef __unix__
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

static void SkipWs(FILE* stream, int* c) {
	assert(stream != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));

	while (*c != EOF && strchr(" \t\n\r", (char)*c) != NULL) {
		*c = fgetc(stream);
	}
}

static bool ReadString(FILE* stream, Ezjson_String* string, int* c) {
	assert(stream != NULL);
	assert(string != NULL && string->data == NULL && string->length == 0);
	assert(c != NULL && *c == (unsigned char)'"');

	string->data = malloc(sizeof(char));
	if (string->data == NULL) return false;
	string->length = 1;
	size_t capacity = 1;

	while (true) {
		*c = fgetc(stream);

		if (*c == (unsigned char)'"')
			break;

		if (*c == (unsigned char)'\\') {
			*c = fgetc(stream);
			if (*c == EOF) goto error;

			static const char escapeKey[] = "\"\\/bfnrt";
			static const char escapeValue[] = "\"\\/\b\f\n\r\t";
			char* p = strchr(escapeKey, (char)*c);

			if (p != NULL) {
				if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
				string->data[string->length - 2] = escapeValue[p - escapeKey];
			} else if (*c == (unsigned char)'u') {
				static const char hexKey[] =
					"0123456789"
					"abcdef"
					"ABCDEF";

				static const unsigned short hexValue[] = {
					0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
					0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
					0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
				};

				char16_t hi = 0;
				char16_t lo = 0;
				char32_t cp = 0;

				for (int i = 0; i < 4; ++i) {
					if ((*c = fgetc(stream)) == EOF) goto error;
					p = strchr(hexKey, (char)*c);
					if (p == NULL) goto error;
					hi = (hi << 4) | hexValue[p - hexKey];
				}

				if (hi < 0xD800 || hi > 0xDFFF) {
					cp = hi;
				} else if (hi >= 0xD800 && hi <= 0xDBFF) {
					if ((*c = fgetc(stream)) != (unsigned char)'\\') goto error;
					if ((*c = fgetc(stream)) != (unsigned char)'u') goto error;

					for (int i = 0; i < 4; ++i) {
						if ((*c = fgetc(stream)) == EOF) goto error;
						p = strchr(hexKey, (char)*c);
						if (p == NULL) goto error;
						lo = (lo << 4) | hexValue[p - hexKey];
					}

					if (lo >= 0xDC00 && lo <= 0xDFFF) {
						cp = (((hi & 0x3FF) << 10) | (lo & 0x3FF)) + 0x10000;
					} else {
						goto error;
					}
				} else {
					goto error;
				}

				if (cp <= 0x7F) {
					if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
					string->data[string->length - 2] = (unsigned char)(0x00 | ((cp >> 0) & 0x7F));
				} else if (cp <= 0x7FF) {
					if (!GrowCharVec(&string->data, &string->length, &capacity, 2)) goto error;
					string->data[string->length - 3] = (unsigned char)(0xC0 | ((cp >> 6) & 0xFF));
					string->data[string->length - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else if (cp <= 0xFFFF) {
					if (!GrowCharVec(&string->data, &string->length, &capacity, 3)) goto error;
					string->data[string->length - 4] = (unsigned char)(0xE0 | ((cp >> 12) & 0x0F));
					string->data[string->length - 3] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
					string->data[string->length - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else if (cp <= 0x10FFFF) {
					if (!GrowCharVec(&string->data, &string->length, &capacity, 4)) goto error;
					string->data[string->length - 5] = (unsigned char)(0xF0 | ((cp >> 18) & 0x07));
					string->data[string->length - 4] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
					string->data[string->length - 3] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
					string->data[string->length - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else {
					goto error;
				}
			} else {
				goto error;
			}

			continue;
		}

		if (*c != EOF) {
			if (!GrowCharVec(&string->data, &string->length, &capacity, 1)) goto error;
			string->data[string->length - 2] = (char)*c;
			continue;
		}

	error:
		free(string->data);
		*string = (Ezjson_String){0};
		return false;
	}

	string->data[string->length - 1] = '\0';
	char* newData = realloc(string->data, sizeof(char) * string->length);
	if (newData != NULL) string->data = newData;
	string->length -= 1;

	*c = fgetc(stream);
	SkipWs(stream, c);
	return true;
}

static bool ReadNumeral(FILE* stream, char** numeral, int* c) {
	assert(stream != NULL);
	assert(numeral != NULL && *numeral == NULL);
	assert(c != NULL && *c != EOF && strchr("0123456789-", (char)*c) != NULL);

	*numeral = malloc(sizeof(char) * 2);
	if (*numeral == NULL) return false;
	(*numeral)[0] = (char)*c;
	size_t size = 2;
	size_t capacity = 2;

	// Data is now: [0-9-]

	if (*c == (unsigned char)'-') {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?[0-9]

	if (*c != (unsigned char)'0') {
		while (true) {
			*c = fgetc(stream);

			if (*c == EOF || strchr("0123456789.eE", (char)*c) == NULL) {
				// Data is now: -?[1-9][0-9]*
				goto success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
			(*numeral)[size - 2] = (char)*c;
		}
	} else {
		*c = fgetc(stream);

		if (*c == EOF || strchr(".eE", (char)*c) == NULL) {
			// Data is now: -?0
			goto success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?([1-9][0-9]*|0)[.eE]

	if (*c == (unsigned char)'.') {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;

		while (true) {
			*c = fgetc(stream);

			if (*c == EOF || strchr("0123456789eE", (char)*c) == NULL) {
				// Data is now: -?([1-9][0-9]*|0)\.[0-9]+
				goto success;
			}

			if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
			(*numeral)[size - 2] = (char)*c;
		}
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE]

	*c = fgetc(stream);
	if (*c == EOF) goto error;
	if (strchr("0123456789+-", (char)*c) == NULL) goto error;

	if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
	(*numeral)[size - 2] = (char)*c;

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][0-9+-]

	if (strchr("+-", (char)*c) != NULL) {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]

	while (true) {
		*c = fgetc(stream);

		if (*c == EOF || strchr("0123456789", (char)*c) == NULL) {
			// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]+
			goto success;
		}

		if (!GrowCharVec(numeral, &size, &capacity, 1)) goto error;
		(*numeral)[size - 2] = (char)*c;
	}

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

static bool ReadValue(FILE* stream, Ezjson_Value* value, int* c);

static bool ReadObject(FILE* stream, Ezjson_Object* object, int* c) {
	assert(stream != NULL);
	assert(object != NULL && object->items == NULL && object->length == 0);
	assert(c != NULL && *c == (unsigned char)'{');

	size_t capacity = 0;

	while (true) {
		*c = fgetc(stream);
		SkipWs(stream, c);
		if (*c != (unsigned char)'"') goto error1;

		if (!GrowKeyValueVec(&object->items, &object->length, &capacity, 1)) goto error1;
		object->items[object->length - 1] = (Ezjson_KeyValue){0};

		if (!ReadString(stream, &object->items[object->length - 1].key, c)) goto error1;
		if (*c != (unsigned char)':') goto error1;

		*c = fgetc(stream);
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

	*c = fgetc(stream);
	SkipWs(stream, c);
	return true;
}

static bool ReadArray(FILE* stream, Ezjson_Array* array, int* c) {
	assert(stream != NULL);
	assert(array != NULL && array->items == NULL && array->length == 0);
	assert(c != NULL && *c == '[');

	size_t capacity = 0;

	while (true) {
		*c = fgetc(stream);
		Ezjson_Value item = {0};
		if (!ReadValue(stream, &item, c)) goto error;
		if (*c == EOF) goto error;

		if (!GrowValueVec(&array->items, &array->length, &capacity, 1)) goto error;
		array->items[array->length - 1] = item;

		if (*c == (unsigned char)',') continue;
		if (*c == (unsigned char)']') break;

	error:
		free(array->items);
		*array = (Ezjson_Array){0};
		return false;
	}

	if (array->length != 0) {
		assert(array->length <= SIZE_MAX / sizeof(Ezjson_Value));
		Ezjson_Value* newItems = realloc(array->items, sizeof(Ezjson_Value) * array->length);
		if (newItems != NULL) array->items = newItems;
	}

	*c = fgetc(stream);
	SkipWs(stream, c);
	return true;
}

static bool ReadValue(FILE* stream, Ezjson_Value* value, int* c) {
	assert(stream != NULL);
	assert(value != NULL);
	assert(c != NULL && (*c == EOF || (*c >= 0 && *c <= UCHAR_MAX)));

	SkipWs(stream, c);

	if (*c == (unsigned char)'{') {
		*value = (Ezjson_Value){.kind = EZJSON_KIND_OBJECT, .object = (Ezjson_Object){0}};
		return ReadObject(stream, &value->object, c);
	}

	if (*c == (unsigned char)'[') {
		*value = (Ezjson_Value){.kind = EZJSON_KIND_ARRAY, .array = (Ezjson_Array){0}};
		return ReadArray(stream, &value->array, c);
	}

	if (*c == (unsigned char)'"') {
		*value = (Ezjson_Value){.kind = EZJSON_KIND_STRING, .string = (Ezjson_String){0}};
		return ReadString(stream, &value->string, c);
	}

	if (*c != EOF && strchr("0123456789-", (char)*c) != NULL) {
		*value = (Ezjson_Value){.kind = EZJSON_KIND_NUMBER, .number = 0.0};

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

		if ((*c = fgetc(stream)) != (unsigned char)'r') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'u') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'e') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'f') {
		value->kind = EZJSON_KIND_FALSE;

		if ((*c = fgetc(stream)) != (unsigned char)'a') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'s') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'e') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		return true;
	}

	if (*c == (unsigned char)'n') {
		value->kind = EZJSON_KIND_NULL;

		if ((*c = fgetc(stream)) != (unsigned char)'u') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		return true;
	}

	return false;
}

bool Ezjson_Read(FILE* stream, Ezjson_Value* json) {
	assert(stream != NULL);
	assert(json != NULL);

#if defined(__unix__)
	flockfile(stream);
#elif defined(_WIN32)
	_lock_file(stream);
#endif
	int c = fgetc(stream);

	if (c == 0xEF) {  // Skip BOM
		if ((c = fgetc(stream)) != 0xBB) return false;
		if ((c = fgetc(stream)) != 0xBF) return false;
		c = fgetc(stream);
	}

	bool ok = ReadValue(stream, json, &c);
	ungetc(c, stream);
#if defined(__unix__)
	funlockfile(stream);
#elif defined(_WIN32)
	_unlock_file(stream);
#endif
	return ok && c == EOF;
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
