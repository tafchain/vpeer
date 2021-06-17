#include "com_cs/cas/committer_agent_service_factory.h"

CCommitterAgentServiceFactory::CCommitterAgentServiceFactory(CCommitterAgentService* pCommitterAgentService)
: m_pCommitterAgentService(pCommitterAgentService)
{

}
CDscService* CCommitterAgentServiceFactory::CreateDscService(void)
{
	return m_pCommitterAgentService;
}


