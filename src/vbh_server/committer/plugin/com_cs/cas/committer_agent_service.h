#ifndef COMMITTER_AGENT_SERVER_H__805384038560923402340021
#define COMMITTER_AGENT_SERVER_H__805384038560923402340021

#include "dsc/container/dsc_type_array.h"
#include "dsc/container/bare_hash_map.h"
#include "dsc/protocol/hts/dsc_hts_service.h"
#include "dsc/protocol/mcp/mcp_asynch_acceptor.h"
#include "dsc/service_timer/dsc_service_timer_handler.h"
#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/protocol/mcp/mcp_server_handler.h"

#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_cas_cs_def.h"
#include "vbh_comm/vbh_encrypt_lib.h"

#include "com_cs/cs/i_committer_service.h"
#include "com_cs/cas/i_committer_agent_service.h"

class PLUGIN_EXPORT CCommitterAgentService final : public CDscHtsServerService, public ICommitterAgentService
{
public:
	enum
	{
		EN_SERVICE_TYPE = VBH::EN_COMMITTER_AGENT_SERVICE_TYPE,
		EN_SESSION_TIMEOUT_VALUE = 60,
		EN_SESSION_MAP_SIZE_BITS = 16
	};


public:
	CCommitterAgentService(const ACE_UINT32 nChannelID, CDscString strIpAddr, ACE_INT32 nPort);

public:
	virtual ACE_INT32 OnInit(void) override;
	virtual ACE_INT32 OnExit(void) override;


protected:
	BEGIN_HTS_MESSAGE_BIND
	BIND_HTS_MESSAGE(VBH::CGetBlockCsCasReq)
	BIND_HTS_MESSAGE(VBH::CCheckBlockHashCsCasReq)
	BIND_HTS_MESSAGE(VBH::CVerifyPeerStateCsCasReq)
	END_HTS_MESSAGE_BIND

public:
    ACE_INT32 OnHtsMsg(VBH::CGetBlockCsCasReq& rGetBlockReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CCheckBlockHashCsCasReq& rCheckHashReq, CMcpHandler* pMcpHandler);
	ACE_INT32 OnHtsMsg(VBH::CVerifyPeerStateCsCasReq& rVerifyReq, CMcpHandler* pMcpHandler);
	void SetCommitterService(ICommitterService* pCommitterService);
private:
	const ACE_UINT32 m_nChannelID;
	CDscString m_strIpAddr;
	ACE_INT32 m_nPort;
	ACE_UINT16 m_nPeerID = 0;
private:

	ICommitterService* m_pCommitterService = nullptr;
	CMcpAsynchAcceptor<CCommitterAgentService>* m_pAcceptor = nullptr;
};


#include "com_cs/cas/committer_agent_service.inl"













#endif // !X_AGENT_SERVER_H__805384038560923402340021

