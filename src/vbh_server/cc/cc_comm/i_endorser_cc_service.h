#ifndef I_ENDORSER_CC_SERVICE_H_78787687146456464646464654
#define I_ENDORSER_CC_SERVICE_H_78787687146456464646464654

#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_es_def.h"

//cc����ʽ�Ϸ�Ϊcomm-cc��outer-cc
//comm-ccΪCC��ͨ�ô���Ϊouter-cc�ṩ�ӿڣ�outer-cc��comm-cc��������

//ICommCcService���ں�endorserͨ��

class IEndorserCcService
{
public:
	virtual ACE_INT32 SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req) = 0;
	virtual ACE_INT32 SendProposeEsCcReq(VBH::CProposeEsCcReq& req) = 0;
	virtual ACE_INT32 SendQueryEsCcReq(VBH::CQueryEsCcReq& req) = 0;
	virtual ACE_INT32 SendLoadOuterCcEsCcReq(VBH::CLoadOuterCcEsCcReq& req) = 0;
};

#endif