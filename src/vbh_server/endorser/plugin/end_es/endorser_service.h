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
		EN_LONG_PROCESS_HASH_MAP_BITS = 20, //�����̵�session-map��ģ,
		EN_SHORT_PROCESS_HASH_MAP_BITS = 16, //�����̵�session-map��ģ
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
		//��network error�У�����handle-session��user-session���飬�����ø� OnNetError ������
		//���������ʵ�֣��ú����в����ٰ��Լ�������session�� handle-session��user-session������ɾ��
		virtual void OnNetError(void) = 0;

	public:
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nCltSessionID;
		CEndorserServiceHandler* m_pEndorserServiceHandler;

	public:
		ACE_UINT32 m_nIndex = 0; //ʹ�� CDscTypeArray ��������߱��Ľӿ�

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
		bool m_bSubmitNode; //���ڵ��Ǹ����ύ�Ľڵ�
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_ccGenerateUserInfo; //CC����ע��ʱ��Ϊ�û����ɵĳ�ʼ��Ϣ

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
		bool m_bSubmitNode; //���ڵ��Ǹ����ύ�Ľڵ�
		ACE_UINT32 m_nChannelID; //channel id  
		CDscString m_strSignature; //��Դ����᰸��ǩ��
		CDscString m_strProposal; //����information���᰸
		CDscString m_strInitValue; //CC����ע��ʱ��Ϊinformation���ɵĳ�ʼֵ

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
		bool m_bSubmitNode; //���ڵ��Ǹ����ύ�Ľڵ�
		ACE_UINT32 m_nChannelID; //channel id  
		ACE_UINT32 m_nActionID;
		CDscString m_strTransContent; //��������

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

	//���úͱ�endorser��������transfer-agent-service
	void SetTransferAgentService(ITransferAgentService* pTas);

	//���úͱ�endorser�������� inner-cc-service-map
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
	void OnRelease(SESSION_TYPE* pSession); //����session��ͳһ�ͷź���

public:
	static void GetConfigTableName(CDscString& strConfigTableName);

protected:
	BEGIN_HTS_MESSAGE_BIND
	/*ע���û��������*/
	BIND_HTS_MESSAGE(VBH::CRegistUserCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitRegistUserTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelRegistUserCltEsReq)
	/*����information�������*/
	BIND_HTS_MESSAGE(VBH::CCreateInformationCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitCreateInformationTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelCreateInformationCltEsReq)
	/*�����᰸�����������*/
	BIND_HTS_MESSAGE(VBH::CProposeCltEsReq)
	BIND_HTS_MESSAGE(VBH::CSubmitProposalTransactionCltEsReq)
	BIND_HTS_MESSAGE(VBH::CCancelProposalTransactionCltEsReq)
	/*ֱ�Ӳ�ѯ�û��������*/
	BIND_HTS_MESSAGE(VBH::CQueryUserInfoCltEsReq)
	/*ֱ�Ӳ�ѯ�����������*/
	BIND_HTS_MESSAGE(VBH::CQueryTransInfoCltEsReq)
	END_HTS_MESSAGE_BIND

public:
	/*ע���û��������*/
	ACE_INT32 OnHtsMsg(VBH::CRegistUserCltEsReq& rRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitRegistUserTransactionCltEsReq& rSubmitRegistUserReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelRegistUserCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*����information�������*/
	ACE_INT32 OnHtsMsg(VBH::CCreateInformationCltEsReq& rCreateInfoReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitCreateInformationTransactionCltEsReq& rSubmitCreateInfoReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelCreateInformationCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*�����᰸�����������*/
	ACE_INT32 OnHtsMsg(VBH::CProposeCltEsReq& rSubmitProposalReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitProposalTransactionCltEsReq& rSubmitTransactionReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCancelProposalTransactionCltEsReq& rCancelReq, CMcpHandler* pMcpHandler);
	/*ֱ�Ӳ�ѯ�û��������*/
	ACE_INT32 OnHtsMsg(VBH::CQueryUserInfoCltEsReq& rQueryUserReq, CMcpHandler* pMcpHandler);
	/*ֱ�Ӳ�ѯ�����������*/
	ACE_INT32 OnHtsMsg(VBH::CQueryTransInfoCltEsReq& rQueryTransReq, CMcpHandler* pMcpHandler);

protected:
	BEGIN_BIND_DSC_MESSAGE
	/*ע���û��������*/
	DSC_BIND_MESSAGE(VBH::CRegistUserCcEsRsp) //endorser�������������Ϣ�Ľ��գ��Լ���outer-cc
	/*����information*/
	DSC_BIND_MESSAGE(VBH::CCreateInformationCcEsRsp) //endorser�������������Ϣ�Ľ��գ��Լ���outer-cc
	/*�����᰸�����������*/
	DSC_BIND_MESSAGE(VBH::CProposeCcEsRsp) //endorser�������������Ϣ�Ľ��գ��Լ���outer-cc
	/*ֱ�Ӳ�ѯ�û��������*/
	DSC_BIND_MESSAGE(VBH::CQueryUserInfoXcsEsRsp)
	/*ֱ�Ӳ�ѯ�����������*/
	DSC_BIND_MESSAGE(VBH::CQueryCryptKeyGetTransXcsEsRsp)
	DSC_BIND_MESSAGE(VBH::CQueryTransInfoXcsEsRsp)
	//CExplorerQueryCcEsRsp �Ǽ���ͨ����ʱ��ʵ�� �������ѯͨ��
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
	//Ϊsession���� mcp-handle-session, mcp-hanle-session����ͨ��
	template<typename SESSION_TYPE>
	void SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler);

private:
	//ע���û�
	using regist_user_session_map_type = CBareHashMap<ACE_UINT32, CRegistUserSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//����information
	using create_information_session_map_type = CBareHashMap<ACE_UINT32, CCreateInformationSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//�����᰸
	using propose_session_map_type = CBareHashMap<ACE_UINT32, CProposeSession, EN_LONG_PROCESS_HASH_MAP_BITS>;
	//��ѯ�û�
	using query_user_info_map_type = CBareHashMap<ACE_UINT32, CQueryUserInfoSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;
	//��ѯ�᰸
	using query_trans_map_type = CBareHashMap<ACE_UINT32, CQueryTransactionSession, EN_SHORT_PROCESS_HASH_MAP_BITS>;

private:
	//tas��inner-cc������Ϊ��endorser������ͬһ���߳��У���������Ϣ���Ϳ���תΪ�������ã�������ﱣ������ָ�룬���㺯������
	ITransferAgentService* m_pTas; 
	IInnerCcService* m_arrHashInnerCcService[1 << 16] = {nullptr}; //cc�������������2^16��

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
