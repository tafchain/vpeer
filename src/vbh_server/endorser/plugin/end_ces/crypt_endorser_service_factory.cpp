#include "end_ces/crypt_endorser_service.h"
#include "end_ces/crypt_endorser_service_factory.h"

CDscService* CCryptEndorserServiceFactory::CreateDscService(void)
{ 
	CCryptEndorserService* pEndorserService = NULL;

	DSC_NEW(pEndorserService, CCryptEndorserService(m_strAddr, m_nPort));

	if(pEndorserService)
	{ 
		pEndorserService->SetType(CCryptEndorserService::EN_SERVICE_TYPE);
		pEndorserService->SetID(m_nServiceID);
	} 

	return pEndorserService;
}
