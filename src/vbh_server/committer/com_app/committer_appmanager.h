#ifndef COMMITTER_APPMANAGER_H_536E87F2997111E982A360F18A3A20D1
#define COMMITTER_APPMANAGER_H_536E87F2997111E982A360F18A3A20D1

#include "dsc/dsc_app_mng.h"

class CCommitterAppManager : public CDscAppManager
{

protected:
	virtual ACE_INT32 OnInit(void);
	virtual ACE_INT32 OnExit(void);
};

#endif
