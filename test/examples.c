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
			.key = {EZJSON_SAS("Image")},
			.value.kind = EZJSON_OBJECT,
			.value.object.length = 6,
			.value.object.items = (Ezjson_KeyValue[]){
				{
					.key = {EZJSON_SAS("Width")},
					.value = {EZJSON_NUMBER, .number = 800.0},
				},
				{
					.key = {EZJSON_SAS("Height")},
					.value = {EZJSON_NUMBER, .number = 600.0},
				},
				{
					.key = {EZJSON_SAS("Title")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("View from 15th Floor")}},
				},
				{
					.key = {EZJSON_SAS("Thumbnail")},
					.value.kind = EZJSON_OBJECT,
					.value.object.length = 3,
					.value.object.items = (Ezjson_KeyValue[]){
						{
							.key = {EZJSON_SAS("Url")},
							.value = {EZJSON_STRING, .string = {EZJSON_SAS("http://www.example.com/image/481989943")}},
						},
						{
							.key = {EZJSON_SAS("Height")},
							.value = {EZJSON_NUMBER, .number = 125.0}
						},
						{
							.key = {EZJSON_SAS("Width")},
							.value = {EZJSON_NUMBER, .number = 100.0},
						},
					},
				},
				{
					.key = {EZJSON_SAS("Animated")},
					.value = {EZJSON_BOOLEAN, .boolean = false},
				},
				{
					.key = {EZJSON_SAS("IDs")},
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
					.key = {EZJSON_SAS("precision")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("zip")}},
				},
				{
					.key = {EZJSON_SAS("Latitude")},
					.value = {EZJSON_NUMBER, .number = 37.7668},
				},
				{
					.key = {EZJSON_SAS("Longitude")},
					.value = {EZJSON_NUMBER, .number = -122.3959},
				},
				{
					.key = {EZJSON_SAS("Address")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("")}},
				},
				{
					.key = {EZJSON_SAS("City")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("SAN FRANCISCO")}},
				},
				{
					.key = {EZJSON_SAS("State")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("CA")}},
				},
				{
					.key = {EZJSON_SAS("Zip")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("94107")}},
				},
				{
					.key = {EZJSON_SAS("Country")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("US")}},
				},
			},
		},
		{
			.kind = EZJSON_OBJECT,
			.object.length = 8,
			.object.items = (Ezjson_KeyValue[]){
				{
					.key = {EZJSON_SAS("precision")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("zip")}},
				},
				{
					.key = {EZJSON_SAS("Latitude")},
					.value = {EZJSON_NUMBER, .number = 37.371991},
				},
				{
					.key = {EZJSON_SAS("Longitude")},
					.value = {.kind = EZJSON_NUMBER, .number = -122.026020},
				},
				{
					.key = {EZJSON_SAS("Address")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("")}},
				},
				{
					.key = {EZJSON_SAS("City")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("SUNNYVALE")}},
				},
				{
					.key = {EZJSON_SAS("State")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("CA")}},
				},
				{
					.key = {EZJSON_SAS("Zip")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("94085")}},
				},
				{
					.key = {EZJSON_SAS("Country")},
					.value = {EZJSON_STRING, .string = {EZJSON_SAS("US")}},
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
	assert(Ezjson_ReadFile(&actual, stream, SIZE_MAX, NULL));
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
	assert(Ezjson_ReadFile(&actual, stream, SIZE_MAX, NULL));
	assert(Ezjson_Equals(&actual, &expected2, SIZE_MAX, NULL));

	return EXIT_SUCCESS;
}
