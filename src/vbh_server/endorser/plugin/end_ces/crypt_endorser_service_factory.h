#ifndef CRYPT_ENDORSER_SERVER_SERVICE_FACTORY_H_DSOIFHEWOIHNEWOIRHEIOUREORHNNVL
#define CRYPT_ENDORSER_SERVER_SERVICE_FACTORY_H_DSOIFHEWOIHNEWOIRHEIOUREORHNNVL

#include "dsc/service/dsc_service_container.h"

class CCryptEndorserServiceFactory : public IDscServiceFactory
{ 
public:
	virtual CDscService* CreateDscService(void);

public:	
	CCryptEndorserService* m_pEs = nullptr;

	CDscString m_strAddr;
	ACE_INT32 m_nPort;
	ACE_UINT16 m_nServiceID;
}; 
#endif