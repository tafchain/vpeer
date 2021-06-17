#ifndef ORDER_MANAGER_APPMANAGER_H_F5B9B72298A211E9A96B60F18A3A20D1
#define ORDER_MANAGER_APPMANAGER_H_F5B9B72298A211E9A96B60F18A3A20D1

#include "dsc/dsc_app_mng.h"

class CEndorserAppManager : public CDscAppManager
{

protected:
	virtual ACE_INT32 OnInit(void);
	virtual ACE_INT32 OnExit(void);
};

#endif
