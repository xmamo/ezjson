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
	{
#ifdef _WIN32
		FILE* stream = NULL;
		_wfopen_s(&stream, L"string1.json", L"r");
#else
		FILE* stream = fopen("string1.json", "r");
#endif
		Ezjson_Value json = {0};
		assert(Ezjson_Read(stream, &json));

		assert(Ezjson_Equal(
			&json,
			&(Ezjson_Value){
				.kind = EZJSON_KIND_STRING,
				.string = {.length = 4, .data = "\xF0\x9D\x84\x9E"}
			}
		));
	}

	{
#ifdef _WIN32
		FILE* stream = NULL;
		_wfopen_s(&stream, L"string2.json", L"r");
#else
		FILE* stream = fopen("string2.json", "r");
#endif
		Ezjson_Value json = {0};
		assert(Ezjson_Read(stream, &json));

		assert(Ezjson_Equal(
			&json,
			&(Ezjson_Value){
				.kind = EZJSON_KIND_STRING,
				.string = {.length = 7, .data = "\xED\xA0\xB4 \xED\xB4\x9E"}
			}
		));
	}

	return EXIT_SUCCESS;
}
