#ifndef CC_COMM_DEF_EXPORT_H_457823497843297231
#define CC_COMM_DEF_EXPORT_H_457823497843297231

#ifdef WIN32
	#ifdef DECLARE_CC_COMM_DEF_EXPORT
		#define CC_COMM_DEF_EXPORT __declspec(dllexport)
	#else
		#define CC_COMM_DEF_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_CC_COMM_DEF_EXPORT)
		#define CC_COMM_DEF_EXPORT __attribute__ ((visibility("default")))
	#else
		#define CC_COMM_DEF_EXPORT
	#endif
#endif

#endif
