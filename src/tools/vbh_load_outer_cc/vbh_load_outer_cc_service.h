#ifndef VBH_PERF_TEST_REGIST_SERVICE_H_8979465465345649684635416548960
#define VBH_PERF_TEST_REGIST_SERVICE_H_8979465465345649684635416548960

#include "dsc/dsc_log.h"
#include "dsc/dsc_reactor.h"
#include "dsc/service_timer/dsc_service_timer_handler.h"
#include "dsc/container/bare_hash_map.h"



#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"


class CVbhLoadOuterCcService : public CDscReactor
{
public:
	enum
	{
		EN_HASH_MAP_BITES = 16,
		EN_SESSION_TIMEOUT_VALUE = 120,
		EN_SERVICE_TYPE = VBH::EN_MIN_VBH_CLIENT_SERVICE_TYPE + 8,
	};
	class CEsCfg
	{
	public:
		ACE_UINT16 m_nPort;
		ACE_UINT16 m_nEsID;
		CDscString m_strIpAddr;
	};

public:
	CVbhLoadOuterCcService(ACE_UINT32 nChannelID, ACE_UINT32 nCcID, CDscString strCcNmae);

private:

	class CLoadOuterCcSession : public CDscServiceTimerHandler
	{
	public:
		CLoadOuterCcSession(CVbhLoadOuterCcService& rService);

	public:
		ACE_UINT32 m_nSessionID = 1;
		ACE_UINT32 m_nChannelID = 0;
		CDscString m_strCcNmae;
		ACE_UINT32 m_nCcID = 0;

	public:
		void OnTimer(void) override;

	public:
		ACE_UINT32 m_nKey = 0;
		CLoadOuterCcSession* m_pPrev = NULL;
		CLoadOuterCcSession* m_pNext = NULL;

	public:
		CVbhLoadOuterCcService& m_rServiece;
	};

public:
	ACE_INT32 OnInit(void);
	ACE_INT32 OnExit(void);

private:
	using es_cfg_list_type = dsc_list_type(CEsCfg);
	using load_outer_cc_map_session = CBareHashMap<ACE_UINT32, CLoadOuterCcSession, EN_HASH_MAP_BITES>;
public:
	void LoadOuterCc(void);
	ACE_INT32 ReadDBConfig(es_cfg_list_type& lstEsCfg);

protected:
	/*注册用户流程相关*/
	BIND_DSC_MESSAGE(VBH::CLoadOuterCcEsCltRsp)

public:
	void OnDscMsg(VBH::CLoadOuterCcEsCltRsp& rsp, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);


private:
	es_cfg_list_type m_lstEsCfg;
	load_outer_cc_map_session m_mapLoadOuterCcSession;
	
	ACE_UINT32 m_nSessionID = 1;
	ACE_UINT32 m_nChannelID = 0;
	CDscString m_strCcName;
	ACE_UINT32 m_nCcID = 0;
	ACE_UINT32 m_isSucess = 1;

};

#endif
