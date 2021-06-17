#include "vbh_comm/vbh_comm_macro_def.h"

inline void CBaseOuterService::OnDscMsg(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseOuterService, VBH::CRegistUserEsCcReq);
	
	ACE_INT32 nReturnCode = this->OnRecvRegistUserEsCcReq(req, rSrcMsgAddr);

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CRegistUserCcEsRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nEsSessionID = req.m_nEsSessionID;
		this->SendDscMessage(rsp, rSrcMsgAddr);
	}
	VBH_MESSAGE_LEAVE_TRACE(CBaseOuterService, VBH::CRegistUserEsCcReq);
}



inline void CBaseOuterService::OnDscMsg(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseOuterService, VBH::CProposeEsCcReq);
	
	ACE_INT32 nReturnCode = this->OnRecvProposalEsCcReq(req, rSrcMsgAddr);

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CProposeCcEsRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nEsSessionID = req.m_nEsSessionID;
		this->SendDscMessage(rsp, rSrcMsgAddr);
	}

	VBH_MESSAGE_LEAVE_TRACE(CBaseOuterService, VBH::CProposeEsCcReq);
}

inline void CBaseOuterService::OnDscMsg(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr)
{
	VBH_MESSAGE_ENTER_TRACE(CBaseOuterService, VBH::CQueryEsCcReq);

	ACE_INT32 nReturnCode = this->OnRecvQueryEsCcReq(req, rSrcMsgAddr);

	if (nReturnCode != VBH::EN_OK_TYPE)
	{
		VBH::CQueryCcEsRsp rsp;

		rsp.m_nReturnCode = nReturnCode;
		rsp.m_nEsSessionID = req.m_nEsSessionID;
		this->SendDscMessage(rsp, rSrcMsgAddr);
	}

	VBH_MESSAGE_LEAVE_TRACE(CBaseOuterService, VBH::CQueryEsCcReq);
}

inline void CBaseOuterService::SendCcMessage(VBH::CRegistUserCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	this->SendDscMessage(rsp, addr);
}


inline void CBaseOuterService::SendCcMessage(VBH::CProposeCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	this->SendDscMessage(rsp, addr);
}

inline void CBaseOuterService::SendCcMessage(VBH::CQueryCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr)
{
	this->SendDscMessage(rsp, addr);
}

