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

static const Ezjson_Value expected1 = {
	.kind = EZJSON_OBJECT,
	.object.length = 1,
	.object.items = (Ezjson_KeyValue[]){
		{
			.key = {"Image", 5},
			.value.kind = EZJSON_OBJECT,
			.value.object.length = 6,
			.value.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"Width", 5},
					.value = {EZJSON_NUMBER, .number = 800.0},
				},
				{
					.key = {"Height", 6},
					.value = {EZJSON_NUMBER, .number = 600.0},
				},
				{
					.key = {"Title", 5},
					.value = {EZJSON_STRING, .string = {"View from 15th Floor", 20}},
				},
				{
					.key = {"Thumbnail", 9},
					.value.kind = EZJSON_OBJECT,
					.value.object.length = 3,
					.value.object.items = (Ezjson_KeyValue[]){
						{
							.key = {"Url", 3},
							.value = {EZJSON_STRING, .string = {"http://www.example.com/image/481989943", 38}},
						},
						{
							.key = {"Height", 6},
							.value = {EZJSON_NUMBER, .number = 125.0}
						},
						{
							.key = {"Width", 5},
							.value = {EZJSON_NUMBER, .number = 100.0},
						},
					},
				},
				{
					.key = {"Animated", 8},
					.value = {EZJSON_BOOLEAN, .boolean = false},
				},
				{
					.key = {"IDs", 3},
					.value.kind = EZJSON_ARRAY,
					.value.array.length = 4,
					.value.array.items = (Ezjson_Value[]){
						{EZJSON_NUMBER, .number = 116.0},
						{EZJSON_NUMBER, .number = 943.0},
						{EZJSON_NUMBER, .number = 234.0},
						{EZJSON_NUMBER, .number = 38793.0},
					},
				},
			},
		},
	},
};

static const Ezjson_Value expected2 = {
	.kind = EZJSON_ARRAY,
	.array.length = 2,
	.array.items = (Ezjson_Value[]){
		{
			.kind = EZJSON_OBJECT,
			.object.length = 8,
			.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"precision", 9},
					.value = {EZJSON_STRING, .string = {"zip", 3}},
				},
				{
					.key = {"Latitude", 8},
					.value = {EZJSON_NUMBER, .number = 37.7668},
				},
				{
					.key = {"Longitude", 9},
					.value = {EZJSON_NUMBER, .number = -122.3959},
				},
				{
					.key = {"Address", 7},
					.value = {EZJSON_STRING, .string = {"", 0}},
				},
				{
					.key = {"City", 4},
					.value = {EZJSON_STRING, .string = {"SAN FRANCISCO", 13}},
				},
				{
					.key = {"State", 5},
					.value = {EZJSON_STRING, .string = {"CA", 2}},
				},
				{
					.key = {"Zip", 3},
					.value = {EZJSON_STRING, .string = {"94107", 5}},
				},
				{
					.key = {"Country", 7},
					.value = {EZJSON_STRING, .string = {"US", 2}},
				},
			},
		},
		{
			.kind = EZJSON_OBJECT,
			.object.length = 8,
			.object.items = (Ezjson_KeyValue[]){
				{
					.key = {"precision", 9},
					.value = {EZJSON_STRING, .string = {"zip", 3}},
				},
				{
					.key = {"Latitude", 8},
					.value = {EZJSON_NUMBER, .number = 37.371991},
				},
				{
					.key = {"Longitude", 9},
					.value = {.kind = EZJSON_NUMBER, .number = -122.026020},
				},
				{
					.key = {"Address", 7},
					.value = {EZJSON_STRING, .string = {"", 0}},
				},
				{
					.key = {"City", 4},
					.value = {EZJSON_STRING, .string = {"SUNNYVALE", 9}},
				},
				{
					.key = {"State", 5},
					.value = {EZJSON_STRING, .string = {"CA", 2}},
				},
				{
					.key = {"Zip", 3},
					.value = {EZJSON_STRING, .string = {"94085", 5}},
				},
				{
					.key = {"Country", 7},
					.value = {EZJSON_STRING, .string = {"US", 2}},
				},
			},
		},
	},
};

int main(void) {
#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
	FILE* stream = NULL;
	errno_t error = fopen_s(&stream, "example1.json", "r");
	assert(error == 0);
#else
	FILE* stream = fopen("example1.json", "r");
	assert(stream != NULL);
#endif
	Ezjson_Value actual = {0};
	assert(Ezjson_ReadFile(stream, &actual, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&actual, &expected1, SIZE_MAX, NULL));

#if defined(__STDC_LIB_EXT1__) || defined(__STDC_SECURE_LIB__)
	stream = NULL;
	error = fopen_s(&stream, "example2.json", "r");
	assert(error == 0);
#else
	stream = fopen("example2.json", "r");
	assert(stream != NULL);
#endif
	actual = (Ezjson_Value){0};
	assert(Ezjson_ReadFile(stream, &actual, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&actual, &expected2, SIZE_MAX, NULL));

	return EXIT_SUCCESS;
}
