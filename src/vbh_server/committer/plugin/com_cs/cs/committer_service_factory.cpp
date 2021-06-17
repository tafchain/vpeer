#include "com_cs/cs/committer_service_factory.h"

CCommitterServiceFactory::CCommitterServiceFactory(CCommitterService* pCommitterService)
: m_pCommitterService(pCommitterService)
{
}

CDscService* CCommitterServiceFactory::CreateDscService(void)
{ 
	return m_pCommitterService;
}
