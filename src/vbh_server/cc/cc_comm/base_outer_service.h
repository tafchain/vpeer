#ifndef BASE_OUTER_SERVICE_H_897UOIHJOIFD9F87U890UYEROIH087TIHKIHLJOI8
#define BASE_OUTER_SERVICE_H_897UOIHJOIFD9F87U890UYEROIH087TIHKIHLJOI8

#include "dsc/service/dsc_service.h"

#include "vbh_comm/vbh_comm_msg_def.h"
#include "cc_comm/cc_comm_def_export.h"

class CBaseOuterService : public CDscService
{
protected:
	BIND_DSC_MESSAGE(
		VBH::CRegistUserEsCcReq,
		VBH::CProposeEsCcReq,
		VBH::CQueryEsCcReq
	)

public:
	void OnDscMsg(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);
	void OnDscMsg(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr);

protected:
	virtual ACE_INT32 OnRecvRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	virtual ACE_INT32 OnRecvProposalEsCcReq(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	virtual ACE_INT32 OnRecvQueryEsCcReq(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	
protected:
	void SendCcMessage(VBH::CRegistUserCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
	void SendCcMessage(VBH::CProposeCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
	void SendCcMessage(VBH::CQueryCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
};

#include "cc_comm/base_outer_service.inl"

#endif // !BASE_OUTER_SERVICE_H_897UOIHJOIFD9F87U890UYEROIH087TIHKIHLJOI8