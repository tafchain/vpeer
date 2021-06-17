#ifndef ENDORSER_SERVICE_COMMON_FACTORY_H_98784654132167983135164567892313213
#define ENDORSER_SERVICE_COMMON_FACTORY_H_98784654132167983135164567892313213

#include "dsc/service/dsc_service_container.h"

#include "end_comm/endorser_common_export.h"

class ENDORSER_COMMON_EXPORT CEndorserServiceCommonFactory : public IDscServiceFactory
{ 
public:
	virtual CDscService* CreateDscService(void) override;

public:	
	CDscService* m_pEndorserService = nullptr;
}; 

#endif