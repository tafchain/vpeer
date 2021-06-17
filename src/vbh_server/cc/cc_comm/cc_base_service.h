#ifndef CC_BASE_SERVICE_H_34297132321723146321385479
#define CC_BASE_SERVICE_H_34297132321723146321385479

#include "dsc/container/bare_hash_map.h"
#include "dsc/container/dsc_queue.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_cs_def.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"
#include "vbh_comm/vbh_comm_func.h"

#include "vbh_server_comm/vbh_committer_router.h"
#include "cc_comm/i_comm_cc_service.h"
#include "cc_comm/cc_comm_def_export.h"
#include "cc_comm/base_inner_service.h"
#include "cc_comm/i_outer_cc_service.h"


class CCcBaseService : public ICommCcService, public CBaseInnerService
{

private:

	class COuterCcCfg
	{
	public:
		ACE_UINT16 m_nChannelID; //channel-id����inner-cc-type
		ACE_UINT16 m_nCcID;
		CDscString m_strCcName;
	};
	class CDBOuterCcCfg
	{
	public:
		CDBOuterCcCfg()
			: m_ccName("CC_NAME")
			, m_channelID("CHANNEL_ID")
			, m_ccID("CC_ID")
		{
		}

	public:
		PER_BIND_ATTR(m_ccName, m_channelID, m_ccID);

	public:
		CColumnWrapper< CDscString > m_ccName;
		CColumnWrapper< ACE_INT32 > m_channelID;
		CColumnWrapper< ACE_INT32 > m_ccID;
	};
	class CDBOuterCcCfgCriterion : public CSelectCriterion
	{
	public:
		virtual void SetCriterion(CPerSelect& rPerSelect)
		{
			rPerSelect.Where(rPerSelect["INUSE"] == 1);
		}
	};

	class ICcSession : public CDscServiceTimerHandler
	{	
	public:
		ICcSession(CCcBaseService& rCcService);

	public:
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nChannelID;
		CDscMsg::CDscMsgAddr m_esAddr;

	protected:
		CCcBaseService& m_rCcService;
	};

	class CRegistUserSession : public ICcSession
	{
	public:
		CRegistUserSession(CCcBaseService& rCcService);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT32 m_nKey = 0;
		CRegistUserSession* m_pPrev = NULL;
		CRegistUserSession* m_pNext = NULL;
	};


	class CProposalSession : public ICcSession
	{
	public:
		CProposalSession(CCcBaseService& rCcService);
		~CProposalSession(); //�����������ͷŶ����е��ڴ�

	public:
		CCcWsKV* FindWriteSet(const ACE_UINT64 nAllocatedID);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT32 m_nActionID;
		VBH::CVbhAllocatedKey m_proposeUserKey; //�᰸������
		CDscString m_signature; //����᰸��ǩ��
		CDscString m_proposal; //�᰸����

		CDscUnboundQueue<CCcWsKV> m_queueWriteSetItem; //�漰 ��ȡ ���û��б�

	public:
		ACE_UINT32 m_nKey = 0;
		CProposalSession* m_pPrev = NULL;
		CProposalSession* m_pNext = NULL;
	};

	class CQueryBatchAtQuery //1����ѯ
	{
	public:
		ACE_UINT8 m_nWsType; //��β�ѯʱ��д���ı�������
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //Ҫ��ѯ��key�б�

	public://ʹ�� CDscTypeArray ��������߱��Ľӿ�
		ACE_UINT32 m_nIndex = CDscTypeArray<CQueryBatchAtQuery>::EN_INVALID_INDEX_ID;
	};

	class CQuerySession : public ICcSession
	{
	public:
		CQuerySession(CCcBaseService& rCcService);
		~CQuerySession();

	public:
		virtual void OnTimer(void) override;

	public:
		//�ϳɲ�ѯʱʹ�õ�session-id
		static ACE_UINT64 CombineID(const ACE_UINT32 nSessionID, const ACE_UINT32 nIndex);
		//��ֲ�ѯʱʹ�õ�session-id
		static void SplitID(ACE_UINT32& nSessionID, ACE_UINT32& nIndex, const ACE_UINT64 nQuerySessionID);

	public:
		VBH::CVbhAllocatedKey m_queryUserKey; //��ѯ������
		CDscTypeArray<CQueryBatchAtQuery> m_vecQueryBatch; //��ѯ�����б�

	public:
		ACE_UINT32 m_nKey = 0;
		CQuerySession* m_pPrev = NULL;
		CQuerySession* m_pNext = NULL;
	};

	//�ռ�������ѯ��session  //�����᰸ʱʹ��
	class CProposeBatchQueryTask : public CDscServiceTimerHandler
	{
	public:
		//������ѯ��item
		class CItem
		{
		public:
			ACE_UINT8 m_nWsType; //��β�ѯʱ��д���ı�������
			ACE_UINT32 m_nCcSessionID; //��β�ѯ�����������ĸ�cc-session
			VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //Ҫ��ѯ��key�б�

		public: //ʹ��CDscUnboundQueue��Ҫ�Ľӿ�
			CItem* m_pNext = NULL;
		};

	public:
		CProposeBatchQueryTask(CCcBaseService& rCcService);

	public:
		//��ղ�ѯitem����
		void ClearQueryQueue(void);

		virtual void OnTimer(void) override;

	public:
		ACE_UINT16 m_nQueryItemCount = 0; //����ѯ�Ķ��г���
		CDscUnboundQueue<CProposeBatchQueryTask::CItem> m_queueQueryItem; //����һ��������ѯ���item�б�

	protected:
		CCcBaseService& m_rCcService;
	};

	//�ȴ���ѯӦ���session //�����᰸ʱʹ��
	class CProposeBatchQueryWaitRspSession
	{
	public:
		class CItem
		{
		public:
			ACE_UINT8 m_nWsType; //��β�ѯʱ��д���ı�������
			ACE_UINT32 m_nCcSessionID; //��β�ѯ�����������ĸ�cc-session
			VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //��cc-sessionҪ��ѯ��key�б�
		};

	public:
		VBH::CSimpleVector<CProposeBatchQueryWaitRspSession::CItem> m_vecQueryItem; //����һ��������ѯ���item�б� //�Ͳ�ѯ������Ӧ���е�˳��һ��

	public:
		ACE_UINT32 m_nKey = 0;
		CProposeBatchQueryWaitRspSession* m_pPrev = NULL;
		CProposeBatchQueryWaitRspSession* m_pNext = NULL;
	};

public:
	CCcBaseService(ACE_UINT32 nChannelID);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;

protected:
	BIND_DSC_MESSAGE(
		/*�᰸����*/
		VBH::CQueryWriteSetListProposeCsCcRsp,
		/*��ѯ����*/
		VBH::CQueryWriteSetListQueryCsCcRsp,
		VBH::CQueryTransactionQueryCsCcRsp,
		/*��ѯ�����漰��Ϣ---��������*/
		VBH::CQueryBlockHeaderInfoExplorerCsCcRsp,
		VBH::CQueryBlockCountExplorerCsCcRsp,
		VBH::CQueryWriteSetExplorerCsCcRsp,
		VBH::CQueryTransInfoExplorerCsCcRsp,
		VBH::CQueryTransListExplorerCsCcRsp,
		VBH::CQueryTransCountExplorerCsCcRsp
	)

public:
	/*�᰸����*/
	void OnDscMsg(VBH::CQueryWriteSetListProposeCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

	/*��ѯ����*/
	void OnDscMsg(VBH::CQueryWriteSetListQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransactionQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

	/*�����ѯ�����漰��Ϣ---��������*/
	void OnDscMsg(VBH::CQueryBlockHeaderInfoExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryBlockCountExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryWriteSetExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransInfoExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransListExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransCountExplorerCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryInformationHistoryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	
protected:
	virtual ACE_INT32 OnRecvRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) override;
	virtual ACE_INT32 OnRecvProposalEsCcReq(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) override;
	virtual ACE_INT32 OnRecvQueryEsCcReq(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) override;
	virtual ACE_INT32 OnLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) override;
	
public:
	void OnTimeOut(CRegistUserSession* pRegistUserSession);
	void OnTimeOut(CProposalSession* pProposalSession);
	void OnTimeOut(CQuerySession* pQuerySession);

	void OnTimeBatchQuery(void);//��ʱ������ѯ



public:
	//��֤������ָ�û��Ƿ����᰸�����û� //�����������û�һ���ǰ� �᰸�û���ʽ���еı���
	virtual ACE_INT32 VerifyProposeUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)override;

	//��֤�����Ƿ��Ƿ����ѯ���û�
	virtual ACE_INT32 VerifyQueryUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)override;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û���Ϣ
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) override;
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//�����ϲ��߼�ʹ�ã� ��ȡ��Ϣ
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û���Ϣ
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//�����ϲ��߼�ʹ�ã� ��ȡ��Ϣ
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û�������Ϣ
	virtual ACE_INT32 GetTransAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;

	virtual ACE_INT32 GetInformationHistoryAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;

	//�����ϲ��߼�ʹ�ã� ע�����������
	virtual ACE_INT32 RegistUserRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, char* pUserInitInfo, const size_t nUserInitInfo)override;

	//�����ϲ��߼�ʹ�ã� �᰸������� //TODO: ��̨CC����ͨ���˺����޸����մ���������е��᰸����
	virtual ACE_INT32 ProposalRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID,
		const CProposeSimpleUser* pUserArry, const ACE_UINT16 nUserArrayLen,
		const CProposeSimpleInformation* pInfoArry = nullptr, const ACE_UINT16 nInfoArrayLen = 0,
		const DSC::CDscShortBlob& receipt = DSC::CDscShortBlob(nullptr, 0))override;

	//��ѯӦ��
	virtual ACE_INT32 QueryRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const DSC::CDscBlob& info)override;

protected: //������ʾ�� ��������

	//��ʽ��Ϊhex��ʽ
	void HexFormat(CDscString& strFormatBuf, const DSC::CDscShortBlob& content);
	IOuterCcService* GetOuterCcService(const ACE_UINT32 nAction);
private:
		using regist_user_session_map_type = CBareHashMap<ACE_UINT32, CRegistUserSession, EN_HASH_MAP_BITES>;
		using proposal_session_map_type = CBareHashMap<ACE_UINT32, CProposalSession, EN_HASH_MAP_BITES>;
		using query_session_map_type = CBareHashMap<ACE_UINT32, CQuerySession, EN_HASH_MAP_BITES>;

		using propose_batch_query_wait_rsp_session_map_type = CBareHashMap<ACE_UINT32, CProposeBatchQueryWaitRspSession, EN_PROPOSE_BATCH_QUERY_HASH_MAP_BITS>;
		using outer_cc_cfg_list_type = dsc_list_type(COuterCcCfg);
		using outer_cc_service_map_type = CBareHashMap<ACE_UINT64, IOuterCcService, EN_HASH_MAP_BITES>;

private:
	ACE_UINT32 AllocSessionID(void);

	//��ȡд������ GetVbhUser �� GetVbhInformation ��ʵ��ʵ�ֺ���
	ACE_INT32 GetWriteSetAtPropose(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen);

	ACE_INT32 GetWriteSetAtQuery(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen);

	ACE_INT32 LoadInnerCcInDLL(const CDscString& strCcName, IOuterCcService** ppzService, ACE_UINT32 nChannelID);
	ACE_INT32  ReadDBConfig(outer_cc_cfg_list_type& lstInnerCcCfg, const char* pInnerCcCfgTableName);



private:
	const ACE_UINT32 m_nChannelID; //��cc�����channel
	CDscMsg::CDscMsgAddr m_xcsAddr; //��channel��Ӧ��xcs��ַ

	ACE_UINT32 m_nSessionID = 0;
	regist_user_session_map_type m_mapRegistUserSession;
	proposal_session_map_type m_mapProposalSession;
	query_session_map_type m_mapQuerySession;

private: //������ѯ //cc��������1��channel�����������ð�channel��map
	CProposeBatchQueryTask* m_pBatchQueryTask = nullptr;
	ACE_UINT32 m_nBatchQueryTimeoutValue = 0; //������ѯ�Ķ�ʱʱ��
	ACE_UINT32 m_nBatchQueryMaxSizeValue = 0; //������ѯ�Ķ�������
	propose_batch_query_wait_rsp_session_map_type m_mapBatchQueryWaitRspSession;
	outer_cc_service_map_type m_mapOuterCcService;
};

class COuterCCServiceFactory : public IDscServiceFactory
{
public:
	virtual CDscService* CreateDscService(void);

public:
	IOuterCcService* m_pOuterCCService = NULL;
};

using CInnerCcBaseService = CCcBaseService;



#include "cc_comm/cc_base_service.inl"

#endif
