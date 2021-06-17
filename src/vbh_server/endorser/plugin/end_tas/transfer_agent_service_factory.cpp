#include "end_tas/transfer_agent_service.h"
#include "end_tas/transfer_agent_service_factory.h"

CDscService* CTransferAgentServiceFactory::CreateDscService(void)
{ 
	return m_pTas;
}
