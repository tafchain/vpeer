#ifndef INNER_CC_SERVICE_FACTORY_H_HJLKDSHFOIEWHFKJBNDKVJNSDKJFUHSDOIRE09
#define INNER_CC_SERVICE_FACTORY_H_HJLKDSHFOIEWHFKJBNDKVJNSDKJFUHSDOIRE09

#include "dsc/service/dsc_service_container.h"

#include "cc_comm/cc_comm_def_export.h"
#include "cc_comm/cc_base_service.h"

class CC_COMM_DEF_EXPORT CInnerCCServiceFactory : public IDscServiceFactory
{ 
public:
	virtual CDscService* CreateDscService(void);

public:
	CCcBaseService* m_pInnerCCService = NULL;
}; 


#endif
