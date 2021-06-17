#ifndef VBH_SERVER_COMM_DEF_EXPORT_H_4654897646413132164859841
#define VBH_SERVER_COMM_DEF_EXPORT_H_4654897646413132164859841

#ifdef WIN32
	#ifdef DECLARE_VBH_SERVER_COMM_DEF_EXPORT
		#define VBH_SERVER_COMM_DEF_EXPORT __declspec(dllexport)
	#else
		#define VBH_SERVER_COMM_DEF_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_VBH_SERVER_COMM_DEF_EXPORT)
		#define VBH_SERVER_COMM_DEF_EXPORT __attribute__ ((visibility("default")))
	#else
		#define VBH_SERVER_COMM_DEF_EXPORT
	#endif
#endif

#endif
