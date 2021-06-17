#include "ace/OS_NS_strings.h"
#include "ace/DLL_Manager.h"
#include "ace/OS_NS_dlfcn.h"

#include "dsc/service/dsc_service_container.h"
#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"
#include "dsc/dispatcher/dsc_dispatcher_center.h"

#include "end_ces/crypt_endorser_service.h"
#include "end_ces/crypt_endorser_service_plugin.h"


ACE_INT32 CCryptEndorserServicePlugin::OnInit(void)
{ 
	CEndorserServiceCreator esCreator;

	if (esCreator.CreateEs<CCryptEndorserService>())
	{
		DSC_RUN_LOG_ERROR("create crypt-endorser-service failed.");
		return -1;
	}
	else
	{
		DSC_RUN_LOG_FINE("regist crypt-endorser-service succeed");
		return 0;
	}
} 

#ifndef DSC_TEST
extern "C" PLUGIN_EXPORT IDscPlugin* CreateDscPlugin(void)
{
	CCryptEndorserServicePlugin* pPlugIn = NULL;
	
	DSC_NEW(pPlugIn, CCryptEndorserServicePlugin);

	return pPlugIn;
}
#endif
