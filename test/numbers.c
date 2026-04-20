#undef NDEBUG

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <ezjson.h>

int main(void) {
	Ezjson_Value json = {0};
	assert(Ezjson_ReadMemory("42", 2, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = 42.0}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("-69.420", 7, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = -69.420}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("1.0E100", 7, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = 1.0E100}, SIZE_MAX, NULL));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("-1E-100", 7, &json, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = -1.0E-100}, SIZE_MAX, NULL));

	return EXIT_SUCCESS;
}
