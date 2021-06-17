#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_macro_def.h"

#include "vbh_server_comm/vbh_committer_router.h"

#include "end_tas/transfer_agent_service.h"




CTransferAgentService::ISession::ISession(CTransferAgentService& rTas)
	: m_rTas(rTas)
{
}

CTransferAgentService::CSubmitRegistUserTransactionSession::CSubmitRegistUserTransactionSession(CTransferAgentService& rTas)
	: ISession(rTas)
{
}


CTransferAgentService::CSubmitProposalTransactionSession::CSubmitProposalTransactionSession(CTransferAgentService& rTas)
	: ISession(rTas)
{
}

CTransferAgentService::CQueryLeaderCpsSession::CQueryLeaderCpsSession(CTransferAgentService& rTas)
	: m_rTas(rTas)
{
}

class CCpsAddr
{
public:
	CCpsAddr()
		: m_ipAddr("CPS_IP_ADDR")
		, m_port("CPS_PORT")
		, m_channelID("CH_ID")
	{
	}

public:
	PER_BIND_ATTR(m_ipAddr, m_port, m_channelID);

public:
	CColumnWrapper< CDscString > m_ipAddr;
	CColumnWrapper< ACE_INT32 > m_port;
	CColumnWrapper< ACE_UINT32 > m_channelID;
};

ACE_INT32 CTransferAgentService::OnInit(void)
{
	ACE_INT32 nRet = 0;
	if (CDscHtsClientService::OnInit())
	{
		DSC_RUN_LOG_ERROR("transform agent service init failed!");
		nRet = -1;
	}
	else
	{
		ACE_INT32 nPeerID;

		if (VBH::GetVbhProfileInt("PEER_ID", nPeerID))
		{
			DSC_RUN_LOG_ERROR("read PEER_ID failed.");
			return -1;
		}
		m_nPeerID = (ACE_UINT16)nPeerID;
		

		CDscDatabase database;
		CDBConnection dbConnection;
		nRet = CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection);

		if (nRet)
		{
			DSC_RUN_LOG_ERROR("connect database failed.");
			return -1;
		}

		CTableWrapper< CCollectWrapper<CCpsAddr> > lstCpsAddr("TAS_USE_CPS_CFG");

		nRet = ::PerSelect(lstCpsAddr, database, dbConnection);
		if (nRet)
		{
			DSC_RUN_LOG_ERROR("select from TAS_USE_CPS_CFG failed");
			return -1;
		}

		CCpsConnectSession* pCpsConnSession;
		PROT_COMM::CDscIpAddr addr;
		for (auto it = lstCpsAddr->begin(); it != lstCpsAddr->end(); ++it)
		{
			pCpsConnSession = DSC_THREAD_TYPE_NEW(CCpsConnectSession) CCpsConnectSession;

			pCpsConnSession->m_nChannelID = *it->m_channelID;
			pCpsConnSession->m_strIpAddr = *it->m_ipAddr;
			pCpsConnSession->m_nPort = *it->m_port;
			pCpsConnSession->m_nHandleID = this->AllocHandleID();
			pCpsConnSession->m_nIsLeaderCps = EN_CPS_FOLLOWER_STATUS;

			m_mapCpsConnectSession.DirectInsert(pCpsConnSession->m_nHandleID, pCpsConnSession);

			addr.SetIpAddr(pCpsConnSession->m_strIpAddr);
			addr.SetPort(pCpsConnSession->m_nPort);

			this->DoConnect(addr, NULL, pCpsConnSession->m_nHandleID);
		}
		 m_pQueryLeaderCpsSession = DSC_THREAD_TYPE_NEW(CQueryLeaderCpsSession) CQueryLeaderCpsSession(*this);
		 m_pQueryLeaderCpsSession->m_nSessionID = 0;
		 this->SetDscTimer(m_pQueryLeaderCpsSession, 1);


		DSC_RUN_LOG_INFO("transform agent service:%d init succeed!", this->GetID());
	}

	return nRet;
}

ACE_INT32 CTransferAgentService::OnExit(void)
{
	CCpsConnectSession* pCpsConnSession;
	for (auto it = m_mapCpsConnectSession.begin(); it != m_mapCpsConnectSession.end();)
	{
		pCpsConnSession = it.second;
		++it;
		DSC_THREAD_TYPE_DELETE(pCpsConnSession);
	}

	ISession* pSession;
	for (auto it = m_mapSubmitTransactionSession.begin(); it != m_mapSubmitTransactionSession.end();)
	{
		pSession = it.second;
		this->CancelDscTimer(pSession);
		++it;
		pSession->OnRelease();
	}

	return CDscHtsClientService::OnExit();
}

void CTransferAgentService::OnTimeOut(CSubmitRegistUserTransactionSession* pSession)
{
	DSC_RUN_LOG_INFO("SubmitRegistTransactionSession timeout, session-id:%d", pSession->m_nTasSessionID);

	VBH::CSubmitRegistUserTransactionTasEsRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nEsSessionID = pSession->m_nEsSessionID;
	m_pEs->SendSubmitRegistUserTransactionTasEsRsp(rsp);

	m_mapSubmitTransactionSession.Erase(pSession);
	DSC_THREAD_TYPE_DELETE(pSession);
}



void CTransferAgentService::OnTimeOut(CSubmitProposalTransactionSession* pSession)
{
	DSC_RUN_LOG_INFO("SubmitProposalTransactionSession timeout, session-id:%d", pSession->m_nTasSessionID);

	VBH::CSubmitProposalTransactionTasEsRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nEsSessionID = pSession->m_nEsSessionID;
	m_pEs->SendSubmitProposalTransactionTasEsRsp(rsp);

	m_mapSubmitTransactionSession.Erase(pSession);
	DSC_THREAD_TYPE_DELETE(pSession);
}


void CTransferAgentService::OnTimeOut(CQueryLeaderCpsSession* pSession)
{
	DSC_RUN_LOG_INFO("CQueryLeaderCpsSession timeout, session-id:%d", pSession->m_nSessionID);

	for (auto it = m_mapCpsConnectSession.begin(); it != m_mapCpsConnectSession.end(); ++it)
	{
		if (NULL != it.second->m_pMcpHandler)
		{
			VBH::CQueryLeaderCpsTasCpsReq req;
				req.m_nPeerID = m_nPeerID;
			
				SendHtsMsg(req, it.second->m_pMcpHandler);
		}
	}
	this->SetDscTimer(m_pQueryLeaderCpsSession, 1);
}


ACE_INT32 CTransferAgentService::OnHtsMsg(VBH::CSubmitRegistUserTransactionCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CTransferAgentService, VBH::CSubmitRegistUserTransactionCpsTasRsp);
	
	ISession* pSession = m_mapSubmitTransactionSession.Erase(rSubmitTransactionRsp.m_nTasSessionID);

	if (pSession)
	{
		VBH::CSubmitRegistUserTransactionTasEsRsp rsp;

		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nEsSessionID = pSession->m_nEsSessionID;
		rsp.m_userKey = rSubmitTransactionRsp.m_userKey;
		rsp.m_registTransUrl = rSubmitTransactionRsp.m_registTransUrl;

		m_pEs->SendSubmitRegistUserTransactionTasEsRsp(rsp);
		
		this->CancelDscTimer(pSession);
		pSession->OnRelease();
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find tas session, session-id:%d", rSubmitTransactionRsp.m_nTasSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CTransferAgentService, VBH::CSubmitRegistUserTransactionCpsTasRsp);

	return 0;
}

ACE_INT32 CTransferAgentService::OnHtsMsg(VBH::CSubmitProposalTransactionCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CTransferAgentService, VBH::CSubmitProposalTransactionCpsTasRsp);
	
	ISession* pSession = m_mapSubmitTransactionSession.Erase(rSubmitTransactionRsp.m_nTasSessionID);

	if (pSession)
	{
		VBH::CSubmitProposalTransactionTasEsRsp rsp;
		rsp.m_nActionID = rSubmitTransactionRsp.m_nActionID;
		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nEsSessionID = pSession->m_nEsSessionID;
		rsp.m_alocTransKey = rSubmitTransactionRsp.m_alocTransKey;
		rsp.m_vecInfoID = rSubmitTransactionRsp.m_vecInfoID;

		m_pEs->SendSubmitProposalTransactionTasEsRsp(rsp);

		this->CancelDscTimer(pSession);
		pSession->OnRelease();
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find tas session, session-id:%d", rSubmitTransactionRsp.m_nTasSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CTransferAgentService, VBH::CSubmitProposalTransactionCpsTasRsp);

	return 0;
}

ACE_INT32  CTransferAgentService::OnHtsMsg(VBH::CQueryLeaderCpsCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler)
{
	DSC_RUN_LOG_INFO("receive CQueryLeaderCpsCpsTasRsp ");
	for (auto it = m_mapCpsConnectSession.begin(); it != m_mapCpsConnectSession.end(); ++it)
	{
		if (pMcpHandler->GetHandleID() == it.second->m_nHandleID)
		{
			DSC_RUN_LOG_INFO("stop query leader timer");
			it.second->m_nIsLeaderCps = EN_CPS_LEADER_STATUS;
			this->CancelDscTimer(m_pQueryLeaderCpsSession);
			break;
		}
	}
	return 0;
}

CTransferAgentService::CCpsConnectSession* CTransferAgentService::FindLeaderCpsSession(ACE_UINT32 nChannelID)
{

	for (auto it = m_mapCpsConnectSession.begin(); it != m_mapCpsConnectSession.end(); ++it)
	{
		if ((nChannelID == it.second->m_nChannelID)&&(EN_CPS_LEADER_STATUS == it.second->m_nIsLeaderCps))
		{
			return it.second;
		}
	}
	return NULL;
}

ACE_INT32 CTransferAgentService::SendSubmitRegistUserTransactionEsTasReq(VBH::CSubmitRegistUserTransactionEsTasReq& rSubmitTransReq)
{
	VBH_MESSAGE_ENTER_TRACE(CTransferAgentService, VBH::CSubmitRegistUserTransactionEsTasReq);

	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CCpsConnectSession* pCpsConnSession = FindLeaderCpsSession(rSubmitTransReq.m_nChannelID);

	if (pCpsConnSession)
	{
		if (pCpsConnSession->m_pMcpHandler)
		{
			VBH::CSubmitRegistUserTransactionTasCpsReq req;

			req.m_nTasSessionID = m_nSessionID;
			req.m_userInfo = rSubmitTransReq.m_userInfo;
			req.m_cltPubKey = rSubmitTransReq.m_cltPubKey;
			req.m_svrPriKey = rSubmitTransReq.m_svrPriKey;
			req.m_envelopeKey = rSubmitTransReq.m_envelopeKey;

			if (SendHtsMsg(req, pCpsConnSession->m_pMcpHandler))
			{
				nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
				DSC_RUN_LOG_ERROR("send hts msg:CSubmitRegistUserTransactionTasCpsReq failed.");
			}
			else
			{
				CSubmitRegistUserTransactionSession* pSession = DSC_THREAD_TYPE_NEW(CSubmitRegistUserTransactionSession) CSubmitRegistUserTransactionSession(*this);

				pSession->m_nTasSessionID = m_nSessionID;
				pSession->m_nEsSessionID = rSubmitTransReq.m_nEsSessionID;

				m_mapSubmitTransactionSession.DirectInsert(m_nSessionID, pSession);
				++m_nSessionID;
				this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE, false);
			}
		}
		else
		{
			nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
			DSC_RUN_LOG_ERROR("lost CPS connect, channel id:%d", rSubmitTransReq.m_nChannelID);
		}
	}
	else //找不到CPS地址
	{
		nReturnCode = VBH::EN_CANNOT_FOUND_CHANNEL_ORDER_CPS_ERROR_TYPE;
		DSC_RUN_LOG_ERROR("can not find channel:%d 's CPS addr.", rSubmitTransReq.m_nChannelID);
	}


	VBH_MESSAGE_LEAVE_TRACE(CTransferAgentService, VBH::CSubmitRegistUserTransactionEsTasReq);

	return nReturnCode;
}

//将transaction 转发到channel对应的pts
ACE_INT32 CTransferAgentService::SendSubmitProposalTransactionEsTasReq(VBH::CSubmitProposalTransactionEsTasReq& rSubmitTransReq)
{
	VBH_MESSAGE_ENTER_TRACE(CTransferAgentService, VBH::CSubmitProposalTransactionEsTasReq);

	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CCpsConnectSession* pCpsConnSession = FindLeaderCpsSession(rSubmitTransReq.m_nChannelID);

	if (pCpsConnSession)
	{
		if (pCpsConnSession->m_pMcpHandler)
		{
			VBH::CSubmitProposalTransactionTasCpsReq req;

			req.m_nActionID = rSubmitTransReq.m_nActionID;
			req.m_nTasSessionID = m_nSessionID;
			req.m_transContent = rSubmitTransReq.m_transContent;

			if (SendHtsMsg(req, pCpsConnSession->m_pMcpHandler))
			{
				nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
				DSC_RUN_LOG_ERROR("send hts msg:CSubmitProposalTransactionTasCpsReq failed.");
			}
			else
			{
				CSubmitProposalTransactionSession* pSession = DSC_THREAD_TYPE_NEW(CSubmitProposalTransactionSession) CSubmitProposalTransactionSession(*this);

				pSession->m_nTasSessionID = m_nSessionID;
				pSession->m_nEsSessionID = rSubmitTransReq.m_nEsSessionID;

				m_mapSubmitTransactionSession.DirectInsert(m_nSessionID, pSession);
				++m_nSessionID;
				this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE, false);
			}
		}
		else
		{
			nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
			DSC_RUN_LOG_ERROR("lost CPS connect, channel id:%d", rSubmitTransReq.m_nChannelID);
		}
	}
	else //找不到CPS地址
	{
		nReturnCode = VBH::EN_CANNOT_FOUND_CHANNEL_ORDER_CPS_ERROR_TYPE;
		DSC_RUN_LOG_ERROR("can not find channel:%d 's CPS addr.", rSubmitTransReq.m_nChannelID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CTransferAgentService, VBH::CSubmitProposalTransactionEsTasReq);

	return nReturnCode;
}

void CTransferAgentService::OnNetworkError(CMcpHandler* pMcpHandler)
{
	//处理和cps连接的网络错误
	for (auto it = m_mapCpsConnectSession.begin(); it != m_mapCpsConnectSession.end(); ++it)
	{
		if (it.second->m_nHandleID == pMcpHandler->GetHandleID())
		{
			it.second->m_pMcpHandler = NULL;
			/*如果*/
			if (EN_CPS_LEADER_STATUS == it.second->m_nIsLeaderCps)
			{
				this->SetDscTimer(m_pQueryLeaderCpsSession, 1);
				it.second->m_nIsLeaderCps = EN_CPS_FOLLOWER_STATUS;
				DSC_RUN_LOG_INFO("restart timer: query leader cps.");
			}
			break;
		}
	}

	CDscHtsClientService::OnNetworkError(pMcpHandler);

	DSC_RUN_LOG_INFO("transform agent service network error.");
}

ACE_INT32 CTransferAgentService::OnConnectedNodify(CMcpClientHandler* pMcpClientHandler)
{
	//和cps的连接建立通知到来时
	for (auto cpsIt = m_mapCpsConnectSession.begin(); cpsIt != m_mapCpsConnectSession.end(); ++cpsIt)
	{
		if (cpsIt.second->m_nHandleID == pMcpClientHandler->GetHandleID())
		{
			cpsIt.second->m_pMcpHandler = pMcpClientHandler;
			break;
		}
	}

	return CDscHtsClientService::OnConnectedNodify(pMcpClientHandler);
}
