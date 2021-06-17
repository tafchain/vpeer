#ifndef CRYPT_ENDORSER_SERVICE_PLUGIN_H_8748946541687846541354
#define CRYPT_ENDORSER_SERVICE_PLUGIN_H_8748946541687846541354

#include "dsc/plugin/i_dsc_plugin.h"
#include "end_comm/endorser_service_creator.h"

class CCryptEndorserServicePlugin : public IDscPlugin
{ 
public: 
	ACE_INT32 OnInit(void); 
}; 
#endif