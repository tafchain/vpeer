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
		ACE_UINT16 m_nChannelID; //channel-id就是inner-cc-type
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
		~CProposalSession(); //析构函数，释放队列中的内存

	public:
		CCcWsKV* FindWriteSet(const ACE_UINT64 nAllocatedID);

	public:
		virtual void OnTimer(void) override;

	public:
		ACE_UINT32 m_nActionID;
		VBH::CVbhAllocatedKey m_proposeUserKey; //提案发起人
		CDscString m_signature; //针对提案的签名
		CDscString m_proposal; //提案内容

		CDscUnboundQueue<CCcWsKV> m_queueWriteSetItem; //涉及 读取 的用户列表

	public:
		ACE_UINT32 m_nKey = 0;
		CProposalSession* m_pPrev = NULL;
		CProposalSession* m_pNext = NULL;
	};

	class CQueryBatchAtQuery //1批查询
	{
	public:
		ACE_UINT8 m_nWsType; //这次查询时，写集的表现类型
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //要查询的key列表

	public://使用 CDscTypeArray 容器必须具备的接口
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
		//合成查询时使用的session-id
		static ACE_UINT64 CombineID(const ACE_UINT32 nSessionID, const ACE_UINT32 nIndex);
		//拆分查询时使用的session-id
		static void SplitID(ACE_UINT32& nSessionID, ACE_UINT32& nIndex, const ACE_UINT64 nQuerySessionID);

	public:
		VBH::CVbhAllocatedKey m_queryUserKey; //查询发起人
		CDscTypeArray<CQueryBatchAtQuery> m_vecQueryBatch; //查询批次列表

	public:
		ACE_UINT32 m_nKey = 0;
		CQuerySession* m_pPrev = NULL;
		CQuerySession* m_pNext = NULL;
	};

	//收集批量查询的session  //处理提案时使用
	class CProposeBatchQueryTask : public CDscServiceTimerHandler
	{
	public:
		//批量查询的item
		class CItem
		{
		public:
			ACE_UINT8 m_nWsType; //这次查询时，写集的表现类型
			ACE_UINT32 m_nCcSessionID; //这次查询任务来自于哪个cc-session
			VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //要查询的key列表

		public: //使用CDscUnboundQueue需要的接口
			CItem* m_pNext = NULL;
		};

	public:
		CProposeBatchQueryTask(CCcBaseService& rCcService);

	public:
		//清空查询item队列
		void ClearQueryQueue(void);

		virtual void OnTimer(void) override;

	public:
		ACE_UINT16 m_nQueryItemCount = 0; //待查询的队列长度
		CDscUnboundQueue<CProposeBatchQueryTask::CItem> m_queueQueryItem; //放在一个批量查询里的item列表

	protected:
		CCcBaseService& m_rCcService;
	};

	//等待查询应答的session //处理提案时使用
	class CProposeBatchQueryWaitRspSession
	{
	public:
		class CItem
		{
		public:
			ACE_UINT8 m_nWsType; //这次查询时，写集的表现类型
			ACE_UINT32 m_nCcSessionID; //这次查询任务来自于哪个cc-session
			VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecAlocKey; //该cc-session要查询的key列表
		};

	public:
		VBH::CSimpleVector<CProposeBatchQueryWaitRspSession::CItem> m_vecQueryItem; //放在一个批量查询里的item列表 //和查询回来的应答中的顺序一致

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
		/*提案流程*/
		VBH::CQueryWriteSetListProposeCsCcRsp,
		/*查询流程*/
		VBH::CQueryWriteSetListQueryCsCcRsp,
		VBH::CQueryTransactionQueryCsCcRsp,
		/*查询流程涉及消息---浏览器相关*/
		VBH::CQueryBlockHeaderInfoExplorerCsCcRsp,
		VBH::CQueryBlockCountExplorerCsCcRsp,
		VBH::CQueryWriteSetExplorerCsCcRsp,
		VBH::CQueryTransInfoExplorerCsCcRsp,
		VBH::CQueryTransListExplorerCsCcRsp,
		VBH::CQueryTransCountExplorerCsCcRsp
	)

public:
	/*提案流程*/
	void OnDscMsg(VBH::CQueryWriteSetListProposeCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

	/*查询流程*/
	void OnDscMsg(VBH::CQueryWriteSetListQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransactionQueryCsCcRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

	/*纯粹查询流程涉及消息---浏览器相关*/
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

	void OnTimeBatchQuery(void);//定时定量查询



public:
	//验证参数所指用户是否是提案发起用户 //参数所传递用户一定是按 提案用户格式进行的编码
	virtual ACE_INT32 VerifyProposeUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)override;

	//验证参数是否是发起查询的用户
	virtual ACE_INT32 VerifyQueryUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey)override;

	//留给上层逻辑使用： 获取用户信息
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) override;
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//留给上层逻辑使用： 获取信息
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//留给上层逻辑使用： 获取用户信息
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//留给上层逻辑使用： 获取信息
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen)override;

	//留给上层逻辑使用： 获取用户交易信息
	virtual ACE_INT32 GetTransAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;

	virtual ACE_INT32 GetInformationHistoryAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key)override;

	//留给上层逻辑使用： 注册请求处理完毕
	virtual ACE_INT32 RegistUserRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, char* pUserInitInfo, const size_t nUserInitInfo)override;

	//留给上层逻辑使用： 提案处理完毕 //TODO: 中台CC可以通过此函数修改最终打包到区块中的提案内容
	virtual ACE_INT32 ProposalRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID,
		const CProposeSimpleUser* pUserArry, const ACE_UINT16 nUserArrayLen,
		const CProposeSimpleInformation* pInfoArry = nullptr, const ACE_UINT16 nInfoArrayLen = 0,
		const DSC::CDscShortBlob& receipt = DSC::CDscShortBlob(nullptr, 0))override;

	//查询应答
	virtual ACE_INT32 QueryRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const DSC::CDscBlob& info)override;

protected: //解析显示类 函数定义

	//格式化为hex格式
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

	//获取写集，是 GetVbhUser 和 GetVbhInformation 的实际实现函数
	ACE_INT32 GetWriteSetAtPropose(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen);

	ACE_INT32 GetWriteSetAtQuery(const ACE_UINT32 nCcSessionID, const ACE_UINT8 nWsType, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen);

	ACE_INT32 LoadInnerCcInDLL(const CDscString& strCcName, IOuterCcService** ppzService, ACE_UINT32 nChannelID);
	ACE_INT32  ReadDBConfig(outer_cc_cfg_list_type& lstInnerCcCfg, const char* pInnerCcCfgTableName);



private:
	const ACE_UINT32 m_nChannelID; //本cc服务的channel
	CDscMsg::CDscMsgAddr m_xcsAddr; //本channel对应的xcs地址

	ACE_UINT32 m_nSessionID = 0;
	regist_user_session_map_type m_mapRegistUserSession;
	proposal_session_map_type m_mapProposalSession;
	query_session_map_type m_mapQuerySession;

private: //批量查询 //cc仅服务于1个channel，批量请求不用按channel做map
	CProposeBatchQueryTask* m_pBatchQueryTask = nullptr;
	ACE_UINT32 m_nBatchQueryTimeoutValue = 0; //批量查询的定时时间
	ACE_UINT32 m_nBatchQueryMaxSizeValue = 0; //批量查询的定量个数
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
