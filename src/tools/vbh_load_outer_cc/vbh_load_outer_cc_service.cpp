#include "ace/OS_NS_sys_stat.h"

#include "dsc/dsc_log.h"

#include "vbh_load_outer_cc/vbh_load_outer_cc_service.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_key_codec.h"
#include "vbh_comm/vbh_comm_macro_def.h"
#include "vbh_comm/test_def.h"

#include "dsc/service/dsc_service_container.h"
#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"
#include "dsc/dispatcher/dsc_dispatcher_center.h"

class CDBEsCfg
{
public:
	CDBEsCfg()
		: m_IpAddr("ES_IP_ADDR")
		, m_port("ES_PORT")
		, m_esID("ES_ID")
	{
	}

public:
	PER_BIND_ATTR(m_IpAddr, m_port, m_esID);

public:
	CColumnWrapper< CDscString > m_IpAddr;
	CColumnWrapper< ACE_INT32 > m_port;
	CColumnWrapper< ACE_INT32 > m_esID;
};


CVbhLoadOuterCcService::CVbhLoadOuterCcService(ACE_UINT32 nChannelID, ACE_UINT32 nCcID, CDscString strCcName)
	: m_nChannelID(nChannelID)
	, m_nCcID(nCcID)
	, m_strCcName(strCcName)
{

}

ACE_INT32 CVbhLoadOuterCcService::OnInit(void)
{
	if (CDscReactor::OnInit())
	{
		DSC_RUN_LOG_ERROR("vbh_load_outer_cc_client service init failed!");

		return -1;
	}

	ACE_OS::sleep(5); //为了比要拉起的插件晚启动
	LoadOuterCc();

	return 0;
}

ACE_INT32 CVbhLoadOuterCcService::OnExit(void)
{
	return CDscReactor::OnExit();
}



void CVbhLoadOuterCcService::OnDscMsg(VBH::CLoadOuterCcEsCltRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{

	CLoadOuterCcSession* pSession = m_mapLoadOuterCcSession.Find(rsp.m_nCltSessionID);

	if (pSession)
	{
		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			ACE_OS::printf("load outer cc sucess : cc id:%d, cc name:%s,es id :%d !\n",m_nCcID, m_strCcName, rsp.m_nCltSessionID);
		}
		else
		{
			ACE_OS::printf("load outer cc failed : cc id:%d, cc name:%s,es id :%d ,error code:%d, error string:%s!\n",
				m_nCcID, m_strCcName, rsp.m_nCltSessionID,rsp.m_nReturnCode, VBH::GetErrorString(rsp.m_nReturnCode));
			m_isSucess = 0;
		}
		DSC_THREAD_TYPE_DELETE(m_mapLoadOuterCcSession.Erase(rsp.m_nCltSessionID));
	}
	else
	{
		ACE_OS::printf("can not find session, session-id:%d", rsp.m_nCltSessionID);
	}
	if (m_mapLoadOuterCcSession.begin() == m_mapLoadOuterCcSession.end())
	{
		if (m_isSucess)
		{
			ACE_OS::printf("load outer cc sucess，now exit");
		}
		else
		{
			ACE_OS::printf("load outer cc failed，now exit");
		}
		CDscAppManager::Instance()->SetAppExited();
	}

}

void CVbhLoadOuterCcService::LoadOuterCc(void)
{
	ReadDBConfig(m_lstEsCfg);
	VBH::CLoadOuterCcUserCltEsReq req;
	ACE_UINT32 nReturn;

	for (auto it = m_lstEsCfg.begin(); it != m_lstEsCfg.end(); ++it)
	{

		const CDscMsg::CDscMsgAddr address(VBH::EN_ENDORSER_APP_TYPE, 0, VBH::EN_CRYPT_ENDORSER_SERVICE_TYPE, it->m_nEsID);
		CLoadOuterCcSession* pSession = DSC_THREAD_TYPE_NEW(CLoadOuterCcSession)CLoadOuterCcSession(*this);

		req.m_nChannelID = m_nChannelID;
		req.m_nSessionID = it->m_nEsID;
		req.m_nCcID = m_nCcID;
		VBH::Assign(req.m_ccName, m_strCcName);

		m_mapLoadOuterCcSession.DirectInsert(it->m_nEsID, pSession);

		if (nReturn = this->SendDscMessage(req, address))
		{
			DSC_RUN_LOG_ERROR("SendDscMessage failed, channdel id:%d, Es id:%d", m_nChannelID, it->m_nEsID);
		}
	}
}


ACE_INT32 CVbhLoadOuterCcService::ReadDBConfig(es_cfg_list_type& lstEsCfg)
{
	CDscDatabase database;
	CDBConnection dbConnection;


	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");
		return -1;
	}

	CTableWrapper< CCollectWrapper<CDBEsCfg> > lstDBEsCfg("CES_CFG");

	if (::PerSelect(lstDBEsCfg, database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("select from %s failed", "CES_CFG");
		return -1;
	}

	CEsCfg esCfg;
	for (auto it = lstDBEsCfg->begin(); it != lstDBEsCfg->end(); ++it)
	{
		esCfg.m_strIpAddr = *it->m_IpAddr;
		esCfg.m_nEsID = (ACE_UINT16)*it->m_esID;
		esCfg.m_nPort = (ACE_UINT16)*it->m_port;

		lstEsCfg.push_back(esCfg);
	}

	return 0;
}

CVbhLoadOuterCcService::CLoadOuterCcSession::CLoadOuterCcSession(CVbhLoadOuterCcService& rService)
	: m_rServiece(rService)
{

}

void CVbhLoadOuterCcService::CLoadOuterCcSession::OnTimer(void)
{
	ACE_OS::printf("load outer cc time out :, cc id:%d, cc name:%s, now exit!\n", m_nCcID, m_strCcNmae);
	CDscAppManager::Instance()->SetAppExited();
	m_rServiece.CancelDscTimer(this);
}
