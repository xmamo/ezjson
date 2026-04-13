#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <ezjson/json.h>

static Ezjson_Value expected = {
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

#ifdef _WIN32
int wmain(void) {
	FILE* stream = NULL;
	_wfopen_s(&stream, L"example2.json", L"r");
#else
int main(void) {
	FILE* stream = fopen("example2.json", "r");
#endif
	Ezjson_Value actual = {0};
	assert(Ezjson_FileRead(stream, &actual));
	assert(Ezjson_Equal(&actual, &expected));
	return EXIT_SUCCESS;
}
