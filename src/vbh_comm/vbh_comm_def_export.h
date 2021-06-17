#ifndef VBH_COMM_DEF_EXPORT_H_43289043287432683286432876
#define VBH_COMM_DEF_EXPORT_H_43289043287432683286432876

#ifdef WIN32
	#ifdef DECLARE_VBH_COMM_DEF_EXPORT
		#define VBH_COMM_DEF_EXPORT __declspec(dllexport)
	#else
		#define VBH_COMM_DEF_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_VBH_COMM_DEF_EXPORT)
		#define VBH_COMM_DEF_EXPORT __attribute__ ((visibility("default")))
	#else
		#define VBH_COMM_DEF_EXPORT
	#endif
#endif

#endif
