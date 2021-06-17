#include "dsc/dsc_comm_func.h"
#include "dsc/dispatcher/dsc_dispatcher_center.h"

#include "vbh_comm/vbh_encrypt_lib.h"

#include "vbh_load_outer_cc/vbh_load_outer_cc_appmanager.h"
#include "vbh_load_outer_cc/vbh_load_outer_cc_service.h"


ACE_INT32 CVbhLoadOuterCcAppManager::OnInit()
{
	VBH::InitOpenSsl();

	DSC_FORWARD_CALL(CDscAppManager::OnInit() );

	CVbhLoadOuterCcService* pVbhLoadOuterCcService;


	DSC_NEW(pVbhLoadOuterCcService, CVbhLoadOuterCcService(m_nChannelID, m_nCcID, m_strCcName));

	CDscDispatcherCenterDemon::instance()->RegistDscTask(pVbhLoadOuterCcService, CVbhLoadOuterCcService::EN_SERVICE_TYPE, 1);

	return 0;
}

void CVbhLoadOuterCcAppManager::OnParam(const int argc, ACE_TCHAR* argv)
{
	if (argc == 'm' || argc == 'M') // cc name
	{
		m_strCcName.assign(argv, strlen(argv));
	}
	else if (argc == 'c' || argc == 'C') //channelºÅ£¬Ä¬ÈÏÎª1
	{
		DSC::DscAtoi(argv, m_nChannelID);
	}
	else if (argc == 'i' || argc == 'D') // cc ID
	{
		DSC::DscAtoi(argv, m_nCcID);
	}

}

