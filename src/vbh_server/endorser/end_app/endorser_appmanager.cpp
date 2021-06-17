#include "dsc/dsc_comm_def.h"

#include "endorser_appmanager.h"
#include "vbh_comm/vbh_encrypt_lib.h"

#ifdef DSC_TEST
#include "end_ces/crypt_endorser_service_plugin.h"
#endif

ACE_INT32 CEndorserAppManager::OnInit()
{
	VBH::InitOpenSsl();
	DSC_FORWARD_CALL(CDscAppManager::OnInit() );

#ifdef DSC_TEST
	//·½±ãvaldring¼ì²â£¬¾²Ì¬¼ÓÔØ²å¼þ
	CCryptEndorserServicePlugin cesPlugin;

	cesPlugin.OnInit();
#endif

	return 0;
}

ACE_INT32 CEndorserAppManager::OnExit()
{
	DSC_FORWARD_CALL(CDscAppManager::OnExit());

	return 0;
}


