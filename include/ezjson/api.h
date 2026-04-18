#ifndef EZJSON_API_H
#define EZJSON_API_H

#if defined(_WIN32)
	#if defined(_MSC_VER)
		#ifdef EZJSON_EXPORTS
			#define EZJSON_API __declspec(dllexport)
		#else
			#define EZJSON_API __declspec(dllimport)
		#endif
	#elif defined(__GNUC__)
		#ifdef EZJSON_EXPORTS
			#define EZJSON_API __attribute__((dllexport))
		#else
			#define EZJSON_API __attribute__((dllimport))
		#endif
	#endif
#elif defined(__unix__)
	#if defined(__GNUC__)
		#ifdef EZJSON_EXPORTS
			#define EZJSON_API __attribute__((visibility("default")))
		#else
			#define EZJSON_API
		#endif
	#endif
#endif

#ifndef EZJSON_API
	#define EZJSON_API
#endif

#endif
