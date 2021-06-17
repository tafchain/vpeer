#ifndef COMMITTER_SERVICE_H_5C38EF2E755111E9957260F18A3A20D1
#define COMMITTER_SERVICE_H_5C38EF2E755111E9957260F18A3A20D1

#include "dsc/container/dsc_type_array.h"
#include "dsc/container/bare_hash_map.h"
#include "dsc/protocol/hts/dsc_hts_service.h"
#include "dsc/service_timer/dsc_service_timer_handler.h"
#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/codec/dsc_codec/dsc_codec.h"

#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_cs_def.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_cas_cs_def.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_cps_cs_def.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"
#include "vbh_comm/vbh_encrypt_lib.h"

#include "com_cs/block_chain_table/block_chain_table.h"
#include "vbh_server_comm/vbh_unite_dqueue.h"
#include "vbh_server_comm/vbh_const_length_storage/vbh_append_table.h"
#include "vbh_server_comm/vbh_const_length_storage/vbh_update_table.h"
#include "vbh_server_comm/vbh_const_length_storage/vbfs/vbfs.h"

#include "com_cs/cs/i_committer_service.h"
#include "com_cs/cas/i_committer_agent_service.h"

/** 可靠存储算法：
 * 1. 对各个独立的存储设施，执行append或update操作；操作只影响内存，不影响永久存储；
 * 2. 保存所有独立存储设施中的log到各自的mem-map内存中；log包括所有变更前的状态，以便于将来恢复；
 * 3. 置位总的标志位，标志位位于committer的mem-map内存中；
 * 4. 对各个独立的存储设施，应用变更(ApplyModify)
 * 5. 清除总的标志位；
 * --------------
 * 如果还没有对任何一个执行ApplyModify前出现异常，对
 * --------------
 * --------------
 * 异常重启后，如果总标志位为 置位 ，则对各个独立存储设置进行 从日志恢复（recover-from-log）操作
 * */
class PLUGIN_EXPORT CCommitterService final : public CDscHtsClientService, public ICommitterService
{
public:
	enum
	{
		EN_SERVICE_TYPE = VBH::EN_COMMITTER_SERVICE_TYPE,
	};

private:
	enum
	{
		EN_SESSION_TIMEOUT_VALUE = 60,
		EN_SYNC_BLOCK_TIMEOUT_VALUE = 3,
		EN_MEM_BLOCK_CACHE_HASH_MAP_BITS = 20 //page-cache的规模
	};

private:
	enum EnModifyStorage
	{
		EN_BEGIN_MODIFY_STORAGE = 1, //开始变更存储
		EN_END_MODIFY_STORAGE = 0 //完成变更存储
	};

	class CCsConfig
	{
	public:
		ACE_INT32 m_nModifyStorageState = EN_END_MODIFY_STORAGE; //是否正在修改存储;
		ACE_UINT64 m_nLastBlockID = 0; //处理的最后1个block-id
	};

private:

	class CQuerySyncSourcePeerSession final : public CDscServiceTimerHandler
	{
	public:
		CQuerySyncSourcePeerSession(CCommitterService& rService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT64 m_nTargetBlockId;
		ACE_UINT32 m_nHandleId; 

	protected:
		CCommitterService& m_rXCommitterService;
	};

	// 只能存在一个验证peer的session
	class CVerifyPeerStateSession final : public CDscServiceTimerHandler
	{
	public:
		CVerifyPeerStateSession(CCommitterService& rService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT64 m_nKafkaBlockID;   //order端当前达成kafka的区块高度
		ACE_UINT64 m_nTargetBlockId;   //需要同步的目标区块高度
		CDscString m_strKafkaBlockHash;
		dsc_vector_type(ACE_UINT32) m_vecCasConnHandleId;//和Cas的连接信息

	protected:
		CCommitterService& m_rXCommitterService;
	};

	// 只能存在一个同步区块的session
	class CSyncBlockSession final : public CDscServiceTimerHandler
	{
	public:
		CSyncBlockSession(CCommitterService& rService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT64 m_nTargetBlockID;   //同步的目标区块高度
		ACE_UINT32 m_nHandleId;
		bool m_bIsCheckBlockHash = false;  //是否检查过hash

	protected:
		CCommitterService& m_rXCommitterService;
	};

	//表示1次打包的变更
	class CUnresolvedBlock
	{
	public:
		CUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block);
		~CUnresolvedBlock();

	public:
		const ACE_UINT64 m_nBlockID;
		DSC::CDscBlob m_vbhBlock;

	public: //使用CDscDqueue时，必须具有的成员
		CUnresolvedBlock* m_pNext = nullptr;
		CUnresolvedBlock* m_pPrev = nullptr;
	};

public:
	CCommitterService(const ACE_UINT32 nChannelID, CDscString strCasIpAddr, ACE_UINT16 nCasPort);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;

public:
	virtual char* ReadBlock(ACE_UINT32& nBlockBufLen, const ACE_UINT64 nBlockID) override;
	virtual ACE_INT32 ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID) override;

protected:
	BEGIN_HTS_MESSAGE_BIND
	BIND_HTS_MESSAGE(VBH::CDistributeBlockCpsCsReq)
	BIND_HTS_MESSAGE(VBH::CSyncBlockOnRegistCpsCsNotify)
	BIND_HTS_MESSAGE(VBH::CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify)
	BIND_HTS_MESSAGE(VBH::CBackspaceBlockCpsCsNotify)
	BIND_HTS_MESSAGE(VBH::CQuerySyncSourcePeerCpsCsRsp)
	BIND_HTS_MESSAGE(VBH::CVerifyPeerStateCasCsRsp)
	BIND_HTS_MESSAGE(VBH::CCheckBlockHashCasCsRsp)
	BIND_HTS_MESSAGE(VBH::CGetBlockCasCsRsp)
	BIND_HTS_MESSAGE(VBH::CMasterSyncVersionTableCpsCsReq)
	BIND_HTS_MESSAGE(VBH::CSlaveSyncVersionTableCpsCsReq)
	BIND_HTS_MESSAGE(VBH::CQueryMaxBlockInfoCpsCsReq)
	END_HTS_MESSAGE_BIND

public:
	ACE_INT32 OnHtsMsg(VBH::CDistributeBlockCpsCsReq& rDistBlockReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSyncBlockOnRegistCpsCsNotify& rSyncNotify, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify& rSyncNotify, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CBackspaceBlockCpsCsNotify& rBackspaceBlockCpsXcsNotify, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CQuerySyncSourcePeerCpsCsRsp& rQuerySyncRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CVerifyPeerStateCasCsRsp& rVerifyPeerStateCasCsRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCheckBlockHashCasCsRsp& rCheckBlockHashCasCsRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CGetBlockCasCsRsp& rGetBlockCasCsRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CMasterSyncVersionTableCpsCsReq& rSyncVersionTableReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSlaveSyncVersionTableCpsCsReq& rSyncVersionTableReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CInvalidPeerCpsCsNotify& rInvalidPeerCpsCsNotify, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CQueryMaxBlockInfoCpsCsReq& rQueryMaxBlockInfoCpsCsReq, CMcpHandler* pMcpHandler);
	

protected:
	BIND_DSC_MESSAGE(
		/*endorser的直接查询*/
		VBH::CQueryCryptKeyProposeEsCsReq,
		VBH::CQueryCryptKeyQueryEsCsReq,

		/*经CC的查询*/
		VBH::CQueryWriteSetListProposeCcCsReq,
		VBH::CQueryWriteSetListQueryCcCsReq,
		VBH::CQueryTransactionQueryCcCsReq,
		VBH::CQueryBlockHeaderInfoExplorerCcCsReq,
		VBH::CQueryBlockCountExplorerCcCsReq,
		VBH::CQueryWriteSetExplorerCcCsReq,
		VBH::CQueryTransInfoExplorerCcCsReq,
		VBH::CQueryTransListExplorerCcCsReq,
		VBH::CQueryTransCountExplorerCcCsReq,
		VBH::CQueryInformationHistoryCcCsReq
	);

public:
	/*endorser的直接查询*/
	void OnDscMsg(VBH::CQueryCryptKeyProposeEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCryptKeyQueryEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	
	/*经CC的查询*/
	void OnDscMsg(VBH::CQueryWriteSetListProposeCcCsReq& rQueryWsReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryWriteSetListQueryCcCsReq& rQueryWsReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransactionQueryCcCsReq& rQueryTransReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryBlockHeaderInfoExplorerCcCsReq& rQueryBlockInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryBlockCountExplorerCcCsReq& rQueryBlockCountReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryWriteSetExplorerCcCsReq& rQueryUserInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransInfoExplorerCcCsReq& rQueryTransInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransListExplorerCcCsReq& rQueryTransInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransCountExplorerCcCsReq& rQueryTransInfoReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryInformationHistoryCcCsReq& rQueryInfoHistoryReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

public:
	void OnTimeOut(CQuerySyncSourcePeerSession* pQuerySyncSourcePeerSession);
	void OnTimeOut(CSyncBlockSession* pSyncBlockSession);
	void OnTimeOut(CVerifyPeerStateSession* pVerifyPeerStateSession);
	void SetCommitterAgentService(ICommitterAgentService* pCommitterAgentService);

private:
	//存储收到的区块
	ACE_INT32 OnReceiveBlock(ACE_UINT64 nBlockID, DSC::CDscBlob& blockData);

	//退格最高的区块
	ACE_INT32 OnBackspaceBlock(void);

	//获取对应区块的kvlst
	ACE_INT32 GetBlockKvLst(const ACE_UINT64 nBlockID, DSC::CDscShortList<VBH::CKeyVersion>& lstKv);

	//缓存未解析的区块
	void CacheUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block);

	//发送验证peer状态请求
	void SendVerifyPeerStateReq(const ACE_UINT64 nKafkaBlockID, CDscString& strKafkaBlockHash, VBH::CSyncSourcePeerCasAddress & casAddress);

	//开始同步区块流程
	void DoSyncBlock(void);

	//发送注册请求给CPS
	void SendRegistReq(ACE_UINT32 nHandleID);

protected:
	virtual ACE_INT32 OnConnectedNodify(CMcpClientHandler* pMcpClientHandler) override;

private:
	//所有存储设施保存日志，出错则提前返回错误
	ACE_INT32 AllTableSaveLog(void);
	//所有存储设施回滚cache操作
	void AllTableRollbackCache(void);

	//所有存储设施提交操作
	void AllTableCommitteTransaction(void);

	//所有带脏标记的存储设备从日志恢复，同时清除自己的脏标记
	ACE_INT32 AllTableRecoverFromLog(void);

	//获取事务内容 //参数指向注册类事务时，返回错误 
	//函数返回后，必须在使用transContent后，调用 ShrinkBlockCache， 否则内存会持续增长 //在使用前调用ShrinkBlockCache则可能会导致操作已释放的内存
	ACE_INT32 GetTransaction(VBH::CProposeTransactionAtQuery& transContent,  VBH::CVbhAllocatedTransactionKey& transKey);

	//获取最新写集值(user-info或information-value)和对应的版本号
	//函数返回后，必须在使用wsValue后，调用 ShrinkBlockCache， 否则内存会持续增长 //在使用前调用ShrinkBlockCache则可能会导致操作已释放的内存
	ACE_INT32 GetLatestWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey);

	ACE_INT32 GetWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey, VBH_CLS::CIndexTableItem& indexTableItem);

	//从世界状态中获取 VBH::CVbhAllocatedKey 中aloc-id对应的sequence-number
	ACE_INT32 GetSequenceNumber(ACE_UINT32& nSeq, const ACE_UINT32 nAlocID);

	//获取用户存储在服务端的秘钥 仅返回信封秘钥 //TODO，将来返回其它秘钥
	//cltPubKey, envelopeKey 指向的缓冲区，在再次读取之前有效
	//函数返回后，必须在使用cltPubKey/envelopeKey后，调用 ShrinkBlockCache， 否则内存会持续增长 //在使用前调用ShrinkBlockCache则可能会导致操作已释放的内存
	ACE_INT32 GetCryptKey(DSC::CDscShortBlob& cltPubKey, DSC::CDscShortBlob& envelopeKey, const VBH::CTransactionUrl& transUrl);

	//读取1个mem-bc-block //首先从缓存读取，//缓存未命中，则从文件读取 //会导致区块缓冲区不断扩大
	VBH::CMemBcBlock* GetMemBcBlock(const ACE_UINT64 nBlockID);

	//将1个区块插入到缓冲区 //插入时，不删除旧的不活跃块 //这样会导致cache不断增大
	void InsertBlockIntoCache(VBH::CMemBcBlock* pBlock);

	//收缩区块缓冲区，使其不至于超过预设的大小 //在区块数据保证不被访问时调用
	void ShrinkBlockCache(void);

private:
	//区块的指针信息
	class CMemBlockPtrInfo
	{
	public:
		~CMemBlockPtrInfo();

	public:
		VBH::CMemBcBlock* m_pBcBlock = nullptr; //区块的指针

	public: //CBareHashMap
		ACE_UINT64 m_nKey = 0;
		CMemBlockPtrInfo* m_pPrev = nullptr;
		CMemBlockPtrInfo* m_pNext = nullptr;

	public: //作为CVbhUniteDqueue的元素需要的成员变量
		CMemBlockPtrInfo* m_pDqueuePrev = nullptr;
		CMemBlockPtrInfo* m_pDqueueNext = nullptr;
	};
	using memblock_map_type = CBareHashMap<ACE_UINT64, CMemBlockPtrInfo, EN_MEM_BLOCK_CACHE_HASH_MAP_BITS>; //block-id -> block-ptr
	using memblock_deque_type = CVbhUniteDqueue<CMemBlockPtrInfo>; 

private:
	const ACE_UINT32 m_nChannelID;
	ACE_UINT16 m_nPeerID = 0;
	ACE_UINT16 m_nCasPort; //代理服务侦听的端口号
	CDscString m_strCasIpAddr; //代理服务侦听的IP地址
	dsc_vector_type(ACE_UINT32) m_vecCpsHandleID;//和cps的handle-id

	CQuerySyncSourcePeerSession* m_pQuerySyncSourcePeerSession = NULL;
	CVerifyPeerStateSession* m_pVerifyPeerStateSession = NULL;
	CSyncBlockSession* m_pSyncBlockSession = NULL;
	
	ACE_Shared_Memory_MM m_shmCfg;
	CCsConfig* m_pCsCfg; //配置相关的共享内存对象和共享内存指针

	VBFS::CVbfs m_vbfs;
	CVbhUpdateTable m_wsIndexTable; //world-state
	CVbhAppendTable m_wsHistTable; //history
	CBlockChainTable m_bcTable; //区块存储表
	
	//-----------区块缓冲区，加速读取
	ACE_UINT32 m_nMaxCachedBlockCount = 0; //可缓冲区块的最大个数，启动时从数据库读取
	ACE_UINT32 m_nCurBlockCacheSize = 0; //已经缓存的区块个数
	memblock_map_type m_mapBlockCache; //区块缓存区的map
	memblock_deque_type m_dequeBlockCache; //区块缓冲区的deque，用于快速定位最不活跃的块，头最不活跃，尾最活跃

	// 在有空洞时缓存下来的区块
	ACE_UINT32 m_nMaxUnresolvedBlockCacheCount = 0; //缓存未解析的区块个数上限
	CDscDqueue<CUnresolvedBlock> m_queueUnresolvedBlock;
	ACE_UINT32 m_nQueueUnresolvedBlockNum = 0; //缓存的区块个数

	ACE_UINT32 m_nSessionID = 0;

public:
	ICommitterAgentService* m_pCommitterAgentService = nullptr;
};

#include "com_cs/cs/committer_service.inl"

#endif