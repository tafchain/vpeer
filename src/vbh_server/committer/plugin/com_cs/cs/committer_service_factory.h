#ifndef COMMITTER_SERVICE_FACTORY_H_587945743759437329847932874987D1
#define COMMITTER_SERVICE_FACTORY_H_587945743759437329847932874987D1

#include "dsc/service/dsc_service_container.h"

#include "com_cs/cs/committer_service.h"

class CCommitterServiceFactory : public IDscServiceFactory
{ 
public:
	CCommitterServiceFactory(CCommitterService* pCommitterService);

public:
	virtual CDscService* CreateDscService(void);

private:	
	CCommitterService* m_pCommitterService;
};
#endif