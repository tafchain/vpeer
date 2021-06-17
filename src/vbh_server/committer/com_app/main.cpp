#include "ace/OS_main.h"
#include "ace/OS_NS_stdio.h"

#include "committer_appmanager.h"
#include "vbh_comm/vbh_comm_id_def.h"

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{ 
	CCommitterAppManager* pCommitterAppManager = ::new(std::nothrow) CCommitterAppManager;
	if(!pCommitterAppManager) 
	{ 
		ACE_OS::printf("failed to new committer appmanager!"); 
		
		return -1; 
	} 

	pCommitterAppManager->SetNodeType(VBH::EN_COMMITTER_APP_TYPE);
	if( pCommitterAppManager->Init(argc, argv) ) 
	{ 
		ACE_OS::printf("committer init failed, now exit!\n"); 
		pCommitterAppManager->Exit();
		delete pCommitterAppManager;

		return -1; 
	} 
	
	ACE_OS::printf("committer init succeed, running...\n"); 
	pCommitterAppManager->Run_Loop(); 
	delete pCommitterAppManager;
	ACE_OS::printf("committer terminated!\n"); 
	
	return 0; 
}