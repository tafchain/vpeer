#include "openssl/rand.h"
#include "openssl/err.h"

#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"

#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_macro_def.h"

#include "end_ces/crypt_endorser_service.h"


void CCryptEndorserService::GetConfigTableName(CDscString& strConfigTableName)
{
	strConfigTableName.assign(DSC_STRING_TYPE_PARAM("CES_CFG"));
}

CCryptEndorserService::IUserSession::IUserSession(CCryptEndorserService& rEndorserService)
	: m_rEndorserService(rEndorserService)
{
}

CCryptEndorserService::CQuerySession::CQuerySession(CCryptEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CCryptEndorserService::CRegistUserSession::CRegistUserSession(CCryptEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}


CCryptEndorserService::CProposeSession::CProposeSession(CCryptEndorserService& rEndorserService)
	: IUserSession(rEndorserService)
{
}

CCryptEndorserService::CLoadCcSession::CLoadCcSession(CCryptEndorserService& rEndorserService)
	: m_rEndorserService(rEndorserService)
{
}
CCryptEndorserService::CCryptEndorserService(const CDscString& strIpAddr, const ACE_UINT16 nPort)
	: m_strIpAddr(strIpAddr)
	, m_nPort(nPort)
{
}

ACE_INT32 CCryptEndorserService::OnInit(void)
{
	if (CDscHtsServerService::OnInit())
	{
		DSC_RUN_LOG_ERROR("bc endorser service init failed!");
		return -1;
	}

	m_pAcceptor = DSC_THREAD_TYPE_NEW(CMcpAsynchAcceptor<CCryptEndorserService>) CMcpAsynchAcceptor<CCryptEndorserService>(*this);
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

	if (m_csRouter.Open())
	{
		DSC_RUN_LOG_ERROR("x committer service router init failed.");
		return -1;
	}

	m_pGroup = VBH::vbhEcGroup();
	if (!m_pGroup)
	{
		DSC_RUN_LOG_ERROR("create ec group failed");
		return-1;
	}

	if (VBH::GetVbhProfileString("PEER_ENVELOPE_KEY", m_peerEnvelopeKey))
	{
		DSC_RUN_LOG_WARNING("cann't read 'PEER_ENVELOPE_KEY' configure item value");
		return -1;
	}
	if (m_peerEnvelopeKey.empty())
	{
		DSC_RUN_LOG_WARNING("'PEER_ENVELOPE_KEY' cann't be empty");
		return -1;
	}

	DSC_RUN_LOG_INFO("crypt endorser service: %d init succeed!", this->GetID());
	return 0;
}

ACE_INT32 CCryptEndorserService::OnExit(void)
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

	CProposeSession* pUpdateUserSession;
	for (auto it = m_mapProposeSession.begin(); it != m_mapProposeSession.end();)
	{
		pUpdateUserSession = it.second;
		this->CancelDscTimer(pUpdateUserSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pUpdateUserSession);
	}

	CQuerySession* pQueryUserInfoSession;
	for (auto it = m_mapQuerySession.begin(); it != m_mapQuerySession.end(); )
	{
		pQueryUserInfoSession = it.second;
		this->CancelDscTimer(pQueryUserInfoSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pQueryUserInfoSession);
	}

	//CQueryTransactionSession* pQueryTransactionSession;
	//for (auto it = m_mapQueryTransSession.begin(); it != m_mapQueryTransSession.end(); )
	//{
	//	pQueryTransactionSession = it.second;
	//	this->CancelDscTimer(pQueryTransactionSession);
	//	++it;
	//	DSC_THREAD_TYPE_DELETE(pQueryTransactionSession);
	//}

	//释放crypt-key缓冲区
	CCryptKey* pCryptKey = m_dqueueCryptKey.PopFront();
	while (pCryptKey)
	{
		DSC_THREAD_TYPE_DELETE(pCryptKey);
		pCryptKey = m_dqueueCryptKey.PopFront();
	}

	m_csRouter.Close();

	if (m_pGroup)
	{
		EC_GROUP_free(m_pGroup);
		m_pGroup = NULL;
	}

	return CDscHtsServerService::OnExit();
}

void CCryptEndorserService::SetInnerCcServiceMap(inner_cc_service_list_type& lstInnerCcService)
{
	for (auto it = lstInnerCcService.begin(); it != lstInnerCcService.end(); ++it)
	{
		m_arrHashInnerCcService[it->first] = it->second;
	}
}

void CCryptEndorserService::OnTimeOut(CRegistUserSession* pRegistUserSession)
{
	DSC_RUN_LOG_INFO("RegistUserSession timeout, es-session-id:%d, client-session-id:%d, channel-id:%d",
		pRegistUserSession->m_nEsSessionID, pRegistUserSession->m_nCltSessionID, pRegistUserSession->m_nChannelID);

	VBH::CCryptRegistUserEsCltRsp rsp;
	ACE_UINT32 nEsSessionID = pRegistUserSession->m_nEsSessionID;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pRegistUserSession->m_nCltSessionID;
	rsp.m_nEsSessionID = nEsSessionID;

	if (this->SendHtsMsg(rsp, pRegistUserSession->m_pEndorserServiceHandler))
	{
		DSC_RUN_LOG_ERROR("send hts message:CCryptRegistUserEsCltRsp failed.");

		ACE_ASSERT(m_mapRegistUserSession.Find(nEsSessionID) == nullptr);
	}
	else
	{
		//SendHtsMsg 会导致session被删除，因此真正删除前要查询一遍
		pRegistUserSession = m_mapRegistUserSession.Erase(nEsSessionID);
		ACE_ASSERT(pRegistUserSession != nullptr);
		OnRelease(pRegistUserSession);
	}
}

void CCryptEndorserService::OnTimeOut(CProposeSession* pProposeSession)
{
	DSC_RUN_LOG_INFO("ProposeSession timeout, trans-content-size:%d, es-session-id:%d, client-session-id:%d, channel-id:%d",
		pProposeSession->m_strTransContent.size(), pProposeSession->m_nEsSessionID, pProposeSession->m_nCltSessionID, pProposeSession->m_nChannelID);

	VBH::CCryptProposeEsCltRsp rsp;
	ACE_UINT32 nEsSessionID = pProposeSession->m_nEsSessionID;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pProposeSession->m_nCltSessionID;
	rsp.m_nEsSessionID = nEsSessionID;

	if (this->SendHtsMsg(rsp, pProposeSession->m_pEndorserServiceHandler))
	{
		DSC_RUN_LOG_ERROR("send hts message:CCryptProposeEsCltRsp failed.");
		ACE_ASSERT(m_mapProposeSession.Find(nEsSessionID) == nullptr);
	}
	else
	{
		pProposeSession = m_mapProposeSession.Erase(nEsSessionID);
		ACE_ASSERT(pProposeSession != nullptr);
		OnRelease(pProposeSession);
	}
}

void CCryptEndorserService::OnTimeOut(CQuerySession* pQueryUserSession)
{
	DSC_RUN_LOG_INFO("CQueryUserInfoSession timeout, es-session-id:%d, client-session-id:%d",
		pQueryUserSession->m_nEsSessionID, pQueryUserSession->m_nCltSessionID);

	VBH::CCryptQueryEsCltRsp rsp;
	ACE_UINT32 nEsSessionID = pQueryUserSession->m_nEsSessionID;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pQueryUserSession->m_nCltSessionID;

	if (this->SendHtsMsg(rsp, pQueryUserSession->m_pEndorserServiceHandler))
	{
		DSC_RUN_LOG_ERROR("send hts message:CCryptQueryUserInfoEsCltRsp failed.");
		ACE_ASSERT(m_mapQuerySession.Find(nEsSessionID) == nullptr);
	}
	else
	{
		pQueryUserSession = m_mapQuerySession.Erase(nEsSessionID);
		ACE_ASSERT(pQueryUserSession != nullptr);
		OnRelease(pQueryUserSession);
	}
}

void CCryptEndorserService::OnTimeOut(CLoadCcSession* pLoadCcSession)
{
	DSC_RUN_LOG_INFO("CLoadCcSession timeout, client-session-id:%d", pLoadCcSession->m_nSessionID);

	VBH::CLoadOuterCcEsCltRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nCltSessionID = pLoadCcSession->m_nSessionID;

	if (this->SendDscMessage(rsp, pLoadCcSession->m_rSrcMsgAddr))
	{
		DSC_RUN_LOG_ERROR("send hts message:CCryptQueryUserInfoEsCltRsp failed.");

	}
	else
	{
		pLoadCcSession = m_mapLoadCcSession.Erase(pLoadCcSession->m_nSessionID);
		DSC_THREAD_TYPE_DELETE(pLoadCcSession);

	}
}


void CCryptEndorserService::OnNetError(CRegistUserSession* pRegistUserSession)
{
	m_mapRegistUserSession.Erase(pRegistUserSession);
	this->CancelDscTimer(pRegistUserSession);
	DSC_THREAD_TYPE_DELETE(pRegistUserSession);
}

void CCryptEndorserService::OnNetError(CProposeSession* pProposeSession)
{
	m_mapProposeSession.Erase(pProposeSession);
	this->CancelDscTimer(pProposeSession);
	DSC_THREAD_TYPE_DELETE(pProposeSession);
}

void CCryptEndorserService::OnNetError(CQuerySession* pQueryUserSession)
{
	m_mapQuerySession.Erase(pQueryUserSession);
	this->CancelDscTimer(pQueryUserSession);
	DSC_THREAD_TYPE_DELETE(pQueryUserSession);
}

void CCryptEndorserService::OnNetError(CLoadCcSession* pLoadCcSession)
{
	m_mapLoadCcSession.Erase(pLoadCcSession);
	this->CancelDscTimer(pLoadCcSession);
	DSC_THREAD_TYPE_DELETE(pLoadCcSession);
}


ACE_INT32 CCryptEndorserService::OnHtsMsg(VBH::CCryptRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CCryptRegistUserCltEsReq);

	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	const ACE_UINT32 nEsSessionID = AllocSessionID(); //记录成功创建session时的session-id，以方便出错时删除session //如果创建session不成功，m_nSessionID是一个空的session-id

	int nDecryptedBufLen;
	char* pDecryptedBuf = VBH::vbhDecrypt((unsigned char*)m_peerEnvelopeKey.data(), nDecryptedBufLen, rRegistUserReq.m_userData.GetBuffer(), rRegistUserReq.m_userData.GetSize());

	if (DSC_LIKELY(pDecryptedBuf))
	{
		bool bSubmitNode;
		ACE_UINT32 nChannelID;
		ACE_INT32 nNonce;
		DSC::CDscShortBlob userInfo;

		VBH::CCryptRegistUserCltEsReqDataWrapper wrapper(bSubmitNode, nChannelID, userInfo, nNonce);

		if (DSC::Decode(wrapper, pDecryptedBuf, nDecryptedBufLen)) //解码成功
		{
			DSC_RUN_LOG_ERROR("decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			CRegistUserSession* pSession = DSC_THREAD_TYPE_NEW(CRegistUserSession) CRegistUserSession(*this);

			pSession->m_nEsSessionID = nEsSessionID;
			pSession->m_nCltSessionID = rRegistUserReq.m_nCltSessionID;
			pSession->m_nChannelID = nChannelID;
			pSession->m_bSubmitNode = bSubmitNode;
			pSession->m_nNonce = nNonce;

			VBH::CRegistUserEsCcReq req;

			req.m_nEsSessionID = nEsSessionID;
			req.m_nChannelID = nChannelID;
			req.m_userInfo = userInfo;

			//在使用Inner-CC时，SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，应答逻辑需要查找session，所以必须在函数执行前，将session插入map
			this->SetMcpHandleSession(pSession, pMcpHandler);
			this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
			m_mapRegistUserSession.DirectInsert(nEsSessionID, pSession);

			IEndorserCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)nChannelID];

			if (pCcService)
			{
				nReturnCode = pCcService->SendRegistUserEsCcReq(req);
			}
			else
			{
				CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, nChannelID, DSC::EN_INVALID_ID);

				nReturnCode = SendDscMessage(req, ccAddr);
			}

			if (nReturnCode != VBH::EN_OK_TYPE)
			{
				DSC_RUN_LOG_ERROR("send CRegistUserEsCcReq message receive error.");
			}
		}

		DSC_THREAD_FREE(pDecryptedBuf);
	}
	else
	{
		DSC_RUN_LOG_INFO("decrypt error.")
			nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
	}

	//单独发送处理错误应答
	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
		CRegistUserSession* pSession = m_mapRegistUserSession.Erase(nEsSessionID);

		if (pSession)
		{
			this->CancelDscTimer(pSession);
			OnRelease(pSession);
		}

		VBH::CCryptRegistUserEsCltRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nCltSessionID = rRegistUserReq.m_nCltSessionID;

		if (this->SendHtsMsg(rsp, pMcpHandler))
		{
			DSC_RUN_LOG_ERROR("send hts message:CCryptRegistUserEsCltRsp failed.");
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CCryptRegistUserCltEsReq);

	return 0;
}

ACE_INT32 CCryptEndorserService::OnHtsMsg(VBH::CCryptSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CCryptSubmitRegistUserTransactionCltEsReq);

	//1. 查找session
	CRegistUserSession* pSession = m_mapRegistUserSession.Find(rSubmitRegistUserReq.m_nEsSessionID);

	if (pSession)
	{
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
		int nDecryptBufLen;
		char* pDecryptBuf = VBH::vbhDecrypt((unsigned char*)m_peerEnvelopeKey.data(), nDecryptBufLen, rSubmitRegistUserReq.m_userData.GetBuffer(), rSubmitRegistUserReq.m_userData.GetSize());

		if (DSC_LIKELY(pDecryptBuf))
		{
			DSC::CDscShortBlob cltPubKey; // 客户端公钥
			ACE_INT32 nNonce;
			VBH::CCryptSubmitRegistUserTransactionCltEsReqDataWrapper wrapper(cltPubKey, nNonce);

			//2. 解密，解码
			if (DSC::Decode(wrapper, pDecryptBuf, nDecryptBufLen))
			{
				DSC_RUN_LOG_ERROR("decode error.");
				nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
			}
			else
			{
				//3. 验证nonce
				if (pSession->m_nNonce == nNonce)
				{
					CDscString strSvrPriKey; //服务端私钥
					CDscString strEnvelopeKey; //对称秘钥 //客户端注册时提交存放在服务端的秘钥，用于客户端和服务端加密通信

					if (GenerateRegistCryptKey(pSession->m_strSvrPubKey, strSvrPriKey, strEnvelopeKey, cltPubKey))
					{
						DSC_RUN_LOG_ERROR("GenerateRegistCryptKey failed.");
						nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
					}
					else
					{
						VBH::CSubmitRegistUserTransactionEsTasReq req;

						//4. 把未打包事务内容发送到 order(经tas转发)
						req.m_nEsSessionID = pSession->m_nEsSessionID;
						req.m_nChannelID = pSession->m_nChannelID;
						VBH::Assign(req.m_userInfo, pSession->m_ccGenerateUserInfo);
						VBH::Assign(req.m_svrPriKey, strSvrPriKey);
						VBH::Assign(req.m_envelopeKey, strEnvelopeKey);
						req.m_cltPubKey = cltPubKey;						

						if (m_pTas->SendSubmitRegistUserTransactionEsTasReq(req))
						{
							DSC_RUN_LOG_ERROR("send submit regist user msg to tas error.");
							nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
						}
					}
				}
				else
				{
					DSC_RUN_LOG_ERROR("nonce verify error.");
					nReturnCode = VBH::EN_NONCE_VERIFY_ERROR_TYPE;
				}
			}

			DSC_THREAD_FREE(pDecryptBuf);
		}
		else
		{
			DSC_RUN_LOG_ERROR("decrypt error.");
			nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
		}

		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			VBH::CCryptSubmitRegistUserTransactionEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pSession->m_nCltSessionID;
			if (this->SendHtsMsg(rsp, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CCryptSubmitRegistUserTransactionEsCltRsp failed.");
				ACE_ASSERT(m_mapRegistUserSession.Find(rSubmitRegistUserReq.m_nEsSessionID) == nullptr);
			}
			else
			{
				//发送hts消息失败时，会导致session被删除，这里要做保护
				pSession = m_mapRegistUserSession.Erase(rSubmitRegistUserReq.m_nEsSessionID);
				ACE_ASSERT(pSession != nullptr);
				this->CancelDscTimer(pSession);
				this->OnRelease(pSession); //释放session
			}
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rSubmitRegistUserReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CCryptSubmitRegistUserTransactionCltEsReq);

	return 0;
}


ACE_INT32 CCryptEndorserService::OnHtsMsg(VBH::CCryptProposeCltEsReq& rProposeReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CCryptProposeCltEsReq);

	CCryptKeyPtrAsMapValue pCryptKey = GetCryptKeyFromCache(rProposeReq.m_nChannelID, rProposeReq.m_userKey.m_nAllocatedID);

	if (pCryptKey)
	{
		CProposeSession* pSession = DSC_THREAD_TYPE_NEW(CProposeSession) CProposeSession(*this);
		const ACE_UINT32 nEsSessionID = AllocSessionID();

		pSession->m_nActionID = rProposeReq.m_nActionID;
		pSession->m_nEsSessionID = nEsSessionID;
		pSession->m_nCltSessionID = rProposeReq.m_nCltSessionID;
		pSession->m_nChannelID = rProposeReq.m_nChannelID;
		pSession->m_userKey = rProposeReq.m_userKey;
		VBH::Assign(pSession->m_userData, rProposeReq.m_userData);

		this->SetMcpHandleSession(pSession, pMcpHandler);
		this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
		m_mapProposeSession.DirectInsert(nEsSessionID, pSession);

		ACE_INT32 nReturnCode = OnRecvCryptKey4Propose(pSession,
			pCryptKey->m_strCltPubKey.data(), pCryptKey->m_strCltPubKey.size(),
			pCryptKey->m_strEnvelopeKey.data(), pCryptKey->m_strEnvelopeKey.size());

		//单独发送处理错误应答
		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
			pSession = m_mapProposeSession.Find(nEsSessionID);
			if (pSession)
			{
				VBH::CCryptProposeEsCltRsp rsp;

				rsp.m_nReturnCode = nReturnCode;
				rsp.m_nCltSessionID = pSession->m_nCltSessionID;

				if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CCryptProposeEsCltRsp failed.");
					ACE_ASSERT(m_mapProposeSession.Find(nEsSessionID) == nullptr);
				}
				else
				{
					pSession = m_mapProposeSession.Erase(nEsSessionID);
					ACE_ASSERT(pSession != nullptr);
					this->CancelDscTimer(pSession);
					this->OnRelease(pSession);
				}
			}
		}
	}
	else
	{
		CDscMsg::CDscMsgAddr addr;
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

		if (m_csRouter.GetCsAddr(addr, rProposeReq.m_nChannelID))
		{
			DSC_RUN_LOG_ERROR("cannot find channel's xcs addr, channel-id:%u.", rProposeReq.m_nChannelID);
			nReturnCode = VBH::EN_CANNOT_FOUND_CHANNEL_COMMITTER_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryCryptKeyProposeEsCsReq req;
			const ACE_UINT32 nEsSessionID = AllocSessionID();

			req.m_nEsSessionID = nEsSessionID;
			req.m_transUrl = rProposeReq.m_transUrl;

			if (this->SendDscMessage(req, addr))
			{
				DSC_RUN_LOG_ERROR("send dsc message failed, dst-addr(node-id:service-id): [%d,%d]", addr.GetNodeID(), addr.GetServiceID());
				nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
			}
			else
			{
				CProposeSession* pSession = DSC_THREAD_TYPE_NEW(CProposeSession) CProposeSession(*this);

				pSession->m_nEsSessionID = nEsSessionID;
				pSession->m_nCltSessionID = rProposeReq.m_nCltSessionID;
				pSession->m_nChannelID = rProposeReq.m_nChannelID;
				pSession->m_userKey = rProposeReq.m_userKey;
				VBH::Assign(pSession->m_userData, rProposeReq.m_userData);

				this->SetMcpHandleSession(pSession, pMcpHandler);
				this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
				m_mapProposeSession.DirectInsert(nEsSessionID, pSession);
			}
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CCryptProposeCltEsReq);

	return 0;
}

ACE_INT32 CCryptEndorserService::OnHtsMsg(VBH::CCryptSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CCryptSubmitProposalTransactionCltEsReq);

	CProposeSession* pProposeSession = m_mapProposeSession.Find(rSubmitTransactionReq.m_nEsSessionID);

	if (DSC_LIKELY(pProposeSession))
	{
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

		int nDecryptBufLen;
		char* pDecryptBuf = VBH::vbhDecrypt((unsigned char*)pProposeSession->m_strEnvelopeKey.data(), nDecryptBufLen,
			rSubmitTransactionReq.m_userData.GetBuffer(), rSubmitTransactionReq.m_userData.GetSize());

		if (DSC_LIKELY(pDecryptBuf))
		{
			ACE_INT32 nNonce = *((ACE_INT32*)pDecryptBuf);
			DSC::DscNtohx(nNonce);

			if (pProposeSession->m_nNonce == nNonce)
			{
#ifdef VBH_USE_SIGNATURE
				if (VBH::vbhVerifySign(pProposeSession->m_strCltPubKey.data(), pProposeSession->m_strCltPubKey.size(), m_pGroup,
					pProposeSession->m_strProposal.data(), pProposeSession->m_strProposal.size(), pProposeSession->m_strSignature.data(), pProposeSession->m_strSignature.size()))
				{
#endif

					VBH::CSubmitProposalTransactionEsTasReq req;

					req.m_nActionID = rSubmitTransactionReq.m_nActionID;
					req.m_nEsSessionID = pProposeSession->m_nEsSessionID;
					req.m_nChannelID = pProposeSession->m_nChannelID;
					VBH::Assign(req.m_transContent, pProposeSession->m_strTransContent);

					if (m_pTas->SendSubmitProposalTransactionEsTasReq(req))
					{
						DSC_RUN_LOG_ERROR("send SubmitProposalTransaction failed.");
						nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
					}
#ifdef VBH_USE_SIGNATURE				
				}
				else
				{
					DSC_RUN_LOG_ERROR("verify signature failed.");
					nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
				}
#endif
			}
			else
			{
				DSC_RUN_LOG_ERROR("nonce verify error.");
				nReturnCode = VBH::EN_NONCE_VERIFY_ERROR_TYPE;
			}

			DSC_THREAD_FREE(pDecryptBuf);
		}
		else
		{
			DSC_RUN_LOG_ERROR("decrypt error.");
			nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
		}

		if (nReturnCode != VBH::EN_OK_TYPE) //出现错误
		{
			VBH::CCryptSubmitProposalTransactionEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pProposeSession->m_nCltSessionID;
			if (this->SendHtsMsg(rsp, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CCryptSubmitProposalTransactionEsCltRsp failed.");
				ACE_ASSERT(m_mapProposeSession.Find(rSubmitTransactionReq.m_nEsSessionID) == nullptr);
			}
			else
			{
				pProposeSession = m_mapProposeSession.Erase(rSubmitTransactionReq.m_nEsSessionID);
				ACE_ASSERT(pProposeSession != nullptr);
				this->CancelDscTimer(pProposeSession);
				this->OnRelease(pProposeSession);
			}
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rSubmitTransactionReq.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CCryptSubmitProposalTransactionCltEsReq);

	return 0;
}

ACE_INT32 CCryptEndorserService::OnHtsMsg(VBH::CCryptQueryCltEsReq& rQueryReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CCryptQueryCltEsReq);

	CCryptKeyPtrAsMapValue pCryptKey = GetCryptKeyFromCache(rQueryReq.m_nChannelID, rQueryReq.m_userKey.m_nAllocatedID);

	if (pCryptKey)
	{
		const ACE_UINT32 nEsSessionID = AllocSessionID();
		CQuerySession* pSession = DSC_THREAD_TYPE_NEW(CQuerySession) CQuerySession(*this);

		VBH::Assign(pSession->m_userData, rQueryReq.m_userData); //记录加密数据
		pSession->m_nEsSessionID = nEsSessionID;
		pSession->m_nCltSessionID = rQueryReq.m_nCltSessionID;
		pSession->m_nChannelID = rQueryReq.m_nChannelID;
		pSession->m_userKey = rQueryReq.m_userKey;

		this->SetMcpHandleSession(pSession, pMcpHandler); //设置session和mcp-handler的勾连关系
		this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
		m_mapQuerySession.DirectInsert(nEsSessionID, pSession);

		ACE_INT32 nReturnCode = OnRecvCryptKey4Query(pSession, pCryptKey->m_strEnvelopeKey.data(), pCryptKey->m_strEnvelopeKey.size());
		
		//单独发送处理错误应答
		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
			pSession = m_mapQuerySession.Find(nEsSessionID);
			if (pSession)
			{
				VBH::CCryptQueryEsCltRsp rsp;

				rsp.m_nReturnCode = nReturnCode;
				rsp.m_nCltSessionID = pSession->m_nCltSessionID;

				if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CCryptProposeEsCltRsp failed.");
					ACE_ASSERT(m_mapQuerySession.Find(nEsSessionID) == nullptr);
				}
				else
				{
					pSession = m_mapQuerySession.Erase(nEsSessionID);
					ACE_ASSERT(pSession != nullptr);
					this->CancelDscTimer(pSession);
					this->OnRelease(pSession);
				}
			}
		}
	}
	else
	{
		ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
		CDscMsg::CDscMsgAddr addr;

		if (m_csRouter.GetCsAddr(addr, rQueryReq.m_nChannelID))
		{
			DSC_RUN_LOG_ERROR("can not find xcs addr. channel-id:%d", rQueryReq.m_nChannelID);
			nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryCryptKeyQueryEsCsReq req;
			const ACE_UINT32 nEsSessionID = AllocSessionID();

			req.m_nEsSessionID = nEsSessionID;
			req.m_transUrl = rQueryReq.m_transUrl;

			if (this->SendDscMessage(req, addr))
			{
				DSC_RUN_LOG_ERROR("network error, send dsc message failed, dst-addr(node-id:service-id):[%d:%d].", addr.GetNodeID(), addr.GetServiceID());
				nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
			}
			else
			{
				CQuerySession* pSession = DSC_THREAD_TYPE_NEW(CQuerySession) CQuerySession(*this);

				VBH::Assign(pSession->m_userData, rQueryReq.m_userData); //记录加密数据
				pSession->m_nEsSessionID = nEsSessionID;
				pSession->m_nCltSessionID = rQueryReq.m_nCltSessionID;
				pSession->m_nChannelID = rQueryReq.m_nChannelID;
				pSession->m_userKey = rQueryReq.m_userKey;
				this->SetMcpHandleSession(pSession, pMcpHandler); //设置session和mcp-handler的勾连关系

				SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
				m_mapQuerySession.DirectInsert(nEsSessionID, pSession);
			}
		}

		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			VBH::CCryptQueryEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = rQueryReq.m_nCltSessionID;

			if (this->SendHtsMsg(rsp, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CCryptQueryEsCltRsp failed.");
			}
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CCryptQueryCltEsReq);

	return 0;
}

void CCryptEndorserService::OnDscMsg(VBH::CRegistUserCcEsRsp& rRegistUserRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendRegistUserCcEsRsp(rRegistUserRsp);
}


void CCryptEndorserService::OnDscMsg(VBH::CQueryCryptKeyProposeCsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CQueryCryptKeyProposeCsEsRsp);

	CProposeSession* pSession = m_mapProposeSession.Find(rQueryCryptKeyRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		ACE_INT32 nReturnCode = rQueryCryptKeyRsp.m_nReturnCode;

		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			InsertCryptKeyIntoCache(pSession->m_nChannelID, pSession->m_userKey.m_nAllocatedID, rQueryCryptKeyRsp.m_cltPubKey, rQueryCryptKeyRsp.m_envelopeKey);

			nReturnCode = OnRecvCryptKey4Propose(pSession,
				rQueryCryptKeyRsp.m_cltPubKey.GetBuffer(), rQueryCryptKeyRsp.m_cltPubKey.GetSize(),
				rQueryCryptKeyRsp.m_envelopeKey.GetBuffer(), rQueryCryptKeyRsp.m_envelopeKey.GetSize());
		}

		//单独发送处理错误应答
		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			//在使用Inner-CC时：SendRegistUserEsCcReq函数调用在函数返回之前，就可能执行了应答处理逻辑，在应答逻辑中可能会删除已经插入的session，所以，这里必须查找一次
			pSession = m_mapProposeSession.Find(rQueryCryptKeyRsp.m_nEsSessionID);
			if (pSession)
			{
				VBH::CCryptProposeEsCltRsp rsp;

				rsp.m_nReturnCode = nReturnCode;
				rsp.m_nCltSessionID = pSession->m_nCltSessionID;

				if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CCryptProposeEsCltRsp failed.");
					ACE_ASSERT(m_mapProposeSession.Find(rQueryCryptKeyRsp.m_nEsSessionID) == nullptr);
				}
				else
				{
					pSession = m_mapProposeSession.Erase(rQueryCryptKeyRsp.m_nEsSessionID);
					ACE_ASSERT(pSession != nullptr);
					this->CancelDscTimer(pSession);
					this->OnRelease(pSession);
				}
			}
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session id:%d", rQueryCryptKeyRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CQueryCryptKeyProposeCsEsRsp);
}

void CCryptEndorserService::OnDscMsg(VBH::CProposeCcEsRsp& rProposeRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendProposeCcEsRsp(rProposeRsp);
}

void CCryptEndorserService::OnDscMsg(VBH::CQueryCryptKeyQueryCsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CQueryCryptKeyQueryCsEsRsp);

	CQuerySession* pSession = m_mapQuerySession.Find(rQueryCryptKeyRsp.m_nEsSessionID);

	if (pSession)
	{
		ACE_INT32 nReturnCode = rQueryCryptKeyRsp.m_nReturnCode;

		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			InsertCryptKeyIntoCache(pSession->m_nChannelID, pSession->m_userKey.m_nAllocatedID, rQueryCryptKeyRsp.m_cltPubKey, rQueryCryptKeyRsp.m_envelopeKey);

			nReturnCode = OnRecvCryptKey4Query(pSession, rQueryCryptKeyRsp.m_envelopeKey.GetBuffer(), rQueryCryptKeyRsp.m_envelopeKey.GetSize());
		}
		else
		{
			DSC_RUN_LOG_ERROR("CQueryCryptKeyQueryCsEsRsp return error:%d", rQueryCryptKeyRsp.m_nReturnCode);
		}

		if (nReturnCode != VBH::EN_OK_TYPE)
		{
			VBH::CCryptQueryEsCltRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nCltSessionID = pSession->m_nCltSessionID;

			if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CCryptQueryTransInfoEsCltRsp failed.");
				ACE_ASSERT(m_mapQuerySession.Find(rQueryCryptKeyRsp.m_nEsSessionID) == nullptr);
			}
			else
			{
				//发送hts消息失败时，会导致session被删除
				pSession = m_mapQuerySession.Erase(rQueryCryptKeyRsp.m_nEsSessionID);
				ACE_ASSERT(pSession != nullptr);
				this->CancelDscTimer(pSession);
				this->OnRelease(pSession);
			}
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("can not find session, session-id:%d", rQueryCryptKeyRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CQueryCryptKeyQueryCsEsRsp);
}

void CCryptEndorserService::OnDscMsg(VBH::CQueryCcEsRsp& rQueryRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	this->SendQueryCcEsRsp(rQueryRsp);
}

void CCryptEndorserService::OnDscMsg(VBH::CLoadOuterCcUserCltEsReq& rLoadOuterReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH::CLoadOuterCcEsCcReq req;


	req.m_nChannelID = rLoadOuterReq.m_nChannelID;
	req.m_nSessionID = rLoadOuterReq.m_nSessionID;;
	req.m_nCcID = rLoadOuterReq.m_nCcID;;
	req.m_ccName = rLoadOuterReq.m_ccName;;
	ACE_INT32 nReturnCode;

	IEndorserCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)rLoadOuterReq.m_nChannelID];
	CLoadCcSession* pSession = DSC_THREAD_TYPE_NEW(CLoadCcSession) CLoadCcSession(*this);
	pSession->m_rSrcMsgAddr = rSrcMsgAddr;
	pSession->m_nSessionID = rLoadOuterReq.m_nSessionID;

	m_mapLoadCcSession.DirectInsert(rLoadOuterReq.m_nSessionID, pSession);

	if (pCcService)
	{
		nReturnCode = pCcService->SendLoadOuterCcEsCcReq(req);
	}
	else
	{
		CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, rLoadOuterReq.m_nChannelID, DSC::EN_INVALID_ID);

		nReturnCode = SendDscMessage(req, ccAddr);
	}

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CLoadOuterCcEsCltRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nCltSessionID = rLoadOuterReq.m_nSessionID;

		if (this->SendDscMessage(rsp, rSrcMsgAddr))
		{
			DSC_RUN_LOG_ERROR("send hts message:CLoadOuterCcEsCltRsp failed.");
		}
	}

}
void CCryptEndorserService::SendSubmitRegistUserTransactionTasEsRsp(VBH::CSubmitRegistUserTransactionTasEsRsp& rSubmitTransactionRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CSubmitRegistUserTransactionTasEsRsp);

	CRegistUserSession* pSession = m_mapRegistUserSession.Find(rSubmitTransactionRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		//1. 向客户端发送应答
		VBH::CCryptSubmitRegistUserTransactionEsCltRsp cltRsp;
		char* pEncryptBuf;

		cltRsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		cltRsp.m_nCltSessionID = pSession->m_nCltSessionID;

		if (cltRsp.m_nReturnCode == VBH::EN_OK_TYPE) //提交成功时，再编码加密待发送数据
		{
			DSC::CDscShortBlob svrPubKey(pSession->m_strSvrPubKey.data(), pSession->m_strSvrPubKey.size());
			VBH::CCryptSubmitRegistUserTransactionEsCltRspDataWrapper wrapper(svrPubKey, rSubmitTransactionRsp.m_userKey, rSubmitTransactionRsp.m_registTransUrl, pSession->m_nNonce);
			char* pMsgBuf = nullptr;
			size_t nMsgBufLen;

			DSC::Encode(wrapper, pMsgBuf, nMsgBufLen);

			int nEncryptBufLen;
			pEncryptBuf = VBH::vbhEncrypt((unsigned char*)m_peerEnvelopeKey.data(), nEncryptBufLen, pMsgBuf, nMsgBufLen);

			DSC_THREAD_SIZE_FREE(pMsgBuf, nMsgBufLen);

			if (pEncryptBuf)
			{
				cltRsp.m_userData.Set(pEncryptBuf, nEncryptBufLen);
			}
			else
			{
				DSC_RUN_LOG_ERROR("encrypt error.");
				cltRsp.m_nReturnCode = VBH::EN_ENCRYPT_ERROR_TYPE;
			}
		}
		else
		{
			pEncryptBuf = nullptr;
		}

		if (this->SendHtsMsg(cltRsp, pSession->m_pEndorserServiceHandler))
		{
			DSC_RUN_LOG_ERROR("send hts message:CCryptSubmitRegistUserTransactionEsCltRsp failed, clt-session-id:%d.", cltRsp.m_nCltSessionID);
			ACE_ASSERT(m_mapRegistUserSession.Find(rSubmitTransactionRsp.m_nEsSessionID) == nullptr);
		}
		else
		{
			//2. 如果提交成功 && 客户端等待应答，则向XCS发送订阅请求
			//SendHtsMsg()可能引发NetworkError，pSession被删除。
			ACE_ASSERT(m_mapRegistUserSession.Find(rSubmitTransactionRsp.m_nEsSessionID) != nullptr);
			m_mapRegistUserSession.Erase(pSession);
			//删除session
			this->CancelDscTimer(pSession);
			this->OnRelease(pSession);
		}

		if (pEncryptBuf)
		{
			DSC_THREAD_FREE(pEncryptBuf);
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rSubmitTransactionRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CSubmitRegistUserTransactionTasEsRsp);
}

void CCryptEndorserService::SendSubmitProposalTransactionTasEsRsp(VBH::CSubmitProposalTransactionTasEsRsp& rSubmitTransactionRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CVbhCryptClientService, VBH::CSubmitProposalTransactionTasEsRsp);

	CProposeSession* pSession = m_mapProposeSession.Find(rSubmitTransactionRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		//1. 向客户端发送应答
		VBH::CCryptSubmitProposalTransactionEsCltRsp rsp;
		char* pEncryptBuf;
		rsp.m_nActionID = rSubmitTransactionRsp.m_nActionID;
		rsp.m_nReturnCode = rSubmitTransactionRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pSession->m_nCltSessionID;
		rsp.m_vecInfoID = rSubmitTransactionRsp.m_vecInfoID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE) //提交成功时，再编码加密待发送数据
		{
			DSC::CDscShortBlob receipt(pSession->m_strCcReceipt.data(), pSession->m_strCcReceipt.size());
			VBH::CCryptSubmitProposalTransactionEsCltRspDataWrapper wrapper(rSubmitTransactionRsp.m_alocTransKey, receipt, pSession->m_nNonce);
			char* pMsgBuf = nullptr;
			size_t nMsgBufLen;

			DSC::Encode(wrapper, pMsgBuf, nMsgBufLen);

			int nEncryptBufLen;
			pEncryptBuf = VBH::vbhEncrypt((unsigned char*)pSession->m_strEnvelopeKey.data(), nEncryptBufLen, pMsgBuf, nMsgBufLen);

			DSC_THREAD_SIZE_FREE(pMsgBuf, nMsgBufLen);

			if (pEncryptBuf)
			{
				rsp.m_userData.Set(pEncryptBuf, nEncryptBufLen);
			}
			else
			{
				DSC_RUN_LOG_ERROR("encrypt error.");
				rsp.m_nReturnCode = VBH::EN_ENCRYPT_ERROR_TYPE;
			}
		}
		else
		{
			pEncryptBuf = nullptr;
		}

		if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
		{
			DSC_RUN_LOG_ERROR("send hts message:CCryptSubmitProposalTransactionEsCltRsp failed, clt-session-id:%d.", rsp.m_nCltSessionID);
			ACE_ASSERT(m_mapProposeSession.Find(rSubmitTransactionRsp.m_nEsSessionID) == nullptr);
		}
		else
		{
			//2. 如果提交成功 && 客户端等待应答，则向XCS发送订阅请求 //发送hts消息失败，会出发network-error,导致业务session被删除
			ACE_ASSERT(m_mapProposeSession.Find(rSubmitTransactionRsp.m_nEsSessionID) != nullptr);
			m_mapProposeSession.Erase(pSession);
			//删除session
			this->CancelDscTimer(pSession);
			this->OnRelease(pSession);
		}

		if (pEncryptBuf)
		{
			DSC_THREAD_FREE(pEncryptBuf);
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rSubmitTransactionRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CVbhCryptClientService, VBH::CSubmitProposalTransactionTasEsRsp);
}

void CCryptEndorserService::SendRegistUserCcEsRsp(VBH::CRegistUserCcEsRsp& rRegistUserRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CRegistUserCcEsRsp);

	//1.找session
	CRegistUserSession* pSession = m_mapRegistUserSession.Find(rRegistUserRsp.m_nEsSessionID);

	if (pSession)
	{
		char* pEncryptBuf;
		VBH::CCryptRegistUserEsCltRsp rsp;

		rsp.m_nReturnCode = rRegistUserRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pSession->m_nCltSessionID;
		rsp.m_nEsSessionID = pSession->m_nEsSessionID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			if (pSession->m_bSubmitNode) //如果是最终提交节点，则记录后续需要提交
			{
				VBH::Assign(pSession->m_ccGenerateUserInfo, rRegistUserRsp.m_userInitInfo);
			}

			VBH::CCryptRegistUserEsCltRspDataWrapper wrapper(rRegistUserRsp.m_userInitInfo, pSession->m_nNonce);
			char* pMsgBuf;
			size_t nMsgBufLen;
			int nEncryptBufLen;

			DSC::Encode(wrapper, pMsgBuf, nMsgBufLen);

			//2. 对要发送回的数据编码，加密
			pEncryptBuf = VBH::vbhEncrypt((unsigned char*)m_peerEnvelopeKey.data(), nEncryptBufLen, pMsgBuf, nMsgBufLen);
			DSC_THREAD_SIZE_FREE(pMsgBuf, nMsgBufLen);

			if (pEncryptBuf)
			{
				rsp.m_nReturnCode = VBH::EN_OK_TYPE;
				rsp.m_userData.Set(pEncryptBuf, nEncryptBufLen);
			}
			else
			{
				rsp.m_nReturnCode = VBH::EN_ENCRYPT_ERROR_TYPE;
			}
		}
		else
		{
			pEncryptBuf = nullptr;
		}

		//3. 发送的应答
		if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
		{
			DSC_RUN_LOG_ERROR("network error, send hts message:CCryptRegistUserEsCltRsp failed.");
			rsp.m_nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
			ACE_ASSERT(m_mapRegistUserSession.Find(rRegistUserRsp.m_nEsSessionID) == nullptr);
		}
		else
		{
			//5. 如果出错或者不是提交节点，则删除session 
			ACE_ASSERT(m_mapRegistUserSession.Find(rRegistUserRsp.m_nEsSessionID) != nullptr);
			if ((rsp.m_nReturnCode != VBH::EN_OK_TYPE) || !pSession->m_bSubmitNode)
			{
				this->CancelDscTimer(pSession);
				m_mapRegistUserSession.Erase(pSession);
				this->OnRelease(pSession);
			}
			else //是提交节点,且不出错,重置定时器
			{
				ResetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
			}
		}

		//4. 清除加密时申请的内存
		if (pEncryptBuf)
		{
			DSC_THREAD_FREE(pEncryptBuf);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", rRegistUserRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CRegistUserCcEsRsp);
}

void CCryptEndorserService::SendProposeCcEsRsp(VBH::CProposeCcEsRsp& rProposeRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CVbhCryptClientService, VBH::CProposeCcEsRsp);

	//1. 找到session
	CProposeSession* pSession = m_mapProposeSession.Find(rProposeRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		if (pSession->m_nSubmitNodeType == VBH::EN_ONLY_ONE_SUBMIT_NODE_TYPE) //是唯一提交节点
		{
			ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

#ifdef VBH_USE_SIGNATURE
			if (VBH::vbhVerifySign(pSession->m_strCltPubKey.data(), pSession->m_strCltPubKey.size(), m_pGroup,
				pSession->m_strProposal.data(), pSession->m_strProposal.size(), pSession->m_strSignature.data(), pSession->m_strSignature.size()))
			{
#endif
				VBH::Assign(pSession->m_strCcReceipt, rProposeRsp.m_receipt);

				VBH::CSubmitProposalTransactionEsTasReq req;

				req.m_nActionID = rProposeRsp.m_nActionID;
				req.m_nEsSessionID = pSession->m_nEsSessionID;
				req.m_nChannelID = pSession->m_nChannelID;
				req.m_transContent = rProposeRsp.m_transContent;

				if (m_pTas->SendSubmitProposalTransactionEsTasReq(req))
				{
					DSC_RUN_LOG_ERROR("send SubmitProposalTransaction failed.");
					nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
				}
#ifdef VBH_USE_SIGNATURE				
			}
			else
			{
				DSC_RUN_LOG_ERROR("verify signature failed.");
				nReturnCode = VBH::EN_SIGNATURE_VERIFY_ERROR_TYPE;
			}
#endif

			if (nReturnCode != VBH::EN_OK_TYPE) //出现错误
			{
				VBH::CCryptSubmitProposalTransactionEsCltRsp rsp;

				rsp.m_nReturnCode = nReturnCode;
				rsp.m_nCltSessionID = pSession->m_nCltSessionID;


				if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CCryptSubmitProposalTransactionEsCltRsp failed.");
					ACE_ASSERT(m_mapProposeSession.Find(rProposeRsp.m_nEsSessionID) == nullptr);
				}
				else
				{
					pSession = m_mapProposeSession.Erase(rProposeRsp.m_nEsSessionID);
					ACE_ASSERT(pSession != nullptr);
					this->CancelDscTimer(pSession);
					this->OnRelease(pSession);
				}
			}
		}
		else //不是唯一提交节点，则走传统回客户端背书，背书通过再提交流程
		{
			char* pEncryptBuf;
			VBH::CCryptProposeEsCltRsp rsp;

			rsp.m_nReturnCode = rProposeRsp.m_nReturnCode;
			rsp.m_nCltSessionID = pSession->m_nCltSessionID;
			rsp.m_nEsSessionID = pSession->m_nEsSessionID;

			if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
			{
				if (pSession->m_nSubmitNodeType == VBH::EN_IS_SUBMIT_NODE_TYPE) //如果是最终提交节点，则记录transaction content，后续需要提交
				{
					VBH::Assign(pSession->m_strTransContent, rProposeRsp.m_transContent);
					VBH::Assign(pSession->m_strCcReceipt, rProposeRsp.m_receipt);
				}

				VBH::CCryptSubmitProposalEsCltRspDataWrapper wrapper(rProposeRsp.m_transContent, pSession->m_nNonce);
				char* pMsgBuf;
				size_t nMsgBufLen;
				int nEncryptBufLen;

				DSC::Encode(wrapper, pMsgBuf, nMsgBufLen);

				//2. 对要发送回的数据编码，加密
				pEncryptBuf = VBH::vbhEncrypt((unsigned char*)pSession->m_strEnvelopeKey.data(), nEncryptBufLen, pMsgBuf, nMsgBufLen);
				DSC_THREAD_SIZE_FREE(pMsgBuf, nMsgBufLen);

				if (pEncryptBuf)
				{
					rsp.m_nReturnCode = VBH::EN_OK_TYPE;
					rsp.m_userData.Set(pEncryptBuf, nEncryptBufLen);
				}
				else
				{
					rsp.m_nReturnCode = VBH::EN_ENCRYPT_ERROR_TYPE;
				}
			}
			else
			{
				pEncryptBuf = nullptr;
				DSC_RUN_LOG_INFO("CProposeCcEsRsp return failed, error-code:%d, es-session-id:%d, client-session-id:%d, channel-id:%d",
					rProposeRsp.m_nReturnCode, pSession->m_nEsSessionID, pSession->m_nCltSessionID, pSession->m_nChannelID);
			}

			//3. 发送的应答
			if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
			{
				DSC_RUN_LOG_ERROR("network error, send hts message:CCryptProposeEsCltRsp failed.");
				rsp.m_nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
				ACE_ASSERT(m_mapProposeSession.Find(rProposeRsp.m_nEsSessionID) == nullptr);
			}
			else
			{
				ACE_ASSERT(m_mapProposeSession.Find(rProposeRsp.m_nEsSessionID) != nullptr);
				//4. 如果出错或者不是提交节点，则删除session
				if ((rsp.m_nReturnCode != VBH::EN_OK_TYPE) || (pSession->m_nSubmitNodeType == VBH::EN_NOT_SUBMIT_NODE_TYPE))
				{
					this->CancelDscTimer(pSession);
					m_mapProposeSession.Erase(pSession);
					this->OnRelease(pSession);
				}
				else //是提交节点,且不出错,重置定时器
				{
					ResetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
				}
			}

			//5. 清除加密时申请的内存
			if (pEncryptBuf)
			{
				DSC_THREAD_FREE(pEncryptBuf);
			}
		}
	}
	else
	{
		//找不到session
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rProposeRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CVbhCryptClientService, VBH::CProposeCcEsRsp);
}

void CCryptEndorserService::SendQueryCcEsRsp(VBH::CQueryCcEsRsp& rQueryRsp)
{
	VBH_MESSAGE_ENTER_TRACE(CCryptEndorserService, VBH::CQueryCcEsRsp);

	//1. 找到session
	CQuerySession* pSession = m_mapQuerySession.Find(rQueryRsp.m_nEsSessionID);

	if (DSC_LIKELY(pSession))
	{
		char* pEncryptBuf;
		VBH::CCryptQueryEsCltRsp rsp;

		rsp.m_nReturnCode = rQueryRsp.m_nReturnCode;
		rsp.m_nCltSessionID = pSession->m_nCltSessionID;

		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			VBH::CCryptQueryEsCltRspDataWrapper wrapper(rQueryRsp.m_info, pSession->m_nNonce);

			char* pMsgBuf;
			size_t nMsgBufLen;
			int nEncryptBufLen;

			DSC::Encode(wrapper, pMsgBuf, nMsgBufLen);

			//2. 对要发送回的数据编码，加密
			pEncryptBuf = VBH::vbhEncrypt((unsigned char*)pSession->m_strEnvelopeKey.data(), nEncryptBufLen, pMsgBuf, nMsgBufLen);
			DSC_THREAD_SIZE_FREE(pMsgBuf, nMsgBufLen);

			if (pEncryptBuf)
			{
				rsp.m_nReturnCode = VBH::EN_OK_TYPE;
				rsp.m_userData.Set(pEncryptBuf, nEncryptBufLen);
			}
			else
			{
				rsp.m_nReturnCode = VBH::EN_ENCRYPT_ERROR_TYPE;
			}
		}
		else
		{
			pEncryptBuf = nullptr;
		}

		//3. 发送的应答
		if (this->SendHtsMsg(rsp, pSession->m_pEndorserServiceHandler))
		{
			DSC_RUN_LOG_ERROR("network error, send hts message:CCryptExplorerQueryEsCltRsp failed.");
			rsp.m_nReturnCode = VBH::EN_NETWORK_ERROR_TYPE;
			ACE_ASSERT(m_mapQuerySession.Find(rQueryRsp.m_nEsSessionID) == nullptr);
		}
		else
		{
			ACE_ASSERT(m_mapQuerySession.Find(rQueryRsp.m_nEsSessionID) != nullptr);
			//4. 删除session
			this->CancelDscTimer(pSession);
			m_mapQuerySession.Erase(pSession);
			this->OnRelease(pSession);
		}

		//5. 清除加密时申请的内存
		if (pEncryptBuf)
		{
			DSC_THREAD_FREE(pEncryptBuf);
		}
	}
	else
	{
		//找不到session
		DSC_RUN_LOG_INFO("cann't find session, session id:%d", rQueryRsp.m_nEsSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCryptEndorserService, VBH::CQueryCcEsRsp);
}

void  CCryptEndorserService::SendLoadOuterCcCcEsRsp(VBH::CLoadOuterCcCcEsRsp& rLoadOuterCcRsp)
{

	CLoadCcSession* pSession = m_mapLoadCcSession.Find(rLoadOuterCcRsp.m_nEsSessionID);

	if (pSession)
	{
		VBH::CLoadOuterCcEsCltRsp rsp;
		rsp.m_nCltSessionID = rLoadOuterCcRsp.m_nEsSessionID;
		rsp.m_nReturnCode = rLoadOuterCcRsp.m_nReturnCode;
		if (this->SendDscMessage(rsp, pSession->m_rSrcMsgAddr))
		{
			DSC_RUN_LOG_ERROR("send dsc message:CLoadOuterCcEsCltRsp failed.");
		}

	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find Session.");
	}

}

void CCryptEndorserService::OnNetworkError(CMcpHandler* pMcpHandler)
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

CCryptEndorserService::CEndorserServiceHandler::CEndorserServiceHandler(CMcpServerService& rService, ACE_HANDLE handle, const ACE_INT32 nHandleID)
	: CMcpServerHandler(rService, handle, nHandleID)
{
}

ACE_INT32 CCryptEndorserService::OnRecvCryptKey4Propose(CProposeSession* pSession, const char* pCltPubKey, size_t nCltPubKeyLen, const char* pEnvelopeKey, size_t nEnvelopeKeyLen)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

	int nDecryptBufLen;
	char* pDecryptBuf = VBH::vbhDecrypt((unsigned char*)pEnvelopeKey, nDecryptBufLen, pSession->m_userData.data(), pSession->m_userData.size());

	if (DSC_LIKELY(pDecryptBuf))
	{
		ACE_UINT32 nActionID;
		DSC::CDscShortBlob signature;
		DSC::CDscShortBlob proposal;
		VBH::CCryptProposeCltEsReqDataWrapper wrapper(pSession->m_nSubmitNodeType, nActionID, signature, proposal, pSession->m_nNonce);

		if (DSC_UNLIKELY(DSC::Decode(wrapper, pDecryptBuf, nDecryptBufLen)))
		{
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
			DSC_RUN_LOG_WARNING("decode error.");
		}
		else
		{
			//将信息完善到session
			pSession->m_strEnvelopeKey.assign(pEnvelopeKey, nEnvelopeKeyLen);
			pSession->m_userData.clear(); //清除解密前的user-data，释放空间

			if ((pSession->m_nSubmitNodeType == VBH::EN_IS_SUBMIT_NODE_TYPE) || (pSession->m_nSubmitNodeType == VBH::EN_ONLY_ONE_SUBMIT_NODE_TYPE))
			{
				VBH::Assign(pSession->m_strSignature, signature);
				VBH::Assign(pSession->m_strProposal, proposal);
				pSession->m_strCltPubKey.assign(pCltPubKey, nCltPubKeyLen);
			}

			//准备发送到cc的请求，并发送到CC
			VBH::CProposeEsCcReq req;

			req.m_nEsSessionID = pSession->m_nEsSessionID;
			req.m_nActionID = nActionID;
			req.m_nChannelID = pSession->m_nChannelID;
			req.m_proposeUserKey = pSession->m_userKey;
			req.m_signature = signature;
			req.m_proposal = proposal;

			IEndorserCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)pSession->m_nChannelID];

			if (pCcService)
			{
				nReturnCode = pCcService->SendProposeEsCcReq(req);
			}
			else
			{
				CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, pSession->m_nChannelID, DSC::EN_INVALID_ID);

				nReturnCode = SendDscMessage(req, ccAddr);
			}

			if (nReturnCode != VBH::EN_OK_TYPE)
			{
				DSC_RUN_LOG_ERROR("send CCreateInformationEsCcReq message receive error.");
			}
		}

		DSC_THREAD_FREE(pDecryptBuf);
	}
	else
	{
		nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
		DSC_RUN_LOG_WARNING("decrypt error, es-sssion-id:%d, clt-session-id:%d, user-aloc-key:%#llX.", pSession->m_nEsSessionID, pSession->m_nCltSessionID, pSession->m_userKey.m_nAllocatedID);
	}

	return nReturnCode;
}

ACE_INT32 CCryptEndorserService::OnRecvCryptKey4Query(CQuerySession* pSession, const char* pEnvelopeKey, size_t nEnvelopeKeyLen)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

	int nDecryptBufLen;
	char* pDecryptBuf = VBH::vbhDecrypt((unsigned char*)pEnvelopeKey, nDecryptBufLen, pSession->m_userData.data(), pSession->m_userData.size());

	if (DSC_LIKELY(pDecryptBuf))
	{
		ACE_UINT32 nActionID;
		DSC::CDscShortBlob param;
		VBH::CCryptQueryCltEsReqDataWrapper wrapper(pSession->m_nNonce, nActionID, param);

		if (DSC_UNLIKELY(DSC::Decode(wrapper, pDecryptBuf, nDecryptBufLen)))
		{
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
			DSC_RUN_LOG_WARNING("decode error.");
		}
		else
		{
			//将信息完善到session
			pSession->m_strEnvelopeKey.assign(pEnvelopeKey, nEnvelopeKeyLen);
			pSession->m_userData.clear(); //清除解密前的user-data，释放空间

			//准备发送到cc的请求，并发送到CC
			VBH::CQueryEsCcReq req;

			req.m_nEsSessionID = pSession->m_nEsSessionID;
			req.m_nActionID = nActionID;
			req.m_nChannelID = pSession->m_nChannelID;
			req.m_param = param;

			IEndorserCcService* pCcService = m_arrHashInnerCcService[(ACE_UINT16)pSession->m_nChannelID];

			if (pCcService)
			{
				nReturnCode = pCcService->SendQueryEsCcReq(req);
			}
			else
			{
				CDscMsg::CDscMsgAddr ccAddr(VBH::EN_CC_APP_TYPE, DSC::EN_INVALID_ID, pSession->m_nChannelID, DSC::EN_INVALID_ID);

				nReturnCode = SendDscMessage(req, ccAddr);
			}

			if (nReturnCode != VBH::EN_OK_TYPE)
			{
				DSC_RUN_LOG_ERROR("send CCreateInformationEsCcReq message receive error.");
			}
		}

		DSC_THREAD_FREE(pDecryptBuf);
	}
	else
	{
		nReturnCode = VBH::EN_DECRYPT_ERROR_TYPE;
		DSC_RUN_LOG_WARNING("decrypt error, es-sssion-id:%d, clt-session-id:%d, user-aloc-key:%#llX.", pSession->m_nEsSessionID, pSession->m_nCltSessionID, pSession->m_userKey.m_nAllocatedID);
	}

	return nReturnCode;
}



void CCryptEndorserService::InsertCryptKeyIntoCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID, const DSC::CDscShortBlob& cltPubKey, const DSC::CDscShortBlob& envelopeKey)
{
	CUserKeyAsMapKey userKey;
	CCryptKeyPtrAsMapValue pCryptKey;

	if (m_nCachedCryptKeyNum > EN_CRYPT_KEY_CACHE_SIZE)
	{
		pCryptKey = m_dqueueCryptKey.PopFront(); //最不活跃的
		m_mapUserCryptKey.erase(pCryptKey->m_mapKey);
	}
	else
	{
		pCryptKey = DSC_THREAD_TYPE_NEW(CCryptKey) CCryptKey;
	}

	userKey.m_nChannelID = nChannelID;
	userKey.m_nUserAllocatedID = nUserAllocatedID;

	pCryptKey->m_mapKey.m_nChannelID = nChannelID;
	pCryptKey->m_mapKey.m_nUserAllocatedID = nUserAllocatedID;
	
	VBH::Assign(pCryptKey->m_strCltPubKey, cltPubKey);
	VBH::Assign(pCryptKey->m_strEnvelopeKey, envelopeKey);

	m_mapUserCryptKey.insert(user_crypt_key_map_type::value_type(userKey, pCryptKey));
	m_dqueueCryptKey.PushBack(pCryptKey);
}

CCryptEndorserService::CCryptKeyPtrAsMapValue CCryptEndorserService::GetCryptKeyFromCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID)
{
	CUserKeyAsMapKey userKey;
	CCryptKeyPtrAsMapValue pCryptKey;

	userKey.m_nChannelID = nChannelID;
	userKey.m_nUserAllocatedID = nUserAllocatedID;

	auto cryptKeyIt = m_mapUserCryptKey.find(userKey);

	if (cryptKeyIt == m_mapUserCryptKey.end())
	{
		pCryptKey = nullptr;
	}
	else
	{
		pCryptKey = cryptKeyIt->second;

		m_dqueueCryptKey.Erase(pCryptKey);
		m_dqueueCryptKey.PushBack(pCryptKey); //更新其访问频率
	}

	return pCryptKey;
}

ACE_INT32 CCryptEndorserService::GenerateRegistCryptKey(CDscString& strSvrPubKey, CDscString& strSvrPrivKey, CDscString& strEnvelopeKey, DSC::CDscShortBlob& cltPubKey)
{
	ACE_INT32 nReturnCode = 0;
	EC_KEY* pServerEcKey = VBH::vbhCreateEcKey(m_pGroup);

	if (pServerEcKey)
	{
		int nServerPublicKeyLen;
		char* pServerPublicKey = VBH::vbhGetBinPublicKeyFromEcKey(nServerPublicKeyLen, pServerEcKey);
		if (pServerPublicKey)
		{
			strSvrPubKey.assign(pServerPublicKey, nServerPublicKeyLen);
			DSC_THREAD_SIZE_FREE(pServerPublicKey, nServerPublicKeyLen);
		}
		else
		{
			DSC_RUN_LOG_ERROR("vbhGetBinPublicKeyFromEcKey failed.");
			nReturnCode = -1;
		}

		int nServerPrivateKeyLen;
		char* pServerPrivateKey = VBH::vbhGetBinPrivateKeyFromEcKey(nServerPrivateKeyLen, pServerEcKey);
		if (pServerPrivateKey)
		{
			strSvrPrivKey.assign(pServerPrivateKey, nServerPrivateKeyLen);
			DSC_THREAD_SIZE_FREE(pServerPrivateKey, nServerPrivateKeyLen);
		}
		else
		{
			DSC_RUN_LOG_ERROR("vbhGetBinPrivateKeyFromEcKey failed.");
			nReturnCode = -1;
		}

		BIGNUM* bn = BN_bin2bn((unsigned char*)cltPubKey.GetBuffer(), cltPubKey.GetSize(), nullptr);
		if (bn)
		{
			EC_POINT* point = EC_POINT_bn2point(m_pGroup, bn, nullptr, nullptr);
			if (point)
			{
				char arrEnvelopeKey[VBH_ENVELOPE_KEY_LENGTH + 1]; //对称秘钥
				int nEnvelopeKeyLen = ECDH_compute_key(arrEnvelopeKey, sizeof(arrEnvelopeKey), point, pServerEcKey, nullptr);
				
				if (nEnvelopeKeyLen > 0)
				{
					strEnvelopeKey.assign(arrEnvelopeKey, nEnvelopeKeyLen);
				}
				else
				{
					char errMsgBuf[1024];

					ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
					DSC_RUN_LOG_ERROR("ECDH_compute_key failed. {error = %s}", errMsgBuf);
					nReturnCode = -1;
				}

				EC_POINT_free(point);
			}
			else
			{
				char errMsgBuf[1024];

				ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
				DSC_RUN_LOG_ERROR("EC_POINT_bn2point failed. {error = %s}", errMsgBuf);
				nReturnCode = -1;
			}

			BN_free(bn);
		}
		else
		{
			char errMsgBuf[1024];

			ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
			DSC_RUN_LOG_ERROR("BN_bin2bn failed. {error = %s}", errMsgBuf);
			nReturnCode = -1;
		}

		EC_KEY_free(pServerEcKey);
	}
	else
	{
		DSC_RUN_LOG_ERROR("create EC_KEY failed.");
		nReturnCode = -1;
	}

	return nReturnCode;
}
