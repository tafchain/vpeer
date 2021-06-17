#ifndef CRYPT_ENDORSER_SERVER_SERVICE_H_5USADOIFUASOFHASLDKFHFHDIOFHEH
#define CRYPT_ENDORSER_SERVER_SERVICE_H_5USADOIFUASOFHASLDKFHFHDIOFHEH

#include "dsc/protocol/hts/dsc_hts_service.h"
#include "dsc/protocol/mcp/mcp_asynch_acceptor.h"
#include "dsc/container/bare_hash_map.h"
#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/protocol/mcp/mcp_server_handler.h"
#include "dsc/container/dsc_dqueue.h"

#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"
#include "vbh_comm/vbh_encrypt_lib.h"
#include "vbh_server_comm/vbh_committer_router.h"

#include "cc_comm/i_endorser_cc_service.h"
#include "end_comm/i_endorser_service.h"
#include "end_comm/i_transfer_agent_service.h"

class PLUGIN_EXPORT CCryptEndorserService : public CDscHtsServerService, public IEndorserService
{
public:
	enum 
	{
		EN_SERVICE_TYPE = VBH::EN_CRYPT_ENDORSER_SERVICE_TYPE,
		EN_SERVICE_CONTAINER_TYPE = VBH::EN_CRYPT_ENDORSER_SERVICE_CONTAINER_TYPE,
		EN_AGENT_SERVICE_TYPE = VBH::EN_CRYPT_TRANSFER_AGENT_SERVICE_TYPE
	};

private:
	enum
	{
		EN_LONG_PROCESS_HASH_MAP_BITS = 20, //长流程的session-map规模,
		EN_SHORT_PROCESS_HASH_MAP_BITS = 16, //短流程的session-map规模
		EN_SESSION_TIMEOUT_VALUE = 90,
		EN_CRYPT_KEY_CACHE_SIZE = 1*1024*1024 //秘钥的最多缓存个数
	};
public:
	using inner_cc_service_list_type = std::list< std::pair<ACE_UINT16, IEndorserCcService*> >;

private:
	class CEndorserServiceHandler;

	class IUserSession : public CDscServiceTimerHandler
	{
	public:
		IUserSession(CCryptEndorserService& rEndorserService);

	public:
		//在network error中，遍历handle-session的user-session数组，来调用该 OnNetError 函数，
		//由于数组的实现，该函数中不能再把自己所处的session从 handle-session的user-session数组中删除
		virtual void OnNetError(void) = 0;

	public:
		ACE_INT32 m_nNonce; //通信中统一要用的nonce

		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nCltSessionID;
		CEndorserServiceHandler* m_pEndorserServiceHandler;

	public:
		ACE_UINT32 m_nIndex = CDscTypeArray<IUserSession>::EN_INVALID_INDEX_ID; //使用 CDscTypeArray 容器必须具备的接口

	protected:
		CCryptEndorserService& m_rEndorserService;	
	};

	class CRegistUserSession : public IUserSession
	{
	public:
		CRegistUserSession(CCryptEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		bool m_bSubmitNode; //本节点是负责提交的节点
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_ccGenerateUserInfo; //CC允许注册时，为用户生成的初始信息
		
		CDscString m_strSvrPubKey; //服务端公钥

	public:
		ACE_UINT32 m_nKey = 0;
		CRegistUserSession* m_pPrev = NULL;
		CRegistUserSession* m_pNext = NULL;
	};

	class CProposeSession : public IUserSession
	{
	public:
		CProposeSession(CCryptEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		ACE_UINT8 m_nSubmitNodeType; //本节点是否负责提交的节点
		ACE_UINT32 m_nChannelID;
		ACE_UINT32 m_nActionID;
		VBH::CVbhAllocatedKey m_userKey; //提案发起者

		CDscString m_userData; //存放客户端发送的加密后user data; 
		CDscString m_strEnvelopeKey; //对称加密秘钥
		CDscString m_strTransContent; //事务内容

		CDscString m_strSignature; //签名，用于验签
		CDscString m_strProposal; //提案，用于验签
		CDscString m_strCltPubKey; //客户端公钥，用于验签
		CDscString m_strCcReceipt; //cc处理提案时生成的回执

	public:
		ACE_UINT32 m_nKey = 0;
		CProposeSession* m_pPrev = NULL;
		CProposeSession* m_pNext = NULL;
	};

	class CQuerySession : public IUserSession 
	{
	public:
		CQuerySession(CCryptEndorserService& rEndorserService);

	public:
		virtual void OnTimer(void) override;
		virtual void OnNetError(void) override;

	public:
		ACE_UINT32 m_nChannelID;
		VBH::CVbhAllocatedKey m_userKey; //发起查询的人
		CDscString m_userData;
		CDscString m_strEnvelopeKey; //存放在服务器侧的对称秘钥

	public:
		ACE_UINT32 m_nKey = 0;
		CQuerySession* m_pPrev = NULL;
		CQuerySession* m_pNext = NULL;
	};


	class CLoadCcSession : public CDscServiceTimerHandler
	{
	public:
		CLoadCcSession(CCryptEndorserService& rEndorserService);

		virtual void OnTimer(void) override;
		void OnNetError(void) ;
	public:
		CDscMsg::CDscMsgAddr m_rSrcMsgAddr;
		ACE_UINT32 m_nSessionID;
	public:
		ACE_UINT32 m_nKey = 0;
		CLoadCcSession* m_pPrev = NULL;
		CLoadCcSession* m_pNext = NULL;
		CEndorserServiceHandler* m_pEndorserServiceHandler;

	protected:
		CCryptEndorserService& m_rEndorserService;

	};

	class CEndorserServiceHandler : public CMcpServerHandler
	{
	public:
		CEndorserServiceHandler(CMcpServerService& rService, ACE_HANDLE handle, const ACE_INT32 nHandleID);

	public:
		CDscTypeArray<IUserSession> m_arrUserSession;
	};

	//存放user-crypt map的key
	class CUserKeyAsMapKey
	{
	public:
		ACE_UINT32 m_nChannelID;
		ACE_UINT64 m_nUserAllocatedID;
	
	public:
		bool operator< (const CUserKeyAsMapKey& key) const;
	};

	//存放user-crypt map的value
	class CCryptKey
	{
	public:
		CDscString m_strCltPubKey;
		CDscString m_strEnvelopeKey;
		
	public:
		CUserKeyAsMapKey m_mapKey; //插入到map中时，对应的map-key //从dqueue中直接删除时，需要用它从map中删除数据

	public: //作为CDDqueue的元素需要的成员变量
		CCryptKey* m_pPrev = nullptr;
		CCryptKey* m_pNext = nullptr;
	};

	using CCryptKeyPtrAsMapValue = CCryptKey *;

public:
	CCryptEndorserService(const CDscString& strIpAddr, const ACE_UINT16 nPort);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;

	//设置和本endorser背靠背的transfer-agent-service
	void SetTransferAgentService(ITransferAgentService* pTas);

	//设置和本endorser背靠背的 inner-cc-service-map
	void SetInnerCcServiceMap(inner_cc_service_list_type& lstInnerCcService);

public:
	void OnTimeOut(CRegistUserSession* pRegistUserSession);
	void OnTimeOut(CProposeSession* pProposeSession);
	void OnTimeOut(CQuerySession* pQueryUserSession);
	void OnTimeOut(CLoadCcSession* pLoadCcSession);


	void OnNetError(CRegistUserSession* pRegistUserSession);
	void OnNetError(CProposeSession* pProposeSession);
	void OnNetError(CQuerySession* pQueryUserSession);
	void OnNetError(CLoadCcSession* pLoadCcSession);


	template <typename SESSION_TYPE>
	void OnRelease(SESSION_TYPE* pSession); //所有session的统一释放函数

public:
	static void GetConfigTableName(CDscString& strConfigTableName);

protected:
	BEGIN_HTS_MESSAGE_BIND
	/*注册用户流程相关*/
	BIND_HTS_MESSAGE(VBH::CCryptRegistUserCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCryptSubmitRegistUserTransactionCltEsReq)
	/*发起提案事务流程相关*/
	BIND_HTS_MESSAGE(VBH::CCryptProposeCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCryptSubmitProposalTransactionCltEsReq)
	/*直接查询用户流程相关*/
	BIND_HTS_MESSAGE(VBH::CCryptQueryCltEsReq)
	/*直接查询事务流程相关*/
	END_HTS_MESSAGE_BIND

public:
	/*注册用户流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CCryptRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCryptSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler);
	/*发起提案事务流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CCryptProposeCltEsReq& rProposeReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCryptSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler);
	/*查询流程相关*/
	ACE_INT32 OnHtsMsg(VBH::CCryptQueryCltEsReq& rQueryReq, CMcpHandler* pMcpHandler);

protected:
	BIND_DSC_MESSAGE(
		/*注册用户流程相关*/
		VBH::CRegistUserCcEsRsp,
		/*发起提案事务流程相关*/
		VBH::CQueryCryptKeyProposeCsEsRsp,
		VBH::CProposeCcEsRsp,
		/*查询流程相关*/
		VBH::CQueryCryptKeyQueryCsEsRsp,
		VBH::CQueryCcEsRsp,
		VBH::CLoadOuterCcUserCltEsReq
	)

public:
	void OnDscMsg(VBH::CRegistUserCcEsRsp& rRegistUserRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCryptKeyProposeCsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CProposeCcEsRsp& rProposeRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCryptKeyQueryCsEsRsp& rQueryCryptKeyRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryCcEsRsp& rQueryRsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CLoadOuterCcUserCltEsReq& rLoadOuterReq, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);


public:
	virtual void SendSubmitRegistUserTransactionTasEsRsp(VBH::CSubmitRegistUserTransactionTasEsRsp& rSubmitTransactionRsp) final;
	virtual void SendSubmitProposalTransactionTasEsRsp(VBH::CSubmitProposalTransactionTasEsRsp& rSubmitTransactionRsp) final;

	virtual void SendRegistUserCcEsRsp(VBH::CRegistUserCcEsRsp& rRegistUserRsp) final;
	virtual void SendProposeCcEsRsp(VBH::CProposeCcEsRsp& rProposeRsp) final;
	virtual void SendQueryCcEsRsp(VBH::CQueryCcEsRsp& rQueryRsp) final;
	virtual void SendLoadOuterCcCcEsRsp(VBH::CLoadOuterCcCcEsRsp& rLoadOuterCcRsp) final;

protected:
	virtual void OnNetworkError(CMcpHandler* pMcpHandler) override;
	virtual CMcpServerHandler* AllocMcpHandler(ACE_HANDLE handle) override;

private:
	//为session设置 mcp-handle-session, mcp-hanle-session用于通信
	template<typename SESSION_TYPE>
	void SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler);

	ACE_UINT32 AllocSessionID(void);

	//提案场景下收到秘钥的处理
	ACE_INT32 OnRecvCryptKey4Propose(CProposeSession* pSession, const char* pCltPubKey, size_t nCltPubKeyLen, const char* pEnvelopeKey, size_t nEnvelopeKeyLen);

	//查询场景下，收到秘钥的处理
	ACE_INT32 OnRecvCryptKey4Query(CQuerySession* pSession, const char* pEnvelopeKey, size_t nEnvelopeKeyLen);

	//插入1个crypt-key到cache中 //如果cache已满，最少被访问的会被删除
	void InsertCryptKeyIntoCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID, const DSC::CDscShortBlob& cltPubKey, const DSC::CDscShortBlob& envelopeKey);
	//从cache中读取指定的crypt-key, 如果不存在返回nullptr
	CCryptKeyPtrAsMapValue GetCryptKeyFromCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID);

	//注册时，生成服务端的公钥，服务端私钥，对称秘钥 //输入Bin格式的客户端公钥
	ACE_INT32 GenerateRegistCryptKey(CDscString& strSvrPubKey, CDscString& strSvrPrivKey, CDscString& strEnvelopeKey, DSC::CDscShortBlob& cltPubKey);

private:
	//注册用户
	using regist_user_session_map_type = CBareHashMap<ACE_UINT32, CRegistUserSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//发起提案
	using propose_session_map_type = CBareHashMap<ACE_UINT32, CProposeSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//直接查询用户
	using query_session_map_type = CBareHashMap<ACE_UINT32, CQuerySession, EN_SHORT_PROCESS_HASH_MAP_BITS>;
	using load_cc_session_map_type = CBareHashMap<ACE_UINT32, CLoadCcSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;

	//存放用户秘钥的map
	using user_crypt_key_map_type = dsc_map_type(CUserKeyAsMapKey, CCryptKeyPtrAsMapValue);

private:
	//tas和inner-cc被设置为和endorser运行于同一个线程中，这样，消息发送可以转为函数调用，因此这里保留对象指针，方便函数调用
	ITransferAgentService* m_pTas; 
	IEndorserCcService* m_arrHashInnerCcService[1 << 16] = {nullptr}; //cc的最多种类数是2^16个

	const CDscString m_strIpAddr;
	const ACE_UINT16 m_nPort;
	CMcpAsynchAcceptor<CCryptEndorserService>* m_pAcceptor = NULL;

	VBH::CCommitterServiceRouter m_csRouter;

	regist_user_session_map_type m_mapRegistUserSession;
	propose_session_map_type m_mapProposeSession;
	query_session_map_type m_mapQuerySession;
	load_cc_session_map_type m_mapLoadCcSession;


	//-----秘钥(公钥+私钥)缓存
	user_crypt_key_map_type m_mapUserCryptKey;
	CDscDqueue<CCryptKey> m_dqueueCryptKey; //用作记录访问频度的dqueue
	ACE_UINT32 m_nCachedCryptKeyNum = 0; //已经缓存的crypt-key个数

	ACE_UINT32 m_nSessionID = 0;

	CDscString m_peerEnvelopeKey; //SDK 和 endorser通信时使用的对称秘钥；TODO: 这个为临时增加的变量，待将来sdk和endorser的秘钥交换通道打通后就会扔掉
	EC_GROUP* m_pGroup = NULL; //创建秘钥所用的group
};

#include "end_ces/crypt_endorser_service.inl"

#endif
