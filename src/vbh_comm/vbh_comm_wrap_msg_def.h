#ifndef __VBH_COMMMON_WRAPPER_MESSAGE_DEF_H
#define __VBH_COMMMON_WRAPPER_MESSAGE_DEF_H

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/container/dsc_array.h"

#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{
	enum __SUBMIT_NODE_TYPE
	{
		EN_NOT_SUBMIT_NODE_TYPE = 0, //非提交节点
		EN_IS_SUBMIT_NODE_TYPE, //提交节点
		EN_ONLY_ONE_SUBMIT_NODE_TYPE //唯一提交节点
	};

	//====================注册用户流程======================
	class CCryptRegistUserCltEsReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptRegistUserCltEsReqDataWrapper(bool& bSubmitNode, ACE_UINT32& nChannelID, DSC::CDscShortBlob& userInfo, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_bSubmitNode, m_nChannelID, m_userInfo, m_nNonce);

	public:
		bool& m_bSubmitNode; //标记是否被选中的
		ACE_UINT32& m_nChannelID; //用户所属channel
		DSC::CDscShortBlob& m_userInfo; //用户信息
		ACE_INT32& m_nNonce; //通信用随机数
	};

	class CCryptRegistUserEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptRegistUserEsCltRspDataWrapper(DSC::CDscShortBlob& userInitInfo, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_userInitInfo, m_nNonce);

	public:
		DSC::CDscShortBlob& m_userInitInfo; //允许注册时的初始信息
		ACE_INT32& m_nNonce; //通信用随机数
	};

	class CCryptSubmitRegistUserTransactionCltEsReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptSubmitRegistUserTransactionCltEsReqDataWrapper(DSC::CDscShortBlob& cltPubKey, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_cltPubKey, m_nNonce);

	public:
		DSC::CDscShortBlob& m_cltPubKey; // 客户端公钥
		ACE_INT32& m_nNonce; //通信用随机数
	};

	class CCryptSubmitRegistUserTransactionEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptSubmitRegistUserTransactionEsCltRspDataWrapper(DSC::CDscShortBlob& svrPubKey, VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_svrPubKey, m_userKey, m_transUrl, m_nNonce);

	public:
		DSC::CDscShortBlob& m_svrPubKey;
		VBH::CVbhAllocatedKey& m_userKey;
		VBH::CTransactionUrl& m_transUrl;
		ACE_INT32& m_nNonce;
	};

	//====================创建information流程======================
	class CCryptCreateInformationCltEsReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptCreateInformationCltEsReqDataWrapper(ACE_UINT8& nSubmitNodeType,
			ACE_INT32& nNonce, ACE_UINT32& nActionID,
			DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal);

	public:
		DSC_BIND_ATTR(m_nSubmitNodeType, m_nNonce, m_nActionID, m_signature, m_proposal);

	public:
		ACE_UINT8& m_nSubmitNodeType; //标记是否被选中的
		ACE_INT32& m_nNonce; //通信用随机数
		ACE_UINT32& m_nActionID;
		DSC::CDscShortBlob& m_signature; //提案签名
		DSC::CDscShortBlob& m_proposal; //提案
	};

	class CCryptCreateInformationEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptCreateInformationEsCltRspDataWrapper(DSC::CDscBlob& initValue, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_initValue, m_nNonce);

	public:
		DSC::CDscBlob& m_initValue; //允许注册时的初始信息
		ACE_INT32& m_nNonce; //通信用随机数
	};

	class CCryptSubmitCreateInformationTransactionEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptSubmitCreateInformationTransactionEsCltRspDataWrapper(VBH::CSimpleVector<VBH::CVbhAllocatedKey>& vecInfoKey, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_vecInfoKey, m_nNonce);

	public:
		VBH::CSimpleVector<VBH::CVbhAllocatedKey>& m_vecInfoKey;
		ACE_INT32& m_nNonce;
	};

	//====================发起提案流程======================
	class CCryptProposeCltEsReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptProposeCltEsReqDataWrapper(ACE_UINT8& nSubmitNodeType, ACE_UINT32& nActionID,
			DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_nSubmitNodeType, m_nActionID, m_signature, m_proposal, m_nNonce);

	public:
		ACE_UINT8& m_nSubmitNodeType; //通知endorser保留session，用于后续commit transaction
		ACE_UINT32& m_nActionID;
		DSC::CDscShortBlob& m_signature; //提案签名
		DSC::CDscShortBlob& m_proposal;
		ACE_INT32& m_nNonce;
	};

	class CCryptSubmitProposalEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptSubmitProposalEsCltRspDataWrapper(DSC::CDscBlob& transContent, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_transContent, m_nNonce);

	public:
		DSC::CDscBlob& m_transContent; //事务内容
		ACE_INT32& m_nNonce;
	};

	class CCryptSubmitProposalTransactionEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptSubmitProposalTransactionEsCltRspDataWrapper(VBH::CVbhAllocatedTransactionKey& alocTransKey, DSC::CDscShortBlob& receipt, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_alocTransKey, m_nNonce);

	public:
		VBH::CVbhAllocatedTransactionKey& m_alocTransKey;
		DSC::CDscShortBlob& m_receipt;
		ACE_INT32& m_nNonce;
	};

	//====================查询流程======================
	class CCryptQueryCltEsReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryCltEsReqDataWrapper(ACE_INT32& nNonce, ACE_UINT32& nActionID, DSC::CDscShortBlob& param);

	public:
		DSC_BIND_ATTR(m_nNonce, m_nActionID, m_param);

	public:
		ACE_INT32& m_nNonce;
		ACE_UINT32& m_nActionID;
		DSC::CDscShortBlob& m_param;
	};

	class CCryptQueryEsCltRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryEsCltRspDataWrapper(DSC::CDscBlob& info, ACE_INT32& nNonce);

	public:
		DSC_BIND_ATTR(m_info, m_nNonce);

	public:
		DSC::CDscBlob& m_info;
		ACE_INT32& m_nNonce;
	};

	//====================直接查询事务流程======================
	class CCryptQueryTransactionReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryTransactionReqDataWrapper(ACE_INT32& nNonce, VBH::CVbhAllocatedTransactionKey& transKey);

	public:
		DSC_BIND_ATTR(m_nNonce, m_transKey);

	public:
		ACE_INT32& m_nNonce;
		VBH::CVbhAllocatedTransactionKey& m_transKey;
	};

	class CCryptQueryTransactionRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryTransactionRspDataWrapper(ACE_INT32& nNonce, VBH::CVbhAllocatedKey& userKey, DSC::CDscShortBlob& signature, DSC::CDscShortBlob& proposal);

	public:
		DSC_BIND_ATTR(m_nNonce, m_userKey, m_signature, m_proposal);

	public:
		ACE_INT32& m_nNonce;
		VBH::CVbhAllocatedKey& m_userKey; //发起提案的用户，也是对提案进行签名的用户
		DSC::CDscShortBlob& m_signature; //提案签名
		DSC::CDscShortBlob& m_proposal; //事务中的提案
	};

	//====================直接查询区块流程======================
	class CCryptQueryBlockReqDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryBlockReqDataWrapper(ACE_INT32& nNonce, ACE_UINT64& nBlockID);

	public:
		DSC_BIND_ATTR(m_nNonce, m_nBlockID);

	public:
		ACE_INT32& m_nNonce;
		ACE_UINT64& m_nBlockID;
	};

	class CCryptQueryBlockRspDataWrapper
	{
	public:
		VBH_COMM_DEF_EXPORT CCryptQueryBlockRspDataWrapper(ACE_UINT16& nTransCount, ACE_INT32& nNonce, ACE_UINT64& nBlockTime, DSC::CDscShortBlob& preBlockHash, DSC::CDscShortBlob& merkelTreeRootHash);

	public:
		DSC_BIND_ATTR(m_nTransCount, m_nNonce, m_nBlockTime, m_preBlockHash, m_merkelTreeRootHash);

	public:
		ACE_UINT16& m_nTransCount; //打包在区块中的事务个数 //区块数最多不能
		ACE_INT32& m_nNonce;
		ACE_UINT64& m_nBlockTime; //区块生成时间
		DSC::CDscShortBlob& m_preBlockHash; //前一区块的hash值
		DSC::CDscShortBlob& m_merkelTreeRootHash; //本区块merkel树根的hash值
	};

}

#endif // !__vbh_commmon_wrapper_message_h

