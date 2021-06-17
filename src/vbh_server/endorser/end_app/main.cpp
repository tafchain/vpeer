#include "ace/OS_main.h"
#include "ace/OS_NS_stdio.h"

#include "endorser_appmanager.h"
#include "vbh_comm/vbh_comm_id_def.h"

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{ 
	CEndorserAppManager* pEndorserAppManager = ::new(std::nothrow) CEndorserAppManager;
	if(!pEndorserAppManager) 
	{ 
		ACE_OS::printf("failed to new endorser appmanager!"); 
		
		return -1; 
	} 

	pEndorserAppManager->SetNodeType(VBH::EN_ENDORSER_APP_TYPE);
	if( pEndorserAppManager->Init(argc, argv) ) 
	{ 
		ACE_OS::printf("endorser appmanager init failed, now exit!\n"); 
		pEndorserAppManager->Exit();
		delete pEndorserAppManager;

		return -1; 
	} 
	
	ACE_OS::printf("endorser appmanager init succeed, running...\n"); 
	pEndorserAppManager->Run_Loop(); 
	delete pEndorserAppManager;
	ACE_OS::printf("endorser appmanager terminated!\n"); 
	
	return 0; 
}