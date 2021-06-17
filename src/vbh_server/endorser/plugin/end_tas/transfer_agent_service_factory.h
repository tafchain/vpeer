#ifndef TRANSFER_AGENT_SERVICE_FACTORY_H_9U83095P3JHNPOIFU9WURPHNLKHNJ
#define TRANSFER_AGENT_SERVICE_FACTORY_H_9U83095P3JHNPOIFU9WURPHNLKHNJ

#include "dsc/service/dsc_service_container.h"

#include "end_tas/transfer_agent_service.h"
#include "end_tas/transfer_agent_export.h"

class TRANSFORM_AGENT_EXPORT CTransferAgentServiceFactory : public IDscServiceFactory
{ 
public:
	virtual CDscService* CreateDscService(void);

public:	
	CTransferAgentService* m_pTas = nullptr;
}; 
#endif