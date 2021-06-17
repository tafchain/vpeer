#ifndef CHAIN_CODE_APPMANAGER_H_AB254B24998611E9A89E60F18A3A20D1
#define CHAIN_CODE_APPMANAGER_H_AB254B24998611E9A89E60F18A3A20D1

#include "dsc/dsc_app_mng.h"

class CChainCodeAppManager : public CDscAppManager
{

protected:
	virtual ACE_INT32 OnInit(void);
	virtual ACE_INT32 OnExit(void);
};

#endif
