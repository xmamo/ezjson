#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <ezjson/json.h>

int main(void) {
	Ezjson_Value json = {0};
	assert(Ezjson_MemoryRead("42", 2, &json));
	assert(Ezjson_Equal(&json, &(Ezjson_Value){EZJSON_KIND_NUMBER, .number = 42.0}));

	json = (Ezjson_Value){0};
	assert(Ezjson_MemoryRead("-69.420", 7, &json));
	assert(Ezjson_Equal(&json, &(Ezjson_Value){EZJSON_KIND_NUMBER, .number = -69.420}));

	json = (Ezjson_Value){0};
	assert(Ezjson_MemoryRead("1.0E100", 7, &json));
	assert(Ezjson_Equal(&json, &(Ezjson_Value){EZJSON_KIND_NUMBER, .number = 1.0E100}));

	json = (Ezjson_Value){0};
	assert(Ezjson_MemoryRead("-1E-100", 7, &json));
	assert(Ezjson_Equal(&json, &(Ezjson_Value){EZJSON_KIND_NUMBER, .number = -1.0E-100}));

	return EXIT_SUCCESS;
}
