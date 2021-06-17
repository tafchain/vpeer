#ifndef COMM_MSG_CLT_CLT_DEF_H_789454464343213764313245646786448575
#define COMM_MSG_CLT_CLT_DEF_H_789454464343213764313245646786448575

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"

namespace VBH
{
		//client上层业务逻辑 -> client service
	class CRegistUserCltCltReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_CLT_CLT_REQ
		};

	public:
		DSC_BIND_ATTR(m_nChannelID, m_nSessionID, m_userInfo, m_cltPubKey);

	public:
		ACE_UINT32 m_nChannelID; //用户所属channel
		ACE_UINT32 m_nSessionID; // 上层业务service发起请求时，本地的sessionID
		DSC::CDscShortBlob m_userInfo; //提交到CC的用户信息，供CC对用户进行特别处理
		DSC::CDscShortBlob m_cltPubKey; //客户端公钥 //Base64编码格式
	};


	//client service -> client上层业务逻辑
	class CRegistUserCltCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGIST_USER_CLT_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nSrcSessionID, m_vbhCryptKey, m_vbhUserID);

	public:
		ACE_INT32 m_nReturnCode; //返回值为VBH::EN_OK_TYPE即是注册成功，其余值为注册失败
		ACE_UINT32 m_nSrcSessionID; // 上层业务service发起请求时，发送方的sessionID
		DSC::CDscShortBlob m_vbhCryptKey; //注册用户时为用户生成的vbh秘钥，对应于VBH::CUserSequenceWrapper 结构 //hex编码
		DSC::CDscShortBlob m_vbhUserID; //注册生成的用户id，在交易中涉及用户时使用， //实际上是hex编码的 VBH::CVbhAllocatedKey
	};

	//client上层业务逻辑 -> client service
	class CProposeCltCltReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_CLT_CLT_REQ
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nSrcSessionID, m_proposal, m_userCryptKey);

	public:
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nSrcSessionID; // 上层业务service发起请求时，本地的sessionID
		DSC::CDscShortBlob m_proposal;
		DSC::CDscShortBlob m_userCryptKey; //创建者的秘钥 //需要 base64解码->V解码，服务端公钥，客户端私钥，对称秘钥
	};

	//client service -> client上层业务逻辑
	class CProposeCltCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_PROPOSE_CLT_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nActionID, m_nReturnCode, m_nSrcSessionID, m_receipt, m_transKey, m_InfoKey);

	public:
		ACE_UINT32 m_nActionID;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nSrcSessionID; // 上层业务service发起请求时，发送方的sessionID
		DSC::CDscShortBlob m_receipt; //cc为提案生成的回执
		DSC::CDscShortBlob m_transKey; //系统为transaction生成的key
		DSC::CDscShortBlob m_InfoKey;
	};
	//===============================查询流程=消息==================================
	//client上层业务 -> client service 
	class CQueryCltCltReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CLT_CLT_REQ
		};

	public:
		DSC_BIND_ATTR(m_nDstPeerID, m_nSrcSessionID, m_param, m_nActionID, m_userCryptKey);

	public:
		ACE_UINT16 m_nDstPeerID; //要查询的peer-id
		ACE_UINT32 m_nSrcSessionID;
		ACE_UINT32 m_nActionID;
		DSC::CDscShortBlob m_param; //送到CC的查询参数
		DSC::CDscShortBlob m_userCryptKey; //base64编码的字符串
	};

	//client service  -> client上层业务
	class CQueryCltCltRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_CLT_CLT_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nActionID, m_nSrcSessionID, m_info);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nActionID;
		ACE_UINT32 m_nSrcSessionID;
		DSC::CDscBlob m_info; //查询得到的信息
	};

}


#endif
