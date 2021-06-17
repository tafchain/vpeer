#include "dsc/dsc_comm_def.h"

#include "chain_code_appmanager.h"
#include "vbh_comm/vbh_encrypt_lib.h"

#ifdef DSC_TEST

#endif

ACE_INT32 CChainCodeAppManager::OnInit()
{
	VBH::InitOpenSsl();
	DSC_FORWARD_CALL(CDscAppManager::OnInit() );

#ifdef DSC_TEST

#endif

	return 0;
}

ACE_INT32 CChainCodeAppManager::OnExit()
{
	DSC_FORWARD_CALL(CDscAppManager::OnExit());

	return 0;
}


