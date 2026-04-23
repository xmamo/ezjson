#undef NDEBUG

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include <ezjson.h>

int main(void) {
	Ezjson_Value json = {0};
	assert(Ezjson_ReadMemory(&json, EZJSON_SAS("\"\\\"\\/\\b\\f\\n\\r\\t\""), SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {EZJSON_SAS("\"/\b\f\n\r\t")}}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory(&json, EZJSON_SAS("\"\\uD834\\uDD1E\""), SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {EZJSON_SAS("\xF0\x9D\x84\x9E")}}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory(&json, EZJSON_SAS("\"\\uD834\\u0000\\uDD1E\""), SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_STRING, .string = {EZJSON_SAS("\xED\xA0\xB4\0\xED\xB4\x9E")}}, SIZE_MAX, NULL));

	return EXIT_SUCCESS;
}
