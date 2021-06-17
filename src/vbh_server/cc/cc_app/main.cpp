#include "ace/OS_main.h"
#include "ace/OS_NS_stdio.h"

#include "chain_code_appmanager.h"
#include "vbh_comm/vbh_comm_id_def.h"

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{ 
	CChainCodeAppManager* pChainCodeAppManager = ::new(std::nothrow) CChainCodeAppManager;
	if(!pChainCodeAppManager) 
	{ 
		ACE_OS::printf("failed to new cc appmanager!"); 
		
		return -1; 
	} 

	pChainCodeAppManager->SetNodeType(VBH::EN_CC_APP_TYPE);
	if( pChainCodeAppManager->Init(argc, argv) ) 
	{ 
		ACE_OS::printf("cc appmanager init failed, now exit!\n"); 
		pChainCodeAppManager->Exit();
		delete pChainCodeAppManager;

		return -1; 
	} 
	
	ACE_OS::printf("cc appmanager init succeed, running...\n"); 
	pChainCodeAppManager->Run_Loop(); 
	delete pChainCodeAppManager;
	ACE_OS::printf("cc appmanager terminated!\n"); 
	
	return 0; 
}