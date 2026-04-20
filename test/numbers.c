#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson.h>

int main(void) {
	Ezjson_Value json = {0};
	assert(Ezjson_ReadMemory("42", 2, &json, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = 42.0}));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("-69.420", 7, &json, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = -69.420}));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("1.0E100", 7, &json, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = 1.0E100}));

	json = (Ezjson_Value){0};
	assert(Ezjson_ReadMemory("-1E-100", 7, &json, NULL));
	assert(Ezjson_Equals(&json, &(Ezjson_Value){EZJSON_NUMBER, .number = -1.0E-100}));

	return EXIT_SUCCESS;
}
