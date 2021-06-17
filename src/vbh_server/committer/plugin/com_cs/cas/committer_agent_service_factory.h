#ifndef COMMITTER_AGENT_SERVER_FACTORY_H_43287923173215632845789432123145
#define COMMITTER_AGENT_SERVER_FACTORY_H_43287923173215632845789432123145

#include "dsc/service/dsc_service_container.h"

#include "com_cs/cas/committer_agent_service.h"

class CCommitterAgentServiceFactory : public IDscServiceFactory
{
public:
	CCommitterAgentServiceFactory(CCommitterAgentService* pCommitterAgentService);
public:
	virtual CDscService* CreateDscService(void);

private:
	CCommitterAgentService* m_pCommitterAgentService;

};
#endif