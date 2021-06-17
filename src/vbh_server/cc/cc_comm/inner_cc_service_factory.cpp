
#include "cc_comm/inner_cc_service_factory.h"

CDscService* CInnerCCServiceFactory::CreateDscService(void)
{ 
	return m_pInnerCCService;
}


extern "C" CC_COMM_DEF_EXPORT void* CreateCommCC(ACE_UINT32 nChannelID)
{
	CCcBaseService* pCommCC = NULL;

	DSC_NEW(pCommCC, CCcBaseService(nChannelID));

	return pCommCC;
}
