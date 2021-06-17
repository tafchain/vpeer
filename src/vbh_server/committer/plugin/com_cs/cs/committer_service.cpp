#include "ace/OS_NS_sys_stat.h"

#include "dsc/configure/dsc_configure.h"
#include "dsc/dsc_log.h"
#include "dsc/codec/dsc_codec/dsc_codec.h"

#include "vbh_comm/vbh_comm_macro_def.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_func.h"
#include "vbh_server_comm/vbh_block_codec.h"
#include "vbh_server_comm/vbh_const_length_storage/vbh_cls_config_param.h"

#include "com_cs/cs/committer_service.h"
 

class CCpsAddr
{
public:
	CCpsAddr()
		: m_ipAddr("CPS_IP_ADDR")
		, m_port("CPS_PORT")
	{
	}

public:
	PER_BIND_ATTR(m_ipAddr, m_port);

public:
	CColumnWrapper< CDscString > m_ipAddr;
	CColumnWrapper< ACE_INT32 > m_port;
};

class CCpsAddrCriterion : public CSelectCriterion
{
public:
	CCpsAddrCriterion(const ACE_UINT32 nChannelID)
		: m_nChannelID(nChannelID)
	{
	}

public:
	virtual void SetCriterion(CPerSelect& rPerSelect) override
	{
		rPerSelect.Where(rPerSelect["CH_ID"] == m_nChannelID);
	}

private:
	const ACE_UINT32 m_nChannelID;
};

CCommitterService::CQuerySyncSourcePeerSession::CQuerySyncSourcePeerSession(CCommitterService& rService)
	: m_rXCommitterService(rService)
{
}

CCommitterService::CVerifyPeerStateSession::CVerifyPeerStateSession(CCommitterService& rService)
	: m_rXCommitterService(rService)
{
}

CCommitterService::CSyncBlockSession::CSyncBlockSession(CCommitterService& rService)
	: m_rXCommitterService(rService)
{
}

CCommitterService::CUnresolvedBlock::CUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block)
	: m_nBlockID(nBlockID)
{
	m_vbhBlock.Clone(block);
}

CCommitterService::CUnresolvedBlock::~CUnresolvedBlock()
{
	m_vbhBlock.FreeBuffer();
}

CCommitterService::CCommitterService(const ACE_UINT32 nChannelID, CDscString strCasIpAddr, ACE_UINT16 nCasPort)
	: m_nChannelID(nChannelID)
	, m_strCasIpAddr(strCasIpAddr)
	, m_nCasPort(nCasPort)
{
}

ACE_INT32 CCommitterService::OnInit(void)
{
	if (CDscHtsClientService::OnInit())
	{
		DSC_RUN_LOG_ERROR("x committer service init failed!");
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

	ACE_INT32 nMaxUnresolvedBlockCacheCount;
	if (VBH::GetVbhProfileInt("MAX_UNRESOLVED_BLOCK_COUNT", nMaxUnresolvedBlockCacheCount))
	{
		DSC_RUN_LOG_ERROR("read MAX_UNRESOLVED_BLOCK_COUNT failed.");
		return -1;
	}
	if (nMaxUnresolvedBlockCacheCount < 0)
	{
		DSC_RUN_LOG_ERROR("MAX_UNRESOLVED_BLOCK_COUNT[%d] value invalid", nMaxUnresolvedBlockCacheCount);
		return -1;
	}
	m_nMaxUnresolvedBlockCacheCount = (ACE_UINT32)nMaxUnresolvedBlockCacheCount;

	ACE_INT32 nMaxCachedBlockCount;
	if (VBH::GetVbhProfileInt("MAX_CACHED_BLOCK_COUNT", nMaxCachedBlockCount))
	{
		DSC_RUN_LOG_ERROR("read MAX_CACHED_BLOCK_COUNT failed.");
		return -1;
	}
	if (nMaxCachedBlockCount < 0)
	{
		DSC_RUN_LOG_ERROR("MAX_CACHED_BLOCK_COUNT[%d] value invalid", nMaxCachedBlockCount);
		return -1;
	}
	m_nMaxCachedBlockCount = (ACE_UINT32)nMaxCachedBlockCount;

	//2. 读取cps地址
	CDscDatabase database;
	CDBConnection dbConnection;
	dsc_vector_type(PROT_COMM::CDscIpAddr) vecCpsAddr; //cps地址列表
	PROT_COMM::CDscIpAddr addr;

	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");
		return -1;
	}
	else
	{
		CTableWrapper< CCollectWrapper<CCpsAddr> > lstCollectCpsAddr("CS_USE_CPS_CFG");
		CCpsAddrCriterion cpsAddrCriterion(m_nChannelID);

		if (::PerSelect(lstCollectCpsAddr, database, dbConnection, &cpsAddrCriterion))
		{
			DSC_RUN_LOG_ERROR("select from CS_USE_CPS_CFG failed");

			return -1;
		}

		for (auto it = lstCollectCpsAddr->begin(); it != lstCollectCpsAddr->end(); ++it)
		{
			addr.SetIpAddr(*it->m_ipAddr);
			addr.SetPort(*it->m_port);

			vecCpsAddr.push_back(addr);
		}
	}

	//3. 加载配置文件
	ACE_stat stat;
	CDscString strBasePath(CDscAppManager::Instance()->GetWorkRoot());//基础路径： $(WORK_ROOT)/storage/channel_x/cs/
	CDscString strCfgFilePath;

	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += "storage";
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += "channel_";
	strBasePath += m_nChannelID;
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += "committer";
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strCfgFilePath = strBasePath;
	strCfgFilePath += "cs.cfg";

	//3.1 如果路径不存在，则创建路径，同时创建相应的 配置 文件
	if (-1 == ACE_OS::stat(strBasePath.c_str(), &stat))
	{
		CCsConfig csCfg;

		//创建基础路径
		if (-1 == DSC::DscRecurMkdir(strBasePath.c_str(), strBasePath.size()))
		{
			DSC_RUN_LOG_ERROR("create needed directory failed.%s", strBasePath.c_str());
			return -1;
		}

		//创建 配置文件
		if (VBH::CreateCfgFile(strCfgFilePath, csCfg))
		{
			DSC_RUN_LOG_ERROR("create file:%s failed.", strCfgFilePath.c_str());
			return -1;
		}
	}

	//3.2 加载配置
	if (VBH::LoadMmapCfgFile(strCfgFilePath, m_shmCfg, m_pCsCfg))
	{
		return -1;
	}

	//4 打开各种存储设备

	//4.1 vbfs
	if (m_vbfs.Open(m_nChannelID))
	{
		DSC_RUN_LOG_ERROR("open vbfs failed, channel:%u", m_nChannelID);

		return -1;
	}

	//4.3 user-index-db
	if (m_wsIndexTable.Open(&m_vbfs, m_nChannelID, VBH_CLS::EN_WRITE_SET_INDEX_TABLE_TYPE))
	{
		DSC_RUN_LOG_ERROR("user index table open falie, channel:%u.", m_nChannelID);
		return -1;
	}

	//4.4 user-history-db
	if (m_wsHistTable.Open(&m_vbfs, m_nChannelID, VBH_CLS::EN_WRITE_SET_HISTORY_TABLE_TYPE))
	{
		DSC_RUN_LOG_ERROR("user history table open falie, channel:%u.", m_nChannelID);
		return -1;
	}

	//4.7 block-chain
	if (m_bcTable.Open(&m_vbfs, m_nChannelID))
	{
		DSC_RUN_LOG_ERROR("block chain open falie, channel:%u.", m_nChannelID);
		return -1;
	}

	//5. 如果有日志的脏标记，则从日志中恢复数据
	if (m_pCsCfg->m_nModifyStorageState == EN_BEGIN_MODIFY_STORAGE)
	{
		if (AllTableRecoverFromLog())
		{
			DSC_RUN_LOG_ERROR("recover from log failed.");
			return -1;
		}

		m_pCsCfg->m_nModifyStorageState = EN_END_MODIFY_STORAGE;
	}

	//6. 连接本channel的所有cps
	for (auto& it : vecCpsAddr)
	{
		ACE_UINT32 nHandleID = this->AllocHandleID();

		m_vecCpsHandleID.push_back(nHandleID);
		this->DoConnect(it, NULL, nHandleID);
	}

	DSC_RUN_LOG_INFO("committer service %d init succeed", this->GetID());

	return 0;
}


ACE_INT32 CCommitterService::OnExit(void)
{
	m_shmCfg.close();

	m_vbfs.Close();
	m_wsIndexTable.Close();
	m_wsHistTable.Close();
	m_bcTable.Close();

	//释放区块缓存 //从1个容器入手释放即可
	CMemBlockPtrInfo* pBlockPtrInfo = m_dequeBlockCache.PopFront();
	while (pBlockPtrInfo)
	{
		DSC_THREAD_TYPE_DELETE(pBlockPtrInfo);

		pBlockPtrInfo = m_dequeBlockCache.PopFront();
	}

	return CDscHtsClientService::OnExit();
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CDistributeBlockCpsCsReq& rDistBlockReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CDistributeBlockCpsCsReq);

	if (!m_pSyncBlockSession && !m_pVerifyPeerStateSession && !m_pQuerySyncSourcePeerSession) //正常状态，没有同步之类的事务在进行
	{
		if ((m_pCsCfg->m_nLastBlockID + 1) == rDistBlockReq.m_nBlockID) //块ID匹配
		{
			if (OnReceiveBlock(rDistBlockReq.m_nBlockID, rDistBlockReq.m_vbhBlock) == VBH::EN_OK_TYPE)
			{
				//发送应答
				VBH::CDistributeBlockXcsCpsRsp rsp;

				rsp.m_nPeerID = m_nPeerID;
				rsp.m_nBlockID = rDistBlockReq.m_nBlockID;

				if (this->SendHtsMsg(rsp, pMcpHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CDistributeBlockXasCpsRsp failed, channel-id:%d", m_nChannelID);
				}
			}
		}
		else if(rDistBlockReq.m_nBlockID <= m_pCsCfg->m_nLastBlockID) //重发的区块，要应答
		{
			//发送应答
			VBH::CDistributeBlockXcsCpsRsp rsp;

			rsp.m_nPeerID = m_nPeerID;
			rsp.m_nBlockID = rDistBlockReq.m_nBlockID;

			if (this->SendHtsMsg(rsp, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message:CDistributeBlockXasCpsRsp failed, channel-id:%d", m_nChannelID);
			}
		}
		else //有洞，要触发同步
		{
			//启动同步
			VBH_TRACE_MESSAGE("CDistributeBlockCpsCsReq,rDistBlockReq nBlockID:%d ,nLastBlockID:%d.\n", rDistBlockReq.m_nBlockID, m_pCsCfg->m_nLastBlockID);
			VBH_TRACE_MESSAGE("SendHtsMsg: CQuerySyncSourcePeerCsCpsReq.\n");

			VBH::CQuerySyncSourcePeerCsCpsReq req;
		
			req.m_nPeerID = m_nPeerID;
			req.m_nTargetBlockID = rDistBlockReq.m_nBlockID - 1;

			if (SendHtsMsg(req, pMcpHandler))
			{
				DSC_RUN_LOG_ERROR("send hts message: CQuerySyncSourcePeerXcsCpsReq  failed");
			}
			else
			{
				m_pQuerySyncSourcePeerSession = DSC_THREAD_TYPE_NEW(CQuerySyncSourcePeerSession) CQuerySyncSourcePeerSession(*this);
				m_pQuerySyncSourcePeerSession->m_nTargetBlockId = rDistBlockReq.m_nBlockID - 1;
				m_pQuerySyncSourcePeerSession->m_nHandleId = pMcpHandler->GetHandleID();

				this->SetDscTimer(m_pQuerySyncSourcePeerSession, EN_SESSION_TIMEOUT_VALUE, true);
			}

			//块差值在一定范围内且连续则缓存
			CacheUnresolvedBlock(rDistBlockReq.m_nBlockID, rDistBlockReq.m_vbhBlock);
		}
	}
	else //有在同步的状态
	{
		//块差值在一定范围内且连续则缓存
		CacheUnresolvedBlock(rDistBlockReq.m_nBlockID, rDistBlockReq.m_vbhBlock);
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CDistributeBlockCpsCsReq);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CSyncBlockOnRegistCpsCsNotify& rSyncNotify, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CSyncBlockOnRegistCpsCsNotify);

	if (!m_pSyncBlockSession && !m_pVerifyPeerStateSession && !m_pQuerySyncSourcePeerSession)
	{
		VBH_TRACE_MESSAGE("CSyncBlockOnRegistCpsCsNotify,KafkaBlockID:%d ,nLastBlockID:%d.\n", rSyncNotify.m_nKafkaBlockID, m_pCsCfg->m_nLastBlockID);
		if (rSyncNotify.m_nKafkaBlockID >= m_pCsCfg->m_nLastBlockID)
		{
			if (m_pCsCfg->m_nLastBlockID != 0)
			{
				char blockHash[VBH_BLOCK_DIGEST_LENGTH];

				if (m_bcTable.ReadBlockHash(blockHash, m_pCsCfg->m_nLastBlockID))
				{
					//TODO:磁盘操作失败
					DSC_RUN_LOG_ERROR("getblockhash failed block-id :lld%", m_pCsCfg->m_nLastBlockID);
				}
				else
				{
					m_pSyncBlockSession = DSC_THREAD_TYPE_NEW(CSyncBlockSession) CSyncBlockSession(*this);
					m_pSyncBlockSession->m_nTargetBlockID = rSyncNotify.m_nKafkaBlockID;
					m_pSyncBlockSession->m_bIsCheckBlockHash = false;
					

					VBH::CCheckBlockHashCsCasReq req;
					PROT_COMM::CDscIpAddr addr(rSyncNotify.m_strSyncSrcIpAddr, rSyncNotify.m_nSyncSrcPort);

					req.m_nBlockID = m_pCsCfg->m_nLastBlockID;
					req.m_strBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH);
					VBH_TRACE_MESSAGE("SendHtsMsg:CCheckBlockHashCsCasReq,KafkaBlockID:%d ,blockHash:%s.\n", rSyncNotify.m_nKafkaBlockID, blockHash);

					if (this->SendHtsMsg(m_pSyncBlockSession->m_nHandleId, req, addr))
					{
						DSC_RUN_LOG_ERROR("send CheckBlockHashCsCasReq failed");
					}
					this->SetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);
				}
			}
			//如果本地一个区块都没有就不需要校验hash了，直接拉取区块
			else
			{
				m_pSyncBlockSession = DSC_THREAD_TYPE_NEW(CSyncBlockSession) CSyncBlockSession(*this);
				m_pSyncBlockSession->m_nTargetBlockID = rSyncNotify.m_nKafkaBlockID;
				m_pSyncBlockSession->m_nHandleId = this->AllocHandleID();
				m_pSyncBlockSession->m_bIsCheckBlockHash = true;
				this->SetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);

				PROT_COMM::CDscIpAddr addr(rSyncNotify.m_strSyncSrcIpAddr, rSyncNotify.m_nSyncSrcPort);

				this->DoConnect(addr, NULL, m_pSyncBlockSession->m_nHandleId);
				VBH_TRACE_MESSAGE("DoSyncBlock because KafkaBlockID:%d.\n", m_pCsCfg->m_nLastBlockID);
				DoSyncBlock();
			}
		}
		else
		{
			//TODO: 给网管报告警
			DSC_RUN_LOG_ERROR("receive CSyncBlockOnRegistCpsCsNotify with error block block-id :%lld, local-block-id:%lld", rSyncNotify.m_nKafkaBlockID, m_pCsCfg->m_nLastBlockID);
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("already sync block...");
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CSyncBlockOnRegistCpsCsNotify);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify& rSyncNotify, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify);

	if (!m_pSyncBlockSession && !m_pVerifyPeerStateSession && !m_pQuerySyncSourcePeerSession)
	{
		if (m_pCsCfg->m_nLastBlockID <= rSyncNotify.m_nKafkaBlockID)
		{
			m_pVerifyPeerStateSession = DSC_THREAD_TYPE_NEW(CVerifyPeerStateSession) CVerifyPeerStateSession(*this);
			m_pVerifyPeerStateSession->m_nKafkaBlockID = rSyncNotify.m_nKafkaBlockID;
			m_pVerifyPeerStateSession->m_nTargetBlockId = rSyncNotify.m_nKafkaBlockID;
			m_pVerifyPeerStateSession->m_strKafkaBlockHash = rSyncNotify.m_strKafkaBlockHash;

			VBH_TRACE_MESSAGE("SendVerifyPeerStateReq,KafkaBlockID:%d ,strKafkaBlockHash:%s.\n", rSyncNotify.m_nKafkaBlockID, rSyncNotify.m_strKafkaBlockHash);
			for (auto& it : rSyncNotify.m_lstPeerAddr)
			{
				if (it.m_nPeerID != m_nPeerID)
				{
					SendVerifyPeerStateReq(rSyncNotify.m_nKafkaBlockID, rSyncNotify.m_strKafkaBlockHash, it);
				}
			}

			this->SetDscTimer(m_pVerifyPeerStateSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);
		}
		else
		{
			DSC_RUN_LOG_ERROR("receive CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify with block-id:%lld, local-max-block-id:%lld", rSyncNotify.m_nKafkaBlockID, m_pCsCfg->m_nLastBlockID)
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("already sync block...");
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CBackspaceBlockCpsCsNotify& rBackspaceBlockCpsXcsNotify, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CBackspaceBlockCpsCsNotify);

	if (m_pCsCfg->m_nLastBlockID == (rBackspaceBlockCpsXcsNotify.m_nKafkaBlockID + 1))
	{
		VBH_TRACE_MESSAGE("CBackspaceBlockCpsCsNotify:.OnBackspaceBlock()\n");
		if (OnBackspaceBlock())
		{
			DSC_RUN_LOG_WARNING("backspace block process error, register block id:%lld, kafka block id:%lld", rBackspaceBlockCpsXcsNotify.m_nRegisterBlockID, rBackspaceBlockCpsXcsNotify.m_nKafkaBlockID);
		}
		else
		{
			SendRegistReq(pMcpHandler->GetHandleID());
		}
	}
	else
	{
		//TODO: 给网管报告警
		DSC_RUN_LOG_WARNING("backspace block msg id error, register block id:%lld, kafka block id:%lld", rBackspaceBlockCpsXcsNotify.m_nRegisterBlockID, rBackspaceBlockCpsXcsNotify.m_nKafkaBlockID);
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CBackspaceBlockCpsCsNotify);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CQuerySyncSourcePeerCpsCsRsp& rQuerySyncRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQuerySyncSourcePeerCpsCsRsp);

	if (m_pQuerySyncSourcePeerSession)
	{
		ACE_UINT64 nTargetBlockId = m_pQuerySyncSourcePeerSession->m_nTargetBlockId;

		this->CancelDscTimer(m_pQuerySyncSourcePeerSession);
		DSC_THREAD_TYPE_DELETE(m_pQuerySyncSourcePeerSession);
		m_pQuerySyncSourcePeerSession = nullptr;

		ACE_ASSERT(m_pSyncBlockSession);
		ACE_ASSERT(m_pVerifyPeerStateSession);

		m_pVerifyPeerStateSession = DSC_THREAD_TYPE_NEW(CVerifyPeerStateSession) CVerifyPeerStateSession(*this);
		m_pVerifyPeerStateSession->m_nKafkaBlockID = rQuerySyncRsp.m_nKafkaBlockID;
		m_pVerifyPeerStateSession->m_nTargetBlockId = nTargetBlockId;
		m_pVerifyPeerStateSession->m_strKafkaBlockHash = rQuerySyncRsp.m_strKafkaBlockHash;
		
		this->SetDscTimer(m_pVerifyPeerStateSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);
		VBH_TRACE_MESSAGE("SendVerifyPeerStateReq,KafkaBlockID:%d ,strKafkaBlockHash:%s.\n", rQuerySyncRsp.m_nKafkaBlockID, rQuerySyncRsp.m_strKafkaBlockHash);

		for (auto& it : rQuerySyncRsp.m_lstPeerAddr)
		{
			SendVerifyPeerStateReq(rQuerySyncRsp.m_nKafkaBlockID, rQuerySyncRsp.m_strKafkaBlockHash, it);
		}
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQuerySyncSourcePeerCpsCsRsp);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CVerifyPeerStateCasCsRsp& rVerifyPeerStateCasCsRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CVerifyPeerStateCasCsRsp);

	if (m_pVerifyPeerStateSession)
	{
		if ((rVerifyPeerStateCasCsRsp.m_nReturnCode == VBH::EN_OK_TYPE)
			&& (rVerifyPeerStateCasCsRsp.m_nVerifyResult == VBH::CVerifyPeerStateCasCsRsp::EN_VERIFY_OK))
		{
			ACE_ASSERT(!m_pSyncBlockSession);

			ACE_UINT32 nTargetBlockId = m_pVerifyPeerStateSession->m_nTargetBlockId; //同步的目标高度
			
			for (auto& it : m_pVerifyPeerStateSession->m_vecCasConnHandleId)//断开其他的连接
			{
				if (pMcpHandler->GetHandleID() != it)
				{
					this->DisConnect(it);
				}
			}
			this->CancelDscTimer(m_pVerifyPeerStateSession);
			DSC_THREAD_TYPE_DELETE(m_pVerifyPeerStateSession);
			m_pVerifyPeerStateSession = nullptr;


			if (m_pCsCfg->m_nLastBlockID != 0)
			{
				//发送校验本地最高块的hash
				char blockHash[VBH_BLOCK_DIGEST_LENGTH];

				if (m_bcTable.ReadBlockHash(blockHash, m_pCsCfg->m_nLastBlockID))
				{
					//TODO: 磁盘失败，校验失败，向网管上报告警，人工介入，断开和order的连接
					DSC_RUN_LOG_ERROR("getblockhash failed block-id :lld%", m_pCsCfg->m_nLastBlockID);
				}
				else
				{
					m_pSyncBlockSession = DSC_THREAD_TYPE_NEW(CSyncBlockSession) CSyncBlockSession(*this);
					m_pSyncBlockSession->m_nTargetBlockID = nTargetBlockId;
					m_pSyncBlockSession->m_nHandleId = pMcpHandler->GetHandleID();
					m_pSyncBlockSession->m_bIsCheckBlockHash = false;
					this->SetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);

					VBH::CCheckBlockHashCsCasReq req;
					VBH_TRACE_MESSAGE("SendHtsMsg:CCheckBlockHashCsCasReq,nLastBlockID:%d ,blockHash:%s.\n", m_pCsCfg->m_nLastBlockID, blockHash);

					req.m_nBlockID = m_pCsCfg->m_nLastBlockID;
					req.m_strBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH);
					if (this->SendHtsMsg(req, m_pSyncBlockSession->m_nHandleId))
					{
						DSC_RUN_LOG_ERROR("send hts message:CCheckBlockHashCsCasReq failed");
					}
				}
			}
			//如果本地一个区块都没有就不需要校验hash了，直接拉取区块
			else
			{
				m_pSyncBlockSession = DSC_THREAD_TYPE_NEW(CSyncBlockSession) CSyncBlockSession(*this);
				m_pSyncBlockSession->m_nTargetBlockID = nTargetBlockId;
				m_pSyncBlockSession->m_nHandleId = pMcpHandler->GetHandleID();
				m_pSyncBlockSession->m_bIsCheckBlockHash = true;
				this->SetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE, true);
				VBH_TRACE_MESSAGE("DoSyncBlock,nLastBlockID:%d .\n", m_pCsCfg->m_nLastBlockID);
				DoSyncBlock();
			}
		}
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CVerifyPeerStateCasCsRsp);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CCheckBlockHashCasCsRsp& rCheckBlockHashCasCsRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CCheckBlockHashCasCsRsp);

	if (m_pSyncBlockSession)
	{
		if (rCheckBlockHashCasCsRsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			VBH_TRACE_MESSAGE("CCheckBlockHashCasCsRsp,CheckResult:%d .\n", rCheckBlockHashCasCsRsp.m_nCheckResult);
			if (rCheckBlockHashCasCsRsp.m_nCheckResult == VBH::CCheckBlockHashCasCsRsp::EN_HASH_CHECK_FAILED)
			{
				//校验失败，先退块，再落块
				if (OnBackspaceBlock() != VBH::EN_OK_TYPE)
				{
					//TODO: 磁盘失败动作
					DSC_RUN_LOG_ERROR("OnBackspaceBlock failed block id:%lld ", rCheckBlockHashCasCsRsp.m_nBlockID);
				}
				else
				{
					VBH_TRACE_MESSAGE("OnBackspaceBlock,Sucess:%d .\n");
					if (OnReceiveBlock(rCheckBlockHashCasCsRsp.m_nBlockID, rCheckBlockHashCasCsRsp.m_blockData) != VBH::EN_OK_TYPE)
					{
						//TODO: 磁盘操作失败
						DSC_RUN_LOG_ERROR("save block failed, block-id:%lld.", rCheckBlockHashCasCsRsp.m_nBlockID);
					}
				}
			}

			//进入同步流程
			ResetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE);
			m_pSyncBlockSession->m_bIsCheckBlockHash = true;

			DoSyncBlock();
		}
		else
		{
			DSC_RUN_LOG_ERROR("CCheckBlockHashCasCsRsp return failed, error-string:%s", VBH::GetErrorString(rCheckBlockHashCasCsRsp.m_nReturnCode));
			this->DisConnect(m_pSyncBlockSession->m_nHandleId);
			this->CancelDscTimer(m_pSyncBlockSession);
			DSC_THREAD_TYPE_DELETE(m_pSyncBlockSession);
			m_pSyncBlockSession = nullptr;
		}

	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CCheckBlockHashCasCsRsp);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CGetBlockCasCsRsp& rGetBlockCasCsRsp, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CGetBlockCasCsRsp);

	if (m_pSyncBlockSession)
	{
		if (rGetBlockCasCsRsp.m_nReturnCode == VBH::EN_OK_TYPE)
		{
			// 一次只拉一个块，这里块号一定连续
			if ((m_pCsCfg->m_nLastBlockID + 1) == rGetBlockCasCsRsp.m_nBlockID)
			{
				if (OnReceiveBlock(rGetBlockCasCsRsp.m_nBlockID, rGetBlockCasCsRsp.m_blockData) == VBH::EN_OK_TYPE)
				{
					DoSyncBlock();
				}
				else
				{
					//TODO: 磁盘操作失败
					DSC_RUN_LOG_ERROR("save block failed.");
				}
			}
			else
			{
				DSC_RUN_LOG_ERROR("receive un-continue block, block-id:%lld, local-max-block-id:%lld", rGetBlockCasCsRsp.m_nBlockID, m_pCsCfg->m_nLastBlockID);
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get block fail,BlockID:%d", rGetBlockCasCsRsp.m_nBlockID);
			this->DisConnect(m_pSyncBlockSession->m_nHandleId);
			this->CancelDscTimer(m_pSyncBlockSession);
			DSC_THREAD_TYPE_DELETE(m_pSyncBlockSession);
			m_pSyncBlockSession = nullptr;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("receive CGetBlockCasCsRsp when m_pSyncBlockSession==null.");
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CGetBlockCasCsRsp);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CMasterSyncVersionTableCpsCsReq& rSyncVersionTableReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CMasterSyncVersionTableCpsCsReq);

	DSC_RUN_LOG_INFO("receive CMasterSyncVersionTableCpsCsReq start block id:%lld ", rSyncVersionTableReq.m_nBlockID);

	if (rSyncVersionTableReq.m_nBlockID > m_pCsCfg->m_nLastBlockID)
	{
		DSC_RUN_LOG_ERROR("master-sync-version-block-id :%lld,  last-block-id: %lld", rSyncVersionTableReq.m_nBlockID, m_pCsCfg->m_nLastBlockID);
	}
	else
	{
		VBH::CMasterSyncVersionTableCsCpsRsp rsp;
		char blockHash[VBH_BLOCK_DIGEST_LENGTH];
		DSC_RUN_LOG_INFO("GetBlockKvLst  start ");

		if (GetBlockKvLst(rSyncVersionTableReq.m_nBlockID, rsp.m_lstKv))
		{
			//TODO: 错误处理
			DSC_RUN_LOG_ERROR("GetBlockKvLst failed block id:%lld ", rSyncVersionTableReq.m_nBlockID);
		}
		else
		{
			DSC_RUN_LOG_INFO("ReadBlockHash  start ");
			if (m_bcTable.ReadBlockHash(blockHash, rSyncVersionTableReq.m_nBlockID))
			{
				//TODO: 错误处理
				DSC_RUN_LOG_ERROR("GetBlockHash failed block id:%lld ", rSyncVersionTableReq.m_nBlockID);
			}
			else
			{
				rsp.m_nBlockID = rSyncVersionTableReq.m_nBlockID;
				rsp.m_strBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH);

				if (this->SendHtsMsg(rsp, pMcpHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CMasterSyncVersionTableXcsCpsRsp failed, block-id:%d", rSyncVersionTableReq.m_nBlockID);
				}
			}
		}
	}

	ShrinkBlockCache();
	DSC_RUN_LOG_INFO("GetBlockKvLst end block id:%lld ", rSyncVersionTableReq.m_nBlockID);
	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CMasterSyncVersionTableCpsCsReq);

	return 0;
}

ACE_INT32 CCommitterService::OnHtsMsg(VBH::CSlaveSyncVersionTableCpsCsReq& rSyncVersionTableReq, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CSlaveSyncVersionTableCpsCsReq);

	if (rSyncVersionTableReq.m_nBlockID > (m_pCsCfg->m_nLastBlockID - 1))
	{
		DSC_RUN_LOG_ERROR("slave-sync-version-block-id :%lld,  last-block-id: %lld", rSyncVersionTableReq.m_nBlockID, m_pCsCfg->m_nLastBlockID);
	}
	else
	{
		VBH::CSlaveSyncVersionTableCsCpsRsp rsp;
		char blockHash[VBH_BLOCK_DIGEST_LENGTH];

		if (GetBlockKvLst(rSyncVersionTableReq.m_nBlockID, rsp.m_lstKv))
		{
			//TODO: 错误处理
			DSC_RUN_LOG_ERROR("GetBlockKvLst failed block id:%lld ", rSyncVersionTableReq.m_nBlockID);
		}
		else
		{
			if (m_bcTable.ReadBlockHash(blockHash, rSyncVersionTableReq.m_nBlockID))
			{
				//TODO: 错误处理
				DSC_RUN_LOG_ERROR("GetBlockHash failed block id:%lld ", rSyncVersionTableReq.m_nBlockID);
			}
			else
			{
				rsp.m_nBlockID = rSyncVersionTableReq.m_nBlockID;
				rsp.m_strBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH);

				if (this->SendHtsMsg(rsp, pMcpHandler))
				{
					DSC_RUN_LOG_ERROR("send hts message:CMasterSyncVersionTableXcsCpsRsp failed, block-id:%d", rSyncVersionTableReq.m_nBlockID);
				}
			}
		}
	}

	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CSlaveSyncVersionTableCpsCsReq);

	return 0;
}


ACE_INT32 CCommitterService::OnHtsMsg(VBH::CInvalidPeerCpsCsNotify& rInvalidPeerCpsCsNotify, CMcpHandler* pMcpHandler)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CInvalidPeerCpsCsNotify);

	/*TODO:需要上报告警*/

	this->DisConnect(pMcpHandler->GetHandleID());

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CInvalidPeerCpsCsNotify);
	return 0;
}


ACE_INT32 CCommitterService::OnHtsMsg(VBH::CQueryMaxBlockInfoCpsCsReq& rQueryMaxBlockInfoCpsCsReq, CMcpHandler* pMcpHandler)
{

	char blockHash[VBH_BLOCK_DIGEST_LENGTH];

	if (m_pCsCfg->m_nLastBlockID >= 1) //是有效的区块ID
	{
		if (m_bcTable.ReadBlockHash(blockHash, m_pCsCfg->m_nLastBlockID))
		{
			//TODO: 磁盘错误
			DSC_RUN_LOG_ERROR("GetBlockHash failed last-block-id: lld%", m_pCsCfg->m_nLastBlockID);
			return 0;
		}
	}
	else
	{
		memset(blockHash, 0, VBH_BLOCK_DIGEST_LENGTH);
	}

	VBH::CQueryMaxBlockInfoCsCpsRsp rsp;

	rsp.m_nPeerID = m_nPeerID;
	rsp.m_strCasIpAddr = m_strCasIpAddr; //代理服务侦听的IP地址
	rsp.m_nCasPort = m_nCasPort; //代理服务侦听的端口号
	rsp.m_nMaxBlockID = m_pCsCfg->m_nLastBlockID; //peer当前最高区块高度
	rsp.m_strMaxBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH); //最高区块的Hash值

	if (this->SendHtsMsg(rsp, pMcpHandler))
	{
		ACE_OS::printf("send CQueryMaxBlockInfoCsCpsRsp failed, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
		DSC_RUN_LOG_ERROR("send hts message:CQueryMaxBlockInfoCsCpsRsp failed, channel-id:%d", m_nChannelID);
	}
	else
	{
		ACE_OS::printf("send CQueryMaxBlockInfoCsCpsRsp sucess, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
		DSC_RUN_LOG_INFO("send hts message:CQueryMaxBlockInfoCsCpsRsp sucess, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
	}

	return 0;
}


void CCommitterService::OnDscMsg(VBH::CQueryCryptKeyProposeEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryCryptKeyProposeEsCsReq);

	VBH::CQueryCryptKeyProposeCsEsRsp rsp;

	rsp.m_nEsSessionID = rQueryCryptKeyReq.m_nEsSessionID;
	rsp.m_nReturnCode = this->GetCryptKey(rsp.m_cltPubKey, rsp.m_envelopeKey, rQueryCryptKeyReq.m_transUrl);

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryCryptKeyProposeEsCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryCryptKeyQueryEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryCryptKeyQueryEsCsReq);

	VBH::CQueryCryptKeyQueryCsEsRsp rsp;

	rsp.m_nEsSessionID = rQueryCryptKeyReq.m_nEsSessionID;
	rsp.m_nReturnCode = this->GetCryptKey(rsp.m_cltPubKey, rsp.m_envelopeKey, rQueryCryptKeyReq.m_transUrl);

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryCryptKeyQueryEsCsReq);
}


void CCommitterService::OnDscMsg(VBH::CQueryWriteSetListProposeCcCsReq& rQueryWsReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryWriteSetListProposeCcCsReq);

	VBH::CQueryWriteSetListProposeCsCcRsp rsp;
	ACE_UINT16 nOuterVecLen = rQueryWsReq.m_vecGroupWsKey.Size();
	ACE_UINT16 nInnerVecLen;
	ACE_UINT16 nInnerIdx;

	rsp.m_vecGroupWsInfo.Open(nOuterVecLen);

	for (ACE_UINT16 nOuterIdx = 0; nOuterIdx < nOuterVecLen; ++nOuterIdx)
	{
		nInnerVecLen = rQueryWsReq.m_vecGroupWsKey[nOuterIdx].m_vecKey.Size();
		rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue.Open(nInnerVecLen);

		for (nInnerIdx = 0; nInnerIdx < nInnerVecLen; ++nInnerIdx)
		{
			rsp.m_nReturnCode = this->GetLatestWriteSetValue(rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue[nInnerIdx].m_value,
				rsp.m_vecGroupWsInfo[nOuterIdx].m_vecValue[nInnerIdx].m_nVersion,
				rQueryWsReq.m_vecGroupWsKey[nOuterIdx].m_vecKey[nInnerIdx]);

			if (rsp.m_nReturnCode != VBH::EN_OK_TYPE)
			{
				rsp.m_vecGroupWsInfo.Close(); //出错，清理掉所有已经取到的数据
				DSC_RUN_LOG_INFO("get write-set value failed, inner-idx:%d, outer-idx:%d.", nInnerIdx, nOuterIdx);
				goto ALL_WS_QUERY_END;
			}
		}
	}

ALL_WS_QUERY_END:
	rsp.m_nCcSessionID = rQueryWsReq.m_nCcSessionID;
	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryWriteSetListProposeCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryWriteSetListQueryCcCsReq& rQueryWsReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryWriteSetListQueryCcCsReq);

	VBH::CQueryWriteSetListQueryCsCcRsp rsp;
	rsp.m_nCcSessionID = rQueryWsReq.m_nCcSessionID;
	rsp.m_vecValue.Open(rQueryWsReq.m_vecKey.Size());

	for (ACE_UINT16 idx = 0; idx < rQueryWsReq.m_vecKey.Size(); ++idx)
	{
		rsp.m_nReturnCode = this->GetLatestWriteSetValue(rsp.m_vecValue[idx].m_value, rsp.m_vecValue[idx].m_nVersion, rQueryWsReq.m_vecKey[idx]);
		if (rsp.m_nReturnCode != VBH::EN_OK_TYPE)
		{
			rsp.m_vecValue.Close(); //出错，清理掉所有已经取到的数据
			break;
		}
	}

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryWriteSetListQueryCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryTransactionQueryCcCsReq& rQueryTransReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryTransactionQueryCcCsReq);

	VBH::CQueryTransactionQueryCsCcRsp rsp;

	rsp.m_nCcSessionID = rQueryTransReq.m_nCcSessionID;
	rsp.m_transKey = rQueryTransReq.m_transKey;
	rsp.m_nReturnCode = GetTransaction(rsp.m_transContent, rQueryTransReq.m_transKey);

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryTransactionQueryCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryBlockHeaderInfoExplorerCcCsReq& rQueryBlockInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryBlockHeaderInfoExplorerCcCsReq);

	VBH::CQueryBlockHeaderInfoExplorerCsCcRsp rsp;

	if ((rQueryBlockInfoReq.m_nBlockID <= m_pCsCfg->m_nLastBlockID) && (rQueryBlockInfoReq.m_nBlockID > 0))
	{
		VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(rQueryBlockInfoReq.m_nBlockID);

		rsp.m_nReturnCode = VBH::EN_OK_TYPE;
		rsp.m_nCcSessionID = rQueryBlockInfoReq.m_nCcSessionID;

		if (pMemBlock)
		{
			rsp.m_nTransCount = pMemBlock->m_nTransCount;
			rsp.m_nBlockID = rQueryBlockInfoReq.m_nBlockID;
			rsp.m_nBlockTime = pMemBlock->m_nBlockTime;
			rsp.m_preBlockHash.Set(pMemBlock->m_preBlockHash, VBH_BLOCK_DIGEST_LENGTH);
			rsp.m_merkelTreeRootHash.Set(pMemBlock->m_merkelTreeRootHash, VBH_BLOCK_DIGEST_LENGTH);
		}
		else
		{
			DSC_RUN_LOG_ERROR("get bc-block failed, block-id:%lld", rQueryBlockInfoReq.m_nBlockID);
			rsp.m_nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("invalid block-id:%lld", rQueryBlockInfoReq.m_nBlockID);
		rsp.m_nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
	}

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryBlockHeaderInfoExplorerCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryBlockCountExplorerCcCsReq& rQueryBlockCountReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryBlockCountExplorerCcCsReq);

	VBH::CQueryBlockCountExplorerCsCcRsp rsp;

	rsp.m_nCcSessionID = rQueryBlockCountReq.m_nCcSessionID;
	rsp.m_nBlockCount = m_pCsCfg->m_nLastBlockID;
	rsp.m_nReturnCode = VBH::EN_OK_TYPE;

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryBlockCountExplorerCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryWriteSetExplorerCcCsReq& rQueryUserInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryWriteSetExplorerCcCsReq);

	VBH::CQueryWriteSetExplorerCsCcRsp rsp;

	ACE_UINT32 nVersion;
	rsp.m_nCcSessionID = rQueryUserInfoReq.m_nCcSessionID;
	rsp.m_nReturnCode = this->GetLatestWriteSetValue(rsp.m_userInfo, nVersion, rQueryUserInfoReq.m_alocKey);

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryWriteSetExplorerCcCsReq);
}

//查询事务的接口要调整
void CCommitterService::OnDscMsg(VBH::CQueryTransInfoExplorerCcCsReq& rQueryTransReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryTransInfoExplorerCcCsReq);

	VBH::CQueryTransInfoExplorerCsCcRsp rsp;

	rsp.m_nCcSessionID = rQueryTransReq.m_nCcSessionID;
	rsp.m_transKey = rQueryTransReq.m_transKey;
	rsp.m_nReturnCode = GetTransaction(rsp.m_transInfo, rQueryTransReq.m_transKey);

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryTransInfoExplorerCcCsReq);
}

//查询事务的接口要调整
void CCommitterService::OnDscMsg(VBH::CQueryTransListExplorerCcCsReq& rQueryTransListReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryTransListExplorerCcCsReq);

	VBH::CQueryTransListExplorerCsCcRsp rsp;

	rsp.m_nReturnCode = VBH::EN_OK_TYPE;
	rsp.m_nCcSessionID = rQueryTransListReq.m_nCcSessionID;
	rsp.m_nBlockID = rQueryTransListReq.m_nBlockID;

	if (rQueryTransListReq.m_nBlockID <= m_pCsCfg->m_nLastBlockID)
	{
		VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(rQueryTransListReq.m_nBlockID);

		if (pMemBlock)
		{
			ACE_UINT16 nPageSize = rQueryTransListReq.m_nPageSize;
			ACE_UINT16 nPageIdx = rQueryTransListReq.m_nPageIndex;
			ACE_UINT16 nTransNum = pMemBlock->m_nTransCount; //实际返回的事务个数
			rsp.m_nBlockTime = pMemBlock->m_nBlockTime;
			char blockHash[VBH_BLOCK_DIGEST_LENGTH];

			if (m_bcTable.ReadBlockHash(blockHash, rQueryTransListReq.m_nBlockID))
			{
				DSC_RUN_LOG_ERROR("getblockhash failed block-id :lld%", rQueryTransListReq.m_nBlockID);
			}
			else
			{
				rsp.m_blockHash.Set(blockHash, VBH_BLOCK_DIGEST_LENGTH);
			}
			rsp.m_preBlockHash.Set(pMemBlock->m_preBlockHash, VBH_BLOCK_DIGEST_LENGTH);
			if (nTransNum)
			{
				VBH::CMemProposeTransaction* pTrans;

				rsp.m_vecTrans.Open(nTransNum);

				for (ACE_UINT16 idx = 0; idx < nTransNum; ++idx)
				{
					rsp.m_vecTrans[idx].m_key.m_nBlockID = rQueryTransListReq.m_nBlockID;
					rsp.m_vecTrans[idx].m_key.m_nSequenceNumber = pMemBlock->m_ppTransaction[idx]->m_nTransSequenceNumber;
					rsp.m_vecTrans[idx].m_key.m_nTransIdx = idx;

					//只支持提类事务的显示
					if (VBH::CTransactionSequenceNumber::GetTransType(rsp.m_vecTrans[idx].m_key.m_nSequenceNumber)
						== VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE)
					{
						ACE_ASSERT(dynamic_cast<VBH::CMemProposeTransaction*>(pMemBlock->m_ppTransaction[idx]) != nullptr);

						pTrans = (VBH::CMemProposeTransaction*)pMemBlock->m_ppTransaction[idx];

						if (GetSequenceNumber(rsp.m_vecTrans[idx].m_content.m_userKey.m_nSequenceNumber, pTrans->m_nUserKeyID))
						{
							DSC_RUN_LOG_ERROR("get sequence-number failed.");
							rsp.m_nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
							rsp.m_vecTrans.Close();

							break;
						}
						rsp.m_vecTrans[idx].m_content.m_userKey.m_nAllocatedID = pTrans->m_nUserKeyID;
						rsp.m_vecTrans[idx].m_content.m_nActionID = pTrans->m_nActionID;

						VBH::Assign(rsp.m_vecTrans[idx].m_content.m_signature, pTrans->m_signature);
						VBH::Assign(rsp.m_vecTrans[idx].m_content.m_proposal, pTrans->m_proposal);
					}
					else
					{
						//非关键路径，消除valgrind报告的未初始化告警
						rsp.m_vecTrans[idx].m_content.m_nActionID = 0;
						rsp.m_vecTrans[idx].m_content.m_userKey.m_nAllocatedID = 0;
						rsp.m_vecTrans[idx].m_content.m_userKey.m_nSequenceNumber = 0;
					}
				}
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get block failed, block-id:%llu", rQueryTransListReq.m_nBlockID);
			rsp.m_nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_INFO("invalid block-id:%llu", rQueryTransListReq.m_nBlockID);
		rsp.m_nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
	}

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryTransListExplorerCcCsReq);
}

void CCommitterService::OnDscMsg(VBH::CQueryTransCountExplorerCcCsReq& rQueryTransCountReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryTransCountExplorerCcCsReq);

	VBH::CQueryTransCountExplorerCsCcRsp rsp;

	rsp.m_nReturnCode = VBH::EN_OK_TYPE;
	rsp.m_nCcSessionID = rQueryTransCountReq.m_nCcSessionID;
	rsp.m_nBlockID = rQueryTransCountReq.m_nBlockID;

	if ((rQueryTransCountReq.m_nBlockID <= m_pCsCfg->m_nLastBlockID) && (rQueryTransCountReq.m_nBlockID > 0))
	{
		VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(rQueryTransCountReq.m_nBlockID);

		if (pMemBlock)
		{
			rsp.m_nTransCount = pMemBlock->m_nTransCount;
		}
		else
		{
			DSC_RUN_LOG_ERROR("get bc-block failed, block-id:%lld", rQueryTransCountReq.m_nBlockID);
			rsp.m_nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("invalid block-id:%lld", rQueryTransCountReq.m_nBlockID);
		rsp.m_nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
	}

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryTransCountExplorerCcCsReq);
}


void  CCommitterService::OnDscMsg(VBH::CQueryInformationHistoryCcCsReq& rQueryInfoHistoryReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CCommitterService, VBH::CQueryTransListExplorerCcCsReq);

	VBH::CQueryInformationHistoryCsCcRsp rsp;

	rsp.m_nReturnCode = VBH::EN_OK_TYPE;
	rsp.m_nCcSessionID = rQueryInfoHistoryReq.m_nCcSessionID;
	rsp.m_InfoKey = rQueryInfoHistoryReq.m_InfoKey;

	VBH_CLS::CIndexTableItem indexTableItem;
	VBH_CLS::CHistoryTableItem historyDBNode;

	//2. 读取 index-table
	if (!m_wsIndexTable.Read(indexTableItem, rsp.m_InfoKey.m_nAllocatedID))
	{
		
		rsp.m_TransList.Open(indexTableItem.m_nLatestVesion);
		ACE_UINT32 nLatestVesion = indexTableItem.m_nLatestVesion;
	    VBH::CVbhAllocatedTransactionKey transKey;

		transKey.m_nBlockID = indexTableItem.m_wsLatestUrl.m_nBlockID;
		transKey.m_nSequenceNumber = 0xffffffff;
		transKey.m_nTransIdx = indexTableItem.m_wsLatestUrl.m_nTransIdx;

		VBH::CProposeTransactionAtQuery transContent;
		for (ACE_UINT16 idx = 0; idx < nLatestVesion; ++idx)
		{
			rsp.m_nReturnCode = GetTransaction(transContent, transKey);
			if (rsp.m_nReturnCode != VBH::EN_OK_TYPE)
			{
				rsp.m_TransList.Close(); //出错，清理掉所有已经取到的数据
				DSC_RUN_LOG_INFO("get Transaction value failed, idx:%d, outer-idx:%d.", idx);
				break;
			}
			else
			{
				rsp.m_TransList[idx].m_key = transKey;
				rsp.m_TransList[idx].m_content.m_nActionID = transContent.m_nActionID;
				rsp.m_TransList[idx].m_content.m_userKey = transContent.m_userKey;
				rsp.m_TransList[idx].m_content.m_signature = transContent.m_signature;
				rsp.m_TransList[idx].m_content.m_proposal = transContent.m_proposal;

			}
			m_wsHistTable.Read(historyDBNode, indexTableItem.m_nPreHistTableIdx);
			indexTableItem.m_nPreHistTableIdx = historyDBNode.m_nPreHistDBIdx;
			transKey.m_nBlockID = historyDBNode.m_wsUrl.m_nBlockID;
			transKey.m_nTransIdx = historyDBNode.m_wsUrl.m_nTransIdx;
		}
	}

	this->SendDscMessage(rsp, rSrcMsgAddr);
	ShrinkBlockCache();

	VBH_MESSAGE_LEAVE_TRACE(CCommitterService, VBH::CQueryTransListExplorerCcCsReq);
}
ACE_INT32 CCommitterService::OnReceiveBlock(ACE_UINT64 nBlockID, DSC::CDscBlob& blockData)
{
	//1. 解码区块数据 //TODO: 此处解码时不进行merkel树的校验
	//1.1. 初始化，创建解码控制器 //开辟mem-block结构 
	DSC::CDscNetCodecDecoder decoder(blockData.GetBuffer(), blockData.GetSize());
	VBH::CBcBlockHeader bcBlockHeader;

	//1.2. 解码区块头
	if (bcBlockHeader.Decode(decoder))
	{
		DSC_RUN_LOG_ERROR("decode received block header error, blcok-id:%llu, block-data-length:%d.", nBlockID, blockData.GetSize());

		return VBH::EN_DECODE_ERROR_TYPE;
	}

	//1.4. 开辟事务数组，循环解码每个事务 
	ACE_ASSERT(bcBlockHeader.m_nTransCount > 0);

	VBH::CMemBcBlock* pMemBcBlock = DSC_THREAD_TYPE_NEW(VBH::CMemBcBlock) VBH::CMemBcBlock(nBlockID, bcBlockHeader.m_nTransCount); //将区块数据解码后的指针

	pMemBcBlock->m_nBlockTime = bcBlockHeader.m_nBlockTime;
	::memcpy(pMemBcBlock->m_preBlockHash, bcBlockHeader.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);
	::memcpy(pMemBcBlock->m_merkelTreeRootHash, bcBlockHeader.m_merkelTreeRootHash.data(), VBH_BLOCK_DIGEST_LENGTH);

	//1.5 遍历事务,进行解码 //遍历过程中使用的变量
	ACE_UINT16 nWsIdx;
	ACE_UINT32 nTransSequenceNumber; //事务的流水号
	VBH::CMemRegistUserTransaction* pMemRegistTrans; //注册类型事务
	VBH::CMemProposeTransaction* pMemPropTrans; //提案类型事务

	VBH_CLS::CIndexTableItem indexTableItem;
	VBH_CLS::CHistoryTableItem historyDBNode;

	for (ACE_UINT16 nTransIdx = 0; nTransIdx < bcBlockHeader.m_nTransCount; ++nTransIdx)
	{
		//解码事务序列号
		if (decoder.Decode(nTransSequenceNumber))
		{
			DSC_THREAD_TYPE_DELETE(pMemBcBlock);
			DSC_RUN_LOG_ERROR("decode sequence number failed, blcok-id:%lld, transaction-idx:%d.", nBlockID, nTransIdx);

			this->AllTableRollbackCache();

			return VBH::EN_DECODE_ERROR_TYPE;
		}

		//判断事务类型，不同事务类型做不同的解析处理
		switch (VBH::CTransactionSequenceNumber::GetTransType(nTransSequenceNumber))
		{
		case VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE: //解码提案类型事务
		{
			//解码，对照VBH::CProposeTransaction 结构
			pMemPropTrans = DSC_THREAD_TYPE_NEW(VBH::CMemProposeTransaction) VBH::CMemProposeTransaction;
			pMemBcBlock->m_ppTransaction[nTransIdx] = pMemPropTrans;
			pMemPropTrans->m_nTransSequenceNumber = nTransSequenceNumber;

			if (pMemPropTrans->Decode(decoder)) //解码整个提案事务
			{
				DSC_RUN_LOG_ERROR("decode CProposeTransaction failed, blcok-id:%lld, transaction-idx:%d.", nBlockID, nTransIdx);

				DSC_THREAD_TYPE_DELETE(pMemBcBlock);
				this->AllTableRollbackCache();

				return VBH::EN_DECODE_ERROR_TYPE;
			}

			for (nWsIdx = 0; nWsIdx < pMemPropTrans->m_vecWsItem.Size(); ++nWsIdx)
			{
				if (pMemPropTrans->m_vecWsItem[nWsIdx].m_nVersion) //version > 0
				{
					if (DSC_UNLIKELY(m_wsIndexTable.Read(indexTableItem, pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID)))
					{
						DSC_RUN_LOG_WARNING("read write-set-index-table failed, aloc-id:%llu.", pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID);

						DSC_THREAD_TYPE_DELETE(pMemBcBlock);
						this->AllTableRollbackCache();

						return VBH::EN_CLS_TABLE_READ_FAILED;
					}
					else
					{
						historyDBNode.m_nPreHistDBIdx = indexTableItem.m_nPreHistTableIdx;
						historyDBNode.m_wsUrl = indexTableItem.m_wsLatestUrl;

						//旧版用户信息写入历史数据库
						if (m_wsHistTable.Append(indexTableItem.m_nPreHistTableIdx, historyDBNode))
						{
							DSC_RUN_LOG_ERROR("write-set-history-table append failed.");

							DSC_THREAD_TYPE_DELETE(pMemBcBlock);
							this->AllTableRollbackCache();

							return VBH::EN_CLS_TABLE_MODIFY_FAILED;
						}

						//更新索引数据库
						ACE_ASSERT(indexTableItem.m_nLatestVesion == pMemPropTrans->m_vecWsItem[nWsIdx].m_nVersion);

						indexTableItem.m_wsLatestUrl.m_nBlockID = nBlockID;
						indexTableItem.m_wsLatestUrl.m_nTransIdx = nTransIdx;
						indexTableItem.m_wsLatestUrl.m_nWsIdx = nWsIdx;
						++indexTableItem.m_nLatestVesion;

						if (m_wsIndexTable.Update(pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID, indexTableItem))
						{
							DSC_RUN_LOG_ERROR("write-set-index-table update failed.");

							DSC_THREAD_TYPE_DELETE(pMemBcBlock);
							this->AllTableRollbackCache();

							return VBH::EN_CLS_TABLE_MODIFY_FAILED;
						}
					}
				}
				else
				{
					indexTableItem.m_wsLatestUrl.m_nBlockID = nBlockID;
					indexTableItem.m_wsLatestUrl.m_nTransIdx = nTransIdx;
					indexTableItem.m_wsLatestUrl.m_nWsIdx = nWsIdx;
					indexTableItem.m_nSequenceNumber4Verify = nTransSequenceNumber;
					indexTableItem.m_nLatestVesion = 1;
					indexTableItem.m_nPreHistTableIdx = DEF_INVALID_HISTORY_TABLE_INDEX;
					
					if (m_wsIndexTable.Append(pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID, indexTableItem))
					{
						DSC_RUN_LOG_ERROR("write-set-index-table append failed.");

						DSC_THREAD_TYPE_DELETE(pMemBcBlock);
						this->AllTableRollbackCache();

						return VBH::EN_CLS_TABLE_MODIFY_FAILED;
					}
				}
			}
		}
		break;
		case VBH::CTransactionSequenceNumber::EN_REGIST_USER_TRANSACTION_TYPE: //解码注册类型事务
		{
			//解码 对照 VBH::CRegistUserTransaction 结构
			pMemRegistTrans = DSC_THREAD_TYPE_NEW(VBH::CMemRegistUserTransaction) VBH::CMemRegistUserTransaction;
			pMemBcBlock->m_ppTransaction[nTransIdx] = pMemRegistTrans;
			pMemRegistTrans->m_nTransSequenceNumber = nTransSequenceNumber;

			if (pMemRegistTrans->Decode(decoder)) //
			{
				DSC_RUN_LOG_ERROR("decode CRegistUserTransaction failed, blcok-id:%lld, transaction-idx:%d.", nBlockID, nTransIdx);

				DSC_THREAD_TYPE_DELETE(pMemBcBlock);
				this->AllTableRollbackCache();

				return VBH::EN_DECODE_ERROR_TYPE;
			}
		}
		break;
		default:
		{
			DSC_THREAD_TYPE_DELETE(pMemBcBlock);
			DSC_RUN_LOG_ERROR("unknown transaction-type:%d, blcok-id:%lld, transaction-idx:%d.",
				VBH::CTransactionSequenceNumber::GetTransType(nTransSequenceNumber), nBlockID, nTransIdx);

			this->AllTableRollbackCache();

			return VBH::EN_INVALID_INPUT_PARAM;
		}
		}
	}

	//2. 写区块到区块缓存
	if (m_bcTable.AppendBlock(nBlockID, blockData, bcBlockHeader.m_preBlockHash))
	{
		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("append block failed, block-id:%lld", nBlockID);

		AllTableRollbackCache(); //回滚写入存储设备的数据

		//todo: 区块存储改造后，这里会不一样，暂时先返回-1
		return -1;
	}

	//3. 开启事务保存
	//3.1 写入日志
	if (AllTableSaveLog())
	{
		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("save device log failed, block-id:%lld", nBlockID);

		AllTableRollbackCache(); //回滚写入存储设备的缓存数据

		//TODO: 磁盘失败处理: 要断开和order的连接，回到初始化完成后的状态
		return VBH::EN_WRITE_DISK_FAILED;
	}

	//3.2 开启更改的总开关 开始最终的数据写入
	m_pCsCfg->m_nModifyStorageState = true; //=============开启总写入开关
	if (m_wsIndexTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = false; //=============关闭总写入开关
		m_wsHistTable.RollbackCache();
		m_bcTable.RollbackCache();

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", nBlockID);

		//TODO: 磁盘失败处理: 要断开和order的连接，回到初始化完成后的状态
		return VBH::EN_WRITE_DISK_FAILED;
	}

	if (m_wsHistTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_wsHistTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = false; //=============关闭总写入开关
		m_bcTable.RollbackCache();

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", nBlockID);

		//TODO: 磁盘失败处理: 要断开和order的连接，回到初始化完成后的状态
		return VBH::EN_WRITE_DISK_FAILED;
	}

	if (m_bcTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_wsHistTable.RollbackTransaction();
		m_bcTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = false; //=============关闭总写入开关

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", nBlockID);

		//TODO: 磁盘失败处理: 要断开和order的连接，回到初始化完成后的状态
		return VBH::EN_WRITE_DISK_FAILED;
	}

	m_pCsCfg->m_nLastBlockID = nBlockID; //记录最后处理的block-id
	m_pCsCfg->m_nModifyStorageState = false; //=============关闭总写入开关

	m_wsIndexTable.CommitteTransaction();
	m_wsHistTable.CommitteTransaction();
	m_bcTable.CommitteTransaction();

	InsertBlockIntoCache(pMemBcBlock); //将区块指针放入缓存

	return VBH::EN_OK_TYPE;
}

ACE_INT32 CCommitterService::OnBackspaceBlock(void)
{
	VBH::CMemBcBlock* pMemBcBlock = GetMemBcBlock(m_pCsCfg->m_nLastBlockID);

	if (NULL == pMemBcBlock)
	{
		DSC_RUN_LOG_ERROR("GetMemBcBlock failed block id:%lld ", m_pCsCfg->m_nLastBlockID);

		return -1;
	}

	ACE_ASSERT(pMemBcBlock->m_nTransCount > 0);

	//遍历事务, //遍历过程中使用的变量
	ACE_INT16 nWsIdx; //类型必须为有符号类型
	VBH::CMemProposeTransaction* pMemPropTrans; //存放于内存中的transaction的指针

	VBH_CLS::CIndexTableItem indexTableItem;
	VBH_CLS::CHistoryTableItem historyTableItem;

	for (ACE_INT16 nTransIdx = pMemBcBlock->m_nTransCount - 1; nTransIdx >= 0; --nTransIdx)
	{
		if (VBH::CTransactionSequenceNumber::GetTransType(pMemBcBlock->m_ppTransaction[nTransIdx]->m_nTransSequenceNumber)
			== VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE) //只用处理提案类型事务
		{
			ACE_ASSERT(dynamic_cast<VBH::CMemProposeTransaction*>(pMemBcBlock->m_ppTransaction[nTransIdx]) != nullptr);

			pMemPropTrans = (VBH::CMemProposeTransaction*)pMemBcBlock->m_ppTransaction[nTransIdx];

			for (nWsIdx = pMemPropTrans->m_vecWsItem.Size() - 1; nWsIdx >= 0; --nWsIdx)
			{
				if (m_wsIndexTable.Read(indexTableItem, pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID))
				{
					DSC_RUN_LOG_WARNING("read write-set-index-table failed, aloc-id:%llu.",
						pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID);

					DSC_THREAD_TYPE_DELETE(pMemBcBlock);
					this->AllTableRollbackCache();

					return -1;
				}
				else
				{
					if (m_wsHistTable.Read(historyTableItem, indexTableItem.m_nPreHistTableIdx))
					{
						DSC_RUN_LOG_ERROR("read write-set-history-table failed.");

						DSC_THREAD_TYPE_DELETE(pMemBcBlock);
						this->AllTableRollbackCache();

						return -1;
					}
					else
					{
						indexTableItem.m_wsLatestUrl = historyTableItem.m_wsUrl;
						--indexTableItem.m_nLatestVesion;
						ACE_UINT64 nHistTableIdx = indexTableItem.m_nPreHistTableIdx;
						indexTableItem.m_nPreHistTableIdx = historyTableItem.m_nPreHistDBIdx;

						if (m_wsIndexTable.Update(pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID, indexTableItem))
						{
							DSC_RUN_LOG_ERROR("[user/information]-index-db update failed.");

							DSC_THREAD_TYPE_DELETE(pMemBcBlock);
							this->AllTableRollbackCache();

							return -1;
						}

						m_wsHistTable.PopBack(nHistTableIdx);
					}
				}
			}
		}
	}

	//2. 删除区块缓存
	if (m_bcTable.PopBack(m_pCsCfg->m_nLastBlockID))
	{
		DSC_RUN_LOG_ERROR("append block failed, block-id:%lld", m_pCsCfg->m_nLastBlockID);

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		AllTableRollbackCache(); //回滚写入存储设备的数据

		return -1;
	}

	//3. 开启事务保存
	//3.1 写入日志
	if (AllTableSaveLog())
	{
		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("save device log failed, block-id:%lld", m_pCsCfg->m_nLastBlockID);

		AllTableRollbackCache(); //回滚写入存储设备的缓存数据

		return -1;
	}

	//3.2 开启更改的总开关 开始最终的数据写入
	m_pCsCfg->m_nModifyStorageState = EN_BEGIN_MODIFY_STORAGE; //=============开启总写入开关

	if (m_wsIndexTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = EN_END_MODIFY_STORAGE; //=============关闭总写入开关
		m_wsHistTable.RollbackCache();
		m_bcTable.RollbackCache();

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", m_pCsCfg->m_nLastBlockID);

		return -1;
	}

	if (m_wsHistTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_wsHistTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = EN_END_MODIFY_STORAGE; //=============关闭总写入开关
		m_bcTable.RollbackCache();

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", m_pCsCfg->m_nLastBlockID);

		return -1;
	}

	if (m_bcTable.Persistence())
	{
		m_wsIndexTable.RollbackTransaction();
		m_wsHistTable.RollbackTransaction();
		m_bcTable.RollbackTransaction();
		m_pCsCfg->m_nModifyStorageState = EN_END_MODIFY_STORAGE; //=============关闭总写入开关

		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("user-index-table persistence failed., block-id:%lld", m_pCsCfg->m_nLastBlockID);

		return -1;
	}

	--m_pCsCfg->m_nLastBlockID; //记录最后处理的block-id
	m_pCsCfg->m_nModifyStorageState = EN_END_MODIFY_STORAGE; //=============关闭总写入开关

	AllTableCommitteTransaction();

	m_mapBlockCache.Erase(pMemBcBlock->m_nBlockID);//将区块指针从缓存中删除
	DSC_THREAD_TYPE_DELETE(pMemBcBlock);

	return 0;
}

ACE_INT32 CCommitterService::GetBlockKvLst(const ACE_UINT64 nBlockID, DSC::CDscShortList<VBH::CKeyVersion>& lstKv)
{
	VBH::CMemBcBlock* pMemBcBlock = GetMemBcBlock(nBlockID);

	if (NULL == pMemBcBlock)
	{
		DSC_RUN_LOG_ERROR("GetMemBcBlock failed block id:%lld ", nBlockID);

		return -1;
	}

	ACE_ASSERT(pMemBcBlock->m_nTransCount > 0);

	// 遍历事务
	ACE_UINT16 nWsIdx;
	VBH::CMemProposeTransaction* pMemPropTrans; //存放于内存中的transaction的指针
	VBH::CKeyVersion keyVersion;

	for (ACE_UINT16 nTransIdx = 0; nTransIdx < pMemBcBlock->m_nTransCount; ++nTransIdx)
	{
		if (VBH::CTransactionSequenceNumber::GetTransType(pMemBcBlock->m_ppTransaction[nTransIdx]->m_nTransSequenceNumber)
			== VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE)
		{
			pMemPropTrans = (VBH::CMemProposeTransaction*)pMemBcBlock->m_ppTransaction[nTransIdx];

			for (nWsIdx = 0; nWsIdx < pMemPropTrans->m_vecWsItem.Size(); ++nWsIdx)
			{
				keyVersion.m_nAllocatedID = pMemPropTrans->m_vecWsItem[nWsIdx].m_nAllocatedID;
				keyVersion.m_nVersion = pMemPropTrans->m_vecWsItem[nWsIdx].m_nVersion;

				lstKv.push_back(keyVersion);
			}
		}
	}

	return 0;
}

void CCommitterService::CacheUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block)
{
	if (m_nQueueUnresolvedBlockNum < m_nMaxUnresolvedBlockCacheCount)
	{
		if (0 != m_nQueueUnresolvedBlockNum)
		{
			CUnresolvedBlock* pUnresolvedBlock = m_queueUnresolvedBlock.Back();

			if ((pUnresolvedBlock->m_nBlockID + 1) == nBlockID)
			{
				pUnresolvedBlock = DSC_THREAD_TYPE_NEW(CUnresolvedBlock) CUnresolvedBlock(nBlockID, block);
				m_queueUnresolvedBlock.PushBack(pUnresolvedBlock);
				++m_nQueueUnresolvedBlockNum;
			}
		}
		else
		{
			CUnresolvedBlock* pUnresolvedBlock = DSC_THREAD_TYPE_NEW(CUnresolvedBlock) CUnresolvedBlock(nBlockID, block);
			m_queueUnresolvedBlock.PushBack(pUnresolvedBlock);
			++m_nQueueUnresolvedBlockNum;
		}
	}
}

void CCommitterService::SendVerifyPeerStateReq(const ACE_UINT64 nKafkaBlockID, CDscString& strKafkaBlockHash, VBH::CSyncSourcePeerCasAddress& casAddress)
{
	VBH::CVerifyPeerStateCsCasReq req;
	PROT_COMM::CDscIpAddr addr;

	addr.SetIpAddr(casAddress.m_strIpAddr);
	addr.SetPort(casAddress.m_nPort);

	req.m_nKafkaBlockID = nKafkaBlockID;
	req.m_strKafkaBlockHash = strKafkaBlockHash;

	ACE_UINT32 nHandleId;

	if (this->SendHtsMsg(nHandleId, req, addr))
	{
		DSC_RUN_LOG_ERROR("send VerifyPeerStateReq failed");
	}

	m_pVerifyPeerStateSession->m_vecCasConnHandleId.push_back(nHandleId);
}

void CCommitterService::DoSyncBlock(void)
{
	if (m_pCsCfg->m_nLastBlockID < m_pSyncBlockSession->m_nTargetBlockID)
	{
		VBH::CGetBlockCsCasReq req;

		req.m_nBlockID = m_pCsCfg->m_nLastBlockID + 1;

		if (this->SendHtsMsg(req, m_pSyncBlockSession->m_nHandleId))
		{
			DSC_RUN_LOG_ERROR("send sync block req failed");
		}

		this->ResetDscTimer(m_pSyncBlockSession, EN_SYNC_BLOCK_TIMEOUT_VALUE);
	}
	else
	{
		this->DisConnect(m_pSyncBlockSession->m_nHandleId);
		this->CancelDscTimer(m_pSyncBlockSession);
		DSC_THREAD_TYPE_DELETE(m_pSyncBlockSession);
		m_pSyncBlockSession = nullptr;

		// 同步完成，把缓存中的区块取出来，落块
		CUnresolvedBlock* pUnresolvedBlock = m_queueUnresolvedBlock.PopFront();

		while (pUnresolvedBlock)
		{
			ACE_ASSERT(pUnresolvedBlock->m_nBlockID == m_pCsCfg->m_nLastBlockID + 1);

			if (OnReceiveBlock(pUnresolvedBlock->m_nBlockID, pUnresolvedBlock->m_vbhBlock) != VBH::EN_OK_TYPE)
			{
				//TODO: 磁盘失败
				DSC_RUN_LOG_ERROR("save block failed, block-id:%lld", pUnresolvedBlock->m_nBlockID);
			}

			DSC_THREAD_TYPE_DELETE(pUnresolvedBlock);
			pUnresolvedBlock = m_queueUnresolvedBlock.PopFront();
		}

		m_nQueueUnresolvedBlockNum = 0;
	}
}

void CCommitterService::SendRegistReq(ACE_UINT32 nHandleID)
{
	char blockHash[VBH_BLOCK_DIGEST_LENGTH];
	
	if (m_pCsCfg->m_nLastBlockID >= 1) //是有效的区块ID
	{
		if (m_bcTable.ReadBlockHash(blockHash, m_pCsCfg->m_nLastBlockID))
		{
			//TODO: 磁盘错误
			DSC_RUN_LOG_ERROR("GetBlockHash failed last-block-id: lld%", m_pCsCfg->m_nLastBlockID);
			return;
		}
	}
	else
	{
		memset(blockHash, 0, VBH_BLOCK_DIGEST_LENGTH);
	}

	VBH::CRegistCsCpsReq req;

	req.m_nPeerID = m_nPeerID;
	req.m_strCasIpAddr = m_strCasIpAddr; //代理服务侦听的IP地址
	req.m_nCasPort = m_nCasPort; //代理服务侦听的端口号
	req.m_nMaxBlockID = m_pCsCfg->m_nLastBlockID; //peer当前最高区块高度
	req.m_strMaxBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH); //最高区块的Hash值

	if (this->SendHtsMsg(req, nHandleID))
	{
		ACE_OS::printf("send CRegisterXcsCpsReq failed, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
		DSC_RUN_LOG_ERROR("send hts message:CRegisterXcsCpsReq failed, channel-id:%d", m_nChannelID);
	}
	else
	{   
		ACE_OS::printf("send CRegisterXcsCpsReq sucess, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
		DSC_RUN_LOG_INFO("send hts message:CRegisterXcsCpsReq sucess, channel-id:%d, maxBlockID:%d", m_nChannelID, m_pCsCfg->m_nLastBlockID);
	}
}

ACE_INT32 CCommitterService::OnConnectedNodify(CMcpClientHandler* pMcpClientHandler)
{
	ACE_UINT32 nHandle = pMcpClientHandler->GetHandleID();

	for (auto& it : m_vecCpsHandleID)
	{
		if (nHandle == it)
		{
			SendRegistReq(nHandle);
			break;
		}
	}

	return CDscHtsClientService::OnConnectedNodify(pMcpClientHandler);
}

ACE_INT32 CCommitterService::AllTableSaveLog(void)
{
	DSC_FORWARD_CALL(m_wsIndexTable.SaveToLog());
	DSC_FORWARD_CALL(m_wsHistTable.SaveToLog());
	DSC_FORWARD_CALL(m_bcTable.SaveToLog());

	return 0;
}

void CCommitterService::AllTableRollbackCache(void)
{
	m_wsIndexTable.RollbackCache();
	m_wsHistTable.RollbackCache();

	m_bcTable.RollbackCache();
}

void CCommitterService::AllTableCommitteTransaction(void)
{
	m_wsIndexTable.CommitteTransaction();
	m_wsHistTable.CommitteTransaction();
	m_bcTable.CommitteTransaction();
}

ACE_INT32 CCommitterService::AllTableRecoverFromLog(void)
{
	if (m_wsIndexTable.RecoverFromLog())
	{
		DSC_RUN_LOG_ERROR("user index db recover from log failed.");
		return -1;
	}

	if (m_wsHistTable.RecoverFromLog())
	{
		DSC_RUN_LOG_ERROR("user history db recover from log failed.");
		return -1;
	}

	if (m_bcTable.RecoverFromLog())
	{
		DSC_RUN_LOG_ERROR("blockchain storage recover from log failed.");
		return -1;
	}

	return 0;
}

ACE_INT32 CCommitterService::GetTransaction(VBH::CProposeTransactionAtQuery& transContent,  VBH::CVbhAllocatedTransactionKey& transKey)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

	if (transKey.m_nBlockID <= m_pCsCfg->m_nLastBlockID)
	{
		VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(transKey.m_nBlockID);

		if (pMemBlock)
		{
			const ACE_UINT16 nTransIdx = transKey.m_nTransIdx;

			if (nTransIdx < pMemBlock->m_nTransCount)
			{
				if ((transKey.m_nSequenceNumber == pMemBlock->m_ppTransaction[nTransIdx]->m_nTransSequenceNumber)
					||(0xffffffff == transKey.m_nSequenceNumber))
				{
					if ((VBH::CTransactionSequenceNumber::GetTransType(transKey.m_nSequenceNumber)
						== VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE)
						||(0xffffffff == transKey.m_nSequenceNumber))
					{
						transKey.m_nSequenceNumber = pMemBlock->m_ppTransaction[nTransIdx]->m_nTransSequenceNumber;
						ACE_ASSERT(dynamic_cast<VBH::CMemProposeTransaction*>(pMemBlock->m_ppTransaction[nTransIdx]) != nullptr);

						VBH::CMemProposeTransaction* pTrans = (VBH::CMemProposeTransaction*)pMemBlock->m_ppTransaction[nTransIdx];

						transContent.m_nActionID = pTrans->m_nActionID;
						VBH::Assign(transContent.m_signature, pTrans->m_signature);
						VBH::Assign(transContent.m_proposal, pTrans->m_proposal);

						transContent.m_userKey.m_nAllocatedID = pTrans->m_nUserKeyID;
						if (GetSequenceNumber(transContent.m_userKey.m_nSequenceNumber, pTrans->m_nUserKeyID))
						{
							DSC_RUN_LOG_ERROR("get sequence-number failed.");
							nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
						}

						if (nReturnCode == VBH::EN_OK_TYPE)
						{
							transContent.m_vecWs.Open(pTrans->m_vecWsItem.Size());

							for (ACE_UINT16 idx = 0; idx < pTrans->m_vecWsItem.Size(); ++idx)
							{
								transContent.m_vecWs[idx].m_nVersion = pTrans->m_vecWsItem[idx].m_nVersion;
								VBH::Assign(transContent.m_vecWs[idx].m_value, pTrans->m_vecWsItem[idx].m_value);
								transContent.m_vecWs[idx].m_key.m_nAllocatedID = pTrans->m_vecWsItem[idx].m_nAllocatedID;
								if (GetSequenceNumber(transContent.m_vecWs[idx].m_key.m_nSequenceNumber,
									pTrans->m_vecWsItem[idx].m_nAllocatedID))
								{
									DSC_RUN_LOG_ERROR("get sequence-number failed.");
									nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
									transContent.m_vecWs.Close();

									break;
								}
							}
						}
					}
					else //只支持提案类型的查询
					{
						DSC_RUN_LOG_INFO("transaction not support query.");
						nReturnCode = VBH::EN_TRANS_NOT_SUPPORT_QUERY;
					}
				}
				else
				{
					DSC_RUN_LOG_ERROR("invalid transction-sequence-number");
					nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
				}
			}
			else
			{
				DSC_RUN_LOG_ERROR("invalid transction-index:%d", nTransIdx);
				nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get bc-block failed, block-id:%lld", transKey.m_nBlockID);
			nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("invalid block-id:%lld", transKey.m_nBlockID);
		nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
	}

	return nReturnCode;
}
ACE_INT32 CCommitterService::GetWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey, VBH_CLS::CIndexTableItem &indexTableItem)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	//校验流水号，流水号本来就是用来校验用的
	if (alocWsKey.m_nSequenceNumber == indexTableItem.m_nSequenceNumber4Verify)
	{
		VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(indexTableItem.m_wsLatestUrl.m_nBlockID);

		if (DSC_LIKELY(pMemBlock))
		{
			if (DSC_LIKELY(indexTableItem.m_wsLatestUrl.m_nTransIdx < pMemBlock->m_nTransCount))
			{
				ACE_ASSERT(dynamic_cast<VBH::CMemProposeTransaction*>(pMemBlock->m_ppTransaction[indexTableItem.m_wsLatestUrl.m_nTransIdx]));

				VBH::CMemProposeTransaction* pTrans = (VBH::CMemProposeTransaction*)pMemBlock->m_ppTransaction[indexTableItem.m_wsLatestUrl.m_nTransIdx];

				if (indexTableItem.m_wsLatestUrl.m_nWsIdx < pTrans->m_vecWsItem.Size())
				{
					nVersion = indexTableItem.m_nLatestVesion; //版本号取最新的版本号
					VBH::Assign(wsValue, pTrans->m_vecWsItem[indexTableItem.m_wsLatestUrl.m_nWsIdx].m_value);
				}
				else
				{
					DSC_RUN_LOG_ERROR("invalid write set index:%d, but should be < %d", indexTableItem.m_wsLatestUrl.m_nWsIdx, pTrans->m_vecWsItem.Size());
					nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
				}
			}
			else
			{
				DSC_RUN_LOG_WARNING("transaction-idx exceed, transaction-idx:%d, transaction-count:%d, block-id:%lld",
					indexTableItem.m_wsLatestUrl.m_nTransIdx, pMemBlock->m_nTransCount, indexTableItem.m_wsLatestUrl.m_nBlockID);
				nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("get bc block failed, block-id:%lld", indexTableItem.m_wsLatestUrl.m_nBlockID);
			nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
		}
	}
	else
	{
		DSC_RUN_LOG_WARNING("write-set-sequence number verify falied, vbh-aloc-id:%llu, input-seq:%#X, expect-seq:%#X.",
			alocWsKey.m_nAllocatedID, alocWsKey.m_nSequenceNumber, indexTableItem.m_nSequenceNumber4Verify);
		nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
	}
	
	return nReturnCode;
}

ACE_INT32 CCommitterService::GetLatestWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;
	VBH_CLS::CIndexTableItem indexTableItem;

	//2. 读取 index-table
	if (VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE != VBH::CTransactionSequenceNumber::GetTransType(alocWsKey.m_nSequenceNumber))
	{
		DSC_RUN_LOG_INFO("error SequenceNumber");
		return VBH::EN_INVALID_INPUT_PARAM;
	}

	if (m_wsIndexTable.Read(indexTableItem, alocWsKey.m_nAllocatedID))
	{
		DSC_RUN_LOG_ERROR("read write-set-index-table failed.");
		nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
	}
	else
	{
		nReturnCode = GetWriteSetValue(wsValue, nVersion, alocWsKey, indexTableItem);
	}

	return nReturnCode;
}

ACE_INT32 CCommitterService::GetCryptKey(DSC::CDscShortBlob& cltPubKey, DSC::CDscShortBlob& envelopeKey, const VBH::CTransactionUrl& transUrl)
{
	ACE_INT32 nReturnCode = VBH::EN_OK_TYPE;

	VBH::CMemBcBlock* pMemBlock = GetMemBcBlock(transUrl.m_nBlockID);

	if (pMemBlock)
	{
		if (transUrl.m_nTransIdx < pMemBlock->m_nTransCount)
		{
			if (VBH::CTransactionSequenceNumber::GetTransType(pMemBlock->m_ppTransaction[transUrl.m_nTransIdx]->m_nTransSequenceNumber)
				== VBH::CTransactionSequenceNumber::EN_REGIST_USER_TRANSACTION_TYPE)
			{
				ACE_ASSERT(dynamic_cast<VBH::CMemRegistUserTransaction*>(pMemBlock->m_ppTransaction[transUrl.m_nTransIdx]));

				VBH::CMemRegistUserTransaction* pTrans = (VBH::CMemRegistUserTransaction*) pMemBlock->m_ppTransaction[transUrl.m_nTransIdx];
				
				VBH::Assign(cltPubKey, pTrans->m_cltPubKey);
				VBH::Assign(envelopeKey, pTrans->m_envelopeKey);
			}
			else
			{
				DSC_RUN_LOG_ERROR("transaction type is invalid, expect regist-type, but it is propose-type, trans-url:%lld.%d",
					transUrl.m_nBlockID, transUrl.m_nTransIdx);
				nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
			}
		}
		else
		{
			DSC_RUN_LOG_ERROR("request transaction-index invalid, req-trans-idx:%d, block-id:%llu, block-trans-count:%d", 
				transUrl.m_nTransIdx, transUrl.m_nBlockID, pMemBlock->m_nTransCount);
			nReturnCode = VBH::EN_INVALID_INPUT_PARAM;
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("read block failed, block-id:%llu", transUrl.m_nBlockID);
		nReturnCode = VBH::EN_SYSTEM_ERROR_TYPE;
	}

	return nReturnCode;
}

void CCommitterService::OnTimeOut(CQuerySyncSourcePeerSession* pQuerySyncSourcePeerSession)
{
	DSC_RUN_LOG_INFO("QuerySyncSourcePeerSession timeout");

	VBH::CQuerySyncSourcePeerCsCpsReq req;

	req.m_nPeerID = m_nPeerID;
	req.m_nTargetBlockID = pQuerySyncSourcePeerSession->m_nTargetBlockId;

	if (this->SendHtsMsg(req, pQuerySyncSourcePeerSession->m_nHandleId))
	{
		DSC_RUN_LOG_ERROR("send hts message:QuerySyncSourcePeerSession failed.");
	}
}

void CCommitterService::OnTimeOut(CVerifyPeerStateSession* pVerifyPeerStateSession)
{
	DSC_RUN_LOG_INFO("VerifyPeerStateSession timeout");

	VBH::CVerifyPeerStateCsCasReq req;
	
	for (auto& it : m_pVerifyPeerStateSession->m_vecCasConnHandleId)
	{
		req.m_nKafkaBlockID = m_pVerifyPeerStateSession->m_nKafkaBlockID;
		req.m_strKafkaBlockHash = m_pVerifyPeerStateSession->m_strKafkaBlockHash;

		if(this->SendHtsMsg(req, it))
		{
			DSC_RUN_LOG_ERROR("send hts message:VerifyPeerStateCsCasReq failed, handle-id:%u.", it);
		}
	}
}

void CCommitterService::OnTimeOut(CSyncBlockSession* pSyncBlockSession)
{
	DSC_RUN_LOG_INFO("CSyncBlockSession timeout");
	if (m_pSyncBlockSession->m_bIsCheckBlockHash) //已经校验过hash，拉取块
	{
		VBH::CGetBlockCsCasReq req;

		req.m_nBlockID = m_pCsCfg->m_nLastBlockID + 1;

		if (this->SendHtsMsg(req, pSyncBlockSession->m_nHandleId))
		{
			DSC_RUN_LOG_ERROR("send sync block req failed");
		}
	}
	else //尚未校验hash
	{
		char blockHash[VBH_BLOCK_DIGEST_LENGTH];

		if (m_bcTable.ReadBlockHash(blockHash, m_pCsCfg->m_nLastBlockID))
		{
			//todo:磁盘失败!!!!
			DSC_RUN_LOG_ERROR("getblockhash failed block-id :lld%", m_pCsCfg->m_nLastBlockID);
		}
		else
		{
			VBH::CCheckBlockHashCsCasReq req;

			req.m_nBlockID = m_pCsCfg->m_nLastBlockID;
			req.m_strBlockHash.assign(blockHash, VBH_BLOCK_DIGEST_LENGTH);

			if (this->SendHtsMsg(req, pSyncBlockSession->m_nHandleId))
			{
				DSC_RUN_LOG_ERROR("send CCheckBlockHashCsCasReq failed");
			}
		}
	}
}

VBH::CMemBcBlock* CCommitterService::GetMemBcBlock(const ACE_UINT64 nBlockID)
{
	CMemBlockPtrInfo* pMemBlockPtrInfo = m_mapBlockCache.Find(nBlockID);

	if (pMemBlockPtrInfo) //找到了，更新活跃度
	{
		m_dequeBlockCache.Erase(pMemBlockPtrInfo);
		m_dequeBlockCache.PushBack(pMemBlockPtrInfo); //更新块的活跃度

		return pMemBlockPtrInfo->m_pBcBlock;
	}
	else //未找到
	{
		VBH::CMemBcBlock* pMemBlock = m_bcTable.ReadDecodeBlock(nBlockID);

		if (pMemBlock)
		{
			InsertBlockIntoCache(pMemBlock);
		}

		return pMemBlock;
	}
}

void CCommitterService::ShrinkBlockCache(void)
{
	CMemBlockPtrInfo* pBlockPtrInfo;

	while (m_nCurBlockCacheSize > m_nMaxCachedBlockCount)
	{
		pBlockPtrInfo = m_dequeBlockCache.PopFront();

		m_mapBlockCache.Erase(pBlockPtrInfo);
		--m_nCurBlockCacheSize;
		DSC_THREAD_TYPE_DELETE(pBlockPtrInfo);
	}
}

CCommitterService::CMemBlockPtrInfo::~CMemBlockPtrInfo()
{
	if (m_pBcBlock)
	{
		DSC_THREAD_TYPE_DELETE(m_pBcBlock);
	}
}