#ifndef ENDORSER_SERVICE_PLUGIN_H_43294394329432684328764317432765
#define ENDORSER_SERVICE_PLUGIN_H_43294394329432684328764317432765

#include "dsc/plugin/i_dsc_plugin.h"
#include "end_comm/endorser_service_creator.h"

class CEndorserServicePlugin : public IDscPlugin
{ 
public: 
	ACE_INT32 OnInit(void); 
}; 
#endif