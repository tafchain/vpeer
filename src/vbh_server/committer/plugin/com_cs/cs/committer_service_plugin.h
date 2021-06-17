#ifndef COMMITTER_SERVICE_PLUGIN_H_52E53C44997711E9B15760F18A3A20D1
#define COMMITTER_SERVICE_PLUGIN_H_52E53C44997711E9B15760F18A3A20D1

#include "dsc/plugin/i_dsc_plugin.h"

class CCommitterServicePlugin : public IDscPlugin
{ 
public: 
	ACE_INT32 OnInit(void); 
	
}; 
#endif