#undef NDEBUG

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include <ezjson.h>

static const Ezjson_Value expected = {
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
	assert(Ezjson_ReadMemory(&json, EZJSON_SAS("[[], [[]]]"), 3, &error));
	assert(error == EZJSON_NO_ERROR);

	error = EZJSON_NO_ERROR;
	assert(Ezjson_Equals(&json, &expected, 3, &error));
	assert(error == EZJSON_NO_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Equals(&json, &expected, 2, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Equals(&json, &expected, 1, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Equals(&json, &expected, 0, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Destroy(&json, 0, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Destroy(&json, 1, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(!Ezjson_Destroy(&json, 2, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	error = EZJSON_NO_ERROR;
	assert(Ezjson_Destroy(&json, 3, &error));
	assert(error == EZJSON_NO_ERROR);

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory(&json, EZJSON_SAS("[[], [[]]]"), 2, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory(&json, EZJSON_SAS("[[], [[]]]"), 1, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	json = (Ezjson_Value){0};
	error = EZJSON_NO_ERROR;
	assert(!Ezjson_ReadMemory(&json, EZJSON_SAS("[[], [[]]]"), 0, &error));
	assert(error == EZJSON_DEPTH_ERROR);

	return EXIT_SUCCESS;
}
