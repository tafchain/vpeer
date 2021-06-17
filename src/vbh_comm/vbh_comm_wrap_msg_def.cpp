#include "vbh_comm/vbh_comm_wrap_msg_def.h"

VBH::CCryptRegistUserCltEsReqDataWrapper::CCryptRegistUserCltEsReqDataWrapper(bool& bSubmitNode, ACE_UINT32& nChannelID, DSC::CDscShortBlob& userInfo, ACE_INT32& nNonce)
	: m_bSubmitNode(bSubmitNode)
	, m_nChannelID(nChannelID)
	, m_userInfo(userInfo)
	, m_nNonce(nNonce)
{
}

VBH::CCryptRegistUserEsCltRspDataWrapper::CCryptRegistUserEsCltRspDataWrapper(DSC::CDscShortBlob& userInitInfo, ACE_INT32& nNonce)
	: m_userInitInfo(userInitInfo)
	, m_nNonce(nNonce)
{
}

VBH::CCryptSubmitRegistUserTransactionCltEsReqDataWrapper::CCryptSubmitRegistUserTransactionCltEsReqDataWrapper(DSC::CDscShortBlob& cltPubKey, ACE_INT32& nNonce)
	: m_cltPubKey(cltPubKey)
	, m_nNonce(nNonce)
{
}

VBH::CCryptSubmitRegistUserTransactionEsCltRspDataWrapper::CCryptSubmitRegistUserTransactionEsCltRspDataWrapper(DSC::CDscShortBlob& svrPubKey, VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl, ACE_INT32& nNonce)
	: m_svrPubKey(svrPubKey)
	, m_userKey(userKey)
	, m_transUrl(transUrl)
	, m_nNonce(nNonce)
{
}

VBH::CCryptCreateInformationCltEsReqDataWrapper::CCryptCreateInformationCltEsReqDataWrapper(ACE_UINT8& nSubmitNodeType, 
	ACE_INT32& nNonce, ACE_UINT32& nActionID,
	DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal)
	: m_nSubmitNodeType(nSubmitNodeType)
	, m_nNonce(nNonce)
	, m_nActionID(nActionID)
	, m_signature(signature)
	, m_proposal(proposal)
{
}

VBH::CCryptCreateInformationEsCltRspDataWrapper::CCryptCreateInformationEsCltRspDataWrapper(DSC::CDscBlob& initValue, ACE_INT32& nNonce)
	: m_initValue(initValue)
	, m_nNonce(nNonce)
{
}

VBH::CCryptSubmitCreateInformationTransactionEsCltRspDataWrapper::CCryptSubmitCreateInformationTransactionEsCltRspDataWrapper(VBH::CSimpleVector<VBH::CVbhAllocatedKey>& vecInfoKey, ACE_INT32& nNonce)
	: m_vecInfoKey(vecInfoKey)
	, m_nNonce(nNonce)
{
}

VBH::CCryptProposeCltEsReqDataWrapper::CCryptProposeCltEsReqDataWrapper(ACE_UINT8& nSubmitNodeType, ACE_UINT32& nActionID, 
	DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal, ACE_INT32& nNonce)
	: m_nSubmitNodeType(nSubmitNodeType)
    , m_nActionID(nActionID)
	, m_signature(signature)
	, m_proposal(proposal)
	, m_nNonce(nNonce)
{
}

VBH::CCryptSubmitProposalEsCltRspDataWrapper::CCryptSubmitProposalEsCltRspDataWrapper(DSC::CDscBlob& transContent, ACE_INT32& nNonce)
	: m_transContent(transContent)
	, m_nNonce(nNonce)
{
}

VBH::CCryptSubmitProposalTransactionEsCltRspDataWrapper::CCryptSubmitProposalTransactionEsCltRspDataWrapper(VBH::CVbhAllocatedTransactionKey& alocTransKey, DSC::CDscShortBlob& receipt, ACE_INT32& nNonce)
	: m_alocTransKey(alocTransKey)
	, m_receipt(receipt)
	, m_nNonce(nNonce)
{
}

VBH::CCryptQueryCltEsReqDataWrapper::CCryptQueryCltEsReqDataWrapper(ACE_INT32& nNonce, ACE_UINT32& nActionID, DSC::CDscShortBlob& param)
	: m_nNonce(nNonce)
	, m_nActionID(nActionID)
	, m_param(param)
{
}

VBH::CCryptQueryEsCltRspDataWrapper::CCryptQueryEsCltRspDataWrapper(DSC::CDscBlob& info, ACE_INT32& nNonce)
	: m_info(info)
	, m_nNonce(nNonce)
{
}

VBH::CCryptQueryTransactionReqDataWrapper::CCryptQueryTransactionReqDataWrapper(ACE_INT32& nNonce, VBH::CVbhAllocatedTransactionKey& transKey)
	:m_nNonce(nNonce)
	,m_transKey(transKey)
{
}

VBH::CCryptQueryTransactionRspDataWrapper::CCryptQueryTransactionRspDataWrapper(ACE_INT32& nNonce, VBH::CVbhAllocatedKey& userKey, DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal)
	: m_nNonce(nNonce)
	, m_userKey(userKey)
	, m_signature(signature)
	, m_proposal(proposal)
{
}

VBH::CCryptQueryBlockReqDataWrapper::CCryptQueryBlockReqDataWrapper(ACE_INT32& nNonce, ACE_UINT64& nBlockID)
	: m_nNonce(nNonce)
	, m_nBlockID(nBlockID)
{
}

VBH::CCryptQueryBlockRspDataWrapper::CCryptQueryBlockRspDataWrapper(ACE_UINT16& nTransCount, ACE_INT32& nNonce, ACE_UINT64& nBlockTime, DSC::CDscShortBlob& preBlockHash, DSC::CDscShortBlob& merkelTreeRootHash)
	: m_nTransCount(nTransCount)
	, m_nNonce(nNonce)
	, m_nBlockTime(nBlockTime)
	, m_preBlockHash(preBlockHash)
	, m_merkelTreeRootHash(merkelTreeRootHash)
{
}



