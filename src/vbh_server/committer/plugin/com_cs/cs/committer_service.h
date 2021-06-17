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

/** �ɿ��洢�㷨��
 * 1. �Ը��������Ĵ洢��ʩ��ִ��append��update����������ֻӰ���ڴ棬��Ӱ�����ô洢��
 * 2. �������ж����洢��ʩ�е�log�����Ե�mem-map�ڴ��У�log�������б��ǰ��״̬���Ա��ڽ����ָ���
 * 3. ��λ�ܵı�־λ����־λλ��committer��mem-map�ڴ��У�
 * 4. �Ը��������Ĵ洢��ʩ��Ӧ�ñ��(ApplyModify)
 * 5. ����ܵı�־λ��
 * --------------
 * �����û�ж��κ�һ��ִ��ApplyModifyǰ�����쳣����
 * --------------
 * --------------
 * �쳣����������ܱ�־λΪ ��λ ����Ը��������洢���ý��� ����־�ָ���recover-from-log������
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
		EN_MEM_BLOCK_CACHE_HASH_MAP_BITS = 20 //page-cache�Ĺ�ģ
	};

private:
	enum EnModifyStorage
	{
		EN_BEGIN_MODIFY_STORAGE = 1, //��ʼ����洢
		EN_END_MODIFY_STORAGE = 0 //��ɱ���洢
	};

	class CCsConfig
	{
	public:
		ACE_INT32 m_nModifyStorageState = EN_END_MODIFY_STORAGE; //�Ƿ������޸Ĵ洢;
		ACE_UINT64 m_nLastBlockID = 0; //��������1��block-id
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

	// ֻ�ܴ���һ����֤peer��session
	class CVerifyPeerStateSession final : public CDscServiceTimerHandler
	{
	public:
		CVerifyPeerStateSession(CCommitterService& rService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT64 m_nKafkaBlockID;   //order�˵�ǰ���kafka������߶�
		ACE_UINT64 m_nTargetBlockId;   //��Ҫͬ����Ŀ������߶�
		CDscString m_strKafkaBlockHash;
		dsc_vector_type(ACE_UINT32) m_vecCasConnHandleId;//��Cas��������Ϣ

	protected:
		CCommitterService& m_rXCommitterService;
	};

	// ֻ�ܴ���һ��ͬ�������session
	class CSyncBlockSession final : public CDscServiceTimerHandler
	{
	public:
		CSyncBlockSession(CCommitterService& rService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT64 m_nTargetBlockID;   //ͬ����Ŀ������߶�
		ACE_UINT32 m_nHandleId;
		bool m_bIsCheckBlockHash = false;  //�Ƿ����hash

	protected:
		CCommitterService& m_rXCommitterService;
	};

	//��ʾ1�δ���ı��
	class CUnresolvedBlock
	{
	public:
		CUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block);
		~CUnresolvedBlock();

	public:
		const ACE_UINT64 m_nBlockID;
		DSC::CDscBlob m_vbhBlock;

	public: //ʹ��CDscDqueueʱ��������еĳ�Ա
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
		/*endorser��ֱ�Ӳ�ѯ*/
		VBH::CQueryCryptKeyProposeEsCsReq,
		VBH::CQueryCryptKeyQueryEsCsReq,

		/*��CC�Ĳ�ѯ*/
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
	/*endorser��ֱ�Ӳ�ѯ*/
	void OnDscMsg(VBH::CQueryCryptKeyProposeEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCryptKeyQueryEsCsReq& rQueryCryptKeyReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	
	/*��CC�Ĳ�ѯ*/
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
	//�洢�յ�������
	ACE_INT32 OnReceiveBlock(ACE_UINT64 nBlockID, DSC::CDscBlob& blockData);

	//�˸���ߵ�����
	ACE_INT32 OnBackspaceBlock(void);

	//��ȡ��Ӧ�����kvlst
	ACE_INT32 GetBlockKvLst(const ACE_UINT64 nBlockID, DSC::CDscShortList<VBH::CKeyVersion>& lstKv);

	//����δ����������
	void CacheUnresolvedBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& block);

	//������֤peer״̬����
	void SendVerifyPeerStateReq(const ACE_UINT64 nKafkaBlockID, CDscString& strKafkaBlockHash, VBH::CSyncSourcePeerCasAddress & casAddress);

	//��ʼͬ����������
	void DoSyncBlock(void);

	//����ע�������CPS
	void SendRegistReq(ACE_UINT32 nHandleID);

protected:
	virtual ACE_INT32 OnConnectedNodify(CMcpClientHandler* pMcpClientHandler) override;

private:
	//���д洢��ʩ������־����������ǰ���ش���
	ACE_INT32 AllTableSaveLog(void);
	//���д洢��ʩ�ع�cache����
	void AllTableRollbackCache(void);

	//���д洢��ʩ�ύ����
	void AllTableCommitteTransaction(void);

	//���д����ǵĴ洢�豸����־�ָ���ͬʱ����Լ�������
	ACE_INT32 AllTableRecoverFromLog(void);

	//��ȡ�������� //����ָ��ע��������ʱ�����ش��� 
	//�������غ󣬱�����ʹ��transContent�󣬵��� ShrinkBlockCache�� �����ڴ��������� //��ʹ��ǰ����ShrinkBlockCache����ܻᵼ�²������ͷŵ��ڴ�
	ACE_INT32 GetTransaction(VBH::CProposeTransactionAtQuery& transContent,  VBH::CVbhAllocatedTransactionKey& transKey);

	//��ȡ����д��ֵ(user-info��information-value)�Ͷ�Ӧ�İ汾��
	//�������غ󣬱�����ʹ��wsValue�󣬵��� ShrinkBlockCache�� �����ڴ��������� //��ʹ��ǰ����ShrinkBlockCache����ܻᵼ�²������ͷŵ��ڴ�
	ACE_INT32 GetLatestWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey);

	ACE_INT32 GetWriteSetValue(DSC::CDscShortBlob& wsValue, ACE_UINT32& nVersion, const VBH::CVbhAllocatedKey& alocWsKey, VBH_CLS::CIndexTableItem& indexTableItem);

	//������״̬�л�ȡ VBH::CVbhAllocatedKey ��aloc-id��Ӧ��sequence-number
	ACE_INT32 GetSequenceNumber(ACE_UINT32& nSeq, const ACE_UINT32 nAlocID);

	//��ȡ�û��洢�ڷ���˵���Կ �������ŷ���Կ //TODO����������������Կ
	//cltPubKey, envelopeKey ָ��Ļ����������ٴζ�ȡ֮ǰ��Ч
	//�������غ󣬱�����ʹ��cltPubKey/envelopeKey�󣬵��� ShrinkBlockCache�� �����ڴ��������� //��ʹ��ǰ����ShrinkBlockCache����ܻᵼ�²������ͷŵ��ڴ�
	ACE_INT32 GetCryptKey(DSC::CDscShortBlob& cltPubKey, DSC::CDscShortBlob& envelopeKey, const VBH::CTransactionUrl& transUrl);

	//��ȡ1��mem-bc-block //���ȴӻ����ȡ��//����δ���У�����ļ���ȡ //�ᵼ�����黺������������
	VBH::CMemBcBlock* GetMemBcBlock(const ACE_UINT64 nBlockID);

	//��1��������뵽������ //����ʱ����ɾ���ɵĲ���Ծ�� //�����ᵼ��cache��������
	void InsertBlockIntoCache(VBH::CMemBcBlock* pBlock);

	//�������黺������ʹ�䲻���ڳ���Ԥ��Ĵ�С //���������ݱ�֤��������ʱ����
	void ShrinkBlockCache(void);

private:
	//�����ָ����Ϣ
	class CMemBlockPtrInfo
	{
	public:
		~CMemBlockPtrInfo();

	public:
		VBH::CMemBcBlock* m_pBcBlock = nullptr; //�����ָ��

	public: //CBareHashMap
		ACE_UINT64 m_nKey = 0;
		CMemBlockPtrInfo* m_pPrev = nullptr;
		CMemBlockPtrInfo* m_pNext = nullptr;

	public: //��ΪCVbhUniteDqueue��Ԫ����Ҫ�ĳ�Ա����
		CMemBlockPtrInfo* m_pDqueuePrev = nullptr;
		CMemBlockPtrInfo* m_pDqueueNext = nullptr;
	};
	using memblock_map_type = CBareHashMap<ACE_UINT64, CMemBlockPtrInfo, EN_MEM_BLOCK_CACHE_HASH_MAP_BITS>; //block-id -> block-ptr
	using memblock_deque_type = CVbhUniteDqueue<CMemBlockPtrInfo>; 

private:
	const ACE_UINT32 m_nChannelID;
	ACE_UINT16 m_nPeerID = 0;
	ACE_UINT16 m_nCasPort; //������������Ķ˿ں�
	CDscString m_strCasIpAddr; //�������������IP��ַ
	dsc_vector_type(ACE_UINT32) m_vecCpsHandleID;//��cps��handle-id

	CQuerySyncSourcePeerSession* m_pQuerySyncSourcePeerSession = NULL;
	CVerifyPeerStateSession* m_pVerifyPeerStateSession = NULL;
	CSyncBlockSession* m_pSyncBlockSession = NULL;
	
	ACE_Shared_Memory_MM m_shmCfg;
	CCsConfig* m_pCsCfg; //������صĹ����ڴ����͹����ڴ�ָ��

	VBFS::CVbfs m_vbfs;
	CVbhUpdateTable m_wsIndexTable; //world-state
	CVbhAppendTable m_wsHistTable; //history
	CBlockChainTable m_bcTable; //����洢��
	
	//-----------���黺���������ٶ�ȡ
	ACE_UINT32 m_nMaxCachedBlockCount = 0; //�ɻ��������������������ʱ�����ݿ��ȡ
	ACE_UINT32 m_nCurBlockCacheSize = 0; //�Ѿ�������������
	memblock_map_type m_mapBlockCache; //���黺������map
	memblock_deque_type m_dequeBlockCache; //���黺������deque�����ڿ��ٶ�λ���Ծ�Ŀ飬ͷ���Ծ��β���Ծ

	// ���пն�ʱ��������������
	ACE_UINT32 m_nMaxUnresolvedBlockCacheCount = 0; //����δ�����������������
	CDscDqueue<CUnresolvedBlock> m_queueUnresolvedBlock;
	ACE_UINT32 m_nQueueUnresolvedBlockNum = 0; //������������

	ACE_UINT32 m_nSessionID = 0;

public:
	ICommitterAgentService* m_pCommitterAgentService = nullptr;
};

#include "com_cs/cs/committer_service.inl"

#endif