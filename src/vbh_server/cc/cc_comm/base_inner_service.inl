#include "vbh_comm/vbh_comm_macro_def.h"

inline void CBaseInnerService::SetEndorserService(IEndorserService* pEs)
{
	m_pEs = pEs;
}

inline ACE_INT32 CBaseInnerService::SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseInnerService, VBH::CRegistUserEsCcReq);

	CDscMsg::CDscMsgAddr addr;

	ACE_INT32 nReturnCode = this->OnRecvRegistUserEsCcReq(req, addr);

	VBH_MESSAGE_LEAVE_TRACE(CBaseInnerService, VBH::CRegistUserEsCcReq);

	return nReturnCode;
}


inline ACE_INT32 CBaseInnerService::SendProposeEsCcReq(VBH::CProposeEsCcReq& req)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseInnerService, VBH::CProposeEsCcReq);

	CDscMsg::CDscMsgAddr addr;

	const ACE_INT32 nReturnCode = this->OnRecvProposalEsCcReq(req, addr);

	VBH_MESSAGE_LEAVE_TRACE(CBaseInnerService, VBH::CProposeEsCcReq);

	return nReturnCode;
}

inline ACE_INT32 CBaseInnerService::SendQueryEsCcReq(VBH::CQueryEsCcReq& req)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseInnerService, VBH::CQueryEsCcReq);

	CDscMsg::CDscMsgAddr addr;

	const ACE_INT32 nReturnCode = this->OnRecvQueryEsCcReq(req, addr);

	VBH_MESSAGE_LEAVE_TRACE(CBaseInnerService, VBH::CQueryEsCcReq);

	return nReturnCode;
}

inline ACE_INT32 CBaseInnerService::SendLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseInnerService, VBH::CLoadOuterCcEsCcReq);

	CDscMsg::CDscMsgAddr addr;
	VBH::CLoadOuterCcCcEsRsp rsp;

	rsp.m_nReturnCode = this->OnLoadOuterCcEsCcReq(req, addr);
	rsp.m_nEsSessionID = req.m_nSessionID;
	SendCcMessage(rsp, addr);

	VBH_MESSAGE_LEAVE_TRACE(CBaseInnerService, VBH::CLoadOuterCcEsCcReq);

	return rsp.m_nReturnCode;
}

inline void CBaseInnerService::SendCcMessage(VBH::CRegistUserCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	m_pEs->SendRegistUserCcEsRsp(rsp);
}


inline void CBaseInnerService::SendCcMessage(VBH::CProposeCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	m_pEs->SendProposeCcEsRsp(rsp);
}

inline void CBaseInnerService::SendCcMessage(VBH::CQueryCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	m_pEs->SendQueryCcEsRsp(rsp);
}

inline void CBaseInnerService::SendCcMessage(VBH::CLoadOuterCcCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	m_pEs->SendLoadOuterCcCcEsRsp(rsp);
}
