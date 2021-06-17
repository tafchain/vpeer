#ifndef ENDORSER_COMMON_EXPORT_H_646131564654321321312654684
#define ENDORSER_COMMON_EXPORT_H_646131564654321321312654684

#ifdef WIN32
	#ifdef DECLARE_ENDORSER_COMMON_EXPORT
		#define ENDORSER_COMMON_EXPORT __declspec(dllexport)
	#else
		#define ENDORSER_COMMON_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_ENDORSER_COMMON_EXPORT)
		#define ENDORSER_COMMON_EXPORT __attribute__ ((visibility("default")))
	#else
		#define ENDORSER_COMMON_EXPORT
	#endif
#endif

#endif
