#ifndef BASE_INNER_SERVICE_H_02U34092U34U32U4320940398987078907
#define BASE_INNER_SERVICE_H_02U34092U34U32U4320940398987078907

#include "dsc/service/dsc_service.h"

#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_es_def.h"

#include "end_comm/i_endorser_service.h"
#include "cc_comm/i_endorser_cc_service.h"

class CBaseInnerService : public CDscService, public IEndorserCcService
{
public:
	virtual ACE_INT32 SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req) override;
	virtual ACE_INT32 SendProposeEsCcReq(VBH::CProposeEsCcReq& req) override;
	virtual ACE_INT32 SendQueryEsCcReq(VBH::CQueryEsCcReq& req) override;
	virtual ACE_INT32 SendLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req) override;

public:
	//设置 和 inner-cc背对背的 endorser-service
	void SetEndorserService(IEndorserService* pEs);

protected:
	virtual ACE_INT32 OnRecvRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	virtual ACE_INT32 OnRecvProposalEsCcReq(VBH::CProposeEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	virtual ACE_INT32 OnRecvQueryEsCcReq(VBH::CQueryEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;
	virtual ACE_INT32 OnLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req, const CDscMsg::CDscMsgAddr& rSrcMsgAddr) = 0;

protected:
	void SendCcMessage(VBH::CRegistUserCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
	void SendCcMessage(VBH::CProposeCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
	void SendCcMessage(VBH::CQueryCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);
	void SendCcMessage(VBH::CLoadOuterCcCcEsRsp& rsp, const CDscMsg::CDscMsgAddr& addr);

protected:
	IEndorserService* m_pEs = nullptr; //背靠背的endorser-service
};

#include "cc_comm/base_inner_service.inl"

#endif // !BASE_INNER_SERVICE_H_02U34092U34U32U4320940398987078907
