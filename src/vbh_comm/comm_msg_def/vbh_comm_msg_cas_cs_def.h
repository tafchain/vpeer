#ifndef COMM_MSG_CAS_CS_DEF_H_456465465461245455678577867968878
#define COMM_MSG_CAS_CS_DEF_H_456465465461245455678577867968878
#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{


	//peer-cs -> other-peer-cas //拉取区块
	class CGetBlockCsCasReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_GET_BLOCK_CS_CAS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nBlockID);

	public:
		ACE_UINT64 m_nBlockID; //要拉取的区块
	};

	//other-peer-cas -> peer-cs //拉取区块应答
	class CGetBlockCasCsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_GET_BLOCK_CAS_CS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nBlockID, m_blockData);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT64 m_nBlockID; //要拉取的区块
		DSC::CDscBlob m_blockData; //区块数据
	};


	//peer-cs -> other-peer-cas //校验指定区块的hash的请求
	class CCheckBlockHashCsCasReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CHECK_BLOCK_HASH_CS_CAS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nBlockID, m_strBlockHash);

	public:
		ACE_UINT64 m_nBlockID; //要校验的区块
		CDscString m_strBlockHash; //指定区块的hash值
	};

	//other-peer-cas -> peer-cs //校验指定区块的hash应答
	class CCheckBlockHashCasCsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_CHECK_BLOCK_HASH_CAS_CS_RSP
		};

	public:
		enum
		{
			EN_HASH_CHECK_FAILED = 0,
			EN_HASH_CHECK_OK
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCheckResult, m_nBlockID, m_blockData);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCheckResult; //区块hash的校验结果，如果校验失败，在会带回正确区块的数据
		ACE_UINT64 m_nBlockID; //要校验的区块
		DSC::CDscBlob m_blockData; //区块数据，如果校验不通过，则该字段携带正确的区块数据
	};

	class CVerifyPeerStateCsCasReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_VERIFY_PEER_STATE_CS_CAS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nKafkaBlockID, m_strKafkaBlockHash);

	public:
		ACE_UINT32 m_nKafkaBlockID;
		CDscString m_strKafkaBlockHash;
	};

	// //验证peer是否正常的请求
	class CVerifyPeerStateCasCsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_VERIFY_PEER_STATE_CAS_CS_RSP
		};

	public:
		enum
		{
			EN_VERIFY_OK, //验证通过
			EN_VERIFY_FAILED //验证未通过
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nVerifyResult, m_CasAddress);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nVerifyResult;
		CSyncSourcePeerCasAddress m_CasAddress;
	};
}
#endif