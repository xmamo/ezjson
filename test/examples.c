#undef NDEBUG
#define __STDC_WANT_LIB_EXT1__
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson/json.h>

static Ezjson_Value expected1 = {
	.kind = EZJSON_KIND_OBJECT,
	.object.length = 1,
	.object.items = (Ezjson_KeyValue[]){
		{
			.key = {"Image", 5},
			.value.kind = EZJSON_KIND_OBJECT,
			.value.object.length = 6,
			.value.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"Width", 5},
					.value = {EZJSON_KIND_NUMBER, .number = 800.0},
				},
				{
					.key = {"Height", 6},
					.value = {EZJSON_KIND_NUMBER, .number = 600.0},
				},
				{
					.key = {"Title", 5},
					.value = {EZJSON_KIND_STRING, .string = {"View from 15th Floor", 20}},
				},
				{
					.key = {"Thumbnail", 9},
					.value.kind = EZJSON_KIND_OBJECT,
					.value.object.length = 3,
					.value.object.items = (Ezjson_KeyValue[]){
						{
							.key = {"Url", 3},
							.value = {EZJSON_KIND_STRING, .string = {"http://www.example.com/image/481989943", 38}},
						},
						{
							.key = {"Height", 6},
							.value = {EZJSON_KIND_NUMBER, .number = 125.0}
						},
						{
							.key = {"Width", 5},
							.value = {EZJSON_KIND_NUMBER, .number = 100.0},
						},
					},
				},
				{
					.key = {"Animated", 8},
					.value.kind = EZJSON_KIND_FALSE,
				},
				{
					.key = {"IDs", 3},
					.value.kind = EZJSON_KIND_ARRAY,
					.value.array.length = 4,
					.value.array.items = (Ezjson_Value[]){
						{EZJSON_KIND_NUMBER, .number = 116.0},
						{EZJSON_KIND_NUMBER, .number = 943.0},
						{EZJSON_KIND_NUMBER, .number = 234.0},
						{EZJSON_KIND_NUMBER, .number = 38793.0},
					},
				},
			},
		},
	},
};

static Ezjson_Value expected2 = {
	.kind = EZJSON_KIND_ARRAY,
	.array.length = 2,
	.array.items = (Ezjson_Value[]){
		{
			.kind = EZJSON_KIND_OBJECT,
			.object.length = 8,
			.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"precision", 9},
					.value = {EZJSON_KIND_STRING, .string = {"zip", 3}},
				},
				{
					.key = {"Latitude", 8},
					.value = {EZJSON_KIND_NUMBER, .number = 37.7668},
				},
				{
					.key = {"Longitude", 9},
					.value = {EZJSON_KIND_NUMBER, .number = -122.3959},
				},
				{
					.key = {"Address", 7},
					.value = {EZJSON_KIND_STRING, .string = {"", 0}},
				},
				{
					.key = {"City", 4},
					.value = {EZJSON_KIND_STRING, .string = {"SAN FRANCISCO", 13}},
				},
				{
					.key = {"State", 5},
					.value = {EZJSON_KIND_STRING, .string = {"CA", 2}},
				},
				{
					.key = {"Zip", 3},
					.value = {EZJSON_KIND_STRING, .string = {"94107", 5}},
				},
				{
					.key = {"Country", 7},
					.value = {EZJSON_KIND_STRING, .string = {"US", 2}},
				},
			},
		},
		{
			.kind = EZJSON_KIND_OBJECT,
			.object.length = 8,
			.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"precision", 9},
					.value = {EZJSON_KIND_STRING, .string = {"zip", 3}},
				},
				{
					.key = {"Latitude", 8},
					.value = {EZJSON_KIND_NUMBER, .number = 37.371991},
				},
				{
					.key = {"Longitude", 9},
					.value = {.kind = EZJSON_KIND_NUMBER, .number = -122.026020},
				},
				{
					.key = {"Address", 7},
					.value = {EZJSON_KIND_STRING, .string = {"", 0}},
				},
				{
					.key = {"City", 4},
					.value = {EZJSON_KIND_STRING, .string = {"SUNNYVALE", 9}},
				},
				{
					.key = {"State", 5},
					.value = {EZJSON_KIND_STRING, .string = {"CA", 2}},
				},
				{
					.key = {"Zip", 3},
					.value = {EZJSON_KIND_STRING, .string = {"94085", 5}},
				},
				{
					.key = {"Country", 7},
					.value = {EZJSON_KIND_STRING, .string = {"US", 2}},
				},
			},
		},
	},
};

int main(void) {
#if (defined(__STDC_LIB_EXT1__) && defined(__STDC_WANT_LIB_EXT1__)) || (defined(__STDC_SECURE_LIB__) && defined(__STDC_WANT_SECURE_LIB__))
	FILE* stream = NULL;
	fopen_s(&stream, "example1.json", "r");
#else
	FILE* stream = fopen("example1.json", "r");
#endif
	Ezjson_Value actual = {0};
	assert(Ezjson_ReadFile(stream, &actual));
	assert(Ezjson_Equals(&actual, &expected1));

#if (defined(__STDC_LIB_EXT1__) && defined(__STDC_WANT_LIB_EXT1__)) || (defined(__STDC_SECURE_LIB__) && defined(__STDC_WANT_SECURE_LIB__))
	stream = NULL;
	fopen_s(&stream, "example2.json", "r");
#else
	stream = fopen("example1.json", "r");
#endif
	actual = (Ezjson_Value){0};
	assert(Ezjson_ReadFile(stream, &actual));
	assert(Ezjson_Equals(&actual, &expected2));

	return EXIT_SUCCESS;
}
