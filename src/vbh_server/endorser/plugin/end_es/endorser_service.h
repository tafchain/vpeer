#ifndef ENDORSER_SERVER_SERVICE_H_516FB934995411E98B1760F18A3A20D1
#define ENDORSER_SERVER_SERVICE_H_516FB934995411E98B1760F18A3A20D1

#include "dsc/protocol/hts/dsc_hts_service.h"
#include "dsc/protocol/mcp/mcp_asynch_acceptor.h"
#include "dsc/container/bare_hash_map.h"
#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/protocol/mcp/mcp_server_handler.h"

#include "vbh_comm/vbh_comm_msg_def.h"
#include "vbh_server_comm/vbh_committer_router.h"

#include "cc_comm/i_comm_cc_service.h"
#include "end_comm/i_endorser_service.h"
#include "end_comm/i_transfer_agent_service.h"

class PLUGIN_EXPORT CEndorserService : public CDscHtsServerService, public IEndorserService
{
public:
	enum 
	{
		EN_SERVICE_TYPE = VBH::EN_ENDORSER_SERVICE_TYPE,
		EN_SERVICE_CONTAINER_TYPE = VBH::EN_ENDORSER_SERVICE_CONTAINER_TYPE,
		EN_AGENT_SERVICE_TYPE = VBH::EN_TRANSFER_AGENT_SERVICE_TYPE
	};

private:
	enum
	{
		EN_LONG_PROCESS_HASH_MAP_BITS = 20, //长流程的session-map规模,
		EN_SHORT_PROCESS_HASH_MAP_BITS = 16, //短流程的session-map规模
		EN_SESSION_TIMEOUT_VALUE = 60
	};
public:
	using inner_cc_service_list_type = std::list< std::pair<ACE_UINT16, ICommCcService*> >;

private:
	class CEndorserServiceHandler;

	class IUserSession : public CDscServiceTimerHandler
	{
	public:
		IUserSession(CEndorserService& rEndorserService);

	public:
		//在network error中，遍历handle-session的user-session数组，来调用该 OnNetError 函数，
		//由于数组的实现，该函数中不能再把自己所处的session从 handle-session的user-session数组中删除
		virtual void OnNetError(void) = 0;

	public:
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nCltSessionID;
		CEndorserServiceHandler* m_pEndorserServiceHandler;

	public:
		ACE_UINT32 m_nIndex = 0; //使用 CDscTypeArray 容器必须具备的接口

	protected:
		CEndorserService& m_rEndorserService;	
	};

	class CRegistUserSession : public IUserSession
	{
	public:
		CRegistUserSession(CEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		bool m_bSubmitNode; //本节点是负责提交的节点
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_ccGenerateUserInfo; //CC允许注册时，为用户生成的初始信息

	public:
		ACE_UINT32 m_nKey = 0;
		CRegistUserSession* m_pPrev = NULL;
		CRegistUserSession* m_pNext = NULL;
	};

	class CCreateInformationSession : public IUserSession
	{
	public:
		CCreateInformationSession(CEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		bool m_bSubmitNode; //本节点是负责提交的节点
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_strSignature; //针对创建提案的签名
		CDscString m_strProposal; //创建information的提案
		CDscString m_strInitValue; //CC允许注册时，为information生成的初始值

	public:
		ACE_UINT32 m_nKey = 0;
		CCreateInformationSession* m_pPrev = NULL;
		CCreateInformationSession* m_pNext = NULL;
	};

	class CProposeSession : public IUserSession
	{
	public:
		CProposeSession(CEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		bool m_bSubmitNode; //本节点是负责提交的节点
		ACE_UINT32 m_nChannelID; //channel id  
		ACE_UINT32 m_nActionID;
		CDscString m_strTransContent; //事务内容

	public:
		ACE_UINT32 m_nKey = 0;
		CProposeSession* m_pPrev = NULL;
		CProposeSession* m_pNext = NULL;
	};

	class CQueryUserInfoSession : public IUserSession 
	{
	public:
		CQueryUserInfoSession(CEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		ACE_UINT32 m_nKey = 0;
		CQueryUserInfoSession* m_pPrev = NULL;
		CQueryUserInfoSession* m_pNext = NULL;
	};

	class CQueryTransactionSession : public IUserSession
	{
	public:
		CQueryTransactionSession(CEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		ACE_UINT32 m_nKey = 0;
		CQueryTransactionSession* m_pPrev = NULL;
		CQueryTransactionSession* m_pNext = NULL;
	};

	class CEndorserServiceHandler : public CMcpServerHandler
	{
	public:
		CEndorserServiceHandler(CMcpServerService& rService, ACE_HANDLE handle, const ACE_INT32 nHandleID);

	public:
		CDscTypeArray<IUserSession> m_arrUserSession;
	};

public:
	CEndorserService(const CDscString& strIpAddr, const ACE_UINT16 nPort);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;

	//设置和本endorser背靠背的transfer-agent-service
	void SetTransferAgentService(ITransferAgentService* pTas);

	//设置和本endorser背靠背的 inner-cc-service-map
	void SetInnerCcServiceMap(inner_cc_service_list_type& lstInnerCcService);

public:
	void OnTimeOut(CRegistUserSession* pRegistUserSession);
	void OnTimeOut(CCreateInformationSession* pCreateInfoSession);
	void OnTimeOut(CProposeSession* pProposeSession);
	void OnTimeOut(CQueryUserInfoSession* pQueryUserSession);
	void OnTimeOut(CQueryTransactionSession* pQueryTransSession);

	void OnNetError(CRegistUserSession* pRegistUserSession);
	void OnNetError(CCreateInformationSession* pCreateInfoSession);
	void OnNetError(CProposeSession* pProposeSession);
	void OnNetError(CQueryUserInfoSession* pQueryUserSession);
	void OnNetError(CQueryTransactionSession* pQueryTransSession);

	template <typename SESSION_TYPE>
	void OnRelease(SESSION_TYPE* pSession); //所有session的统一释放函数

public:
	static void GetConfigTableName(CDscString& strConfigTableName);

protected:
	BEGIN_HTS_MESSAGE_BIND
	/*注册用户流程相关*/
	BIND_HTS_MESSAGE(VBH::CRegistUserCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitRegistUserTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelRegistUserCltEsReq)
	/*创建information流程相关*/
	BIND_HTS_MESSAGE(VBH::CCreateInformationCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitCreateInformationTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelCreateInformationCltEsReq)
	/*发起提案事务流程相关*/
	BIND_HTS_MESSAGE(VBH::CProposeCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitProposalTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelProposalTransactionCltEsReq)
	/*直接查询用户流程相关*/
	BIND_HTS_MESSAGE(VBH::CQueryUserInfoCltEsReq)
	/*直接查询事务流程相关*/
	BIND_HTS_MESSAGE(VBH::CQueryTransInfoCltEsReq)
	END_HTS_MESSAGE_BIND

public:
	/*注册用户流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelRegistUserCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*创建information流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CCreateInformationCltEsReq& rCreateInfoReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitCreateInformationTransactionCltEsReq& rSubmitCreateInfoReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelCreateInformationCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*发起提案事务流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CProposeCltEsReq& rSubmitProposalReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelProposalTransactionCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*直接查询用户流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CQueryUserInfoCltEsReq& rQueryUserReq, CMcpHandler* pMcpHandler);
	/*直接查询事务流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CQueryTransInfoCltEsReq& rQueryTransReq, CMcpHandler* pMcpHandler);

protected:
	BEGIN_BIND_DSC_MESSAGE
	/*注册用户流程相关*/
	DSC_BIND_MESSAGE(VBH::CRegistUserCcEsRsp) //endorser继续保留这个消息的接收，以兼容outer-cc
	/*创新information*/
	DSC_BIND_MESSAGE(VBH::CCreateInformationCcEsRsp) //endorser继续保留这个消息的接收，以兼容outer-cc
	/*发起提案事务流程相关*/
	DSC_BIND_MESSAGE(VBH::CProposeCcEsRsp) //endorser继续保留这个消息的接收，以兼容outer-cc
	/*直接查询用户流程相关*/
	DSC_BIND_MESSAGE(VBH::CQueryUserInfoXcsEsRsp)
	/*直接查询事务流程相关*/
	DSC_BIND_MESSAGE(VBH::CQueryCryptKeyGetTransXcsEsRsp)
	DSC_BIND_MESSAGE(VBH::CQueryTransInfoXcsEsRsp)
	//CExplorerQueryCcEsRsp 非加密通道暂时不实现 浏览器查询通道
	END_BIND_DSC_MESSAGE

public:
	void OnDscMsg(VBH::CRegistUserCcEsRsp& rRegistUserRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CCreateInformationCcEsRsp& rCreateInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CProposeCcEsRsp& rSubmitProposalRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryUserInfoXcsEsRsp& rQueryUserInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCryptKeyGetTransXcsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryTransInfoXcsEsRsp& rQueryTransInfoRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

public:
	virtual void SendSubmitRegistUserTransactionTasEsRsp(VBH::CSubmitRegistUserTransactionTasEsRsp& rSubmitTransactionRsp) final;
	virtual void SendSubmitCreateInformationTransactionTasEsRsp(VBH::CSubmitCreateInformationTransactionTasEsRsp& rSubmitTransactionRsp) final;
	virtual void SendSubmitProposalTransactionTasEsRsp(VBH::CSubmitProposalTransactionTasEsRsp& rSubmitTransactionRsp) final;

	virtual void SendRegistUserCcEsRsp(VBH::CRegistUserCcEsRsp& rRegistUserRsp) final;
	virtual void SendCreateInformationCcEsRsp(VBH::CCreateInformationCcEsRsp& rCreateInfoRsp) final;
	virtual void SendProposeCcEsRsp(VBH::CProposeCcEsRsp& rSubmitProposalRsp) final;
	virtual void SendExplorerQueryCcEsRsp(VBH::CExplorerQueryCcEsRsp& rsp) final;

protected:
	virtual void OnNetworkError(CMcpHandler* pMcpHandler) override;
	virtual CMcpServerHandler* AllocMcpHandler(ACE_HANDLE handle) override;

private:
	//为session设置 mcp-handle-session, mcp-hanle-session用于通信
	template<typename SESSION_TYPE>
	void SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler);

private:
	//注册用户
	using regist_user_session_map_type = CBareHashMap<ACE_UINT32, CRegistUserSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//创建information
	using create_information_session_map_type = CBareHashMap<ACE_UINT32, CCreateInformationSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//发起提案
	using propose_session_map_type = CBareHashMap<ACE_UINT32, CProposeSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//查询用户
	using query_user_info_map_type = CBareHashMap<ACE_UINT32, CQueryUserInfoSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;
	//查询提案
	using query_trans_map_type = CBareHashMap<ACE_UINT32, CQueryTransactionSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;

private:
	//tas和inner-cc被设置为和endorser运行于同一个线程中，这样，消息发送可以转为函数调用，因此这里保留对象指针，方便函数调用
	ITransferAgentService* m_pTas; 
	IInnerCcService* m_arrHashInnerCcService[1 << 16] = {nullptr}; //cc的最多种类数是2^16个

	const CDscString m_strIpAddr;
	const ACE_UINT16 m_nPort;
	CMcpAsynchAcceptor<CEndorserService>* m_pAcceptor = NULL;

	VBH::CXCommitterServiceRouter m_xcsRouter;

	regist_user_session_map_type m_mapRegistUserSession;
	create_information_session_map_type m_mapCreateInfoSession;
	propose_session_map_type m_mapProposeSession;
	query_user_info_map_type m_mapQueryUserInfoSession;
	query_trans_map_type m_mapQueryTransSession;

	ACE_UINT32 m_nSessionID = 0;
};

#include "end_es/endorser_service.inl"

#endif
