#include "ezjson/json.h"

#include <assert.h>
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
	assert(c != NULL);

	while (*c != EOF && strchr(" \t\n\r", (char)*c) != NULL) {
		*c = fgetc(stream);
	}
}

static bool ReadString(FILE* stream, Ezjson_String* string, int* c) {
	assert(stream != NULL);
	assert(string != NULL);
	assert(c != NULL && *c == (unsigned char)'"');

	char* data = malloc(sizeof(char));
	size_t size = 1;
	size_t capacity = 1;
	if (data == NULL) return false;

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
				if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
				data[size - 2] = escapeValue[p - escapeKey];
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
					if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
					data[size - 2] = (unsigned char)(0x00 | ((cp >> 0) & 0x7F));
				} else if (cp <= 0x7FF) {
					if (!GrowCharVec(&data, &size, &capacity, 2)) goto error;
					data[size - 3] = (unsigned char)(0xC0 | ((cp >> 6) & 0xFF));
					data[size - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else if (cp <= 0xFFFF) {
					if (!GrowCharVec(&data, &size, &capacity, 3)) goto error;
					data[size - 4] = (unsigned char)(0xE0 | ((cp >> 12) & 0x0F));
					data[size - 3] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
					data[size - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else if (cp <= 0x10FFFF) {
					if (!GrowCharVec(&data, &size, &capacity, 4)) goto error;
					data[size - 5] = (unsigned char)(0xF0 | ((cp >> 18) & 0x07));
					data[size - 4] = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
					data[size - 3] = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
					data[size - 2] = (unsigned char)(0x80 | ((cp >> 0) & 0x3F));
				} else {
					goto error;
				}
			} else {
				goto error;
			}

			continue;
		}

		if (*c != EOF) {
			if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
			data[size - 2] = (char)*c;
			continue;
		}

	error:
		free(data);
		return false;
	}

	data[size - 1] = '\0';
	char* newData = realloc(data, sizeof(char) * size);
	if (newData != NULL) data = newData;

	*c = fgetc(stream);
	SkipWs(stream, c);
	string->data = data;
	string->length = size - 1;
	return true;
}

static bool ReadNumeral(FILE* stream, char** numeral, int* c) {
	assert(stream != NULL);
	assert(numeral != NULL);
	assert(c != NULL && *c != EOF && strchr("0123456789-", (char)*c) != NULL);

	char* data = malloc(sizeof(char) * 2);
	size_t size = 2;
	size_t capacity = 2;
	if (data == NULL) return false;
	data[0] = (char)*c;

	// Data is now: [0-9-]

	if (*c == (unsigned char)'-') {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
		data[size - 2] = (char)*c;
	}

	// Data is now: -?[0-9]

	if (*c != (unsigned char)'0') {
		while (true) {
			*c = fgetc(stream);

			if (*c == EOF || strchr("0123456789.eE", (char)*c) == NULL) {
				// Data is now: -?[1-9][0-9]*
				goto success;
			}

			if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
			data[size - 2] = (char)*c;
		}
	} else {
		*c = fgetc(stream);

		if (*c == EOF || strchr(".eE", (char)*c) == NULL) {
			// Data is now: -?0
			goto success;
		}

		if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
		data[size - 2] = (char)*c;
	}

	// Data is now: -?([1-9][0-9]*|0)[.eE]

	if (*c == (unsigned char)'.') {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
		data[size - 2] = (char)*c;

		while (true) {
			*c = fgetc(stream);

			if (*c == EOF || strchr("0123456789eE", (char)*c) == NULL) {
				// Data is now: -?([1-9][0-9]*|0)\.[0-9]+
				goto success;
			}

			if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
			data[size - 2] = (char)*c;
		}
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE]

	*c = fgetc(stream);
	if (*c == EOF) goto error;
	if (strchr("0123456789+-", (char)*c) == NULL) goto error;

	if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
	data[size - 2] = (char)*c;

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][0-9+-]

	if (strchr("+-", (char)*c) != NULL) {
		*c = fgetc(stream);
		if (*c == EOF) goto error;
		if (strchr("0123456789", (char)*c) == NULL) goto error;

		if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
		data[size - 2] = (char)*c;
	}

	// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]

	while (true) {
		*c = fgetc(stream);

		if (*c == EOF || strchr("0123456789", (char)*c) == NULL) {
			// Data is now: -?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]?[0-9]+
			goto success;
		}

		if (!GrowCharVec(&data, &size, &capacity, 1)) goto error;
		data[size - 2] = (char)*c;
	}

success:
	data[size - 1] = '\0';
	char* newData = realloc(data, sizeof(char) * size);
	if (newData != NULL) data = newData;

	SkipWs(stream, c);
	*numeral = data;
	return true;

error:
	free(numeral);
	return false;
}

static bool ReadValue(FILE* stream, Ezjson_Value* value, int* c);

static bool ReadObject(FILE* stream, Ezjson_Object* object, int* c) {
	assert(stream != NULL);
	assert(object != NULL);
	assert(c != NULL && *c == (unsigned char)'{');

	Ezjson_KeyValue* data = NULL;
	size_t length = 0;
	size_t capacity = 0;

	while (true) {
		*c = fgetc(stream);
		SkipWs(stream, c);
		if (*c != (unsigned char)'"') goto error1;

		if (!GrowKeyValueVec(&data, &length, &capacity, 1)) goto error1;

		if (!ReadString(stream, &data[length - 1].key, c)) goto error1;
		if (*c != (unsigned char)':') goto error1;

		*c = fgetc(stream);
		if (!ReadValue(stream, &data[length - 1].value, c)) goto error2;
		if (*c == (unsigned char)',') continue;
		if (*c == (unsigned char)'}') break;

	error2:
		free(data[length - 1].key.data);
	error1:
		free(data);
		return false;
	}

	if (length != 0) {
		assert(length <= SIZE_MAX / sizeof(Ezjson_KeyValue));
		Ezjson_KeyValue* newData = realloc(data, sizeof(Ezjson_KeyValue) * length);
		if (newData != NULL) data = newData;
	}

	*c = fgetc(stream);
	SkipWs(stream, c);
	object->items = data;
	object->length = length;
	return true;
}

static bool ReadArray(FILE* stream, Ezjson_Array* array, int* c) {
	assert(stream != NULL);
	assert(array != NULL);
	assert(c != NULL && *c == '[');

	Ezjson_Value* data = NULL;
	size_t length = 0;
	size_t capacity = 0;

	while (true) {
		*c = fgetc(stream);
		Ezjson_Value item;
		if (!ReadValue(stream, &item, c)) goto error;
		if (*c == EOF) goto error;

		if (!GrowValueVec(&data, &length, &capacity, 1)) return false;
		data[length - 1] = item;

		if (*c == (unsigned char)',') continue;
		if (*c == (unsigned char)']') break;

	error:
		free(data);
		return false;
	}

	if (length != 0) {
		assert(length <= SIZE_MAX / sizeof(Ezjson_Value));
		Ezjson_Value* newData = realloc(data, sizeof(Ezjson_Value) * length);
		if (newData != NULL) data = newData;
	}

	*c = fgetc(stream);
	SkipWs(stream, c);
	array->items = data;
	array->length = length;
	return true;
}

static bool ReadValue(FILE* stream, Ezjson_Value* value, int* c) {
	assert(stream != NULL);
	assert(value != NULL);
	assert(c != NULL);

	SkipWs(stream, c);

	if (*c == (unsigned char)'{') {
		Ezjson_Object object;
		if (!ReadObject(stream, &object, c)) return false;

		value->kind = EZJSON_KIND_OBJECT;
		value->object = object;
		return true;
	}

	if (*c == (unsigned char)'[') {
		Ezjson_Array array;
		if (!ReadArray(stream, &array, c)) return false;

		value->kind = EZJSON_KIND_ARRAY;
		value->array = array;
		return true;
	}

	if (*c == (unsigned char)'"') {
		Ezjson_String string;
		if (!ReadString(stream, &string, c)) return false;

		value->kind = EZJSON_KIND_STRING;
		value->string = string;
		return true;
	}

	if (*c != EOF && strchr("0123456789-", (char)*c) != NULL) {
		char* numeral;
		if (!ReadNumeral(stream, &numeral, c)) return false;

		char* end;
		double number = strtod(numeral, &end);
		bool ok = *end == '\0';
		free(numeral);
		if (!ok) return false;

		value->kind = EZJSON_KIND_NUMBER;
		value->number = number;
		return true;
	}

	if (*c == (unsigned char)'t') {
		if ((*c = fgetc(stream)) != (unsigned char)'r') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'u') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'e') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		value->kind = EZJSON_KIND_TRUE;
		return true;
	}

	if (*c == (unsigned char)'f') {
		if ((*c = fgetc(stream)) != (unsigned char)'a') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'s') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'e') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		value->kind = EZJSON_KIND_FALSE;
		return true;
	}

	if (*c == (unsigned char)'n') {
		if ((*c = fgetc(stream)) != (unsigned char)'u') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;
		if ((*c = fgetc(stream)) != (unsigned char)'l') return false;

		*c = fgetc(stream);
		SkipWs(stream, c);
		value->kind = EZJSON_KIND_NULL;
		return true;
	}

	return false;
}

bool Ezjson_Read(FILE* stream, Ezjson_Value* json) {
	assert(stream != NULL);
	assert(json != NULL);

	int c = fgetc(stream);

	if (c == (unsigned char)'\xEF') {  // Skip BOM
		if ((c = fgetc(stream)) != (unsigned char)'\xBB') return false;
		if ((c = fgetc(stream)) != (unsigned char)'\xBF') return false;
		c = fgetc(stream);
	}

	Ezjson_Value value;
	bool ok = ReadValue(stream, &value, &c);
	ungetc(c, stream);

	if (ok && c == EOF) {
		*json = value;
		return true;
	} else {
		return false;
	}
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
			const Ezjson_KeyValue* rightItem = &left->object.items[i];

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

	if (left->kind == EZJSON_KIND_NUMBER)
		return left->number == right->number;

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
