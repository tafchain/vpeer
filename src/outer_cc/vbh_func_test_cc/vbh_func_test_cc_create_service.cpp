#include "dsc/service/dsc_service_container.h"

#include "vbh_func_test_cc/vbh_func_test_cc_service.h"


extern "C" INNER_CC_DEF_EXPORT void* CreateOuterCC(ACE_UINT32 nChannelID)
{
	CVbhFuncTestCcService* pOuterCC = NULL;
	
	DSC_NEW(pOuterCC, CVbhFuncTestCcService(nChannelID));

	return pOuterCC;
}

