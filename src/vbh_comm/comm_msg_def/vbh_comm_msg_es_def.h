#ifndef COMM_MSG_ES_DEF_H_78979876467967967674579878949678979454678
#define COMM_MSG_ES_DEF_H_78979876467967967674579878949678979454678

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{

	
	// client -> endorser 
	class CLoadOuterCcUserCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_LOAD_OUTER_CC_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nChannelID, m_nSessionID, m_nCcID, m_ccName);

	public:
		ACE_UINT32 m_nChannelID; //用户所属channel
		ACE_UINT32 m_nSessionID; // 上层业务service发起请求时，本地的sessionID
		ACE_UINT32 m_nCcID;       //cc_id
		DSC::CDscShortBlob m_ccName; //CC的dll名字
	};

	// endorser -> client
	class CLoadOuterCcEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_LOAD_OUTER_CC_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
	};

//===============================注册流程=消息==================================

	class CCryptRegistUserCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_REGIST_USER_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCltSessionID, m_userData);

	public:
		ACE_UINT32 m_nCltSessionID;
		DSC::CDscShortBlob m_userData; //bWaitNotify + bSubmitNode + cc-id + channel-id + user-info + nonce
	};

	// endorser -> client
	class CCryptRegistUserEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_REGIST_USER_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID, m_nEsSessionID, m_userData);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
		ACE_UINT32 m_nEsSessionID; //endorser 端的session id
		DSC::CDscBlob m_userData; //加密后的用户数据：用户初始信息 + 随机数
	};

	// client -> endorser 
	class CRegistUserCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_bSubmitNode, m_nCltSessionID, m_nChannelID, m_userInfo);

	public:
		bool m_bSubmitNode; //标记是否被选中的
		ACE_UINT32 m_nCltSessionID;
		ACE_UINT32 m_nChannelID; //用户所属channel
		DSC::CDscShortBlob m_userInfo; //用户信息
	};

	// endorser -> client
	class CRegistUserEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID, m_nEsSessionID, m_userInitInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
		ACE_UINT32 m_nEsSessionID; //endorser 端的session id
		DSC::CDscShortBlob m_userInitInfo; //允许注册时的初始信息
	};

//client -> endorser //背书通过后最终提交用户注册事务的请求
	class CCryptSubmitRegistUserTransactionCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_SUBMIT_REGIST_USER_TRANSACTION_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_userData);

	public:
		ACE_UINT32 m_nEsSessionID; //endorser 端的sessionID
		DSC::CDscShortBlob m_userData; //加密后: 客户端公钥 + 服务端私钥 + 对称秘钥 随机数
	};

	//endorser -> client
	class CCryptSubmitRegistUserTransactionEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_SUBMIT_REGIST_USER_TRANSACTION_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID, m_userData);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
		DSC::CDscShortBlob m_userData; //加密后数据: nonce +  allocUserKey+svrPubCryptKey+cltPriCryptKey+envelopeKey
	};

	//client -> endorser //背书通过后最终提交用户注册的请求
	class CSubmitRegistUserTransactionCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_cltPubKey, m_svrPriKey, m_envelopeKey);

	public:
		ACE_UINT32 m_nEsSessionID; //endorser 端的sessionID
		DSC::CDscShortBlob m_cltPubKey; // 客户端公钥
		DSC::CDscShortBlob m_svrPriKey; //服务端私钥
		DSC::CDscShortBlob m_envelopeKey; //对称秘钥
	};

	//endorser -> client
	//返回给客户端的秘钥，延迟到客户端背书通过后在endorser端生成
	class CSubmitRegistUserTransactionEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID, m_userKey);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
		VBH::CVbhAllocatedKey m_userKey; //vbh系统为注册用户生成的key
	};

	// endorser service -> transform agent service
	class CSubmitRegistUserTransactionEsTasReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_ES_TAS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_nChannelID, m_userInfo, m_cltPubKey, m_svrPriKey, m_envelopeKey);

	public:
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nChannelID;

		DSC::CDscShortBlob m_userInfo; //事务内容 -//cc为用户生成的初始信息
		DSC::CDscShortBlob m_cltPubKey; //事务内容  -// 客户端公钥
		DSC::CDscShortBlob m_svrPriKey; //事务内容  -//服务端私钥
		DSC::CDscShortBlob m_envelopeKey; //事务内容  -//对称秘钥
	};

	//transform agent service -> endorser service  提交注册事务应答
	class CSubmitRegistUserTransactionTasEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_TAS_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_userKey, m_registTransUrl);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		VBH::CVbhAllocatedKey m_userKey; //注册生成的user-key
		VBH::CTransactionUrl m_registTransUrl; //注册事务的URL，事务中包含秘钥 //
	};

	//transform agent service -> order-cps
	class CSubmitRegistUserTransactionTasCpsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_TAS_CPS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nTasSessionID, m_userInfo, m_cltPubKey, m_svrPriKey, m_envelopeKey);

	public:
		ACE_UINT32 m_nTasSessionID;

		DSC::CDscShortBlob m_userInfo; //事务内容 -//cc为用户生成的初始信息
		DSC::CDscShortBlob m_cltPubKey; //事务内容  -//注册时生成秘钥， 客户端公钥
		DSC::CDscShortBlob m_svrPriKey; //事务内容  -//注册时生成秘钥，服务端私钥
		DSC::CDscShortBlob m_envelopeKey; //事务内容  -//注册时生成秘钥，对称秘钥
	};

	//order-cps -> transform agent service  提交注册事务应答
	class CSubmitRegistUserTransactionCpsTasRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_REGIST_USER_TRANSACTION_CPS_TAS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nTasSessionID, m_userKey, m_registTransUrl);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nTasSessionID;
		VBH::CVbhAllocatedKey m_userKey; //注册生成的user-key
		VBH::CTransactionUrl m_registTransUrl; //注册事务的URL，事务中包含秘钥 //
	};

	//transform agent service ->order-cps   查询是否为Leader Cps 请求
	class CQueryLeaderCpsTasCpsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_LEADER_CPS_TAS_CPS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nPeerID);

	public:

		ACE_UINT32 m_nPeerID;

	};

	//order-cps -> transform agent service  查询是否为Leader Cps 应答
	class CQueryLeaderCpsCpsTasRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_LEADER_CPS_TAS_CPS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nIsLeaderCps, m_nPeerID);

	public:
		ACE_UINT32 m_nIsLeaderCps;
		ACE_UINT32 m_nPeerID;
	};

	//提交打包后事务同提案流程

	

	//提交打包后事务同提案流程

	//===============================发起提案 流程=消息==================================
//client -> endorser
	class CCryptProposeCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_PROPOSE_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nChannelID, m_nCltSessionID, m_userKey, m_transUrl, m_userData);

	public:
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nChannelID;
		ACE_UINT32 m_nCltSessionID; //客户端sessionID
		VBH::CVbhAllocatedKey m_userKey; //提案发起人 //注册成功时带回的，系统为该注册用户分配的user-key
		VBH::CTransactionUrl m_transUrl; //用户注册时，存放注册信息的事务url
		DSC::CDscBlob m_userData; //加密后数据：nonce + signature + proposal + action-id
	};

	//endorser -> client
	class CCryptProposeEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_PROPOSE_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nEsSessionID, m_nCltSessionID, m_userData);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID; //endorser session id
		ACE_UINT32 m_nCltSessionID; //client session id
		DSC::CDscBlob m_userData; //加密后数据： nonce + transaction-content(transaction的contentnt按拆分形式打包)
	};

	//client -> endorser
	class CProposeCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_bSubmitNode, m_nChannelID, m_nCltSessionID, m_nActionID, m_userKey, m_signature, m_proposal);

	public:
		bool m_bSubmitNode; //通知endorser保留session，用于后续commit transaction
		ACE_UINT32 m_nChannelID;
		ACE_UINT32 m_nCltSessionID; //客户端sessionID
		ACE_UINT32 m_nActionID;
		VBH::CVbhAllocatedKey m_userKey; //提案发起人
		DSC::CDscShortBlob m_signature; //针对提案的签名
		DSC::CDscShortBlob m_proposal; //提案
	};

	//endorser -> client
	//从cc生成的事务内容就是打包好的，只是打包时，新建information和更新information处于不同的队列（维持目前三个队列的状态）
	class CProposeEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_nCltSessionID, m_transContent);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID; //endorser session id
		ACE_UINT32 m_nCltSessionID; //client session id
		DSC::CDscBlob m_transContent; //事务内容
	};

	//endoser -> x committer service
	class CQueryCryptKeyProposeEsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CRYPT_KEY_PROPOSE_ES_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_transUrl, m_nEsSessionID);

	public:
		VBH::CTransactionUrl m_transUrl; //用户注册时，存放注册信息的transaction-url
		ACE_UINT32 m_nEsSessionID;
	};

	//x committer service -> endorser
	class CQueryCryptKeyProposeCsEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CRYPT_KEY_PROPOSE_CS_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_cltPubKey, m_envelopeKey);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscShortBlob m_cltPubKey; //用户公钥
		DSC::CDscShortBlob m_envelopeKey;  //得到的对称秘钥
	};

// client -> endorser
	class CCryptSubmitProposalTransactionCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_SUBMIT_PROPOSAL_TRANSACTION_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nEsSessionID, m_userData);

	public:
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscShortBlob m_userData; //加密后数据：nonce
	};

	// endorser -> sdk 提交提案事务的应答
	class CCryptSubmitProposalTransactionEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_SUBMIT_PROPOSAL_TRANSACTION_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID,m_nReturnCode, m_nCltSessionID, m_userData,m_vecInfoID);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID; //client session id
		DSC::CDscShortBlob m_userData; //加密后数据: nonce + gen-trans-key
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecInfoID;
	};

	// client -> endorser
	class CSubmitProposalTransactionCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID);

	public:
		ACE_UINT32 m_nEsSessionID;
	};

	// endorser -> sdk 提交提案事务的应答
	class CSubmitProposalTransactionEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nCltSessionID, m_alocTransKey,m_vecInfoID);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID; //client session id
		VBH::CVbhAllocatedTransactionKey m_alocTransKey;
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecInfoID;
	};

	// endorser service -> transform agent service
	class CSubmitProposalTransactionEsTasReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_ES_TAS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nEsSessionID, m_nChannelID, m_transContent);

	public:
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nChannelID;
		DSC::CDscBlob m_transContent; //事务内容
	};

	//transform agent service -> endorser service 提交提案事务应答
	class CSubmitProposalTransactionTasEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_TAS_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nEsSessionID, m_alocTransKey, m_vecInfoID);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		VBH::CVbhAllocatedTransactionKey m_alocTransKey;
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecInfoID;
	};

	//transform agent service -> order-cps
	class CSubmitProposalTransactionTasCpsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_TAS_CPS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nTasSessionID, m_nType, m_transContent);

	public:
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nTasSessionID;
		ACE_UINT32 m_nType;
		DSC::CDscBlob m_transContent; //事务内容
	};

	//order-cps -> transform agent service  提交提案事务应答
	class CSubmitProposalTransactionCpsTasRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SUBMIT_PROPOSAL_TRANSACTION_CPS_TAS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nTasSessionID, m_alocTransKey, m_vecInfoID);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nTasSessionID;
		VBH::CVbhAllocatedTransactionKey m_alocTransKey;
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecInfoID;
	};



// client -> endorser
	class CCryptQueryCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_QUERY_USER_INFO_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCltSessionID, m_nChannelID, m_userKey, m_transUrl, m_userData);

	public:
		ACE_UINT32 m_nCltSessionID;
		ACE_UINT32 m_nChannelID; //user所处的channel
		VBH::CVbhAllocatedKey m_userKey; //注册成功时带回的，系统为该注册用户分配的user-key
		VBH::CTransactionUrl m_transUrl; //存放用户注册信息的transaction-url, 该transaction中存放着秘钥
		DSC::CDscShortBlob m_userData; //加密后数据：nonce + action-id + param
	};

	//endorser -> client
	class CCryptQueryEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CRYPT_QUERY_USER_INFO_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCltSessionID, m_userData);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCltSessionID;
		DSC::CDscShortBlob m_userData; //加密后数据：nonce + info // CQueryUserInfoRspWrapper解包
	};

	//todo:非加密通道，待调整
	// client -> endorser
	class CQueryUserInfoCltEsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_USER_INFO_CLT_ES_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCltSessionID, m_nChannelID, m_userKey);

	public:
		ACE_UINT32 m_nCltSessionID;
		ACE_UINT32 m_nChannelID;
		VBH::CVbhAllocatedKey m_userKey; //注册成功时带回的，系统为该注册用户分配的user-key
	};

	//endorser -> client
	class CQueryUserInfoEsCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_USER_INFO_ES_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nSessionID, m_userInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nSessionID;
		DSC::CDscShortBlob m_userInfo;
	};

	//endorser -> committer service
	class CQueryCryptKeyQueryEsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CRYPT_KEY_QUERY_ES_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_transUrl);

	public:
		ACE_UINT32 m_nEsSessionID;
		VBH::CTransactionUrl m_transUrl; 
	};

	//committer service -> endorser
	class CQueryCryptKeyQueryCsEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CRYPT_KEY_QUERY_CS_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_cltPubKey, m_envelopeKey);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscShortBlob m_cltPubKey; //用户公钥
		DSC::CDscShortBlob m_envelopeKey;  //对称秘钥
	};


}


#endif
