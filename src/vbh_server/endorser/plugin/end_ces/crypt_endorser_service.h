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
		EN_LONG_PROCESS_HASH_MAP_BITS = 20, //�����̵�session-map��ģ,
		EN_SHORT_PROCESS_HASH_MAP_BITS = 16, //�����̵�session-map��ģ
		EN_SESSION_TIMEOUT_VALUE = 90,
		EN_CRYPT_KEY_CACHE_SIZE = 1*1024*1024 //��Կ����໺�����
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
		//��network error�У�����handle-session��user-session���飬�����ø� OnNetError ������
		//���������ʵ�֣��ú����в����ٰ��Լ�������session�� handle-session��user-session������ɾ��
		virtual void OnNetError(void) = 0;

	public:
		ACE_INT32 m_nNonce; //ͨ����ͳһҪ�õ�nonce

		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nCltSessionID;
		CEndorserServiceHandler* m_pEndorserServiceHandler;

	public:
		ACE_UINT32 m_nIndex = CDscTypeArray<IUserSession>::EN_INVALID_INDEX_ID; //ʹ�� CDscTypeArray ��������߱��Ľӿ�

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
		bool m_bSubmitNode; //���ڵ��Ǹ����ύ�Ľڵ�
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_ccGenerateUserInfo; //CC����ע��ʱ��Ϊ�û����ɵĳ�ʼ��Ϣ
		
		CDscString m_strSvrPubKey; //����˹�Կ

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
		ACE_UINT8 m_nSubmitNodeType; //���ڵ��Ƿ����ύ�Ľڵ�
		ACE_UINT32 m_nChannelID;
		ACE_UINT32 m_nActionID;
		VBH::CVbhAllocatedKey m_userKey; //�᰸������

		CDscString m_userData; //��ſͻ��˷��͵ļ��ܺ�user data; 
		CDscString m_strEnvelopeKey; //�ԳƼ�����Կ
		CDscString m_strTransContent; //��������

		CDscString m_strSignature; //ǩ����������ǩ
		CDscString m_strProposal; //�᰸��������ǩ
		CDscString m_strCltPubKey; //�ͻ��˹�Կ��������ǩ
		CDscString m_strCcReceipt; //cc�����᰸ʱ���ɵĻ�ִ

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
		VBH::CVbhAllocatedKey m_userKey; //�����ѯ����
		CDscString m_userData;
		CDscString m_strEnvelopeKey; //����ڷ�������ĶԳ���Կ

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

	//���user-crypt map��key
	class CUserKeyAsMapKey
	{
	public:
		ACE_UINT32 m_nChannelID;
		ACE_UINT64 m_nUserAllocatedID;
	
	public:
		bool operator< (const CUserKeyAsMapKey& key) const;
	};

	//���user-crypt map��value
	class CCryptKey
	{
	public:
		CDscString m_strCltPubKey;
		CDscString m_strEnvelopeKey;
		
	public:
		CUserKeyAsMapKey m_mapKey; //���뵽map��ʱ����Ӧ��map-key //��dqueue��ֱ��ɾ��ʱ����Ҫ������map��ɾ������

	public: //��ΪCDDqueue��Ԫ����Ҫ�ĳ�Ա����
		CCryptKey* m_pPrev = nullptr;
		CCryptKey* m_pNext = nullptr;
	};

	using CCryptKeyPtrAsMapValue = CCryptKey *;

public:
	CCryptEndorserService(const CDscString& strIpAddr, const ACE_UINT16 nPort);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;

	//���úͱ�endorser��������transfer-agent-service
	void SetTransferAgentService(ITransferAgentService* pTas);

	//���úͱ�endorser�������� inner-cc-service-map
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
	void OnRelease(SESSION_TYPE* pSession); //����session��ͳһ�ͷź���

public:
	static void GetConfigTableName(CDscString& strConfigTableName);

protected:
	BEGIN_HTS_MESSAGE_BIND
	/*ע���û��������*/
	BIND_HTS_MESSAGE(VBH::CCryptRegistUserCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCryptSubmitRegistUserTransactionCltEsReq)
	/*�����᰸�����������*/
	BIND_HTS_MESSAGE(VBH::CCryptProposeCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCryptSubmitProposalTransactionCltEsReq)
	/*ֱ�Ӳ�ѯ�û��������*/
	BIND_HTS_MESSAGE(VBH::CCryptQueryCltEsReq)
	/*ֱ�Ӳ�ѯ�����������*/
	END_HTS_MESSAGE_BIND

public:
	/*ע���û��������*/
	ACE_INT32 OnHtsMsg(VBH::CCryptRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCryptSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler);
	/*�����᰸�����������*/
	ACE_INT32 OnHtsMsg(VBH::CCryptProposeCltEsReq& rProposeReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCryptSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler);
	/*��ѯ�������*/
	ACE_INT32 OnHtsMsg(VBH::CCryptQueryCltEsReq& rQueryReq, CMcpHandler* pMcpHandler);

protected:
	BIND_DSC_MESSAGE(
		/*ע���û��������*/
		VBH::CRegistUserCcEsRsp,
		/*�����᰸�����������*/
		VBH::CQueryCryptKeyProposeCsEsRsp,
		VBH::CProposeCcEsRsp,
		/*��ѯ�������*/
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
	//Ϊsession���� mcp-handle-session, mcp-hanle-session����ͨ��
	template<typename SESSION_TYPE>
	void SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler);

	ACE_UINT32 AllocSessionID(void);

	//�᰸�������յ���Կ�Ĵ���
	ACE_INT32 OnRecvCryptKey4Propose(CProposeSession* pSession, const char* pCltPubKey, size_t nCltPubKeyLen, const char* pEnvelopeKey, size_t nEnvelopeKeyLen);

	//��ѯ�����£��յ���Կ�Ĵ���
	ACE_INT32 OnRecvCryptKey4Query(CQuerySession* pSession, const char* pEnvelopeKey, size_t nEnvelopeKeyLen);

	//����1��crypt-key��cache�� //���cache���������ٱ����ʵĻᱻɾ��
	void InsertCryptKeyIntoCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID, const DSC::CDscShortBlob& cltPubKey, const DSC::CDscShortBlob& envelopeKey);
	//��cache�ж�ȡָ����crypt-key, ��������ڷ���nullptr
	CCryptKeyPtrAsMapValue GetCryptKeyFromCache(const ACE_UINT32 nChannelID, const ACE_UINT64 nUserAllocatedID);

	//ע��ʱ�����ɷ���˵Ĺ�Կ�������˽Կ���Գ���Կ //����Bin��ʽ�Ŀͻ��˹�Կ
	ACE_INT32 GenerateRegistCryptKey(CDscString& strSvrPubKey, CDscString& strSvrPrivKey, CDscString& strEnvelopeKey, DSC::CDscShortBlob& cltPubKey);

private:
	//ע���û�
	using regist_user_session_map_type = CBareHashMap<ACE_UINT32, CRegistUserSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//�����᰸
	using propose_session_map_type = CBareHashMap<ACE_UINT32, CProposeSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//ֱ�Ӳ�ѯ�û�
	using query_session_map_type = CBareHashMap<ACE_UINT32, CQuerySession, EN_SHORT_PROCESS_HASH_MAP_BITS>;
	using load_cc_session_map_type = CBareHashMap<ACE_UINT32, CLoadCcSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;

	//����û���Կ��map
	using user_crypt_key_map_type = dsc_map_type(CUserKeyAsMapKey, CCryptKeyPtrAsMapValue);

private:
	//tas��inner-cc������Ϊ��endorser������ͬһ���߳��У���������Ϣ���Ϳ���תΪ�������ã�������ﱣ������ָ�룬���㺯������
	ITransferAgentService* m_pTas; 
	IEndorserCcService* m_arrHashInnerCcService[1 << 16] = {nullptr}; //cc�������������2^16��

	const CDscString m_strIpAddr;
	const ACE_UINT16 m_nPort;
	CMcpAsynchAcceptor<CCryptEndorserService>* m_pAcceptor = NULL;

	VBH::CCommitterServiceRouter m_csRouter;

	regist_user_session_map_type m_mapRegistUserSession;
	propose_session_map_type m_mapProposeSession;
	query_session_map_type m_mapQuerySession;
	load_cc_session_map_type m_mapLoadCcSession;


	//-----��Կ(��Կ+˽Կ)����
	user_crypt_key_map_type m_mapUserCryptKey;
	CDscDqueue<CCryptKey> m_dqueueCryptKey; //������¼����Ƶ�ȵ�dqueue
	ACE_UINT32 m_nCachedCryptKeyNum = 0; //�Ѿ������crypt-key����

	ACE_UINT32 m_nSessionID = 0;

	CDscString m_peerEnvelopeKey; //SDK �� endorserͨ��ʱʹ�õĶԳ���Կ��TODO: ���Ϊ��ʱ���ӵı�����������sdk��endorser����Կ����ͨ����ͨ��ͻ��ӵ�
	EC_GROUP* m_pGroup = NULL; //������Կ���õ�group
};

#include "end_ces/crypt_endorser_service.inl"

#endif
