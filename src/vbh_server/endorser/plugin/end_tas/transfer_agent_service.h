#ifndef TRANSFER_AGENT_SERVICE_H_JFIOSDJFOIEUREWPORJLKNVLUYOU
#define TRANSFER_AGENT_SERVICE_H_JFIOSDJFOIEUREWPORJLKNVLUYOU

#include "dsc/protocol/hts/dsc_hts_service.h"
#include "dsc/protocol/mcp/mcp_asynch_acceptor.h"
#include "dsc/container/bare_hash_map.h"

#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"

#include "end_comm/i_endorser_service.h"
#include "end_comm/i_transfer_agent_service.h"

#include "end_tas/transfer_agent_export.h"


class TRANSFORM_AGENT_EXPORT CTransferAgentService : public CDscHtsClientService, public ITransferAgentService
{
private:
	enum
	{
		EN_HASH_MAP_BITES = 16,
		EN_SESSION_TIMEOUT_VALUE = 60,
		EN_CPS_LEADER_STATUS = 55,
		EN_CPS_FOLLOWER_STATUS = 66,
	};

private:
	class CCpsConnectSession
	{
	public:
		ACE_UINT16 m_nPort;
		ACE_UINT32 m_nHandleID;
		ACE_UINT32 m_nChannelID;
		CMcpHandler* m_pMcpHandler = NULL; //NULL时，标志连接未建立成功
		CDscString m_strIpAddr;
		ACE_UINT32 m_nIsLeaderCps;

	public:
		ACE_UINT32 m_nKey = 0;
		CCpsConnectSession* m_pPrev = NULL;
		CCpsConnectSession* m_pNext = NULL;
	};

	class ISession : public CDscServiceTimerHandler
	{
	public:
		ISession(CTransferAgentService& rTas);

	public: 
		virtual void OnRelease(void) = 0; //释放自身的函数，用于从基类删除指针时，调用到子类的 DSC_THREAD_TYPE_DELETE宏

	public:
		ACE_UINT32 m_nTasSessionID;
		ACE_UINT32 m_nEsSessionID;

	public:
		ACE_UINT32 m_nKey = 0;
		ISession* m_pPrev = NULL;
		ISession* m_pNext = NULL;

	protected:
		CTransferAgentService& m_rTas;
	};

	class CSubmitRegistUserTransactionSession : public ISession
	{
	public:
		CSubmitRegistUserTransactionSession(CTransferAgentService& rTas);

	public:
		virtual void OnTimer(void) override;
		virtual void OnRelease(void) override;
	};
	class CSubmitProposalTransactionSession : public ISession
	{
	public:
		CSubmitProposalTransactionSession(CTransferAgentService& rTas);

	public:
		virtual void OnTimer(void) override;
		virtual void OnRelease(void) override;
	};

	class CQueryLeaderCpsSession : public CDscServiceTimerHandler
	{
	public:
		CQueryLeaderCpsSession(CTransferAgentService& rTas);
	public:
		ACE_UINT32 m_nSessionID;

	public:
		virtual void OnTimer(void) override;
	protected:
		CTransferAgentService& m_rTas;
	};

public:
	virtual ACE_INT32 OnInit(void);
	virtual ACE_INT32 OnExit(void);

	//设置endorser service
	void SetEndorserService(IEndorserService* pEs);
	CCpsConnectSession* FindLeaderCpsSession(ACE_UINT32 nChannelID);

public:
	void OnTimeOut(CSubmitRegistUserTransactionSession* pSession);
	void OnTimeOut(CSubmitProposalTransactionSession* pSession);
	void OnTimeOut(CQueryLeaderCpsSession* pSession);

protected:
	BEGIN_HTS_MESSAGE_BIND
	BIND_HTS_MESSAGE(VBH::CSubmitRegistUserTransactionCpsTasRsp)
	BIND_HTS_MESSAGE(VBH::CSubmitProposalTransactionCpsTasRsp)
	BIND_HTS_MESSAGE(VBH::CQueryLeaderCpsCpsTasRsp)
	END_HTS_MESSAGE_BIND

public:
	ACE_INT32 OnHtsMsg(VBH::CSubmitRegistUserTransactionCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CSubmitProposalTransactionCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CQueryLeaderCpsCpsTasRsp& rSubmitTransactionRsp, CMcpHandler* pMcpHandler);
		
public:
	virtual ACE_INT32 SendSubmitRegistUserTransactionEsTasReq(VBH::CSubmitRegistUserTransactionEsTasReq& rSubmitTransReq) final;
	virtual ACE_INT32 SendSubmitProposalTransactionEsTasReq(VBH::CSubmitProposalTransactionEsTasReq& rSubmitTransReq) final;

protected:
	virtual void OnNetworkError(CMcpHandler* pMcpHandler) override;

	virtual ACE_INT32 OnConnectedNodify(CMcpClientHandler* pMcpClientHandler) override;

private:
	using cps_connect_session_map_type = CBareHashMap<ACE_UINT32, CCpsConnectSession, EN_HASH_MAP_BITES>;
	using submit_transaction_session_map_type = CBareHashMap<ACE_UINT32, ISession, EN_HASH_MAP_BITES>;

private:
	IEndorserService* m_pEs = nullptr;
	ACE_UINT16 m_nPeerID = 0;
	cps_connect_session_map_type m_mapCpsConnectSession;
	CQueryLeaderCpsSession* m_pQueryLeaderCpsSession = nullptr;
	
	submit_transaction_session_map_type m_mapSubmitTransactionSession;
	ACE_UINT32 m_nSessionID = 0;
};

#include "end_tas/transfer_agent_service.inl"

#endif
