#include "vbh_comm/vbh_key_codec.h"
#include "vbh_comm/vbh_comm_macro_def.h"
#include "vbh_server_comm/vbh_transaction_def.h"

#include "vbh_comm/cc_comm_def.h"
#include "vbh_comm/cc_explorer_query_msg_def.h"

#include "ace/OS_NS_strings.h"
#include "ace/DLL_Manager.h"
#include "ace/OS_NS_dlfcn.h"

#include "dsc/service/dsc_service_container.h"
#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"
#include "dsc/dispatcher/dsc_dispatcher_center.h"
#include "cc_comm/inner_cc_service_factory.h"

#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_cs_def.h"




CCcBaseService::ICcSession::ICcSession(CCcBaseService& rCcService)
	: m_rCcService(rCcService)
{
}


CCcBaseService::CRegistUserSession::CRegistUserSession(CCcBaseService& rCcService)
	: CCcBaseService::ICcSession(rCcService)
{
}

CCcBaseService::CProposalSession::CProposalSession(CCcBaseService& rCcService)
	: CCcBaseService::ICcSession(rCcService)
{
}

CCcBaseService::CProposalSession::~CProposalSession()
{
	VBH::ClearDscUnboundQueue(m_queueWriteSetItem);
}

CCcBaseService::CQuerySession::CQuerySession(CCcBaseService& rCcService)
	: CCcBaseService::ICcSession(rCcService)
{
}

CCcBaseService::CQuerySession::~CQuerySession()
{
	CQueryBatchAtQuery* pQueryBatch;	
	for (ACE_UINT32 idx = 0; idx < m_vecQueryBatch.Size(); ++idx)
	{
		pQueryBatch = m_vecQueryBatch[idx];
		DSC_THREAD_TYPE_DELETE(pQueryBatch);
	}
}

inline void CCcBaseService::CQuerySession::OnTimer(void)
{
	this->m_rCcService.OnTimeOut(this);
}

ACE_UINT64 CCcBaseService::CQuerySession::CombineID(const ACE_UINT32 nSessionID, const ACE_UINT32 nIndex)
{
	ACE_UINT64 nId = nSessionID;

	nId <<= 32;
	nId |= nId;

	return nId;
}

void CCcBaseService::CQuerySession::SplitID(ACE_UINT32& nSessionID, ACE_UINT32& nIndex, const ACE_UINT64 nQuerySessionID)
{
	nIndex = nQuerySessionID & 0xFFFFFFFF;
	nSessionID = (nQuerySessionID >> 32) & 0xFFFFFFFF;
}

 CCcWsKV* CCcBaseService::CProposalSession::FindWriteSet(const ACE_UINT64 nAllocatedID)
{
	for (CCcWsKV* pItem = this->m_queueWriteSetItem.Front(); pItem; pItem = pItem->m_pNext)
	{
		if (pItem->m_nAllocatedID == nAllocatedID)
		{
			return pItem;
		}
	}

	return nullptr;
}

inline void CCcBaseService::CRegistUserSession::OnTimer(void)
{
	this->m_rCcService.OnTimeOut(this);
}

inline void CCcBaseService::CProposalSession::OnTimer(void)
{
	this->m_rCcService.OnTimeOut(this);
}

CCcBaseService::CProposeBatchQueryTask::CProposeBatchQueryTask(CCcBaseService& rCcService)
	: m_rCcService(rCcService)
{
}

inline void CCcBaseService::CProposeBatchQueryTask::ClearQueryQueue(void)
{
	if (m_nQueryItemCount)
	{
		VBH::ClearDscUnboundQueue(this->m_queueQueryItem);
		m_nQueryItemCount = 0;
	}
}

void CCcBaseService::CProposeBatchQueryTask::OnTimer(void)
{
	this->m_rCcService.OnTimeBatchQuery();
}


CCcBaseService::CCcBaseService(ACE_UINT32 nChannelID)
	: m_nChannelID(nChannelID)
{
}


ACE_INT32 CCcBaseService::OnInit(void)
{
	CBaseInnerService::OnInit();
	VBH::CCommitterServiceRouter csRouter;
	if (csRouter.Open())
	{
		DSC_RUN_LOG_ERROR("x committer router open failed.");
		return -1;
	}
	if (csRouter.GetCsAddr(m_xcsAddr, m_nChannelID))
	{
		DSC_RUN_LOG_ERROR("can not find channel[%d]'s xcs addr.", m_nChannelID);
		return -1;
	}
	csRouter.Close();


	ACE_INT32 nBatchQueryTimeoutValue; //批量查询的定时时间
	ACE_INT32 nBatchQueryMaxSizeValue; //批量查询的定量个数

	if (VBH::GetVbhProfileInt("BATCH_QUERY_TIMEOUT", nBatchQueryTimeoutValue))
	{
		DSC_RUN_LOG_ERROR("read BATCH_QUERY_TIMEOUT failed.");
		return -1;
	}
	if (nBatchQueryTimeoutValue < 0)
	{
		DSC_RUN_LOG_ERROR("BATCH_QUERY_TIMEOUT[%d] value invalid", nBatchQueryTimeoutValue);
		return -1;
	}
	m_nBatchQueryTimeoutValue = (ACE_UINT32)nBatchQueryTimeoutValue;

	if (VBH::GetVbhProfileInt("BATCH_QUERY_MAX_SIZE", nBatchQueryMaxSizeValue))
	{
		DSC_RUN_LOG_ERROR("read BATCH_QUERY_MAX_SIZE failed.");
		return -1;
	}
	if (nBatchQueryMaxSizeValue < 0)
	{
		DSC_RUN_LOG_ERROR("BATCH_QUERY_MAX_SIZE[%d] value invalid", nBatchQueryMaxSizeValue);
		return -1;
	}
	m_nBatchQueryMaxSizeValue = (ACE_UINT32)nBatchQueryMaxSizeValue;


	m_pBatchQueryTask = DSC_THREAD_TYPE_NEW(CProposeBatchQueryTask) CProposeBatchQueryTask(*this);
	this->SetDscTimer(m_pBatchQueryTask, m_nBatchQueryTimeoutValue, true);


	outer_cc_cfg_list_type lstOuterCcCfg;


	if (ReadDBConfig(lstOuterCcCfg,  "OUTER_CC_CFG"))
	{
		DSC_RUN_LOG_ERROR("read config from database failed.");
		return -1;
	}

	COuterCCServiceFactory outerCcFactory;

	CDscReactorServiceContainerFactory dscReactorServiceContainerFactory;
	IDscTask* pDscServiceContainer = nullptr;

	ACE_UINT16 nContainerID = 201;

	//2.注册container
	CDscDispatcherCenterDemon::instance()->AcquireWrite();
	pDscServiceContainer = CDscDispatcherCenterDemon::instance()->GetDscTask_i(VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE, nContainerID);
	if (!pDscServiceContainer)
	{
		pDscServiceContainer = dscReactorServiceContainerFactory.CreateDscServiceContainer();
		if (CDscDispatcherCenterDemon::instance()->RegistDscTask_i(pDscServiceContainer, VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE, nContainerID))
		{
			DSC_RUN_LOG_ERROR("regist endorser container error, type:%d, id:%d.", VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE, nContainerID);
			CDscDispatcherCenterDemon::instance()->Release();

			return -1;
		}
	}
	if (!pDscServiceContainer)
	{
		DSC_RUN_LOG_ERROR("cann't create container.");
		CDscDispatcherCenterDemon::instance()->Release();

		return -1;
	}
	CDscDispatcherCenterDemon::instance()->Release();
	for (auto it = lstOuterCcCfg.begin(); it != lstOuterCcCfg.end(); ++it)
	{
		IOuterCcService* pOuterCcService = NULL;
		if (LoadInnerCcInDLL(it->m_strCcName, &pOuterCcService, it->m_nChannelID))
		{
			DSC_RUN_LOG_ERROR("load outer cc from dll failed, cc-name:%s", it->m_strCcName.c_str() );
			return -1;
		}

		outerCcFactory.m_pOuterCCService = pOuterCcService;
		outerCcFactory.m_pOuterCCService->SetType(it->m_nCcID);
		outerCcFactory.m_pOuterCCService->SetID(DSC::EN_INVALID_ID);
		outerCcFactory.m_pOuterCCService->SetCommCcService(this); 

		CDscSynchCtrlMsg ctrlCcFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &outerCcFactory);

		if (pDscServiceContainer->PostDscMessage(&ctrlCcFactory))
		{
			DSC_RUN_LOG_ERROR("Failed to push queue at container:%d.", VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE);

			return -1;
		}
		pOuterCcService->SetCommCcService(this);
		m_mapOuterCcService.DirectInsert(it->m_nCcID, pOuterCcService);
	}

	DSC_RUN_LOG_FINE("cc service:%d init succeed!", this->GetID());

	return 0;
}

ACE_INT32 CCcBaseService::OnExit(void)
{
	CProposeBatchQueryWaitRspSession* pBatchQueryWaitRspSession;
	for (auto it = m_mapBatchQueryWaitRspSession.begin(); it != m_mapBatchQueryWaitRspSession.end();)
	{
		pBatchQueryWaitRspSession = it.second;
		++it;
		DSC_THREAD_TYPE_DEALLOCATE(pBatchQueryWaitRspSession);
	}

	if (m_pBatchQueryTask)
	{
		this->CancelDscTimer(m_pBatchQueryTask);
		DSC_THREAD_TYPE_DEALLOCATE(m_pBatchQueryTask);
	}

	CRegistUserSession* pRegistUserSession;
	for (auto it = m_mapRegistUserSession.begin(); it != m_mapRegistUserSession.end();)
	{
		pRegistUserSession = it.second;
		this->CancelDscTimer(pRegistUserSession);
		++it;
		DSC_THREAD_TYPE_DEALLOCATE(pRegistUserSession);
	}

	CProposalSession* pProposalSession;
	for (auto it = m_mapProposalSession.begin(); it != m_mapProposalSession.end();)
	{
		pProposalSession = it.second;
		this->CancelDscTimer(pProposalSession);
		++it;
		DSC_THREAD_TYPE_DEALLOCATE(pProposalSession);
	}

	CQuerySession* pQueryProposalSession;
	for (auto it = m_mapQuerySession.begin(); it != m_mapQuerySession.end();)
	{
		pQueryProposalSession = it.second;
		this->CancelDscTimer(pQueryProposalSession);
		++it;
		DSC_THREAD_TYPE_DEALLOCATE(pQueryProposalSession);
	}

	return 0;
}

ACE_INT32 CCcBaseService::OnLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	COuterCCServiceFactory outerCcFactory;

	CDscReactorServiceContainerFactory dscReactorServiceContainerFactory;
	IDscTask* pDscServiceContainer = nullptr;

	ACE_UINT16 nContainerID = 201;

	//2.获取container
	CDscDispatcherCenterDemon::instance()->AcquireWrite();
	pDscServiceContainer = CDscDispatcherCenterDemon::instance()->GetDscTask_i(VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE, nContainerID);

	if (!pDscServiceContainer)
	{
		DSC_RUN_LOG_ERROR("cann't get container.");
		CDscDispatcherCenterDemon::instance()->Release();

		return -1;
	}
	CDscDispatcherCenterDemon::instance()->Release();

	IOuterCcService* pOuterCcService = NULL;
	CDscString strCcName;
	strCcName.assign(req.m_ccName.GetBuffer(), req.m_ccName.GetSize());
	if (LoadInnerCcInDLL(strCcName, &pOuterCcService, req.m_nChannelID))
	{
		DSC_RUN_LOG_ERROR("load outer cc from dll failed, cc-name:%s", strCcName.c_str());
		return -1;
	}

	outerCcFactory.m_pOuterCCService = pOuterCcService;
	outerCcFactory.m_pOuterCCService->SetType(req.m_nCcID);
	outerCcFactory.m_pOuterCCService->SetID(DSC::EN_INVALID_ID);
	outerCcFactory.m_pOuterCCService->SetCommCcService(this); 

	CDscSynchCtrlMsg ctrlCcFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &outerCcFactory);

	if (pDscServiceContainer->PostDscMessage(&ctrlCcFactory))
	{
		DSC_RUN_LOG_ERROR("Failed to push queue at container:%d.", VBH::EN_MIN_VBH_OUTER_CC_SERVICE_TYPE);

		return -1;
	}
	pOuterCcService->SetCommCcService(this);
	m_mapOuterCcService.DirectInsert(req.m_nCcID, pOuterCcService);
	return 0;
}


ACE_INT32 CCcBaseService::OnRecvRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	CRegistUserSession* pSession = DSC_THREAD_TYPE_NEW(CRegistUserSession) CRegistUserSession(*this);

	pSession->m_nCcSessionID = AllocSessionID();
	pSession->m_nEsSessionID = req.m_nEsSessionID;
	pSession->m_nChannelID = req.m_nChannelID;
	pSession->m_esAddr = rSrcMsgAddr;

	this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
	m_mapRegistUserSession.DirectInsert(pSession->m_nCcSessionID, pSession);
	IOuterCcService* pOuterCcService = GetOuterCcService(1);

	if (pOuterCcService->RegistUserProc(pSession->m_nCcSessionID, req.m_userInfo))
	{
		//RegistUserProc函数返回前，有可能已经已经执行了应答逻辑，并删除了session，所以，这里要对session是否存在进行判断
		pSession = m_mapRegistUserSession.Erase(pSession->m_nCcSessionID);
		if (pSession)
		{
			this->CancelDscTimer(pSession);
			DSC_THREAD_TYPE_DEALLOCATE(pSession);
		}

		DSC_RUN_LOG_INFO("cc report error");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	return VBH::EN_OK_TYPE;
}


ACE_INT32 CCcBaseService::OnRecvProposalEsCcReq(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	const ACE_UINT32 nCcSessionID = AllocSessionID();
	CProposalSession* pSession = DSC_THREAD_TYPE_NEW(CProposalSession) CProposalSession(*this);

	pSession->m_proposeUserKey = req.m_proposeUserKey; //记录提案发起人
	VBH::Assign(pSession->m_signature, req.m_signature); //签名
	VBH::Assign(pSession->m_proposal, req.m_proposal); //提案
	pSession->m_nCcSessionID = nCcSessionID;
	pSession->m_nEsSessionID = req.m_nEsSessionID;
	pSession->m_nChannelID = req.m_nChannelID;
	pSession->m_nActionID = req.m_nActionID;
	pSession->m_esAddr = rSrcMsgAddr;

	this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
	m_mapProposalSession.DirectInsert(nCcSessionID, pSession);
	IOuterCcService* pOuterCcService = GetOuterCcService(req.m_nActionID);

	if (pOuterCcService->ProposalProc(nCcSessionID, req.m_nActionID, req.m_proposal))
	{
		pSession = m_mapProposalSession.Erase(nCcSessionID); //ProposalProc 处理过程中，可能导致session的删除
		if (pSession)
		{
			this->CancelDscTimer(pSession);
			DSC_THREAD_TYPE_DEALLOCATE(pSession);
		}

		DSC_RUN_LOG_INFO("cc report error");

		return VBH::EN_CC_COMMON_ERROR_TYPE;
	}

	return VBH::EN_OK_TYPE;
}


ACE_INT32 CCcBaseService::OnRecvQueryEsCcReq(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	const ACE_UINT32 nCcSessionID = AllocSessionID();
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	CQuerySession* pSession = DSC_THREAD_TYPE_NEW(CQuerySession) CQuerySession(*this);

	pSession->m_nCcSessionID = nCcSessionID;
	pSession->m_nEsSessionID = req.m_nEsSessionID;
	pSession->m_nChannelID = req.m_nChannelID;
	pSession->m_esAddr = rSrcMsgAddr;

	this->SetDscTimer(pSession, EN_SESSION_TIMEOUT_VALUE);
	m_mapQuerySession.DirectInsert(nCcSessionID, pSession);

	//先兼容处理 区块链浏览器的 查询请求
	switch (req.m_nActionID)
	{
	case EXPLORER_QUERY::CQueryBlockHeaderInfoAction::EN_ACTION_ID: //查询区块信息
	{
		EXPLORER_QUERY::CQueryBlockHeaderInfoAction action;

		if (DSC::Decode(action, req.m_param.GetBuffer(), req.m_param.GetSize()))
		{
			DSC_RUN_LOG_ERROR("Decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryBlockHeaderInfoExplorerCcCsReq req;
			req.m_nCcSessionID = nCcSessionID;
			req.m_nBlockID = action.m_nBlockID;

			nReturnCode = this->SendDscMessage(req, m_xcsAddr);
		}
	}
	break;
	case EXPLORER_QUERY::CQueryBlockCountAction::EN_ACTION_ID: //查询区块个数
	{
		VBH::CQueryBlockCountExplorerCcCsReq req;
		req.m_nCcSessionID = nCcSessionID;

		nReturnCode = this->SendDscMessage(req, m_xcsAddr);
	}
	break;
	case EXPLORER_QUERY::CQueryWriteSetAction::EN_ACTION_ID: //查询写集内容
	{
		EXPLORER_QUERY::CQueryWriteSetAction action;

		if (DSC::Decode(action, req.m_param.GetBuffer(), req.m_param.GetSize()))
		{
			DSC_RUN_LOG_ERROR("Decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryWriteSetExplorerCcCsReq req;

			if (VBH::HexDecode(req.m_alocKey, action.m_userID.GetBuffer(), action.m_userID.GetSize()))
			{
				DSC_RUN_LOG_ERROR("Decode error.");
				nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
			}
			else
			{
				req.m_nCcSessionID = nCcSessionID;

				nReturnCode = this->SendDscMessage(req, m_xcsAddr);
			}
		}
	}
	break;
	case EXPLORER_QUERY::CQueryTransInfoAction::EN_ACTION_ID: //查询事务内容
	{
		EXPLORER_QUERY::CQueryTransInfoAction action;
		if (DSC::Decode(action, req.m_param.GetBuffer(), req.m_param.GetSize()))
		{
			DSC_RUN_LOG_ERROR("Decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryTransInfoExplorerCcCsReq req;

			if (VBH::HexDecode(req.m_transKey, action.m_transKey.GetBuffer(), action.m_transKey.GetSize()))
			{
				DSC_RUN_LOG_ERROR("Decode error.");
				nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
			}
			else
			{
				req.m_nCcSessionID = nCcSessionID;
				nReturnCode = this->SendDscMessage(req, m_xcsAddr);
			}
		}
	}
	break;
	case EXPLORER_QUERY::CQueryBlockTransCountAction::EN_ACTION_ID: //查询区块区块中事务个数
	{
		EXPLORER_QUERY::CQueryBlockTransCountAction action;
		if (DSC::Decode(action, req.m_param.GetBuffer(), req.m_param.GetSize()))
		{
			DSC_RUN_LOG_ERROR("Decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryTransCountExplorerCcCsReq req;

			req.m_nCcSessionID = nCcSessionID;
			req.m_nBlockID = action.m_nBlockID;
			nReturnCode = this->SendDscMessage(req, m_xcsAddr);
		}
	}
	break;
	case EXPLORER_QUERY::CQueryBlockTransListAction::EN_ACTION_ID: //查询区块区块中事务列表
	{
		EXPLORER_QUERY::CQueryBlockTransListAction action;
		if (DSC::Decode(action, req.m_param.GetBuffer(), req.m_param.GetSize()))
		{
			DSC_RUN_LOG_ERROR("Decode error.");
			nReturnCode = VBH::EN_DECODE_ERROR_TYPE;
		}
		else
		{
			VBH::CQueryTransListExplorerCcCsReq req;

			req.m_nCcSessionID = nCcSessionID;
			req.m_nBlockID = action.m_nBlockID;
			req.m_nPageIndex = action.m_nPageIndex;
			req.m_nPageSize = action.m_nPageSize;
			nReturnCode = this->SendDscMessage(req, m_xcsAddr);
		}
	}
	break;
	default:
	{
		IOuterCcService* pOuterCcService = GetOuterCcService(req.m_nActionID);
		nReturnCode = pOuterCcService->QueryProc(nCcSessionID, req.m_nActionID, req.m_param);
	}
	}

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		pSession = m_mapQuerySession.Erase(nCcSessionID); //ProposalProc 处理过程中，可能导致session的删除
		if (pSession)
		{
			this->CancelDscTimer(pSession);
			DSC_THREAD_TYPE_DEALLOCATE(pSession);
		}

		DSC_RUN_LOG_INFO("cc report error");
	}

	return nReturnCode;
}

//TODO: 提案的时候，要在session中记录数据的版本号；其他情况下不需要记录版本号
void CCcBaseService::OnDscMsg(VBH::CQueryWriteSetListProposeCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryWriteSetListProposeCsCcRsp);

	CProposeBatchQueryWaitRspSession* pBatchQueryWaitRspSession = m_mapBatchQueryWaitRspSession.Erase(rsp.m_nCcSessionID);

	if (pBatchQueryWaitRspSession)
	{
		if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			ACE_ASSERT(pBatchQueryWaitRspSession->m_vecQueryItem.Size() == rsp.m_vecGroupWsInfo.Size());

			CProposalSession* pProposalSession;
			CProposeSimpleWriteSet* pSimpleWsVec;
			CCcWsKV* pWs;
			ACE_UINT8 nWsType;
			ACE_UINT16 nInnerVecLen;
			ACE_UINT16 nInnerIdx;
			ACE_UINT32 nProposalCcSessionID; //TODO: 这里不一定是提案，也有可能是其他类型的session
			char* pInfoBuf;
			size_t nInfoBufLen;

			for (ACE_UINT16 nOuterIdx = 0; nOuterIdx < pBatchQueryWaitRspSession->m_vecQueryItem.Size(); ++nOuterIdx)
			{
				ACE_ASSERT(pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey.Size() == rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue.Size());
				
				nInnerVecLen = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey.Size();
				nProposalCcSessionID = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nCcSessionID;
				nWsType = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nWsType;
				pProposalSession = m_mapProposalSession.Find(nProposalCcSessionID);

				ACE_ASSERT((nWsType == EN_WS_USER_TYPE) || (nWsType == EN_WS_INFORMATION_TYPE));

				if (pProposalSession)
				{
					DSC_THREAD_TYPE_ALLOCATE_ARRAY(pSimpleWsVec, nInnerVecLen);

					for (nInnerIdx = 0; nInnerIdx < nInnerVecLen; ++nInnerIdx)
					{
						pWs = pProposalSession->FindWriteSet(pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey[nInnerIdx].m_nAllocatedID);

						if (pWs)
						{
							pWs->m_nVersion = rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue[nInnerIdx].m_nVersion;
						}
						else
						{
							pWs = DSC_THREAD_TYPE_NEW(CCcWsKV) CCcWsKV;
							pWs->m_nAllocatedID = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey[nInnerIdx].m_nAllocatedID;
							pWs->m_nVersion = rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue[nInnerIdx].m_nVersion;

							pProposalSession->m_queueWriteSetItem.PushBack(pWs);
						}

						nInfoBufLen = VBH::HexEncode(pInfoBuf, pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey[nInnerIdx]);
						pSimpleWsVec[nInnerIdx].m_key.Set(pInfoBuf, nInfoBufLen);
						pSimpleWsVec[nInnerIdx].m_value = rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue[nInnerIdx].m_value;
					}
					IOuterCcService* pOuterCcService = GetOuterCcService(0);
					if (nWsType == EN_WS_USER_TYPE)
					{
						if (nInnerVecLen == 1) //长度只有1时，调用单参数的应答函数
						{
							pOuterCcService->OnGetUserRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, pSimpleWsVec[0]);
						}
						else
						{
							pOuterCcService->OnGetUserRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, pSimpleWsVec, nInnerVecLen);
						}
					}
					else
					{
						if (nInnerVecLen == 1) //长度只有1时，调用单参数的应答函数
						{
							pOuterCcService->OnGetInformationRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, pSimpleWsVec[0]);
						}
						else
						{
							pOuterCcService->OnGetInformationRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, pSimpleWsVec, nInnerVecLen);
						}
					}

					//回调上层业务逻辑后，释放开辟的内存空间
					for (nInnerIdx = 0; nInnerIdx < nInnerVecLen; ++nInnerIdx)
					{
						pSimpleWsVec[nInnerIdx].m_key.FreeBuffer();
					}
					DSC_THREAD_TYPE_DEALLOCATE_ARRAY(pSimpleWsVec, nInnerVecLen);
				}
				else
				{
					DSC_RUN_LOG_ERROR("cann't find prosal session, session id:%d", pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nCcSessionID);
				}
			}
		}
		else //出错的话，给上层业务回错误应答
		{
			CProposalSession* pProposalSession;
			ACE_UINT32 nProposalCcSessionID;
			ACE_UINT8 nWsType;
			ACE_UINT16 nInnerVecLen;

			for (ACE_UINT16 nOuterIdx = 0; nOuterIdx < pBatchQueryWaitRspSession->m_vecQueryItem.Size(); ++nOuterIdx)
			{
				nInnerVecLen = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey.Size();
				nProposalCcSessionID = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nCcSessionID;
				nWsType = pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nWsType;
				pProposalSession = m_mapProposalSession.Find(nProposalCcSessionID);

				ACE_ASSERT((nWsType == EN_WS_USER_TYPE) || (nWsType == EN_WS_INFORMATION_TYPE));

				if (pProposalSession)
				{
					IOuterCcService* pOuterCcService = GetOuterCcService(0);
					if (nWsType == EN_WS_USER_TYPE)
					{
						if (nInnerVecLen == 1) //长度只有1时，调用单参数的应答函数
						{
							CProposeSimpleWriteSet simpleWs;
							pOuterCcService->OnGetUserRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, simpleWs);
						}
						else
						{
							pOuterCcService->OnGetUserRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, nullptr, 0);
						}
					}
					else
					{
						if (nInnerVecLen == 1) //长度只有1时，调用单参数的应答函数
						{
							CProposeSimpleWriteSet simpleWs;
							pOuterCcService->OnGetInformationRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, simpleWs);
						}
						else
						{
							pOuterCcService->OnGetInformationRspAtPropose(rsp.m_nReturnCode, nProposalCcSessionID, nullptr, 0);
						}
					}

				}
				else
				{
					DSC_RUN_LOG_ERROR("cann't find prosal session, session id:%d", pBatchQueryWaitRspSession->m_vecQueryItem[nOuterIdx].m_nCcSessionID);
				}
			}

			DSC_RUN_LOG_INFO("CQueryUserInfoListXcsCcRsp failed, error-code:%d, cc-session-id:%d", rsp.m_nReturnCode, rsp.m_nCcSessionID);
		}

		DSC_THREAD_TYPE_DELETE(pBatchQueryWaitRspSession); //清空该session
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find batch-query-wait-rsp session, session id:%d", rsp.m_nCcSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryWriteSetListProposeCsCcRsp);
}

void CCcBaseService::OnDscMsg(VBH::CQueryWriteSetListQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryWriteSetListQueryCsCcRsp);

	ACE_UINT32 nSessionID;
	ACE_UINT32 nIndex;

	CQuerySession::SplitID(nSessionID, nIndex, rsp.m_nCcSessionID);
	CQuerySession* pSession = m_mapQuerySession.Find(nSessionID);

	if (pSession)
	{
		if (VBH::EN_OK_TYPE == rsp.m_nReturnCode)
		{
			ACE_ASSERT(nIndex < pSession->m_vecQueryBatch.Size());

			CQueryBatchAtQuery* pBatch = pSession->m_vecQueryBatch[nIndex];
			CQuerySimpleWriteSet* pSimpleWsVec;
			char* pKeyBuf;
			size_t nKeyBufLen;
			const ACE_UINT16 nVecLen = pBatch->m_vecAlocKey.Size();

			ACE_ASSERT(rsp.m_vecValue.Size() == pBatch->m_vecAlocKey.Size());
			ACE_ASSERT((pBatch->m_nWsType == EN_WS_USER_TYPE) || (pBatch->m_nWsType == EN_WS_INFORMATION_TYPE));

			DSC_THREAD_TYPE_ALLOCATE_ARRAY(pSimpleWsVec, nVecLen);
			
			for (ACE_UINT16 idx = 0; idx < nVecLen; ++idx)
			{
				nKeyBufLen = VBH::HexEncode(pKeyBuf, pBatch->m_vecAlocKey[idx]);
				pSimpleWsVec[idx].m_key.Set(pKeyBuf, nKeyBufLen);
				pSimpleWsVec[idx].m_value = rsp.m_vecValue[idx].m_value;
				pSimpleWsVec[idx].m_nVersion = rsp.m_vecValue[idx].m_nVersion;
			}
			IOuterCcService* pOuterCcService = GetOuterCcService(0);
			if (pBatch->m_nWsType == EN_WS_USER_TYPE)
			{
				if (nVecLen == 1) //长度只有1时，调用单参数的应答函数
				{
					pOuterCcService->OnGetUserRspAtQuery(rsp.m_nReturnCode, nSessionID, pSimpleWsVec[0]);
				}
				else
				{
					pOuterCcService->OnGetUserRspAtQuery(rsp.m_nReturnCode, nSessionID, pSimpleWsVec, nVecLen);
				}
			}
			else
			{
				if (nVecLen == 1) //长度只有1时，调用单参数的应答函数
				{
					pOuterCcService->OnGetInformationRspAtQuery(rsp.m_nReturnCode, nSessionID, pSimpleWsVec[0]);
				}
				else
				{
					pOuterCcService->OnGetInformationRspAtQuery(rsp.m_nReturnCode, nSessionID, pSimpleWsVec, nVecLen);
				}
			}

			//回调上层业务逻辑后，释放开辟的内存空间
			for (ACE_UINT16 idx = 0; idx < nVecLen; ++idx)
			{
				pSimpleWsVec[idx].m_key.FreeBuffer();
			}
			DSC_THREAD_TYPE_DEALLOCATE_ARRAY(pSimpleWsVec, nVecLen);
		}
		else
		{
			ACE_ASSERT(nIndex < pSession->m_vecQueryBatch.Size());

			CQueryBatchAtQuery* pQueryBatch = pSession->m_vecQueryBatch[nIndex];
			const ACE_UINT16 nVecLen = pQueryBatch->m_vecAlocKey.Size();

			ACE_ASSERT(rsp.m_vecValue.Size() == pQueryBatch->m_vecAlocKey.Size());
			ACE_ASSERT((pQueryBatch->m_nWsType == EN_WS_USER_TYPE) || (pQueryBatch->m_nWsType == EN_WS_INFORMATION_TYPE));
			IOuterCcService* pOuterCcService = GetOuterCcService(0);
			if (pQueryBatch->m_nWsType == EN_WS_USER_TYPE)
			{
				if (nVecLen == 1) //长度只有1时，调用单参数的应答函数
				{
					CQuerySimpleWriteSet ws;

					pOuterCcService->OnGetUserRspAtQuery(rsp.m_nReturnCode, nSessionID, ws);
				}
				else
				{
					pOuterCcService->OnGetUserRspAtQuery(rsp.m_nReturnCode, nSessionID, nullptr, 0);
				}
			}
			else
			{
				if (nVecLen == 1) //长度只有1时，调用单参数的应答函数
				{
					CQuerySimpleWriteSet ws;

					pOuterCcService->OnGetInformationRspAtQuery(rsp.m_nReturnCode, nSessionID, ws);
				}
				else
				{
					pOuterCcService->OnGetInformationRspAtQuery(rsp.m_nReturnCode, nSessionID, nullptr, 0);
				}
			}
		}
	}
	else
	{
		DSC_RUN_LOG_WARNING("can not find query session, session-id:%u", nSessionID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryWriteSetListQueryCsCcRsp);
}

void CCcBaseService::OnDscMsg(VBH::CQueryTransactionQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryTransactionQueryCsCcRsp);

	ACE_INT32 nReturnCode = rsp.m_nReturnCode;
	CQuerySimpleTransaction trans;
	char* pMsgBuf;
	size_t nMsgBufLen;

	if (nReturnCode == VBH::EN_OK_TYPE)
	{
		trans.m_nActionID = rsp.m_transContent.m_nActionID;
		
		nMsgBufLen = VBH::HexEncode(pMsgBuf, rsp.m_transKey);
		trans.m_transKey.Set(pMsgBuf, nMsgBufLen);

		nMsgBufLen = VBH::HexEncode(pMsgBuf, rsp.m_transContent.m_userKey);
		trans.m_userKey.Set(pMsgBuf, nMsgBufLen);

		trans.m_signature = rsp.m_transContent.m_signature;
		trans.m_proposal = rsp.m_transContent.m_proposal;

		trans.m_vecWs.Open(rsp.m_transContent.m_vecWs.Size());
		for (ACE_UINT16 idx = 0; idx < rsp.m_transContent.m_vecWs.Size(); ++idx)
		{
			nMsgBufLen = VBH::HexEncode(pMsgBuf, rsp.m_transContent.m_vecWs[idx].m_key);
			trans.m_vecWs[idx].m_key.Set(pMsgBuf, nMsgBufLen);

			trans.m_vecWs[idx].m_value = rsp.m_transContent.m_vecWs[idx].m_value;
			trans.m_vecWs[idx].m_nVersion = rsp.m_transContent.m_vecWs[idx].m_nVersion;
		}
	}
	IOuterCcService* pOuterCcService = GetOuterCcService(0);
	pOuterCcService->OnGetTransRspAtQuery(nReturnCode, rsp.m_nCcSessionID, trans);

	trans.m_transKey.FreeBuffer();
	trans.m_userKey.FreeBuffer();
	for (ACE_UINT16 idx = 0; idx < rsp.m_transContent.m_vecWs.Size(); ++idx)
	{
		trans.m_vecWs[idx].m_key.FreeBuffer();
	}

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryTransactionQueryCsCcRsp);
}

void CCcBaseService::OnDscMsg(VBH::CQueryBlockHeaderInfoExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryBlockHeaderInfoExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;
	
	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pEncodeBuf;
		size_t nEncodeSize;
		EXPLORER_QUERY::CQueryBlockHeaderInfoRsp ActionRsp;

		//解析事务的提案内容
		ActionRsp.blockID = rsp.m_nBlockID;

		//前1区块hash
		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_preBlockHash.GetBuffer(), rsp.m_preBlockHash.GetSize());
		ActionRsp.preHash.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

		//merkel树hash
		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_merkelTreeRootHash.GetBuffer(), rsp.m_merkelTreeRootHash.GetSize());
		ActionRsp.transactionMroot.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

		//区块时间
		DSC::CDscDateTime::GetTimeString(ActionRsp.timeStamp, rsp.m_nBlockTime);

		//事务个数
		ActionRsp.transCount = rsp.m_nTransCount;

		DSC::Encode(ActionRsp, pEncodeBuf, nEncodeSize);
		blobInfo.Set(pEncodeBuf, nEncodeSize);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);
	
	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryBlockHeaderInfoExplorerCsCcRsp);
}


void CCcBaseService::OnDscMsg(VBH::CQueryBlockCountExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryBlockCountExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;
	
	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pMsgBuf;
		size_t nMsgBufLen;
		EXPLORER_QUERY::CQueryBlockCountRsp actionRsp;

		actionRsp.blockCount = rsp.m_nBlockCount;
		DSC::Encode(actionRsp, pMsgBuf, nMsgBufLen);

		blobInfo.Set(pMsgBuf, nMsgBufLen);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);

	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryBlockCountExplorerCsCcRsp);
}


void CCcBaseService::OnDscMsg(VBH::CQueryWriteSetExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryWriteSetExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;

	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pMsgBuf;
		size_t nMsgBufLen;
		EXPLORER_QUERY::CQueryWriteSetRsp actionRsp;

		//cc无法得知道这个是用户，还是information，浏览器中，cc仅将其解析为hex编码

		DSC::Encode(actionRsp, pMsgBuf, nMsgBufLen);
		blobInfo.Set(pMsgBuf, nMsgBufLen);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);

	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryWriteSetExplorerCsCcRsp);
}


void CCcBaseService::OnDscMsg(VBH::CQueryTransInfoExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryTransInfoExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;

	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pEncodeBuf;
		size_t nEncodeSize;
		EXPLORER_QUERY::CQueryTransInfoRsp actionRsp;

		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_transKey);
		actionRsp.transId.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_transInfo.m_userKey);
		actionRsp.sender.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

		actionRsp.blockId = rsp.m_transKey.m_nBlockID;

#ifdef VBH_USE_SIGNATURE
		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_transInfo.m_signature.GetBuffer(), rsp.m_transInfo.m_signature.GetSize());
		actionRsp.signatures.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);
#endif
		IOuterCcService* pOuterCcService = GetOuterCcService(0);
		if (4 == rsp.m_transInfo.m_nActionID)
		{
			EXPLORER_QUERY::CQueryTransInfoRsp::CInfoJson informationJson;
			EXPLORER_QUERY::CCommitInformationAction informationAction;
			DSC::Decode(informationAction, rsp.m_transInfo.m_proposal.GetBuffer(), rsp.m_transInfo.m_proposal.GetSize());
			informationJson.informationKey.assign(informationAction.InformationKey.GetBuffer(), informationAction.InformationKey.GetSize());
			informationJson.informationValue.assign(informationAction.InformationValue.GetBuffer(), informationAction.InformationValue.GetSize());
			SJsonWriter* writer = JSON_CODE::Encode(informationJson);
			if (writer)
			{
				actionRsp.proposal.assign(writer->m_pBuffer, writer->m_nCurrentPos);
				json_writer_free(writer);
			}
		}
		else
		{
			pOuterCcService->FormatProposal(actionRsp.proposal, rsp.m_transInfo.m_nActionID, rsp.m_transInfo.m_proposal);
		}

		DSC::Encode(actionRsp, pEncodeBuf, nEncodeSize);
		blobInfo.Set(pEncodeBuf, nEncodeSize);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);

	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryTransInfoExplorerCsCcRsp);
}


void CCcBaseService::OnDscMsg(VBH::CQueryTransListExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryTransListExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;

	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pEncodeBuf;
		size_t nEncodeSize;
		EXPLORER_QUERY::CQueryBlockTransListRsp actionRsp;
		EXPLORER_QUERY::CQueryBlockTransListRsp::CTrans trans;
		EXPLORER_QUERY::CCommitInformationAction informationAction;

		actionRsp.transCount = 0;

		for (ACE_UINT16 idx = 0; idx < rsp.m_vecTrans.Size(); ++idx)
		{
			nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_vecTrans[idx].m_key);
			trans.txid.assign(pEncodeBuf, nEncodeSize);
			DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

			//只有提案类型的事务才支持显示
			if (VBH::CTransactionSequenceNumber::GetTransType(rsp.m_vecTrans[idx].m_key.m_nSequenceNumber)
				== VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE)
			{
				nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_vecTrans[idx].m_content.m_userKey);
				trans.sender.assign(pEncodeBuf, nEncodeSize);
				DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);

#ifdef VBH_USE_SIGNATURE
				nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_vecTrans[idx].m_content.m_signature.GetBuffer(), rsp.m_vecTrans[idx].m_content.m_signature.GetSize());
				trans.signatures.assign(pEncodeBuf, nEncodeSize);
				DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);
#endif

				//提案解析
				if (4 == rsp.m_vecTrans[idx].m_content.m_nActionID)
				{
					DSC::Decode(informationAction, rsp.m_vecTrans[idx].m_content.m_proposal.GetBuffer(), rsp.m_vecTrans[idx].m_content.m_proposal.GetSize());
					trans.proposal.informationKey.assign(informationAction.InformationKey.GetBuffer(), informationAction.InformationKey.GetSize());
					trans.proposal.informationValue.assign(informationAction.InformationValue.GetBuffer(), informationAction.InformationValue.GetSize());
					actionRsp.transList.push_back(trans);
					actionRsp.transCount++;
				}

			}
			else 
			{
				//注册类型的事务，在提案内容中显示不支
				trans.sender.clear();
#ifdef VBH_USE_SIGNATURE
				trans.signatures.clear();
#endif
				trans.proposal.informationKey.clear();
				trans.proposal.informationValue.clear();
			}
			
		}
		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_blockHash.GetBuffer(), rsp.m_blockHash.GetSize());
		actionRsp.blockHash.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);
		nEncodeSize = VBH::HexEncode(pEncodeBuf, rsp.m_preBlockHash.GetBuffer(), rsp.m_preBlockHash.GetSize());
		actionRsp.preBlockHash.assign(pEncodeBuf, nEncodeSize);
		DSC_THREAD_SIZE_FREE(pEncodeBuf, nEncodeSize);
		actionRsp.blockTime =(ACE_INT64) rsp.m_nBlockTime;
		DSC::Encode(actionRsp, pEncodeBuf, nEncodeSize);
		blobInfo.Set(pEncodeBuf, nEncodeSize);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);

	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryTransListExplorerCsCcRsp);
}


void CCcBaseService::OnDscMsg(VBH::CQueryInformationHistoryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	

}

void CCcBaseService::OnDscMsg(VBH::CQueryTransCountExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCcBaseService, VBH::CQueryTransCountExplorerCsCcRsp);

	DSC::CDscBlob blobInfo;

	if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
	{
		char* pMsgBuf;
		size_t nMsgBufLen;
		EXPLORER_QUERY::CQueryBlockTransCountRsp msg;

		msg.m_nTransCount = rsp.m_nTransCount;
		DSC::Encode(msg, pMsgBuf, nMsgBufLen);

		blobInfo.Set(pMsgBuf, nMsgBufLen);
	}

	this->QueryRsp(rsp.m_nReturnCode, rsp.m_nCcSessionID, blobInfo);

	blobInfo.FreeBuffer();

	VBH_MESSAGE_LEAVE_TRACE(CCcBaseService, VBH::CQueryTransCountExplorerCsCcRsp);
}


void CCcBaseService::OnTimeOut(CRegistUserSession* pRegistUserSession)
{
	VBH::CRegistUserCcEsRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nEsSessionID = pRegistUserSession->m_nEsSessionID;

	this->SendCcMessage(rsp, pRegistUserSession->m_esAddr);
	m_mapRegistUserSession.Erase(pRegistUserSession);
	DSC_THREAD_TYPE_DELETE(pRegistUserSession);

	DSC_RUN_LOG_INFO("cc regist user session timeout, session-id:%d.", pRegistUserSession->m_nCcSessionID);
}


void CCcBaseService::OnTimeOut(CProposalSession* pProposalSession)
{
	VBH::CProposeCcEsRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nEsSessionID = pProposalSession->m_nEsSessionID;

	this->SendCcMessage(rsp, pProposalSession->m_esAddr);
	m_mapProposalSession.Erase(pProposalSession);

	DSC_RUN_LOG_INFO("cc proposal session timeout, session-id:%d", pProposalSession->m_nCcSessionID);

	DSC_THREAD_TYPE_DELETE(pProposalSession);
}


void CCcBaseService::OnTimeOut(CQuerySession* pQuerySession)
{
	VBH::CQueryCcEsRsp rsp;

	rsp.m_nReturnCode = VBH::EN_TIMEOUT_ERROR_TYPE;
	rsp.m_nEsSessionID = pQuerySession->m_nEsSessionID;

	this->SendCcMessage(rsp, pQuerySession->m_esAddr);
	m_mapQuerySession.Erase(pQuerySession);

	DSC_RUN_LOG_INFO("query session timeout, session-id:%d", pQuerySession->m_nCcSessionID);

	DSC_THREAD_TYPE_DELETE(pQuerySession);
}


void CCcBaseService::OnTimeBatchQuery(void)
{
	if (m_pBatchQueryTask->m_nQueryItemCount)
	{
		//构造请求和等待应答的session
		VBH::CQueryWriteSetListProposeCcCsReq req;
		CProposeBatchQueryWaitRspSession* pWaitRspSession = DSC_THREAD_TYPE_NEW(CProposeBatchQueryWaitRspSession) CProposeBatchQueryWaitRspSession;
		auto pBatchQueryTaskItem = m_pBatchQueryTask->m_queueQueryItem.Front();

		req.m_vecGroupWsKey.Open(m_pBatchQueryTask->m_nQueryItemCount);
		pWaitRspSession->m_vecQueryItem.Open(m_pBatchQueryTask->m_nQueryItemCount);

		const ACE_UINT32 nBatchQuerySessionID = AllocSessionID();
		ACE_UINT16 nInnerVecLen;
		ACE_UINT16 nInnerIdx;
		for (ACE_UINT16 nOuterIdx = 0; nOuterIdx < m_pBatchQueryTask->m_nQueryItemCount; ++nOuterIdx)
		{
			pWaitRspSession->m_vecQueryItem[nOuterIdx].m_nWsType = pBatchQueryTaskItem->m_nWsType;
			pWaitRspSession->m_vecQueryItem[nOuterIdx].m_nCcSessionID = pBatchQueryTaskItem->m_nCcSessionID;
			nInnerVecLen = pBatchQueryTaskItem->m_vecAlocKey.Size();

			req.m_vecGroupWsKey[nOuterIdx].m_vecKey.Open(nInnerVecLen);
			pWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey.Open(nInnerVecLen);

			for (nInnerIdx = 0; nInnerIdx < nInnerVecLen; ++nInnerIdx)
			{
				req.m_vecGroupWsKey[nOuterIdx].m_vecKey[nInnerIdx] = pBatchQueryTaskItem->m_vecAlocKey[nInnerIdx];
				pWaitRspSession->m_vecQueryItem[nOuterIdx].m_vecAlocKey[nInnerIdx] = pBatchQueryTaskItem->m_vecAlocKey[nInnerIdx];
			}

			pBatchQueryTaskItem = pBatchQueryTaskItem->m_pNext;
		}

		req.m_nCcSessionID = nBatchQuerySessionID; //是batch-query的session-id， 只不过这里顺便使用了cc的session-id分配机制

		if (this->SendDscMessage(req, m_xcsAddr))
		{
			DSC_THREAD_TYPE_DELETE(pWaitRspSession);

			DSC_RUN_LOG_ERROR("SendDscMessage() failed, dest-node-id:%d, dest-service-id:%d, channel-id:%d",
				m_xcsAddr.GetNodeID(), m_xcsAddr.GetServiceID(), m_nChannelID);
		}
		else
		{
			m_mapBatchQueryWaitRspSession.DirectInsert(nBatchQuerySessionID, pWaitRspSession);

			m_pBatchQueryTask->ClearQueryQueue(); //成功发送后，清空队列
		}
	}
}


ACE_INT32 CCcBaseService::VerifyProposeUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)
{
	CProposalSession* pSession = m_mapProposalSession.Find(nCcSessionID);

	if (pSession)
	{
		VBH::CVbhAllocatedKey vbhUserKey;

		if (VBH::HexDecode(vbhUserKey, userKey.GetBuffer(), userKey.GetSize()))
		{
			DSC_RUN_LOG_ERROR("decode gen-user-key error.");

			return VBH::EN_DECODE_ERROR_TYPE;
		}

		if (vbhUserKey == pSession->m_proposeUserKey)
		{
			return VBH::EN_OK_TYPE;
		}
		else
		{
			DSC_RUN_LOG_ERROR("verify propose user failed.");

			return VBH::EN_LOGIC_FAILED_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session id:%d", nCcSessionID);

		return VBH::EN_SYSTEM_ERROR_TYPE;
	}
}


ACE_INT32 CCcBaseService::VerifyQueryUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)
{
	CQuerySession* pSession = m_mapQuerySession.Find(nCcSessionID);

	if (pSession)
	{
		VBH::CVbhAllocatedKey vbhUserKey;

		if (VBH::HexDecode(vbhUserKey, userKey.GetBuffer(), userKey.GetSize()))
		{
			DSC_RUN_LOG_ERROR("decode gen-user-key error.");

			return VBH::EN_DECODE_ERROR_TYPE;
		}

		if (vbhUserKey == pSession->m_queryUserKey)
		{
			return VBH::EN_OK_TYPE;
		}
		else
		{
			DSC_RUN_LOG_ERROR("verify query user failed.");

			return VBH::EN_LOGIC_FAILED_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session id:%d", nCcSessionID);

		return VBH::EN_SYSTEM_ERROR_TYPE;
	}
}


inline ACE_INT32 CCcBaseService::GetUserAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{	
	return this->GetWriteSetAtPropose(nCcSessionID, EN_WS_USER_TYPE, &key, 1);
}


inline ACE_INT32 CCcBaseService::GetUserAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	return this->GetWriteSetAtPropose(nCcSessionID, EN_WS_USER_TYPE, pKeyVec, nVecLen);
}


inline ACE_INT32 CCcBaseService::GetInformationAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{
	return this->GetWriteSetAtPropose(nCcSessionID, EN_WS_INFORMATION_TYPE, &key, 1);
}


inline ACE_INT32 CCcBaseService::GetInformationAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	return this->GetWriteSetAtPropose(nCcSessionID, EN_WS_INFORMATION_TYPE, pKeyVec, nVecLen);
}


inline ACE_INT32 CCcBaseService::GetUserAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{
	return this->GetWriteSetAtQuery(nCcSessionID, EN_WS_USER_TYPE, &key, 1);
}


inline ACE_INT32 CCcBaseService::GetUserAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	return this->GetWriteSetAtQuery(nCcSessionID, EN_WS_USER_TYPE, pKeyVec, nVecLen);
}


inline ACE_INT32 CCcBaseService::GetInformationAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{
	return this->GetWriteSetAtQuery(nCcSessionID, EN_WS_INFORMATION_TYPE, &key, 1);
}


inline ACE_INT32 CCcBaseService::GetInformationAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	return this->GetWriteSetAtQuery(nCcSessionID, EN_WS_INFORMATION_TYPE, pKeyVec, nVecLen);
}


ACE_INT32 CCcBaseService::GetTransAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{
	VBH::CQueryTransactionQueryCcCsReq req;

	if (VBH::HexDecode(req.m_transKey, key.GetBuffer(), key.GetSize()))
	{
		DSC_RUN_LOG_ERROR("Decode error.");
		return VBH::EN_DECODE_ERROR_TYPE;
	}
	else
	{
		req.m_nCcSessionID = nCcSessionID;
		return this->SendDscMessage(req, m_xcsAddr);
	}
}


ACE_INT32 CCcBaseService::GetInformationHistoryAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)
{
	VBH::CQueryInformationHistoryCcCsReq req;

	if (VBH::HexDecode(req.m_InfoKey, key.GetBuffer(), key.GetSize()))
	{
		DSC_RUN_LOG_ERROR("Decode error.");
		return VBH::EN_DECODE_ERROR_TYPE;
	}
	else
	{
		req.m_nCcSessionID = nCcSessionID;
		return this->SendDscMessage(req, m_xcsAddr);
	}

	return 0;
}


ACE_INT32 CCcBaseService::RegistUserRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, char* pUserInitInfo, const size_t nUserInitInfo)
{
	CRegistUserSession* pSession = m_mapRegistUserSession.Erase(nCcSessionID); //最后一次使用，直接删除

	if (pSession)
	{
		VBH::CRegistUserCcEsRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nEsSessionID = pSession->m_nEsSessionID;
		rsp.m_userInitInfo.Set(pUserInitInfo, nUserInitInfo);

		this->SendCcMessage(rsp, pSession->m_esAddr);

		//清理session
		this->CancelDscTimer(pSession);
		DSC_THREAD_TYPE_DELETE(pSession);
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not find session, session-id:%d", nCcSessionID);

		return -1;
	}

	return 0;
}




ACE_INT32 CCcBaseService::ProposalRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID,
	const CProposeSimpleUser* pUserArry, const ACE_UINT16 nUserArrayLen,
	const CProposeSimpleInformation* pInfoArry, const ACE_UINT16 nInfoArrayLen,
	const DSC::CDscShortBlob& receipt)
{
	if ((pUserArry && nUserArrayLen) || (pInfoArry && nInfoArrayLen))
	{
		CProposalSession* pSession = m_mapProposalSession.Erase(nCcSessionID);

		if (pSession)
		{
			VBH::CProposeCcEsRsp rsp;

			rsp.m_nReturnCode = nReturnCode;
			rsp.m_nEsSessionID = pSession->m_nEsSessionID;
			rsp.m_receipt = receipt;
			rsp.m_nActionID = pSession->m_nActionID;

			if ((rsp.m_nReturnCode == VBH::EN_OK_TYPE)||(rsp.m_nReturnCode == 2))
			{
				ACE_UINT16 nWsIdx;
				char* pTransContent = nullptr;
				size_t nTransContentLen;
				VBH::CProposeTransaction transaction;
				CCcWsKV* pCcWsItem;
				VBH::CVbhAllocatedKey alocKey;

				transaction.m_nActionID = pSession->m_nActionID;
				transaction.m_nUserKeyID = pSession->m_proposeUserKey.m_nAllocatedID;
				VBH::Assign(transaction.m_signature, pSession->m_signature);
				VBH::Assign(transaction.m_proposal, pSession->m_proposal);
				transaction.m_vecWsItem.Open(nUserArrayLen + nInfoArrayLen);

				//遍历user信息，准备编码
				if (rsp.m_nReturnCode == VBH::EN_OK_TYPE)
				{

					for (nWsIdx = 0; nWsIdx < nUserArrayLen; ++nWsIdx)
					{
						if (VBH::HexDecode(alocKey, pUserArry[nWsIdx].m_key.GetBuffer(), pUserArry[nWsIdx].m_key.GetSize()))
						{
							DSC_RUN_LOG_ERROR("decode error.");
							return -1;
						}

						transaction.m_vecWsItem[nWsIdx].m_nAllocatedID = alocKey.m_nAllocatedID;

						pCcWsItem = pSession->FindWriteSet(alocKey.m_nAllocatedID);
						if (pCcWsItem)
						{
							transaction.m_vecWsItem[nWsIdx].m_nVersion = pCcWsItem->m_nVersion;
							transaction.m_vecWsItem[nWsIdx].m_value = pUserArry[nWsIdx].m_value;
						}
						else
						{
							DSC_RUN_LOG_ERROR("can not find write-set-item.");

							return -1;
						}
					}
				}
				else
				{
					for (nWsIdx = 0; nWsIdx < nUserArrayLen; ++nWsIdx)
					{
						transaction.m_vecWsItem[nWsIdx].m_nVersion = 0;
						transaction.m_vecWsItem[nWsIdx].m_value = pUserArry[nWsIdx].m_value;
					}
					rsp.m_nReturnCode = VBH::EN_OK_TYPE;
				}
				
				//遍历information信息，准备编码
				for (nWsIdx = 0; nWsIdx < nInfoArrayLen; ++nWsIdx)
				{
					if (VBH::HexDecode(alocKey, pInfoArry[nWsIdx].m_key.GetBuffer(), pInfoArry[nWsIdx].m_key.GetSize()))
					{
						DSC_RUN_LOG_ERROR("decode error.");
						return -1;
					}

					transaction.m_vecWsItem[nUserArrayLen + nWsIdx].m_nAllocatedID = alocKey.m_nAllocatedID;

					pCcWsItem = pSession->FindWriteSet(alocKey.m_nAllocatedID);
					if (pCcWsItem)
					{
						transaction.m_vecWsItem[nUserArrayLen + nWsIdx].m_nVersion = pCcWsItem->m_nVersion;
						transaction.m_vecWsItem[nUserArrayLen + nWsIdx].m_value = pInfoArry[nWsIdx].m_value;
					}
					else
					{
						DSC_RUN_LOG_ERROR("can not find write-set-item.");

						return -1;
					}
				}

				DSC::Encode(transaction, pTransContent, nTransContentLen);

				//1.4.设置rsp中的数据
				rsp.m_transContent.Set(pTransContent, nTransContentLen);
			}

			else
			{
				rsp.m_nReturnCode = VBH::EN_CC_COMMON_ERROR_TYPE;
			}

			this->SendCcMessage(rsp, pSession->m_esAddr);
			this->CancelDscTimer(pSession);
			DSC_THREAD_TYPE_DEALLOCATE(pSession);
			rsp.m_transContent.FreeBuffer();

			return 0;
		}
		else
		{
			DSC_RUN_LOG_ERROR("can not find session, session id:%d", nCcSessionID);

			return -1;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("can not contain none write-set(user or information)");

		return -1;
	}
}



ACE_INT32 CCcBaseService::QueryRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const DSC::CDscBlob& info)
{
	CQuerySession* pSession = m_mapQuerySession.Erase(nCcSessionID);

	if (pSession)
	{
		VBH::CQueryCcEsRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nEsSessionID = pSession->m_nEsSessionID;
		rsp.m_info = info;

		this->SendCcMessage(rsp, pSession->m_esAddr);

		this->CancelDscTimer(pSession); //先停止计时器
		DSC_THREAD_TYPE_DELETE(pSession);

		return 0;
	}
	else
	{
		DSC_RUN_LOG_ERROR("cann't find query session, session id:%d", nCcSessionID);

		return -1;
	}
}

//默认的提案解析仅将提案转换为16进制字符串显示

//void CCcBaseService::FormatProposal(CDscString& strFormatBuf, const ACE_UINT32 nActionID, DSC::CDscShortBlob& proposal)
//{
//	HexFormat(strFormatBuf, proposal);
//}


inline void CCcBaseService::HexFormat(CDscString& strFormatBuf, const DSC::CDscShortBlob& content)
{
	char* pBuf;
	size_t nBufLen = VBH::HexEncode(pBuf, content.GetBuffer(), content.GetSize());

	strFormatBuf.assign(pBuf, nBufLen);

	DSC_THREAD_SIZE_FREE(pBuf, nBufLen);
}


IOuterCcService* CCcBaseService::GetOuterCcService(const ACE_UINT32 nAction) 
{
	return m_mapOuterCcService.Find(1);

}

inline ACE_UINT32 CCcBaseService::AllocSessionID(void)
{
	return ++m_nSessionID ? m_nSessionID : ++m_nSessionID; //规避0
}


ACE_INT32 CCcBaseService::GetWriteSetAtPropose(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	if (pKeyVec && nVecLen)
	{
		CProposalSession* pSession = m_mapProposalSession.Find(nCcSessionID);

		if (pSession)
		{
			auto pItem = DSC_THREAD_TYPE_NEW(typename CCcBaseService::CProposeBatchQueryTask::CItem) typename CCcBaseService::CProposeBatchQueryTask::CItem;

			pItem->m_nWsType = nWsType;
			pItem->m_nCcSessionID = nCcSessionID;
			pItem->m_vecAlocKey.Open(nVecLen);

			//如果可以从session中找到，则使用session中缓存数据,不再查询
			for (ACE_UINT16 idx = 0; idx < nVecLen; ++idx)
			{
				if (VBH::HexDecode(pItem->m_vecAlocKey[idx], pKeyVec[idx].GetBuffer(), pKeyVec[idx].GetSize()))
				{
					DSC_THREAD_TYPE_DELETE(pItem);
					DSC_RUN_LOG_ERROR("decode gen-user-key error.");
					
					return -1;
				}
			}

			m_pBatchQueryTask->m_queueQueryItem.PushBack(pItem);
			++m_pBatchQueryTask->m_nQueryItemCount;

			if (m_pBatchQueryTask->m_nQueryItemCount > m_nBatchQueryMaxSizeValue)
			{
				this->OnTimeBatchQuery();
				this->ResetDscTimer(m_pBatchQueryTask, m_nBatchQueryTimeoutValue);
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("can not find propose session, session id:%d", nCcSessionID);
			
			return -1;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("input param(key-vector) is invalid.");

		return -1;
	}

	return 0;
}


ACE_INT32 CCcBaseService::GetWriteSetAtQuery(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)
{
	if (pKeyVec && nVecLen)
	{
		CQuerySession* pSession = m_mapQuerySession.Find(nCcSessionID);

		if (pSession)
		{
			auto pItem = DSC_THREAD_TYPE_NEW(typename CCcBaseService::CQueryBatchAtQuery) typename CCcBaseService::CQueryBatchAtQuery;

			pItem->m_nWsType = nWsType;
			pItem->m_vecAlocKey.Open(nVecLen);

			//如果可以从session中找到，则使用session中缓存数据,不再查询
			for (ACE_UINT16 idx = 0; idx < nVecLen; ++idx)
			{
				if (VBH::HexDecode(pItem->m_vecAlocKey[idx], pKeyVec[idx].GetBuffer(), pKeyVec[idx].GetSize()))
				{
					DSC_THREAD_TYPE_DELETE(pItem);
					DSC_RUN_LOG_ERROR("decode gen-user-key error.");

					return -1;
				}
			}

			pSession->m_vecQueryBatch.Insert(pItem);

			VBH::CQueryWriteSetListQueryCcCsReq req;

			req.m_nCcSessionID = CQuerySession::CombineID(nCcSessionID, pItem->m_nIndex);
			req.m_vecKey = pItem->m_vecAlocKey;

			if (this->SendDscMessage(req, m_xcsAddr))
			{
				pSession->m_vecQueryBatch.Erase(pItem);
				DSC_THREAD_TYPE_DELETE(pItem);

				DSC_RUN_LOG_ERROR("send dsc message failed.");

				return -1;
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("can not find query session, session id:%d", nCcSessionID);

			return -1;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("input param(key-vector) is invalid.");

		return -1;
	}

	return 0;
}
extern "C" typedef void* (*CreateOuterCC) (ACE_UINT32 nChannelID);

ACE_INT32  CCcBaseService::ReadDBConfig( outer_cc_cfg_list_type& lstOuterCcCfg,  const char* pInnerCcCfgTableName)
{
	CDscDatabase database;
	CDBConnection dbConnection;

	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");
		return -1;
	}


	CTableWrapper< CCollectWrapper<CDBOuterCcCfg> > lstDBInnerCcCfg(pInnerCcCfgTableName);
	CDBOuterCcCfgCriterion dbOuterCcCriterion;



	if (::PerSelect(lstDBInnerCcCfg, database, dbConnection, &dbOuterCcCriterion))
	{
		DSC_RUN_LOG_ERROR("select from %s failed", pInnerCcCfgTableName);
		return -1;
	}


	COuterCcCfg ccCfg;
	for (auto it = lstDBInnerCcCfg->begin(); it != lstDBInnerCcCfg->end(); ++it)
	{
		ccCfg.m_strCcName = *it->m_ccName;
		ccCfg.m_nChannelID = (ACE_UINT16)*it->m_channelID;
		ccCfg.m_nCcID = (ACE_UINT16)*it->m_ccID;

		lstOuterCcCfg.push_back(ccCfg);
	}

	return 0;
}


ACE_INT32  CCcBaseService::LoadInnerCcInDLL(const CDscString& strCcName, IOuterCcService** ppzService, ACE_UINT32 nChannelID)
{
	const char* pszHomeDir = CDscAppManager::Instance()->GetWorkRoot();
	if (!pszHomeDir)
	{
		DSC_RUN_LOG_ERROR("can't get work directory");
		return -1;
	}

	CDscString strCcPathName(pszHomeDir);

	strCcPathName += DSC_FILE_PATH_SPLIT_CHAR;
	strCcPathName.append(DSC_STRING_TYPE_PARAM("plugin"));

	strCcPathName += DSC_FILE_PATH_SPLIT_CHAR;
#if !( defined(ACE_WIN32) || defined(ACE_WIN64) )
	strCcPathName.append(DSC_STRING_TYPE_PARAM("lib"));
#endif
	strCcPathName += strCcName;
#if defined(ACE_WIN32) || defined(ACE_WIN64)
	strCcPathName.append(DSC_STRING_TYPE_PARAM(".dll"));
#else
	strCcPathName.append(DSC_STRING_TYPE_PARAM(".so"));
#endif

	ACE_DLL_Handle* handle = ACE_DLL_Manager::instance()->open_dll(ACE_TEXT(strCcPathName.c_str()), ACE_DEFAULT_SHLIB_MODE, 0);
	if (!handle)
	{
		DSC_RUN_LOG_ERROR("Can't open the plugin:%s, error info:%s.", strCcPathName.c_str(), ACE_OS::dlerror());
		return -1;
	}

	CreateOuterCC pCallBack = reinterpret_cast<CreateOuterCC>(handle->symbol(ACE_TEXT("CreateOuterCC")));
	if (!pCallBack)
	{
		DSC_RUN_LOG_ERROR("Don't find the CreateInnerCC interface of plugin:%s.", strCcPathName.c_str());
		return -1;
	}

	*ppzService = reinterpret_cast<IOuterCcService*>(pCallBack(nChannelID));

	if (!*ppzService)
	{
		DSC_RUN_LOG_ERROR("Can't create plugin:%s.", strCcPathName.c_str());
		return -1;
	}

	return 0;
}
CDscService* COuterCCServiceFactory::CreateDscService(void)
{
	return m_pOuterCCService;
}