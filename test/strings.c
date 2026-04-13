#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson/json.h>

#ifdef _WIN32
int wmain(void) {
#else
int main(void) {
#endif
	Ezjson_Value json = {0};
	assert(Ezjson_MemoryRead("\"\\uD834\\uDD1E\"", 14, &json));

	assert(Ezjson_Equal(
		&json,
		&(Ezjson_Value){
			.kind = EZJSON_KIND_STRING,
			.string = {.length = 4, .data = "\xF0\x9D\x84\x9E"}
		}
	));

	json = (Ezjson_Value){0};
	assert(Ezjson_MemoryRead("\"\\uD834 \\uDD1E\"", 15, &json));

	assert(Ezjson_Equal(
		&json,
		&(Ezjson_Value){
			.kind = EZJSON_KIND_STRING,
			.string = {.length = 7, .data = "\xED\xA0\xB4 \xED\xB4\x9E"}
		}
	));

	return EXIT_SUCCESS;
}
