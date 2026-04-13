#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ezjson/json.h>

static const Ezjson_Value expected = {
	.kind = EZJSON_KIND_OBJECT,
	.object = {
		.length = 1,
		.items = (Ezjson_KeyValue[]){
			{
				.key = {.length = 5, .data = "Image"},
				.value = {
					.kind = EZJSON_KIND_OBJECT,
					.object = {
						.length = 6,
						.items = (Ezjson_KeyValue[]){
							{
								.key = {.length = 5, .data = "Width"},
								.value = {
									.kind = EZJSON_KIND_NUMBER,
									.number = 800.0,
								},
							},
							{
								.key = {.length = 6, .data = "Height"},
								.value = {
									.kind = EZJSON_KIND_NUMBER,
									.number = 600.0,
								},
							},
							{
								.key = {.length = 5, .data = "Title"},
								.value = {
									.kind = EZJSON_KIND_STRING,
									.string = {.length = 20, .data = "View from 15th Floor"},
								},
							},
							{
								.key = {.length = 9, .data = "Thumbnail"},
								.value = {
									.kind = EZJSON_KIND_OBJECT,
									.object = {
										.length = 3,
										.items = (Ezjson_KeyValue[]){
											{
												.key = {.length = 3, .data = "Url"},
												.value = {
													.kind = EZJSON_KIND_STRING,
													.string = {
														.length = 38,
														.data = "http://www.example.com/image/481989943",
													},
												},
											},
											{
												.key = {.length = 6, .data = "Height"},
												.value = {.kind = EZJSON_KIND_NUMBER, .number = 125.0}
											},
											{
												.key = {.length = 5, .data = "Width"},
												.value = {.kind = EZJSON_KIND_NUMBER, .number = 100.0},
											},
										},
									},
								},
							},
							{
								.key = {.length = 8, .data = "Animated"},
								.value = {.kind = EZJSON_KIND_FALSE},
							},
							{
								.key = {.length = 3, .data = "IDs"},
								.value = {
									.kind = EZJSON_KIND_ARRAY,
									.array = {
										.length = 4,
										.items = (Ezjson_Value[]){
											{.kind = EZJSON_KIND_NUMBER, .number = 116.0},
											{.kind = EZJSON_KIND_NUMBER, .number = 943.0},
											{.kind = EZJSON_KIND_NUMBER, .number = 234.0},
											{.kind = EZJSON_KIND_NUMBER, .number = 38793.0},
										},
									},
								},
							},
						},
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
