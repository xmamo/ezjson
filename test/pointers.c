#undef NDEBUG
#define __STDC_WANT_LIB_EXT1__ 1
#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson.h>

static const Ezjson_Value expected = {
	.kind = EZJSON_OBJECT,
	.object.length = 10,
	.object.items = (Ezjson_KeyValue[]){
		{
			.key = {EZJSON_SAS("foo")},
			.value.kind = EZJSON_ARRAY,
			.value.array.length = 2,
			.value.array.items = (Ezjson_Value[]){
				{EZJSON_STRING, .string = {EZJSON_SAS("bar")}},
				{EZJSON_STRING, .string = {EZJSON_SAS("baz")}},
			},
		},
		{
			.key = {EZJSON_SAS("")},
			.value = {EZJSON_NUMBER, .number = 0.0},
		},
		{
			.key = {EZJSON_SAS("a/b")},
			.value = {EZJSON_NUMBER, .number = 1.0},
		},
		{
			.key = {EZJSON_SAS("c%d")},
			.value = {EZJSON_NUMBER, .number = 2.0},
		},
		{
			.key = {EZJSON_SAS("e^f")},
			.value = {EZJSON_NUMBER, .number = 3.0},
		},
		{
			.key = {EZJSON_SAS("g|h")},
			.value = {EZJSON_NUMBER, .number = 4.0},
		},
		{
			.key = {EZJSON_SAS("i\\j")},
			.value = {EZJSON_NUMBER, .number = 5.0},
		},
		{
			.key = {EZJSON_SAS("k\"l")},
			.value = {EZJSON_NUMBER, .number = 6.0},
		},
		{
			.key = {EZJSON_SAS(" ")},
			.value = {EZJSON_NUMBER, .number = 7.0},
		},
		{
			.key = {EZJSON_SAS("m~n")},
			.value = {EZJSON_NUMBER, .number = 8.0},
		},
	},
};

int main(void) {
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
	FILE* stream = NULL;
	errno_t error = fopen_s(&stream, "pointers.json", "r");
	assert(error == 0);
#else
	FILE* stream = fopen("pointers.json", "r");
	assert(stream != NULL);
#endif

	Ezjson_Value actual = {0};
	assert(Ezjson_ReadFile(&actual, stream, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&actual, &expected, SIZE_MAX, NULL));

	assert(Ezjson_At(&actual, EZJSON_SAS(""), NULL) == &actual);
	assert(Ezjson_At(&actual, EZJSON_SAS("/foo"), NULL) == &actual.object.items[0].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/foo/0"), NULL) == &actual.object.items[0].value.array.items[0]);
	assert(Ezjson_At(&actual, EZJSON_SAS("/"), NULL) == &actual.object.items[1].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/a~1b"), NULL) == &actual.object.items[2].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/c%d"), NULL) == &actual.object.items[3].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/e^f"), NULL) == &actual.object.items[4].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/g|h"), NULL) == &actual.object.items[5].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/i\\j"), NULL) == &actual.object.items[6].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/k\"l"), NULL) == &actual.object.items[7].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/ "), NULL) == &actual.object.items[8].value);
	assert(Ezjson_At(&actual, EZJSON_SAS("/m~0n"), NULL) == &actual.object.items[9].value);

	return EXIT_SUCCESS;
}
