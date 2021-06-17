#include "dsc/dsc_comm_def.h"

#include "committer_appmanager.h"
#include "vbh_comm/vbh_encrypt_lib.h"

#ifdef DSC_TEST
#include "com_cs/cs/committer_service_plugin.h"
#endif

ACE_INT32 CCommitterAppManager::OnInit()
{
	VBH::InitOpenSsl();
	DSC_FORWARD_CALL(CDscAppManager::OnInit() );

#ifdef DSC_TEST
	//·½±ãvaldring¼ì²â£¬¾²Ì¬¼ÓÔØ²å¼þ
	CCommitterServicePlugin csPlugin;

	csPlugin.OnInit();
#endif

	return 0;
}

ACE_INT32 CCommitterAppManager::OnExit()
{
	DSC_FORWARD_CALL(CDscAppManager::OnExit());

	return 0;
}
