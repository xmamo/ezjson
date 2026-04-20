#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ezjson.h>

static Ezjson_Value expected = {
	.kind = EZJSON_ARRAY,
	.array.length = 2,
	.array.items = (Ezjson_Value[]){
		{
			.kind = EZJSON_ARRAY,
			.array.length = 0,
			.array.items = NULL,
		},
		{
			.kind = EZJSON_ARRAY,
			.array.length = 1,
			.array.items = (Ezjson_Value[]){
				{
					.kind = EZJSON_ARRAY,
					.array.length = 0,
					.array.items = NULL,
				},
			},
		},
	},
};

int main(void) {
	Ezjson_Value json = {0};
	Ezjson_Error error = EZJSON_NO_ERROR;
	assert(Ezjson_ReadMemory("[[], [[]]]", 10, &json, 3, &error));
	assert(Ezjson_Equals(&json, &expected, 3, &error));
	assert(!Ezjson_Equals(&json, &expected, 2, &error) && error == EZJSON_DEPTH_ERROR);
	assert(!Ezjson_Equals(&json, &expected, 1, &error) && error == EZJSON_DEPTH_ERROR);
	assert(!Ezjson_Equals(&json, &expected, 0, &error) && error == EZJSON_DEPTH_ERROR);
	assert(!Ezjson_Destroy(&json, 0, &error) && error == EZJSON_DEPTH_ERROR);
	assert(!Ezjson_Destroy(&json, 1, &error) && error == EZJSON_DEPTH_ERROR);
	assert(!Ezjson_Destroy(&json, 2, &error) && error == EZJSON_DEPTH_ERROR);
	assert(Ezjson_Destroy(&json, 3, &error));

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory("[[], [[]]]", 10, &json, 2, &error) && error == EZJSON_DEPTH_ERROR);

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory("[[], [[]]]", 10, &json, 1, &error) && error == EZJSON_DEPTH_ERROR);

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory("[[], [[]]]", 10, &json, 0, &error) && error == EZJSON_DEPTH_ERROR);

	return EXIT_SUCCESS;
}
