#undef NDEBUG

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include <ezjson.h>

int main(void) {
	Ezjson_Value json = {0};
	assert(Ezjson_ReadMemory("\"\\\"\\/\\b\\f\\n\\r\\t\"", 16, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {"\"/\b\f\n\r\t", 7}}, SIZE_MAX, NULL));

	assert(Ezjson_ReadMemory("\"\\uD834\\uDD1E\"", 14, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {"\xF0\x9D\x84\x9E", 4}}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("\"\\uD834\\u0000\\uDD1E\"", 20, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {"\xED\xA0\xB4\0\xED\xB4\x9E", 7}}, SIZE_MAX, NULL));

	return EXIT_SUCCESS;
}
