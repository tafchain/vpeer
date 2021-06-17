#ifndef VBH_LOAD_OUTER_CC_APPMANAGER_H_6587789797976446543434
#define VBH_LOAD_OUTER_CC_APPMANAGER_H_6587789797976446543434

#include "dsc/dsc_app_mng.h"
#include "dsc/container/dsc_string.h"

class CVbhLoadOuterCcAppManager : public CDscAppManager
{

protected:
	virtual ACE_INT32 OnInit(void);
	virtual void OnParam(const int argc, ACE_TCHAR* argv);

public:

	ACE_UINT32 m_nChannelID = 0;
	CDscString m_strCcName;
	ACE_UINT32 m_nCcID = 0;
};

#endif
