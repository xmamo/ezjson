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
			.key = {"foo", 3},
			.value.kind = EZJSON_ARRAY,
			.value.array.length = 2,
			.value.array.items = (Ezjson_Value[]){
				{EZJSON_STRING, .string = {"bar", 3}},
				{EZJSON_STRING, .string = {"baz", 3}},
			},
		},
		{
			.key = {"", 0},
			.value = {EZJSON_NUMBER, .number = 0.0},
		},
		{
			.key = {"a/b", 3},
			.value = {EZJSON_NUMBER, .number = 1.0},
		},
		{
			.key = {"c%d", 3},
			.value = {EZJSON_NUMBER, .number = 2.0},
		},
		{
			.key = {"e^f", 3},
			.value = {EZJSON_NUMBER, .number = 3.0},
		},
		{
			.key = {"g|h", 3},
			.value = {EZJSON_NUMBER, .number = 4.0},
		},
		{
			.key = {"i\\j", 3},
			.value = {EZJSON_NUMBER, .number = 5.0},
		},
		{
			.key = {"k\"l", 3},
			.value = {EZJSON_NUMBER, .number = 6.0},
		},
		{
			.key = {" ", 1},
			.value = {EZJSON_NUMBER, .number = 7.0},
		},
		{
			.key = {"m~n", 3},
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
	assert(Ezjson_ReadFile(stream, &actual, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&actual, &expected, SIZE_MAX, NULL));

	assert(Ezjson_At(&actual, "", 0, NULL) == &actual);
	assert(Ezjson_At(&actual, "/foo", 4, NULL) == &actual.object.items[0].value);
	assert(Ezjson_At(&actual, "/foo/0", 6, NULL) == &actual.object.items[0].value.array.items[0]);
	assert(Ezjson_At(&actual, "/", 1, NULL) == &actual.object.items[1].value);
	assert(Ezjson_At(&actual, "/a~1b", 5, NULL) == &actual.object.items[2].value);
	assert(Ezjson_At(&actual, "/c%d", 4, NULL) == &actual.object.items[3].value);
	assert(Ezjson_At(&actual, "/e^f", 4, NULL) == &actual.object.items[4].value);
	assert(Ezjson_At(&actual, "/g|h", 4, NULL) == &actual.object.items[5].value);
	assert(Ezjson_At(&actual, "/i\\j", 4, NULL) == &actual.object.items[6].value);
	assert(Ezjson_At(&actual, "/k\"l", 4, NULL) == &actual.object.items[7].value);
	assert(Ezjson_At(&actual, "/ ", 2, NULL) == &actual.object.items[8].value);
	assert(Ezjson_At(&actual, "/m~0n", 5, NULL) == &actual.object.items[9].value);

	return EXIT_SUCCESS;
}
