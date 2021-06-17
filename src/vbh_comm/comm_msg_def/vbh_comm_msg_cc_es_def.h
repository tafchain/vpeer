#ifndef COMM_MSG_CC_ES_DEF_H_784546743434576437867967354976543246754
#define COMM_MSG_CC_ES_DEF_H_784546743434576437867967354976543246754

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{
	// endorser -> cc
	class CRegistUserEsCcReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_ES_CC_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_nChannelID, m_userInfo);

	public:
		ACE_UINT32 m_nEsSessionID; //endorser 端的sessionID
		ACE_UINT32 m_nChannelID; //用户所属channel
		DSC::CDscShortBlob m_userInfo; //注册时携带用户信息
	};

	//cc -> endorser
	class CRegistUserCcEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_CC_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_userInitInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscShortBlob m_userInitInfo; //注册后的初始信息，待背书内容
	};

		//endorser -> cc
	class CProposeEsCcReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_ES_CC_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_nActionID, m_nChannelID, m_proposeUserKey, m_signature, m_proposal);

	public:
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nChannelID;
		VBH::CVbhAllocatedKey m_proposeUserKey; //提案发起人
		DSC::CDscShortBlob m_signature; //客户端提交的，针对提案的签名
		DSC::CDscShortBlob m_proposal;//客户端提交的提案
	};

	//cc -> endorser
	class CProposeCcEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_CC_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nEsSessionID, m_receipt, m_transContent);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscShortBlob m_receipt; //cc为提案生成的回执
		DSC::CDscBlob m_transContent; //事务内容 对应 CRoughProposalTransactionWrapper 结构， 为了做好封装和提高效率，将离散的数据打包，但是该打包并非最终区块中的事务
	};


	
	
	// endorser -> cc
	class CLoadOuterCcEsCcReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_LOAD_OUTER_CC_ES_CC_REQ
		};

	public:
	public:
		DSC_BIND_ATTR(m_nChannelID, m_nSessionID, m_nCcID, m_ccName);

	public:
		ACE_UINT32 m_nChannelID; //用户所属channel
		ACE_UINT32 m_nSessionID; // 上层业务service发起请求时，本地的sessionID
		ACE_UINT32 m_nCcID;       //cc_id
		DSC::CDscShortBlob m_ccName; //CC的dll名字
	};
	//cc -> endorser
	class CLoadOuterCcCcEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_LOAD_OUTER_CC_CC_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
	};
	
		//endorser -> cc
	class CQueryEsCcReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_ES_CC_REQ
		};

	public:
		DSC_BIND_ATTR(m_nEsSessionID, m_nActionID, m_nChannelID, m_param);

	public:
		ACE_UINT32 m_nEsSessionID;
		ACE_UINT32 m_nActionID; //查询时指定的action-id
		ACE_UINT32 m_nChannelID;
		DSC::CDscShortBlob m_param; //查询参数
	};

	//cc -> endorser
	class CQueryCcEsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CC_ES_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nEsSessionID, m_info);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nEsSessionID;
		DSC::CDscBlob m_info; //cc生成的查询内容
	};

}


#endif
