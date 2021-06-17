#ifndef TRANSFORM_AGENT_EXPORT_H_74379834674356467956843684
#define TRANSFORM_AGENT_EXPORT_H_74379834674356467956843684

#ifdef WIN32
	#ifdef DECLARE_TRANSFORM_AGENT_EXPORT
		#define TRANSFORM_AGENT_EXPORT __declspec(dllexport)
	#else
		#define TRANSFORM_AGENT_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && defined(DECLARE_TRANSFORM_AGENT_EXPORT)
		#define TRANSFORM_AGENT_EXPORT __attribute__ ((visibility("default")))
	#else
		#define TRANSFORM_AGENT_EXPORT
	#endif
#endif

#endif
