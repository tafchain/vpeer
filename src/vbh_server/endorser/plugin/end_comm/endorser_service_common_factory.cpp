#include "end_comm/endorser_service_common_factory.h"

CDscService* CEndorserServiceCommonFactory::CreateDscService(void)
{ 
	return m_pEndorserService;
}
