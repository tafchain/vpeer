#ifndef VBH_FUNC_TEST_CC_SERVICE_H_3428723472314231213863217
#define VBH_FUNC_TEST_CC_SERVICE_H_3428723472314231213863217

#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/container/bare_hash_map.h"
#include "cc_comm/i_outer_cc_service.h"
#include "cc_comm/inner_cc_def_export.h"
#include "cc_comm/i_comm_cc_service.h"


#include "vbh_comm/test_def.h"

//编码约束：必须使用底层传递的session-id作为自己的session-id
class INNER_CC_DEF_EXPORT CVbhFuncTestCcService : public IOuterCcService
{
protected:
	class CSession : public CDscServiceTimerHandler
	{
	public:
		CSession(CVbhFuncTestCcService& rVbhFuncTestCcService);

		virtual void OnTimer(void);

	public:
		ACE_UINT32 m_nCcSessionID;

	public:
		ACE_UINT32 m_nAsset;
		ACE_UINT64 m_nPhoneNo;
		CDscString m_userName;
		CDscString m_address;
		CDscString m_userKey;

	public:
		ACE_UINT32 m_nKey = 0;
		CSession* m_pPrev = NULL;
		CSession* m_pNext = NULL;

	protected:
		CVbhFuncTestCcService& m_rCcService;
	};

	//提案相关的session
	class CProposeSession : public CDscServiceTimerHandler
	{
	public:
		CProposeSession(CVbhFuncTestCcService& rVbhFuncTestCcService);

		virtual void OnTimer(void);

	public:
		ACE_UINT32 m_nCcSessionID;

	public:
		ACE_UINT32 m_nAsset;
		CDscString m_fromUser;
		CDscString m_toUser;

		TEST_CC::CUserInfo m_fromUserInfo;
		TEST_CC::CUserInfo m_toUserInfo;

	public:
		ACE_UINT32 m_nKey = 0;
		CProposeSession* m_pPrev = NULL;
		CProposeSession* m_pNext = NULL;

	protected:
		CVbhFuncTestCcService& m_rCcService;
	};

	//提案相关的session
	class CProposeInfoSession : public CDscServiceTimerHandler
	{
	public:
		CProposeInfoSession(CVbhFuncTestCcService& rVbhFuncTestCcService);

		virtual void OnTimer(void);

	public:
		ACE_UINT32 m_nCcSessionID;

	public:
		DSC::CDscShortBlob m_key;
		DSC::CDscShortBlob m_value;

	public:
		ACE_UINT32 m_nKey = 0;
		CProposeInfoSession* m_pPrev = NULL;
		CProposeInfoSession* m_pNext = NULL;

	protected:
		CVbhFuncTestCcService& m_rCcService;
	};
public:
	CVbhFuncTestCcService(ACE_UINT32 nChannelID);

public:
	virtual ACE_INT32 OnInit(void);
	virtual ACE_INT32 OnExit(void);

public:
	void OnTimeOut(CSession* pUserSession);
	void OnTimeOut(CProposeSession* pTradeSession);
	void OnTimeOut(CProposeInfoSession* pInfoSession);

protected:
	virtual ACE_INT32 RegistUserProc(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& userInfo) override;

protected:
	BEGIN_BIND_PROPOSAL_PROC
	BIND_PROPOSAL_PROC(TEST_CC::CAlterInformationAction)
	BIND_PROPOSAL_PROC(TEST_CC::CTradeAction)
	BIND_PROPOSAL_PROC(TEST_CC::CCreateInformationAction)
	BIND_PROPOSAL_PROC(TEST_CC::CCommitInformationAction)
	END_BIND_PROPOSAL_PROC

protected:
	ACE_INT32 OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CAlterInformationAction& rAlterTestUserInfoAction);
	ACE_INT32 OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CTradeAction& rTradeInfo);
	ACE_INT32 OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CCreateInformationAction& rCreateInformationAction);
	ACE_INT32 OnPorposal(const ACE_UINT32 nSessionID, TEST_CC::CCommitInformationAction& rAction);

protected:
	BEGIN_BIND_QUERY_PROC
	BIND_QUERY_PROC(TEST_CC::CQueryUserAction)
	BIND_QUERY_PROC(TEST_CC::CQueryTransAction)
	BIND_QUERY_PROC(TEST_CC::CQueryInfoHistoryListAction)
	END_BIND_QUERY_PROC

protected:
	ACE_INT32 OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryUserAction& action);
	ACE_INT32 OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryTransAction& action);
	ACE_INT32 OnQuery(const ACE_UINT32 nSessionID, TEST_CC::CQueryInfoHistoryListAction& action);

protected:
	BEGIN_BIND_FORMAT_PROPOSAL
	BIND_FORMAT_PROPOSAL(TEST_CC::CTradeAction)
	END_BIND_FORMAT_PROPOSAL

protected:
	void OnFormatProposal(CDscString& strFormatBuf, TEST_CC::CTradeAction& action);

protected:
	virtual void FormatUser(CDscString& strFormatBuf, DSC::CDscShortBlob& value) override;

protected:
	virtual void OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CProposeSimpleUser& rUser) override;
	virtual void OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, const CProposeSimpleUser* pUserVec, const ACE_UINT16 nVecLen) override;
	virtual void OnGetInformationRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CProposeSimpleInformation& rInfo)override;
	virtual void OnGetUserRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nSessionID, CQuerySimpleUser& rUser) override;

	virtual void OnGetTransRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CQuerySimpleTransaction& rTrans) override;

private:
	using session_type = CBareHashMap<ACE_UINT32, CSession, EN_HASH_MAP_BITES>;
	using trade_session_type = CBareHashMap<ACE_UINT32, CProposeSession, EN_HASH_MAP_BITES>;
	using info_session_type = CBareHashMap<ACE_UINT32, CProposeInfoSession, EN_HASH_MAP_BITES>;

	session_type m_mapSession;
	trade_session_type m_mapTradeSession;
	info_session_type  m_mapInfoSession;

};

#include "vbh_func_test_cc/vbh_func_test_cc_service.inl"

#endif
