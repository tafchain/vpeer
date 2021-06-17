#ifndef INNER_CC_DEF_EXPORT_H_48795OIHJKLHJKSDY8IYHH
#define INNER_CC_DEF_EXPORT_H_48795OIHJKLHJKSDY8IYHH

#ifdef WIN32
	#ifdef DECLARE_INNER_CC_DEF_EXPORT
		#define INNER_CC_DEF_EXPORT __declspec(dllexport)
	#else
		#define INNER_CC_DEF_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_INNER_CC_DEF_EXPORT)
		#define INNER_CC_DEF_EXPORT __attribute__ ((visibility("default")))
	#else
		#define INNER_CC_DEF_EXPORT
	#endif
#endif

#endif
