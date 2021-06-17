#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"

#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_macro_def.h"

#include "end_es/endorser_service.h"


void CEndorserService::GetConfigTableName(CDscString& strConfigTableName)
{
	strConfigTableName.assign(DSC_STRING_TYPE_PARAM("ES_CFG"));
}

CEndorserService::IUserSession::IUserSession(CEndorserService& rEndorserService)
	: m_rEndorserService(rEndorserService)
{
}

CEndorserService::CQueryUserInfoSession::CQueryUserInfoSession(CEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CEndorserService::CRegistUserSession::CRegistUserSession(CEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CEndorserService::CCreateInformationSession::CCreateInformationSession(CEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CEndorserService::CProposeSession::CProposeSession(CEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CEndorserService::CQueryTransactionSession::CQueryTransactionSession(CEndorserService& rEndorserService)
	:IUserSession(rEndorserService)
{
}

CEndorserService::CEndorserService(const CDscString& strIpAddr, const ACE_UINT16 nPort)
	: m_strIpAddr(strIpAddr)
	, m_nPort(nPort)
{
}

ACE_INT32 CEndorserService::OnInit(void)
{
	if (CDscHtsServerService::OnInit())
	{
		DSC_RUN_LOG_ERROR("bc endorser service init failed!");

		return -1;
	}

	m_pAcceptor = DSC_THREAD_TYPE_NEW(CMcpAsynchAcceptor<CEndorserService>) CMcpAsynchAcceptor<CEndorserService>(*this);
	if (m_pAcceptor->Open(m_nPort, m_strIpAddr.c_str()))
	{
		DSC_THREAD_TYPE_DEALLOCATE(m_pAcceptor);
		m_pAcceptor = NULL;
		DSC_RUN_LOG_ERROR("acceptor failed, ip addr:%s, port:%d", m_strIpAddr.c_str(), m_nPort);

		return -1;
	}
	else
	{
		this->RegistHandler(m_pAcceptor, ACE_Event_Handler::ACCEPT_MASK);
	}

	if (m_xcsRouter.Open())
	{
		DSC_RUN_LOG_ERROR("x committer service router init failed.");

		return -1;
	}

	DSC_RUN_LOG_INFO("endorser service:%d init succeed!", this->GetID());

	return 0;
}

ACE_INT32 CEndorserService::OnExit(void)
{
	if (m_pAcceptor)
	{
		this->UnRegistHandler(m_pAcceptor, ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL);
		m_pAcceptor->ReleaseServiceHandler();
	}

	CRegistUserSession* pRegistUserSession;
	for (auto it = m_mapRegistUserSession.begin(); it != m_mapRegistUserSession.end();)
	{
		pRegistUserSession = it.second;
		this->CancelDscTimer(pRegistUserSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pRegistUserSession);
	}

	CCreateInformationSession* pCreateInformationSession;
	for (auto it = m_mapCreateInfoSession.begin(); it != m_mapCreateInfoSession.end();)
	{
		pCreateInformationSession = it.second;
		this->CancelDscTimer(pCreateInformationSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pCreateInformationSession);
	}

	CProposeSession* pUpdateUserSession;
	for (auto it = m_mapProposeSession.begin(); it != m_mapProposeSession.end();)
	{
		pUpdateUserSession = it.second;
		this->CancelDscTimer(pUpdateUserSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pUpdateUserSession);
	}

	CQueryUserInfoSession* pQueryUserInfoSession;
	for (auto it = m_mapQueryUserInfoSession.begin(); it != m_mapQueryUserInfoSession.end(); )
	{
		pQueryUserInfoSession = it.second;
		this->CancelDscTimer(pQueryUserInfoSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pQueryUserInfoSession);
	}

	CQueryTransactionSession* pQueryTransactionSession;
	for (auto it = m_mapQueryTransSession.begin(); it != m_mapQueryTransSession.end(); )
	{
		pQueryTransactionSession = it.second;
		this->CancelDscTimer(pQueryTransactionSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pQueryTransactionSession);
	}

	m_xcsRouter.Close();

	return CDscHtsServerService::OnExit();
}

void CEndorserService::SetInnerCcServiceMap(inner_cc_service_list_type& lstInnerCcService)
{
	for (auto it = lstInnerCcService.begin(); it != lstInnerCcService.end(); ++it)
	{
		m_arrHashInnerCcService[it->first] = it->second;
	}
}

void CEndorserService::OnTimeOut(CRegistUserSession* pRegistUserSession)
{
	DSC_RUN_LOG_INFO("RegistUserSession timeout, es-session-id:%d", pRegistUserSession->m_nEsSessionID);

	VBH::CRegistUserEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pRegistUserSession->m_nCltSessionID;
	this->SendHtsMsg(rsp, pRegistUserSession->m_pEndorserServiceHandler);

	m_mapRegistUserSession.Erase(pRegistUserSession->m_nEsSessionID);
	OnRelease(pRegistUserSession);
}

void CEndorserService::OnTimeOut(CCreateInformationSession* pCreateInfoSession)
{
	DSC_RUN_LOG_INFO("CCreateInformationSession timeout, es-session-id:%d", pCreateInfoSession->m_nEsSessionID);

	VBH::CCreateInformationEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pCreateInfoSession->m_nCltSessionID;
	this->SendHtsMsg(rsp, pCreateInfoSession->m_pEndorserServiceHandler);

	m_mapCreateInfoSession.Erase(pCreateInfoSession->m_nEsSessionID);
	OnRelease(pCreateInfoSession);
}

void CEndorserService::OnTimeOut(CProposeSession* pProposeSession)
{
	DSC_RUN_LOG_INFO("ProposeSession timeout, es-session-id:%d", pProposeSession->m_nEsSessionID);

	VBH::CProposeEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pProposeSession->m_nCltSessionID;
	SendHtsMsg(rsp, pProposeSession->m_pEndorserServiceHandler);

	m_mapProposeSession.Erase(pProposeSession->m_nEsSessionID);
	OnRelease(pProposeSession);
}

void CEndorserService::OnTimeOut(CQueryUserInfoSession* pQueryUserSession)
{
	DSC_RUN_LOG_INFO("CQueryUserInfoSession timeout, session-id%d", pQueryUserSession->m_nEsSessionID);

	VBH::CQueryUserInfoEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nSessionID = pQueryUserSession->m_nCltSessionID;
	this->SendHtsMsg(rsp, pQueryUserSession->m_pEndorserServiceHandler);

	m_mapQueryUserInfoSession.Erase(pQueryUserSession->m_nEsSessionID);
	OnRelease(pQueryUserSession);
}

void CEndorserService::OnTimeOut(CQueryTransactionSession* pQueryTransSession)
{
	DSC_RUN_LOG_INFO("CQueryTransactionSession timeout, session-id%d", pQueryTransSession->m_nEsSessionID);

	VBH::CQueryTransInfoEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pQueryTransSession->m_nCltSessionID;
	this->SendHtsMsg(rsp, pQueryTransSession->m_pEndorserServiceHandler);

	m_mapQueryTransSession.Erase(pQueryTransSession->m_nEsSessionID);
	OnRelease(pQueryTransSession);
}

void CEndorserService::OnNetError(CRegistUserSession* pRegistUserSession)
{
	m_mapRegistUserSession.Erase(pRegistUserSession);
	this->CancelDscTimer(pRegistUserSession);
	DSC_THREAD_TYPE_DELETE(pRegistUserSession);
}

void CEndorserService::OnNetError(CCreateInformationSession* pCreateInfoSession)
{
	m_mapCreateInfoSession.Erase(pCreateInfoSession);
	this->CancelDscTimer(pCreateInfoSession);
	DSC_THREAD_TYPE_DELETE(pCreateInfoSession);
}

void CEndorserService::OnNetError(CProposeSession* pProposeSession)
{
	m_mapProposeSession.Erase(pProposeSession);
	this->CancelDscTimer(pProposeSession);
	DSC_THREAD_TYPE_DELETE(pProposeSession);
}

void CEndorserService::OnNetError(CQueryUserInfoSession* pQueryUserSession)
{
	m_mapQueryUserInfoSession.Erase(pQueryUserSession);
	this->CancelDscTimer(pQueryUserSession);
	DSC_THREAD_TYPE_DELETE(pQueryUserSession);
}

void CEndorserService::OnNetError(CQueryTransactionSession* pQueryTransSession)
{
	m_mapQueryTransSession.Erase(pQueryTransSession);
	this->CancelDscTimer(pQueryTransSession);
	DSC_THREAD_TYPE_DELETE(pQueryTransSession);
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CRegistUserCltEsReq);
	
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CRegistUserSession* pRegistUserSession = DSC_THREAD_TYPE_NEW(CRegistUserSession) CRegistUserSession(*this);

	pRegistUserSession->m_nEsSessionID = m_nSessionID;
	pRegistUserSession->m_nCltSessionID = rRegistUserReq.m_nCltSessionID;
	pRegistUserSession->m_nChannelID = rRegistUserReq.m_nChannelID;
	pRegistUserSession->m_bSubmitNode = rRegistUserReq.m_bSubmitNode;

	VBH::CRegistUserEsCcReq req;

	req.m_nEsSessionID = m_nSessionID;
	req.m_nChannelID = rRegistUserReq.m_nChannelID;
	req.m_userInfo = rRegistUserReq.m_userInfo;

	//在使用Inner-CC时，SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，应答逻辑需要查找session，所以必须在函数执行前，将session插入map
	this->SetMcpHandleSession(pRegistUserSession, pMcpHandler);
	SetDscTimer(pRegistUserSession, EN_SESSION_TIMEOUT_VALUE);
	m_mapRegistUserSession.DirectInsert(m_nSessionID, pRegistUserSession);
	++m_nSessionID;

	IInnerCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)rRegistUserReq.m_nChannelID];
	
	if (pCcService)
	{
		nReturnCode = pCcService->SendRegistUserEsCcReq(req);
	}
	else
	{
		CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, rRegistUserReq.m_nChannelID, DSC::EN_INVALID_ID);

		nReturnCode = SendDscMessage(req, ccAddr);
	}

	//单独发送处理错误应答
	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
		CRegistUserSession* pRegistUserSession = m_mapRegistUserSession.Erase(req.m_nEsSessionID);
		
		if (pRegistUserSession)
		{
			this->CancelDscTimer(pRegistUserSession);
			OnRelease(pRegistUserSession);

			VBH::CRegistUserEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = rRegistUserReq.m_nCltSessionID;

			SendHtsMsg(rsp, pMcpHandler->GetHandleID());
			DSC_RUN_LOG_ERROR("send CRegistUserEsCcReq message receive error.");
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CRegistUserCltEsReq);
	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitRegistUserTransactionCltEsReq);
	
	//1. 查找session
	CRegistUserSession* pRegistUserSession = m_mapRegistUserSession.Find(rSubmitRegistUserReq.m_nEsSessionID);

	if (pRegistUserSession)
	{
		VBH::CSubmitRegistUserTransactionEsTasReq req;
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

		//2. 把未打包事务内容发送到 order(经tas转发)
		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			req.m_nEsSessionID = pRegistUserSession->m_nEsSessionID;
			req.m_nChannelID = pRegistUserSession->m_nChannelID;
			VBH::Assign(req.m_userInfo, pRegistUserSession->m_ccGenerateUserInfo);
			req.m_cltPubKey = rSubmitRegistUserReq.m_cltPubKey;
			req.m_svrPriKey = rSubmitRegistUserReq.m_svrPriKey;
			req.m_envelopeKey = rSubmitRegistUserReq.m_envelopeKey;

			if (m_pTas->SendSubmitRegistUserTransactionEsTasReq(req))
			{
				DSC_RUN_LOG_ERROR("send dsc msg error.");
				nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
			}
		}

		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			VBH::CSubmitRegistUserTransactionEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pRegistUserSession->m_nCltSessionID;
			this->SendHtsMsg(rsp, pMcpHandler);

			m_mapRegistUserSession.Erase(pRegistUserSession);
			this->CancelDscTimer(pRegistUserSession);
			this->OnRelease(pRegistUserSession); //释放session
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rSubmitRegistUserReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitRegistUserTransactionCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CCancelRegistUserCltEsReq& rCancelReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CCancelRegistUserCltEsReq);
	
	CRegistUserSession* pRegistUserSession = m_mapRegistUserSession.Erase(rCancelReq.m_nEsSessionID);

	if (pRegistUserSession)
	{
		this->CancelDscTimer(pRegistUserSession);
		this->OnRelease(pRegistUserSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rCancelReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CCancelRegistUserCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CCreateInformationCltEsReq& rCreateInfoReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CCreateInformationCltEsReq);

	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CCreateInformationSession* pCreateInfoSession = DSC_THREAD_TYPE_NEW(CCreateInformationSession) CCreateInformationSession(*this);

	pCreateInfoSession->m_nEsSessionID = m_nSessionID;
	pCreateInfoSession->m_nCltSessionID = rCreateInfoReq.m_nCltSessionID;
	pCreateInfoSession->m_nChannelID = rCreateInfoReq.m_nChannelID;
	pCreateInfoSession->m_bSubmitNode = rCreateInfoReq.m_bSubmitNode;

	if (rCreateInfoReq.m_bSubmitNode)
	{
		VBH::Assign(pCreateInfoSession->m_strSignature, rCreateInfoReq.m_signature);
		VBH::Assign(pCreateInfoSession->m_strProposal, rCreateInfoReq.m_proposal);
	}

	VBH::CCreateInformationEsCcReq req;

	req.m_nEsSessionID = m_nSessionID;
	req.m_nChannelID = rCreateInfoReq.m_nChannelID;
	req.m_proposal = rCreateInfoReq.m_proposal;

	//在使用Inner-CC时，SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，应答逻辑需要查找session，所以必须在函数执行前，将session插入map
	this->SetMcpHandleSession(pCreateInfoSession, pMcpHandler);
	SetDscTimer(pCreateInfoSession, EN_SESSION_TIMEOUT_VALUE);
	m_mapCreateInfoSession.DirectInsert(m_nSessionID, pCreateInfoSession);
	++m_nSessionID;

	IInnerCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)rCreateInfoReq.m_nChannelID];

	if (pCcService)
	{
		nReturnCode = pCcService->SendCreateInformationEsCcReq(req);
	}
	else
	{
		CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, rCreateInfoReq.m_nChannelID, DSC::EN_INVALID_ID);

		nReturnCode = SendDscMessage(req, ccAddr);
	}

	//单独发送处理错误应答
	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
		CRegistUserSession* pRegistUserSession = m_mapRegistUserSession.Erase(req.m_nEsSessionID);

		if (pRegistUserSession)
		{
			this->CancelDscTimer(pRegistUserSession);
			OnRelease(pRegistUserSession);

			VBH::CRegistUserEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = rCreateInfoReq.m_nCltSessionID;

			SendHtsMsg(rsp, pMcpHandler->GetHandleID());
			DSC_RUN_LOG_ERROR("send CRegistUserEsCcReq message receive error.");
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CCreateInformationCltEsReq);
	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CSubmitCreateInformationTransactionCltEsReq& rSubmitCreateInfoReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitCreateInformationTransactionCltEsReq);

	//1. 查找session
	CCreateInformationSession* pCreateInfoSession = m_mapCreateInfoSession.Find(rSubmitCreateInfoReq.m_nEsSessionID);

	if (pCreateInfoSession)
	{
		VBH::CSubmitCreateInformationTransactionEsTasReq req;
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

		//2. 把未打包事务内容发送到 order(经tas转发)
		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			req.m_nEsSessionID = pCreateInfoSession->m_nEsSessionID;
			req.m_nChannelID = pCreateInfoSession->m_nChannelID;
			VBH::Assign(req.m_signature, pCreateInfoSession->m_strSignature);
			VBH::Assign(req.m_proposal, pCreateInfoSession->m_strProposal);
			VBH::Assign(req.m_value, pCreateInfoSession->m_strInitValue);

			if (m_pTas->SendSubmitCreateInformationTransactionEsTasReq(req))
			{
				DSC_RUN_LOG_ERROR("send dsc msg error.");
				nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
			}
		}

		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			VBH::CSubmitCreateInformationTransactionEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pCreateInfoSession->m_nCltSessionID;
			this->SendHtsMsg(rsp, pMcpHandler);

			m_mapCreateInfoSession.Erase(pCreateInfoSession);
			this->CancelDscTimer(pCreateInfoSession);
			this->OnRelease(pCreateInfoSession); //释放session
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rSubmitCreateInfoReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitCreateInformationTransactionCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CCancelCreateInformationCltEsReq& rCancelReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CCancelCreateInformationCltEsReq);

	CCreateInformationSession* pCreateInfoSession = m_mapCreateInfoSession.Erase(rCancelReq.m_nEsSessionID);

	if (pCreateInfoSession)
	{
		this->CancelDscTimer(pCreateInfoSession);
		this->OnRelease(pCreateInfoSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rCancelReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CCancelCreateInformationCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CProposeCltEsReq& rSubmitProposalReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CProposeCltEsReq);
	
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CProposeSession* pProposeSession = DSC_THREAD_TYPE_NEW(CProposeSession) CProposeSession(*this);
	VBH::CProposeEsCcReq req;

	pProposeSession->m_bSubmitNode = rSubmitProposalReq.m_bSubmitNode;
	pProposeSession->m_nEsSessionID = m_nSessionID;
	pProposeSession->m_nCltSessionID = rSubmitProposalReq.m_nCltSessionID;
	pProposeSession->m_nChannelID = rSubmitProposalReq.m_nChannelID;

	req.m_nEsSessionID = m_nSessionID;
	req.m_nAction = rSubmitProposalReq.m_nAction;
	req.m_nChannelID = rSubmitProposalReq.m_nChannelID;
	req.m_proposeUserKey = rSubmitProposalReq.m_userKey;
	req.m_signature = rSubmitProposalReq.m_signature;
	req.m_proposal = rSubmitProposalReq.m_proposal;

	//在使用Inner-CC时，SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，应答逻辑需要查找session，所以必须在函数执行前，将session插入map
	this->SetMcpHandleSession(pProposeSession, pMcpHandler);
	this->SetDscTimer(pProposeSession, CEndorserService::EN_SESSION_TIMEOUT_VALUE);
	m_mapProposeSession.DirectInsert(m_nSessionID, pProposeSession);
	++m_nSessionID;

	IInnerCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)rSubmitProposalReq.m_nChannelID];

	if (pCcService)
	{
		nReturnCode = pCcService->SendProposeEsCcReq(req);
	}
	else
	{
		CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, rSubmitProposalReq.m_nChannelID, DSC::EN_INVALID_ID);

		nReturnCode = SendDscMessage(req, ccAddr);
	}

	//单独发送处理错误应答
	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
		CProposeSession* pProposeSession = m_mapProposeSession.Erase(req.m_nEsSessionID);

		if (pProposeSession)
		{
			this->CancelDscTimer(pProposeSession);
			OnRelease(pProposeSession);

			VBH::CProposeEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = rSubmitProposalReq.m_nCltSessionID;

			SendHtsMsg(rsp, pMcpHandler->GetHandleID());
			DSC_RUN_LOG_ERROR("send CRegistUserEsCcReq message receive error.");
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CProposeCltEsReq);
	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitProposalTransactionCltEsReq);
	
	CProposeSession* pProposeSession = m_mapProposeSession.Find(rSubmitTransactionReq.m_nEsSessionID);

	if (DSC_LIKELY(pProposeSession))
	{
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

		VBH::CSubmitProposalTransactionEsTasReq req;
		req.m_nActionID = pProposeSession->m_nActionID;
		req.m_nEsSessionID = pProposeSession->m_nEsSessionID;
		req.m_nChannelID = pProposeSession->m_nChannelID;
		VBH::Assign(req.m_transContent, pProposeSession->m_strTransContent);

		if (m_pTas->SendSubmitProposalTransactionEsTasReq(req))
		{
			DSC_RUN_LOG_ERROR("send dsc msg failed.");
			nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}

		if (nReturnCode != VBH::EN_OK_TYPE) //出现错误
		{
			VBH::CSubmitProposalTransactionEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pProposeSession->m_nCltSessionID;
			this->SendHtsMsg(rsp, pMcpHandler);

			m_mapProposeSession.Erase(pProposeSession);
			this->CancelDscTimer(pProposeSession);
			this->OnRelease(pProposeSession);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rSubmitTransactionReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitProposalTransactionCltEsReq);
	
	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CCancelProposalTransactionCltEsReq& rCancelReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CCancelProposalTransactionCltEsReq);
	
	CProposeSession* pSession = m_mapProposeSession.Erase(rCancelReq.m_nEsSessionID);

	if (pSession)
	{
		this->CancelDscTimer(pSession);
		this->OnRelease(pSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rCancelReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CCancelProposalTransactionCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CQueryUserInfoCltEsReq& rQueryUserReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CQueryUserInfoCltEsReq);

	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CDscMsg::CDscMsgAddr addr;

	if (m_xcsRouter.GetXcsAddr(addr, rQueryUserReq.m_nChannelID))
	{
		DSC_RUN_LOG_INFO("can not find channel's xcs addr, channel-id:%d.", rQueryUserReq.m_nChannelID);

		nReturnCode = VBH::EN_CANNOT_FOUND_CHANNEL_COMMITTER_ERROR_TYPE;
	}
	else
	{
		VBH::CQueryUserInfoEsXcsReq req;

		req.m_userKey = rQueryUserReq.m_userKey;
		req.m_nEsSessionID = m_nSessionID;

		if (this->SendDscMessage(req, addr))
		{
			DSC_RUN_LOG_ERROR("send dsc message failed, dst-addr(node-id:service-id):[%d:%d].", addr.GetNodeID(), addr.GetServiceID());

			nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
		}
		else
		{
			CQueryUserInfoSession* pSession = DSC_THREAD_TYPE_NEW(CQueryUserInfoSession) CQueryUserInfoSession(*this);

			pSession->m_nCltSessionID = rQueryUserReq.m_nCltSessionID;

			this->SetMcpHandleSession(pSession, pMcpHandler);
			SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
			m_mapQueryUserInfoSession.DirectInsert(m_nSessionID, pSession);
			++m_nSessionID;
		}
	}

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CQueryUserInfoEsCltRsp rsp;
		
		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nSessionID = rQueryUserReq.m_nCltSessionID;
		
		SendHtsMsg(rsp, pMcpHandler);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CQueryUserInfoCltEsReq);

	return 0;
}

ACE_INT32 CEndorserService::OnHtsMsg(VBH::CQueryTransInfoCltEsReq& rQueryTransReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CQueryTransInfoCltEsReq);
	
	CDscMsg::CDscMsgAddr addr;
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

	if (m_xcsRouter.GetXcsAddr(addr, rQueryTransReq.m_nChannelID))
	{
		DSC_RUN_LOG_ERROR("can not find xcs addr. channel-id:%d", rQueryTransReq.m_nChannelID);

		nReturnCode = VBH::EN_CANNOT_FOUND_CHANNEL_COMMITTER_ERROR_TYPE;
	}
	else
	{
		VBH::CQueryTransInfoEsXcsReq req;

		req.m_nEsSessionID = m_nSessionID;
		req.m_transKey = rQueryTransReq.m_transKey;

		if (this->SendDscMessage(req, addr))
		{
			DSC_RUN_LOG_ERROR("send dsc message failed, dst-addr(node-id:service-id):[%d:%d].", addr.GetNodeID(), addr.GetServiceID());

			nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
		else
		{
			CQueryTransactionSession* pSession = DSC_THREAD_TYPE_NEW(CQueryTransactionSession) CQueryTransactionSession(*this);

			pSession->m_nCltSessionID = rQueryTransReq.m_nCltSessionID;
			pSession->m_nEsSessionID = m_nSessionID;

			this->SetMcpHandleSession(pSession, pMcpHandler);
			SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
			m_mapQueryTransSession.DirectInsert(m_nSessionID, pSession);
			++m_nSessionID;
		}
	}

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CQueryTransInfoEsCltRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nCltSessionID = rQueryTransReq.m_nCltSessionID;

		SendHtsMsg(rsp, pMcpHandler);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CQueryTransInfoCltEsReq);

	return 0;
}

void CEndorserService::OnDscMsg(VBH::CRegistUserCcEsRsp& rRegistUserRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendRegistUserCcEsRsp(rRegistUserRsp);
}

void CEndorserService::OnDscMsg(VBH::CCreateInformationCcEsRsp& rCreateInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendCreateInformationCcEsRsp(rCreateInfoRsp);
}

void CEndorserService::OnDscMsg(VBH::CProposeCcEsRsp& rSubmitProposalRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendProposeCcEsRsp(rSubmitProposalRsp);
}

void CEndorserService::OnDscMsg(VBH::CQueryUserInfoXcsEsRsp& rQueryUserInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CQueryUserInfoXcsEsRsp);
	
	CQueryUserInfoSession* pQueryUserInfoSession = m_mapQueryUserInfoSession.Erase(rQueryUserInfoRsp.m_nEsSessionID);

	if (pQueryUserInfoSession)
	{
		VBH::CQueryUserInfoEsCltRsp rsp;

		rsp.m_nSessionID = pQueryUserInfoSession->m_nCltSessionID;
		rsp.m_nReturnCode = rQueryUserInfoRsp.m_nReturnCode;
		rsp.m_userInfo = rQueryUserInfoRsp.m_userInfo;

		this->SendHtsMsg(rsp, pQueryUserInfoSession->m_pEndorserServiceHandler);
		this->CancelDscTimer(pQueryUserInfoSession);
		this->OnRelease(pQueryUserInfoSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find query user info session, session id:%d", rQueryUserInfoRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CQueryUserInfoXcsEsRsp);
}

void CEndorserService::OnDscMsg(VBH::CQueryCryptKeyGetTransXcsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CQueryCryptKeyGetTransXcsEsRsp);
	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CQueryCryptKeyGetTransXcsEsRsp);
}

void CEndorserService::OnDscMsg(VBH::CQueryTransInfoXcsEsRsp& rQueryTransInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CQueryTransInfoXcsEsRsp);
	
	CQueryTransactionSession* pQueryTransSession = m_mapQueryTransSession.Erase(rQueryTransInfoRsp.m_nEsSessionID);

	if (pQueryTransSession)
	{
		VBH::CQueryTransInfoEsCltRsp rsp;

		rsp.m_nCltSessionID = pQueryTransSession->m_nCltSessionID;
		rsp.m_nReturnCode = rQueryTransInfoRsp.m_nReturnCode;
		//TODO:===========20191218
		//rsp.m_transInfo = rQueryTransInfoRsp.m_transInfo;

		this->CancelDscTimer(pQueryTransSession);
		this->SendHtsMsg(rsp, pQueryTransSession->m_pEndorserServiceHandler);
		this->OnRelease(pQueryTransSession);
	}
	else
	{
		DSC_RUN_LOG_INFO("can not find session.");
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CQueryTransInfoXcsEsRsp);
}

void CEndorserService::SendSubmitRegistUserTransactionTasEsRsp(VBH::CSubmitRegistUserTransactionTasEsRsp& rSubmitTransactionRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitRegistUserTransactionTasEsRsp);

	CRegistUserSession* pRegistSession = m_mapRegistUserSession.Erase(rSubmitTransactionRsp.m_nEsSessionID);

	if (DSC_LIKELY(pRegistSession))
	{
		//1. 向客户端发送应答
		VBH::CSubmitRegistUserTransactionEsCltRsp rsp;

		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pRegistSession->m_nCltSessionID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE) //提交成功时，再编码加密待发送数据
		{
			rsp.m_userKey.m_nSequenceNumber = rSubmitTransactionRsp.m_nSequenceNumber;
			rsp.m_userKey.m_nAllocatedID = rSubmitTransactionRsp.m_nUserID;
		}
		this->SendHtsMsg(rsp, pRegistSession->m_pEndorserServiceHandler);

		//3. 删除session
		this->CancelDscTimer(pRegistSession);
		this->OnRelease(pRegistSession);
	}
	else
	{
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rSubmitTransactionRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitRegistUserTransactionTasEsRsp);
}

void CEndorserService::SendSubmitCreateInformationTransactionTasEsRsp(VBH::CSubmitCreateInformationTransactionTasEsRsp& rSubmitTransactionRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitCreateInformationTransactionTasEsRsp);

	CCreateInformationSession* pCreateInfoSession = m_mapCreateInfoSession.Erase(rSubmitTransactionRsp.m_nEsSessionID);

	if (DSC_LIKELY(pCreateInfoSession))
	{
		//1. 向客户端发送应答
		VBH::CSubmitCreateInformationTransactionEsCltRsp rsp;

		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pCreateInfoSession->m_nCltSessionID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE) //提交成功时，再编码加密待发送数据
		{
			rsp.m_infoKey.m_nSequenceNumber = rSubmitTransactionRsp.m_nSequenceNumber;
			rsp.m_infoKey.m_nAllocatedID = rSubmitTransactionRsp.m_nInfoID;
		}
		this->SendHtsMsg(rsp, pCreateInfoSession->m_pEndorserServiceHandler);

		//3. 删除session
		this->CancelDscTimer(pCreateInfoSession);
		this->OnRelease(pCreateInfoSession);
	}
	else
	{
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rSubmitTransactionRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitCreateInformationTransactionTasEsRsp);
}

void CEndorserService::SendSubmitProposalTransactionTasEsRsp(VBH::CSubmitProposalTransactionTasEsRsp& rSubmitTransactionRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CSubmitProposalTransactionTasEsRsp);

	CProposeSession* pSession = m_mapProposeSession.Erase(rSubmitTransactionRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		//1. 向客户端发送应答
		VBH::CSubmitProposalTransactionEsCltRsp rsp;

		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pSession->m_nCltSessionID;
		rsp.m_alocTransKey = rSubmitTransactionRsp.m_alocTransKey;
		rsp.m_vecInfoID = rSubmitTransactionRsp.m_vecInfoID;

		this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler);

		//3. 删除session
		this->CancelDscTimer(pSession);
		this->OnRelease(pSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find session, session id:%d", rSubmitTransactionRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CSubmitProposalTransactionTasEsRsp);
}

void CEndorserService::SendRegistUserCcEsRsp(VBH::CRegistUserCcEsRsp& rRegistUserRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CRegistUserCcEsRsp);

	//1.找session
	CRegistUserSession* pRegistUserSession = m_mapRegistUserSession.Find(rRegistUserRsp.m_nEsSessionID);

	if (pRegistUserSession)
	{
		ACE_INT32 nReturnCode = rRegistUserRsp.m_nReturnCode;

		//2.回应答
		VBH::CRegistUserEsCltRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nCltSessionID = pRegistUserSession->m_nCltSessionID;
		rsp.m_nEsSessionID = pRegistUserSession->m_nEsSessionID;
		rsp.m_userInitInfo = rRegistUserRsp.m_userInitInfo;

		if (pRegistUserSession->m_bSubmitNode) //失败时拷贝效率低，但是正常时的判断条件简单 //正常路径高效，异常路径作对就好
		{
			VBH::Assign(pRegistUserSession->m_ccGenerateUserInfo, rRegistUserRsp.m_userInitInfo);
		}

		if (this->SendHtsMsg(rsp, pRegistUserSession->m_pEndorserServiceHandler))
		{
			nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
		}

		//3.删session
		if ((nReturnCode != VBH::EN_OK_TYPE) || !pRegistUserSession->m_bSubmitNode)
		{
			this->CancelDscTimer(pRegistUserSession);
			m_mapRegistUserSession.Erase(pRegistUserSession);
			OnRelease(pRegistUserSession);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rRegistUserRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CRegistUserCcEsRsp);
}

void CEndorserService::SendCreateInformationCcEsRsp(VBH::CCreateInformationCcEsRsp& rCreateInfoRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CCreateInformationCcEsRsp);

	//1.找session
	CCreateInformationSession* pCreateInfoSession = m_mapCreateInfoSession.Find(rCreateInfoRsp.m_nEsSessionID);

	if (pCreateInfoSession)
	{
		ACE_INT32 nReturnCode = rCreateInfoRsp.m_nReturnCode;

		//2.回应答
		VBH::CCreateInformationEsCltRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nCltSessionID = pCreateInfoSession->m_nCltSessionID;
		rsp.m_nEsSessionID = pCreateInfoSession->m_nEsSessionID;
		rsp.m_initValue = rCreateInfoRsp.m_initValue;

		if (pCreateInfoSession->m_bSubmitNode)
		{
			VBH::Assign(pCreateInfoSession->m_strInitValue, rCreateInfoRsp.m_initValue);
		}

		if (this->SendHtsMsg(rsp, pCreateInfoSession->m_pEndorserServiceHandler))
		{
			nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
		}

		//3.删session
		if ((nReturnCode != VBH::EN_OK_TYPE) || !pCreateInfoSession->m_bSubmitNode)
		{
			this->CancelDscTimer(pCreateInfoSession);
			m_mapCreateInfoSession.Erase(pCreateInfoSession);
			OnRelease(pCreateInfoSession);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rCreateInfoRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CCreateInformationCcEsRsp);
}

void CEndorserService::SendProposeCcEsRsp(VBH::CProposeCcEsRsp& rSubmitProposalRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CEndorserService, VBH::CProposeCcEsRsp);

	CProposeSession* pProposeSession = m_mapProposeSession.Find(rSubmitProposalRsp.m_nEsSessionID);

	if (DSC_LIKELY(pProposeSession))
	{
		VBH::CProposeEsCltRsp rsp;

		rsp.m_nReturnCode = rSubmitProposalRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pProposeSession->m_nCltSessionID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			if (pProposeSession->m_bSubmitNode) //如果是最终提交节点，则记录transaction content，后续需要提交
			{
				VBH::Assign(pProposeSession->m_strTransContent, rSubmitProposalRsp.m_transContent);
			}

			rsp.m_nEsSessionID = pProposeSession->m_nEsSessionID;
			rsp.m_transContent = rSubmitProposalRsp.m_transContent;
		}

		//发送应答到客户端
		this->SendHtsMsg(rsp, pProposeSession->m_pEndorserServiceHandler);

		if (!pProposeSession->m_bSubmitNode) //不是提交节点，则删除session
		{
			this->CancelDscTimer(pProposeSession);
			m_mapProposeSession.Erase(pProposeSession);
			OnRelease(pProposeSession);
		}
	}
	else
	{
		//找不到session
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rSubmitProposalRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CEndorserService, VBH::CProposeCcEsRsp);
}

void CEndorserService::SendExplorerQueryCcEsRsp(VBH::CExplorerQueryCcEsRsp& rsp)
{

}

void CEndorserService::OnNetworkError(CMcpHandler* pMcpHandler)
{
	CEndorserServiceHandler* pEndorserServiceHandler = (CEndorserServiceHandler*)pMcpHandler;

	CDscTypeArray<IUserSession>& arrUserSession = pEndorserServiceHandler->m_arrUserSession;
	const ACE_UINT32 nSize = arrUserSession.Size();

	for (ACE_UINT32 i = 0; i < nSize; ++i)
	{
		arrUserSession[i]->OnNetError();
	}
	arrUserSession.Clear();

	CDscHtsServerService::OnNetworkError(pMcpHandler);

	DSC_RUN_LOG_INFO("endorser network error.");
}

CEndorserService::CEndorserServiceHandler::CEndorserServiceHandler(CMcpServerService& rService, ACE_HANDLE handle, const ACE_INT32 nHandleID)
: CMcpServerHandler(rService, handle, nHandleID)
{
}
