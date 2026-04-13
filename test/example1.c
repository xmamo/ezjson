#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson/json.h>

static const Ezjson_Value expected = {
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

#ifdef _WIN32
int wmain(void) {
	FILE* stream = NULL;
	_wfopen_s(&stream, L"example1.json", L"r");
#else
int main(void) {
	FILE* stream = fopen("example1.json", "r");
#endif
	Ezjson_Value actual = {0};
	assert(Ezjson_FileRead(stream, &actual));
	assert(Ezjson_Equal(&actual, &expected));
	return EXIT_SUCCESS;
}
