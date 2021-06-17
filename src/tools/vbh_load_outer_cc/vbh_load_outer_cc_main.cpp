#include "ace/OS_main.h"
#include "ace/OS_NS_stdio.h"

#include "dsc/mem_mng/dsc_allocator.h"


#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_load_outer_cc/vbh_load_outer_cc_appmanager.h"


int ACE_TMAIN(int argc, ACE_TCHAR *argv[]) 
{ 
	CVbhLoadOuterCcAppManager* pAppManager = NULL;

	DSC_NEW(pAppManager, CVbhLoadOuterCcAppManager);
	if(!pAppManager) 
	{ 
		ACE_OS::printf("failed to new vbh_load_outer_cc appmanager!"); 
		
		return -1; 
	} 

	pAppManager->SetNodeType(VBH::EN_MIN_VBH_APP_TYPE + 4);
	if( pAppManager->Init(argc, argv) )
	{ 
		ACE_OS::printf("vbh_load_outer_cc init failed, now exit!\n"); 
		pAppManager->Exit();
		delete pAppManager;

		return -1; 
	} 
	
	ACE_OS::printf("vbh_load_outer_cc init succeed, running...\n"); 
	pAppManager->Run_Loop(); 
	delete pAppManager;
	ACE_OS::printf("vbh_load_outer_cc terminated!\n"); 
	
	return 0; 
}