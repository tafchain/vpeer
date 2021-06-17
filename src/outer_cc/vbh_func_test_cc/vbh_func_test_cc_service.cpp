#include "ace/Assert.h"

#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_error_code.h"

#include "vbh_func_test_cc/vbh_func_test_cc_service.h"

CVbhFuncTestCcService::CSession::CSession(CVbhFuncTestCcService& rVbhFuncTestCcService)
	: m_rCcService(rVbhFuncTestCcService)
{
}

void CVbhFuncTestCcService::CSession::OnTimer()
{
	m_rCcService.OnTimeOut(this);
}

CVbhFuncTestCcService::CProposeSession::CProposeSession(CVbhFuncTestCcService& rVbhFuncTestCcService)
	: m_rCcService(rVbhFuncTestCcService)
{
}

void CVbhFuncTestCcService::CProposeSession::OnTimer()
{
	m_rCcService.OnTimeOut(this);
}

CVbhFuncTestCcService::CProposeInfoSession::CProposeInfoSession(CVbhFuncTestCcService& rVbhFuncTestCcService)
	: m_rCcService(rVbhFuncTestCcService)
{
}

void CVbhFuncTestCcService::CProposeInfoSession::OnTimer()
{
	m_rCcService.OnTimeOut(this);
}

CVbhFuncTestCcService::CVbhFuncTestCcService(ACE_UINT32 nChannelID)

{
}

ACE_INT32 CVbhFuncTestCcService::OnInit(void)
{

	DSC_RUN_LOG_INFO("vbh func test cc service init succeed!");

	return 0;
}

ACE_INT32 CVbhFuncTestCcService::OnExit(void)
{
	CSession* pSession;

	for (auto it = m_mapSession.begin(); it != m_mapSession.end();)
	{
		pSession = it.second;
		this->CancelDscTimer(pSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pSession);
	}

	CProposeSession* pTradeSession;

	for (auto it = m_mapTradeSession.begin(); it != m_mapTradeSession.end();)
	{
		pTradeSession = it.second;
		this->CancelDscTimer(pTradeSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pTradeSession);
	}

	CProposeInfoSession* pInfoSession;

	for (auto it = m_mapInfoSession.begin(); it != m_mapInfoSession.end();)
	{
		pInfoSession = it.second;
		this->CancelDscTimer(pInfoSession);
		++it;
		DSC_THREAD_TYPE_DELETE(pInfoSession);
	}

	return 0;
}

ACE_INT32 CVbhFuncTestCcService::RegistUserProc(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& userInfo)
{
	TEST_CC::CCcCheckInfoOnUserRegist checkInfo;

	if (DSC::Decode(checkInfo, userInfo.GetBuffer(), userInfo.GetSize()))
	{
		DSC_RUN_LOG_ERROR("decode CCcCheckInfoOnUserRegist failed.");

		m_pCommCcService->RegistUserRsp(VBH::EN_DECODE_ERROR_TYPE, nCcSessionID, nullptr, 0);
	}
	else
	{
		TEST_CC::CUserInfo userInfo;
		char* pBuf;
		size_t nBufLen;

		userInfo.m_nAsset = checkInfo.m_nAsset;
		DEF_ENCODE(userInfo, pBuf, nBufLen);

		m_pCommCcService->RegistUserRsp(VBH::EN_OK_TYPE, nCcSessionID, pBuf, nBufLen);
	}

	return 0;
}


ACE_INT32 CVbhFuncTestCcService::OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CAlterInformationAction& rAlterTestUserInfoAction)
{
	if (m_pCommCcService->VerifyProposeUser(nSessionID, rAlterTestUserInfoAction.m_userKey))
	{
		DSC_RUN_LOG_ERROR("update from user is the person who submit proposer.");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	if (m_pCommCcService->GetUserAtPropose(nSessionID, rAlterTestUserInfoAction.m_userKey)) //在OnPorposal中，遇到失败，必须返回错误码，并
	{
		DSC_RUN_LOG_ERROR("get vbh user failed.");
		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}
	else
	{
		CSession* pSession = DSC_THREAD_TYPE_NEW(CSession) CSession(*this);

		pSession->m_nCcSessionID = nSessionID;
		pSession->m_nAsset = rAlterTestUserInfoAction.m_nAsset;
		pSession->m_nPhoneNo = rAlterTestUserInfoAction.m_nPhoneNo;
		VBH::Assign(pSession->m_address, rAlterTestUserInfoAction.m_address);
		VBH::Assign(pSession->m_userName, rAlterTestUserInfoAction.m_userName);
		VBH::Assign(pSession->m_userKey, rAlterTestUserInfoAction.m_userKey);

		this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
		m_mapSession.Insert(nSessionID, pSession);

	}

	return 0;
}

ACE_INT32 CVbhFuncTestCcService::OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CCreateInformationAction& rCreateInformationAction)
{
	ACE_UINT16 idx = 0;
	ACE_UINT16 nVecUserCount = rCreateInformationAction.m_lstInitInfo.size();

	CProposeSimpleWriteSet *pVecUser;

	DSC_THREAD_TYPE_ALLOCATE_ARRAY(pVecUser, nVecUserCount);

	for (auto& it : rCreateInformationAction.m_lstInitInfo)
	{
		pVecUser[idx].m_value = it;
		++idx;
	}
	m_pCommCcService->ProposalRsp(2, nSessionID, pVecUser, idx);

	DSC_THREAD_TYPE_DEALLOCATE_ARRAY(pVecUser, nVecUserCount);

	return 0;
}

ACE_INT32 CVbhFuncTestCcService::OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CCommitInformationAction& rAction)
{

	DSC::CDscShortBlob vecUser[1];
	vecUser[0] = rAction.m_InformationKey;
	if (m_pCommCcService->GetInformationAtPropose(nSessionID, vecUser, 1))//在OnPorposal中，遇到失败，必须返回错误码，并
	{
		DSC_RUN_LOG_WARNING("get vbh user failed.");
		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}
	else
	{
		CProposeInfoSession* pSession = DSC_THREAD_TYPE_NEW(CProposeInfoSession) CProposeInfoSession(*this);

		pSession->m_nCcSessionID = nSessionID;
		pSession->m_key.Set(rAction.m_InformationKey.GetBuffer(), rAction.m_InformationKey.GetSize());
		pSession->m_value.Set(rAction.m_InformationValue.GetBuffer(), rAction.m_InformationValue.GetSize());


		this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
		m_mapInfoSession.Insert(nSessionID, pSession);
	}
	return 0;
}

ACE_INT32 CVbhFuncTestCcService::OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CTradeAction& rTradeInfo)
{
	//交易合法性检查
	if (!rTradeInfo.m_nAsset)
	{
		DSC_RUN_LOG_INFO("btc cann't be zero.");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	if (rTradeInfo.m_fromUserKey == rTradeInfo.m_toUserKey)
	{
		DSC_RUN_LOG_INFO("trade from user cann't equeal to user.");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	if (m_pCommCcService->VerifyProposeUser(nSessionID, rTradeInfo.m_fromUserKey))
	{
		DSC_RUN_LOG_INFO("trade from user is the person who submit proposer.");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	DSC::CDscShortBlob vecUser[2];

	vecUser[0] = rTradeInfo.m_fromUserKey;
	vecUser[1] = rTradeInfo.m_toUserKey;

	if (m_pCommCcService->GetUserAtPropose(nSessionID, vecUser, 2))//在OnPorposal中，遇到失败，必须返回错误码，并
	{
		DSC_RUN_LOG_WARNING("get vbh user failed.");
		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}
	else
	{
		CProposeSession* pSession = DSC_THREAD_TYPE_NEW(CProposeSession) CProposeSession(*this);

		pSession->m_nCcSessionID = nSessionID;
		pSession->m_nAsset = rTradeInfo.m_nAsset;
		VBH::Assign(pSession->m_fromUser, rTradeInfo.m_fromUserKey);
		VBH::Assign(pSession->m_toUser, rTradeInfo.m_toUserKey);

		this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
		m_mapTradeSession.Insert(nSessionID, pSession);
	}

	return 0;
}

void CVbhFuncTestCcService::OnTimeOut(CSession* pUserSession)
{
	DSC_RUN_LOG_INFO("cc timeout, session id:%d", pUserSession->m_nCcSessionID);
	m_mapSession.Erase(pUserSession);
	DSC_THREAD_TYPE_DEALLOCATE(pUserSession);
}

void CVbhFuncTestCcService::OnTimeOut(CProposeSession* pTradeSession)
{
	DSC_RUN_LOG_INFO("cc timeout, session id:%d", pTradeSession->m_nCcSessionID);
	m_mapTradeSession.Erase(pTradeSession);
	DSC_THREAD_TYPE_DEALLOCATE(pTradeSession);
}
void CVbhFuncTestCcService::OnTimeOut(CProposeInfoSession* pInfoSession)
{
	DSC_RUN_LOG_INFO("cc timeout, session id:%d", pInfoSession->m_nCcSessionID);
	m_mapInfoSession.Erase(pInfoSession);
	DSC_THREAD_TYPE_DEALLOCATE(pInfoSession);
}

ACE_INT32 CVbhFuncTestCcService::OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryUserAction& action)
{
	//如果信息只允许自己查询，可以调用 VerifyQueryUser 进行判断

	return m_pCommCcService->GetUserAtQuery(nSessionID, action.m_userID);
}

ACE_INT32 CVbhFuncTestCcService::OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryTransAction& action)
{
	return m_pCommCcService->GetTransAtQuery(nSessionID, action.m_transID);
}

ACE_INT32 CVbhFuncTestCcService::OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryInfoHistoryListAction& action)
{
    return m_pCommCcService->GetInformationHistoryAtQuery(nSessionID, action.m_informationID);
}
void CVbhFuncTestCcService::OnFormatProposal(CDscString& strFormatBuf, TEST_CC::CTradeAction& action)
{
	CDscString tmp;

	VBH::Assign(tmp, action.m_fromUserKey);
	strFormatBuf = "form:";
	strFormatBuf += tmp;
	strFormatBuf += "; ";

	VBH::Assign(tmp, action.m_toUserKey);
	strFormatBuf += "to:";
	strFormatBuf += tmp;
	strFormatBuf += "; ";

	strFormatBuf += "asset:";
	strFormatBuf += action.m_nAsset;
}

void CVbhFuncTestCcService::FormatUser(CDscString& strFormatBuf, DSC::CDscShortBlob& value)
{
	TEST_CC::CUserInfo userInfo;

	if (DSC::Decode(userInfo, value.GetBuffer(), value.GetSize()))
	{
		DSC_RUN_LOG_ERROR("decode user-info failed.");
	}
	else
	{
		strFormatBuf = "asset=";
		strFormatBuf += userInfo.m_nAsset;
	}
}

void CVbhFuncTestCcService::OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CProposeSimpleUser& rUser)
{
	CSession* pSession = m_mapSession.Find(nSessionID);

	if (pSession)
	{
		ACE_INT32 nTempReturnCode = nReturnCode;

		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			TEST_CC::CUserInfo userInfo;

			if (DSC::Decode(userInfo, rUser.m_value.GetBuffer(), rUser.m_value.GetSize()))
			{
				nTempReturnCode = VBH::EN_DECODE_ERROR_TYPE;
				DSC_RUN_LOG_ERROR("decode user-info failed, session-id:%d", nSessionID);
			}
			else
			{
				if (VBH::IsEqual(pSession->m_userKey, rUser.m_key))
				{
					CProposeSimpleWriteSet User[1];
					char* pUserInfoBuf;
					size_t nUserInfoLen;

					VBH::Assign(User->m_key, pSession->m_userKey);

					userInfo.m_nAsset = pSession->m_nAsset;
					DSC::Encode(userInfo, pUserInfoBuf, nUserInfoLen);
					User->m_value.Set(pUserInfoBuf, nUserInfoLen);

					m_pCommCcService->ProposalRsp(nTempReturnCode, nSessionID, User, 1);
					DSC_THREAD_SIZE_FREE(pUserInfoBuf, nUserInfoLen);
				}
				else
				{
					nTempReturnCode = VBH::EN_CC_COMMON_ERROR_TYPE;
					DSC_RUN_LOG_INFO("user key error, session-id:%d", nSessionID);
				}
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get user failed, session-id:%d", nSessionID);
		}

		if (nTempReturnCode != VBH::EN_OK_TYPE) //出错了，返回
		{
			m_pCommCcService->ProposalRsp(nTempReturnCode, nSessionID, nullptr, 0);
		}
		else
		{
			//没有出错，则在正确的路径上，完成处理后返回
		}

		this->CancelDscTimer(pSession);
		m_mapSession.Erase(nSessionID);
		DSC_THREAD_TYPE_DEALLOCATE(pSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find session:%d", nSessionID);
		m_pCommCcService->ProposalRsp(VBH::EN_CC_COMMON_ERROR_TYPE, nSessionID, nullptr, 0);
	}
}
void CVbhFuncTestCcService::OnGetInformationRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CProposeSimpleInformation& rInfo)
{
	CProposeInfoSession* pSession = m_mapInfoSession.Find(nSessionID);

	if (pSession)
	{
		ACE_INT32 nTempReturnCode = nReturnCode;

		if ((nReturnCode == VBH::EN_OK_TYPE)&& (pSession->m_key == rInfo.m_key))
		{
			CProposeSimpleWriteSet Info[1];
			Info->m_key.Set(pSession->m_key.GetBuffer(), pSession->m_key.GetSize());
			Info->m_value.Set(pSession->m_value.GetBuffer(), pSession->m_value.GetSize());

			m_pCommCcService->ProposalRsp(nTempReturnCode, nSessionID, Info, 1);

		}
		else
		{
		    nTempReturnCode = VBH::EN_CC_COMMON_ERROR_TYPE;
			DSC_RUN_LOG_ERROR("get user failed, session-id:%d", nSessionID);
			m_pCommCcService->ProposalRsp(nTempReturnCode, nSessionID, nullptr, 0);
		}

		this->CancelDscTimer(pSession);
		m_mapSession.Erase(nSessionID);
		DSC_THREAD_TYPE_DEALLOCATE(pSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find session:%d", nSessionID);
		m_pCommCcService->ProposalRsp(VBH::EN_CC_COMMON_ERROR_TYPE, nSessionID, nullptr, 0);
	}
}
void CVbhFuncTestCcService::OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, const CProposeSimpleUser* pUserVec, const ACE_UINT16 nVecLen)
{
	CProposeSession* pSession = m_mapTradeSession.Find(nSessionID);

	if (pSession)
	{
		if (nReturnCode == VBH::EN_OK_TYPE)
		{
			//返回肯定是2个，第一个是from-user，第二个是to-user //底层是完全可信想
			ACE_ASSERT(nVecLen == 2);
			//VBH::IsEqual(pSession->m_fromUser, pUserVec[0]) == true
			//VBH::IsEqual(pSession->m_toUser, pUserVec[1]) == true

			if (DSC::Decode(pSession->m_fromUserInfo, pUserVec[0].m_value.GetBuffer(), pUserVec[0].m_value.GetSize()))
			{
				this->CancelDscTimer(pSession);
				m_mapTradeSession.Erase(nSessionID);
				DSC_THREAD_TYPE_DEALLOCATE(pSession);
				m_pCommCcService->ProposalRsp(VBH::EN_DECODE_ERROR_TYPE, nSessionID, nullptr, 0);

				DSC_RUN_LOG_ERROR("decode from-user-info failed.");

				return;
			}

			if (DSC::Decode(pSession->m_toUserInfo, pUserVec[1].m_value.GetBuffer(), pUserVec[1].m_value.GetSize()))
			{
				this->CancelDscTimer(pSession);
				m_mapTradeSession.Erase(nSessionID);
				DSC_THREAD_TYPE_DEALLOCATE(pSession);
				m_pCommCcService->ProposalRsp(VBH::EN_DECODE_ERROR_TYPE, nSessionID, nullptr, 0);

				DSC_RUN_LOG_ERROR("decode to-user-info failed.");

				return;
			}

			if (pSession->m_fromUserInfo.m_nAsset >= pSession->m_nAsset) //满足交易条件，
			{
				char* pFromUserInfo;
				char* pToUserInfo;
				size_t nFromUserInfo;
				size_t nToUserInfo;

				CProposeSimpleWriteSet vecUser[2];

				pSession->m_fromUserInfo.m_nAsset -= pSession->m_nAsset;
				pSession->m_toUserInfo.m_nAsset += pSession->m_nAsset;

				DEF_ENCODE(pSession->m_fromUserInfo, pFromUserInfo, nFromUserInfo);
				DEF_ENCODE(pSession->m_toUserInfo, pToUserInfo, nToUserInfo);

				VBH::Assign(vecUser[0].m_key, pSession->m_fromUser);
				vecUser[0].m_value.Set(pFromUserInfo, nFromUserInfo);

				VBH::Assign(vecUser[1].m_key, pSession->m_toUser);
				vecUser[1].m_value.Set(pToUserInfo, nToUserInfo);

				this->CancelDscTimer(pSession);
				m_mapTradeSession.Erase(nSessionID);

				m_pCommCcService->ProposalRsp(nReturnCode, nSessionID, vecUser, 2);
				DSC_THREAD_TYPE_DEALLOCATE(pSession);
			}
			else
			{
				m_pCommCcService->ProposalRsp(VBH::EN_COMMON_ERROR_CODE_NUM, nSessionID, nullptr, 0);

				DSC_RUN_LOG_INFO("from user can not launch this transaction.");

				return;
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get user failed, session-id:%d, error-code:%d", nSessionID, nReturnCode);
			this->CancelDscTimer(pSession);
			m_mapTradeSession.Erase(nSessionID);

			m_pCommCcService->ProposalRsp(VBH::EN_CC_COMMON_ERROR_TYPE, nSessionID, nullptr, 0);
			DSC_THREAD_TYPE_DEALLOCATE(pSession);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find session:%d", nSessionID);
		m_pCommCcService->ProposalRsp(VBH::EN_CC_COMMON_ERROR_TYPE, nSessionID, nullptr, 0);
	}
}

void CVbhFuncTestCcService::OnGetUserRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CQuerySimpleUser& rUser)
{
	char* pRspBuf;
	size_t nRspLen;
	TEST_CC::CQueryUserRsp rsp;

	this->FormatUser(rsp.m_value, rUser.m_value);
	VBH::Assign(rsp.m_key, rUser.m_key);
	rsp.m_nVersion = rUser.m_nVersion;

	DSC::Encode(rsp, pRspBuf, nRspLen);

	DSC::CDscBlob blobInfo(pRspBuf, nRspLen);

	m_pCommCcService->QueryRsp(VBH::EN_OK_TYPE, nSessionID, blobInfo);

	blobInfo.FreeBuffer();
}

void CVbhFuncTestCcService::OnGetTransRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CQuerySimpleTransaction& rTrans)
{
	char* pRspBuf;
	size_t nRspLen;
	TEST_CC::CQueryTransRsp rsp;
	DSC::CDscBlob blobInfo;

	//这个cc里，所有的写集都是user
	if (nReturnCode == VBH::EN_OK_TYPE)
	{
		VBH::Assign(rsp.m_transKey, rTrans.m_transKey);
		VBH::Assign(rsp.m_userKey, rTrans.m_userKey);
		VBH::Assign(rsp.m_signature, rTrans.m_signature);
		this->FormatProposal(rsp.m_proposal, rTrans.m_nActionID, rTrans.m_proposal);
		rsp.m_vecWs.Open(rTrans.m_vecWs.Size());
		if (TEST_CC::EN_COMMIT_INFORMATION_ACTION_ID == rTrans.m_nActionID)
		{
			rsp.m_vecWs[0].m_nVersion = rTrans.m_vecWs[0].m_nVersion;
			VBH::Assign(rsp.m_vecWs[0].m_key, rTrans.m_vecWs[0].m_key);
			VBH::Assign(rsp.m_vecWs[0].m_value, rTrans.m_vecWs[0].m_value);
		}
		else
		{
			
			for (ACE_UINT16 idx = 0; idx < rTrans.m_vecWs.Size(); ++idx)
			{
				rsp.m_vecWs[idx].m_nVersion = rTrans.m_vecWs[idx].m_nVersion;
				VBH::Assign(rsp.m_vecWs[idx].m_key, rTrans.m_vecWs[idx].m_key);
				this->FormatUser(rsp.m_vecWs[idx].m_value, rTrans.m_vecWs[idx].m_value);
			}
		}


		DSC::Encode(rsp, pRspBuf, nRspLen);

		blobInfo.Set(pRspBuf, nRspLen);
		m_pCommCcService->QueryRsp(VBH::EN_OK_TYPE, nCcSessionID, blobInfo);

		blobInfo.FreeBuffer();
	}
	else
	{
		m_pCommCcService->QueryRsp(nReturnCode, nCcSessionID, blobInfo);
	}

}
