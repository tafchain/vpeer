#ifndef I_ENDORSER_CC_SERVICE_H_78787687146456464646464654
#define I_ENDORSER_CC_SERVICE_H_78787687146456464646464654

#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_es_def.h"

//cc在形式上分为comm-cc和outer-cc
//comm-cc为CC的通用处理，为outer-cc提供接口，outer-cc在comm-cc中拉起来

//ICommCcService用于和endorser通信

class IEndorserCcService
{
public:
	virtual ACE_INT32 SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req) = 0;
	virtual ACE_INT32 SendProposeEsCcReq(VBH::CProposeEsCcReq& req) = 0;
	virtual ACE_INT32 SendQueryEsCcReq(VBH::CQueryEsCcReq& req) = 0;
	virtual ACE_INT32 SendLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req) = 0;
};

#endif