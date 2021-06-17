#ifndef I_OUTER_CC_SERVICE_H_89877979746576432463242641
#define I_OUTER_CC_SERVICE_H_89877979746576432463242641


#include "vbh_comm/cc_comm_def.h"
#include "dsc/service/dsc_service.h"
#include "cc_comm/i_comm_cc_service.h"

//IOuterCcService用于外部的合约交流
class IOuterCcService: public CDscService
{

public:
	//回调上层实现逻辑：是否可以创建该用户，如果可以，则创建初始信息；返回非0值，表示处理失败
	//userInfo: 输入参数，注册时携带的用户信息
	virtual ACE_INT32 RegistUserProc(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& userInfo) = 0;


	//回调上层实现逻辑：提案处理，返回非0值，表示处理失败 
	virtual ACE_INT32 ProposalProc(const ACE_UINT32 nCcSessionID, const ACE_UINT32 nActionID, DSC::CDscShortBlob& proposal) = 0;

	//回调上层实现逻辑：获得用户信息
	virtual void OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CProposeSimpleUser& rUser) {};
	virtual void OnGetUserRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const CProposeSimpleUser* pUserVec, const ACE_UINT16 nVecLen) {};
	//回调上层实现逻辑：获得信息 
	virtual void OnGetInformationRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CProposeSimpleInformation& rInfo) {};
	virtual void OnGetInformationRspAtPropose(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const CProposeSimpleInformation* pInfoVec, const ACE_UINT16 nVecLen) {};

	//回调上层实现逻辑：获得用户信息
	virtual void OnGetUserRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CQuerySimpleUser& rUser) {};
	virtual void OnGetUserRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const CQuerySimpleUser* pUserVec, const ACE_UINT16 nVecLen) {};
	//回调上层实现逻辑：获得信息 
	virtual void OnGetInformationRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CQuerySimpleInformation& rInfo) {};
	virtual void OnGetInformationRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const CQuerySimpleInformation* pInfoVec, const ACE_UINT16 nVecLen) {};

	//回调上层实现逻辑：获得事务
	virtual void OnGetTransRspAtQuery(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, CQuerySimpleTransaction& rTrans) {};

	//普通的查询处理, 回调上层实现逻辑：查询处理，返回非0值，表示处理失败 
	virtual ACE_INT32 QueryProc(const ACE_UINT32 nCcSessionID, const ACE_UINT32 nActionID, DSC::CDscShortBlob& param) = 0;

	//解析user-value内容 //默认实现，打印为hex内容
	virtual void FormatUser(CDscString& strFormatBuf, DSC::CDscShortBlob& value) = 0;

	//根据action-id解析提案的内容
	virtual ACE_INT32 FormatProposal(CDscString& strFormatBuf, const ACE_UINT32 nActionID, DSC::CDscShortBlob& proposal) = 0;
public:
	ICommCcService* m_pCommCcService;
	inline void SetCommCcService(ICommCcService* pCommCCs)
	{
		m_pCommCcService = pCommCCs;
	}

	ACE_UINT32 m_nKey = 0;
	IOuterCcService* m_pPrev = NULL;
	IOuterCcService* m_pNext = NULL;
};



//-------------------------------提案处理宏----------------------------------

#define BEGIN_BIND_PROPOSAL_PROC \
	ACE_INT32 ProposalProc(const ACE_UINT32 nCcSessionID, const ACE_UINT32 nActionID, DSC::CDscShortBlob& proposal) \
	{ \
		switch(nActionID) \
		{ \

#define BIND_PROPOSAL_PROC(PROPOSAL_TYPE) \
		case PROPOSAL_TYPE::EN_ACTION_ID: \
		{ \
			PROPOSAL_TYPE prop; \
			if (DSC_UNLIKELY(DSC::Decode(prop, proposal.GetBuffer(), proposal.GetSize()))) \
			{ \
				return -1; \
			} \
			else \
			{ \
				return this->OnPorposal(nCcSessionID, prop); \
			} \
		}

#define END_BIND_PROPOSAL_PROC \
		default: \
		{ \
			DSC_RUN_LOG_FINE("cann't proc propose action:%d", nActionID); \
			return -1; \
		} \
		} \
	}

//-------------------------------查询处理宏----------------------------------
#define BEGIN_BIND_QUERY_PROC \
	ACE_INT32 QueryProc(const ACE_UINT32 nCcSessionID, const ACE_UINT32 nActionID, DSC::CDscShortBlob& param) \
	{ \
		switch(nActionID) \
		{ \

#define BIND_QUERY_PROC(PARAM_TYPE) \
		case PARAM_TYPE::EN_ACTION_ID: \
		{ \
			PARAM_TYPE action; \
			if (DSC_UNLIKELY(DSC::Decode(action, param.GetBuffer(), param.GetSize()))) \
			{ \
				return -1; \
			} \
			else \
			{ \
				return this->OnQuery(nCcSessionID, action); \
			} \
		}

#define END_BIND_QUERY_PROC \
		default: \
		{ \
			DSC_RUN_LOG_FINE("cann't proc query action:%d", nActionID); \
			return -1; \
		} \
		} \
	}

//---------------------------------提案解析宏-------------------------------------

#define BEGIN_BIND_FORMAT_PROPOSAL \
	ACE_INT32 FormatProposal(CDscString& strFormatBuf, const ACE_UINT32 nActionID, DSC::CDscShortBlob& proposal) \
	{ \
		switch(nActionID) \
		{ \

#define BIND_FORMAT_PROPOSAL(PROPOSAL_TYPE) \
		case PROPOSAL_TYPE::EN_ACTION_ID: \
		{ \
			PROPOSAL_TYPE prop; \
			if (DSC_UNLIKELY(DSC::Decode(prop, proposal.GetBuffer(), proposal.GetSize()))) \
			{ \
				DSC_RUN_LOG_ERROR("decode proposal failed."); \
				return -1; \
			} \
			else \
			{ \
				this->OnFormatProposal(strFormatBuf, prop); \
				return 0; \
			} \
		}

#define END_BIND_FORMAT_PROPOSAL \
		default: \
		{ /*没有定义解析方法时，默认采用父类的解析方法：格式化为Hex格式*/\
			DSC_RUN_LOG_FINE("cann't proc query action:%d", nActionID); \
			return -1; \
		} \
		} \
	}
//-----------------------------------------------------------------------------

#endif