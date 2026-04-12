#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <ezjson/json.h>

static Ezjson_Value expected = {
	.kind = EZJSON_KIND_ARRAY,
	.array = {
		.length = 2,
		.items = (Ezjson_Value[]){
			{
				.kind = EZJSON_KIND_OBJECT,
				.object = {
					.length = 8,
					.items = (Ezjson_KeyValue[]){
						{
							.key = {.length = 9, .data = "precision"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 3, .data = "zip"},
							},
						},
						{
							.key = {.length = 8, .data = "Latitude"},
							.value = {.kind = EZJSON_KIND_NUMBER, .number = 37.7668},
						},
						{
							.key = {.length = 9, .data = "Longitude"},
							.value = {.kind = EZJSON_KIND_NUMBER, .number = -122.3959},
						},
						{
							.key = {.length = 7, .data = "Address"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 0, .data = ""},
							},
						},
						{
							.key = {.length = 4, .data = "City"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 13, .data = "SAN FRANCISCO"},
							},
						},
						{
							.key = {.length = 5, .data = "State"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 2, .data = "CA"},
							},
						},
						{
							.key = {.length = 3, .data = "Zip"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 5, .data = "94107"},
							},
						},
						{
							.key = {.length = 7, .data = "Country"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 2, .data = "US"},
							},
						},
					},
				},
			},
			{
				.kind = EZJSON_KIND_OBJECT,
				.object = {
					.length = 8,
					.items = (Ezjson_KeyValue[]){
						{
							.key = {.length = 9, .data = "precision"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 3, .data = "zip"},
							},
						},
						{
							.key = {.length = 8, .data = "Latitude"},
							.value = {.kind = EZJSON_KIND_NUMBER, .number = 37.371991},
						},
						{
							.key = {.length = 9, .data = "Longitude"},
							.value = {.kind = EZJSON_KIND_NUMBER, .number = -122.026020},
						},
						{
							.key = {.length = 7, .data = "Address"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 0, .data = ""},
							},
						},
						{
							.key = {.length = 4, .data = "City"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 9, .data = "SUNNYVALE"},
							},
						},
						{
							.key = {.length = 5, .data = "State"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 2, .data = "CA"},
							},
						},
						{
							.key = {.length = 3, .data = "Zip"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 5, .data = "94085"},
							},
						},
						{
							.key = {.length = 7, .data = "Country"},
							.value = {
								.kind = EZJSON_KIND_STRING,
								.string = {.length = 2, .data = "US"},
							},
						},
					},
				},
			},
		},
	},
};

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
	(void)argc;
	FILE* stream = NULL;
	_wfopen_s(&stream, argv[1], L"r");
#else
int main(int argc, char** argv) {
	(void)argc;
	FILE* stream = fopen(argv[1], "r");
#endif
	Ezjson_Value actual = {0};
	assert(Ezjson_Read(stream, &actual));
	assert(Ezjson_Equal(&actual, &expected));
	return EXIT_SUCCESS;
}
