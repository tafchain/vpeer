#ifndef I_ENDORSER_SERVICE_H_78432978432133217423762341327
#define I_ENDORSER_SERVICE_H_78432978432133217423762341327

#include "vbh_comm/comm_msg_def/vbh_comm_msg_cc_es_def.h"
#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"

class IEndorserService
{
public: //��tas�����Ľӿں���
	virtual void SendSubmitRegistUserTransactionTasEsRsp(VBH::CSubmitRegistUserTransactionTasEsRsp& rSubmitTransactionRsp) = 0;
	virtual void SendSubmitProposalTransactionTasEsRsp(VBH::CSubmitProposalTransactionTasEsRsp& rSubmitTransactionRsp) = 0;

public: //��inner-cc�����Ľӿں���
	virtual void SendRegistUserCcEsRsp(VBH::CRegistUserCcEsRsp& rsp) = 0;
	virtual void SendProposeCcEsRsp(VBH::CProposeCcEsRsp& rsp) = 0;
	virtual void SendQueryCcEsRsp(VBH::CQueryCcEsRsp& rsp) = 0;
	virtual void SendLoadOuterCcCcEsRsp(VBH::CLoadOuterCcCcEsRsp& rsp) = 0;
};

#endif
