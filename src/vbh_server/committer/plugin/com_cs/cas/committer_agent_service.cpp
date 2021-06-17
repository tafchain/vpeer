#include "ace/OS_NS_sys_stat.h"

#include "dsc/configure/dsc_configure.h"
#include "dsc/dsc_log.h"
#include "dsc/codec/dsc_codec/dsc_codec.h"

#include "vbh_comm/vbh_comm_macro_def.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_func.h"

#include "com_cs/cas/committer_agent_service.h"

CCommitterAgentService::CCommitterAgentService(const ACE_UINT32 nChannelID, CDscString strIpAddr, ACE_INT32 nPort)
:m_nChannelID(nChannelID)
, m_strIpAddr(strIpAddr)
, m_nPort(nPort)
{
}

ACE_INT32 CCommitterAgentService::OnInit(void)
{
	//父类初始化
	if (CDscHtsServerService::OnInit())
	{
		DSC_RUN_LOG_ERROR("CCommitterAgentService service init failed!");

		return -1;
	}

	//1. 读取数据中的配置参数
	ACE_INT32 nPeerID;
	if (VBH::GetVbhProfileInt("PEER_ID", nPeerID))
	{
		DSC_RUN_LOG_ERROR("read PEER_ID failed.");
		return -1;
	}
	if (nPeerID < 0)
	{
		DSC_RUN_LOG_ERROR("PEER_ID[%d] value invalid", nPeerID);
		return -1;
	}
	m_nPeerID = (ACE_UINT16)nPeerID;

	//开启hts服务
	m_pAcceptor = DSC_THREAD_TYPE_NEW(CMcpAsynchAcceptor<CCommitterAgentService>) CMcpAsynchAcceptor<CCommitterAgentService>(*this);
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

	return 0;
}

ACE_INT32 CCommitterAgentService::OnExit(void)
{
	if (m_pAcceptor)
	{
		this->UnRegistHandler(m_pAcceptor, ACE_Event_Handler::ALL_EVENTS_MASK | ACE_Event_Handler::DONT_CALL);
		m_pAcceptor->ReleaseServiceHandler();
	}

	return CDscHtsServerService::OnExit();
}


ACE_INT32 CCommitterAgentService::OnHtsMsg(VBH::CGetBlockCsCasReq& rGetBlockReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterAgentService, CGetBlockCsCasReq);

	VBH::CGetBlockCasCsRsp rsp;
	ACE_UINT32 nBlockDataLen;
	char* pBlockData = m_pCommitterService->ReadBlock(nBlockDataLen, rGetBlockReq.m_nBlockID);

	if (pBlockData)
	{
		rsp.m_nReturnCode = VBH::EN_OK_TYPE;
		rsp.m_blockData.Set(pBlockData, nBlockDataLen);
	}
	else
	{
		rsp.m_nReturnCode = VBH::EN_MAX_COMMON_ERROR_VALUE;
	}
	rsp.m_nBlockID = rGetBlockReq.m_nBlockID;

	if (this->SendHtsMsg(rsp, pMcpHandler))
	{
		DSC_RUN_LOG_ERROR("send hts message:cgetblockxasxcsrsp failed, block id:%d", rGetBlockReq.m_nBlockID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCommitterAgentService, CGetBlockCsCasReq);

	return 0;
}

ACE_INT32 CCommitterAgentService::OnHtsMsg(VBH::CCheckBlockHashCsCasReq& rCheckHashReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterAgentService, CCheckBlockHashCsCasReq);

	char blockHash[VBH_BLOCK_DIGEST_LENGTH];
	VBH::CCheckBlockHashCasCsRsp rsp;

	ACE_UINT32 nBlockDataLen;
	char* pBlockData = m_pCommitterService->ReadBlock(nBlockDataLen, rCheckHashReq.m_nBlockID);

	if (nullptr == pBlockData)
	{
		rsp.m_nReturnCode = VBH::EN_MAX_COMMON_ERROR_VALUE;
		DSC_RUN_LOG_ERROR("GetBlock failed, block id:%d", rCheckHashReq.m_nBlockID);
	}
	else
	{
		VBH::vbhDigest(pBlockData, nBlockDataLen, blockHash);
		rsp.m_nReturnCode = VBH::EN_OK_TYPE;

		if (DSC_UNLIKELY(memcmp(blockHash, rCheckHashReq.m_strBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH)))
		{
			rsp.m_nBlockID = rCheckHashReq.m_nBlockID;
			rsp.m_nCheckResult = VBH::CCheckBlockHashCasCsRsp::EN_HASH_CHECK_FAILED;
			rsp.m_blockData.Set(pBlockData, nBlockDataLen);
		}
		else
		{
			rsp.m_nBlockID = rCheckHashReq.m_nBlockID;
			rsp.m_nCheckResult = VBH::CCheckBlockHashCasCsRsp::EN_HASH_CHECK_OK;
		}
	}

	if (this->SendHtsMsg(rsp, pMcpHandler))
	{
		DSC_RUN_LOG_ERROR("send hts message:CCheckBlockHashXasXcsRsp failed, block id:%d", rCheckHashReq.m_nBlockID);
	}

	VBH_MESSAGE_LEAVE_TRACE(CCommitterAgentService, CCheckBlockHashCsCasReq);

	return 0;
}

ACE_INT32 CCommitterAgentService::OnHtsMsg(VBH::CVerifyPeerStateCsCasReq& rVerifyReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterAgentService, CVerifyPeerStateCsCasReq);

	ACE_UINT32 nReturnCode;
	char blockHash[VBH_BLOCK_DIGEST_LENGTH];

	nReturnCode = m_pCommitterService->ReadBlockHash(blockHash, rVerifyReq.m_nKafkaBlockID);
	if (VBH::EN_OK_TYPE != nReturnCode)
	{
		//区块获取失败不回消息
		DSC_RUN_LOG_ERROR("GetBlockHash failed, block id:%d", rVerifyReq.m_nKafkaBlockID);
	}
	else
	{
		if (DSC_UNLIKELY(memcmp(blockHash, rVerifyReq.m_strKafkaBlockHash.c_str(), VBH_BLOCK_DIGEST_LENGTH)))
		{
			//hash校验失败不回消息
		}
		else
		{
			VBH::CVerifyPeerStateCasCsRsp rsp;
			rsp.m_nReturnCode = VBH::EN_OK_TYPE;
			rsp.m_nVerifyResult = VBH::CVerifyPeerStateCasCsRsp::EN_VERIFY_OK;
			rsp.m_CasAddress.m_nPeerID = m_nPeerID;
			rsp.m_CasAddress.m_nPort = m_nPort;
			rsp.m_CasAddress.m_strIpAddr = m_strIpAddr;

			if (this->SendHtsMsg(rsp, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CVerifyPeerStateCasCsRsp failed, block id:%d", rVerifyReq.m_nKafkaBlockID);
			}
			
		}
	}

	VBH_MESSAGE_LEAVE_TRACE(CCommitterAgentService, CVerifyPeerStateCsCasReq);

	return 0;
}
