#ifndef I_TRANSFER_AGENT_SERVICE_H_4237894239723186512365236543277
#define I_TRANSFER_AGENT_SERVICE_H_4237894239723186512365236543277

#include "vbh_comm/comm_msg_def/vbh_comm_msg_es_def.h"

class ITransferAgentService
{
public:
	virtual ACE_INT32 SendSubmitRegistUserTransactionEsTasReq(VBH::CSubmitRegistUserTransactionEsTasReq& rSubmitTransReq) = 0;
	virtual ACE_INT32 SendSubmitProposalTransactionEsTasReq(VBH::CSubmitProposalTransactionEsTasReq& rSubmitTransReq) = 0;
};

#endif
