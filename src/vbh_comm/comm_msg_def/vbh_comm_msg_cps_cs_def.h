#ifndef COMM_MSG_CLT_CLT_DEF_H_789454464343213764313245646786448575
#define COMM_MSG_CLT_CLT_DEF_H_789454464343213764313245646786448575

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{
	//cs(x committer service) -> order-cps(channel process service)
	class CRegistCsCpsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_REGISTER_CS_CPS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nPeerID, m_nCasPort, m_nMaxBlockID, m_strCasIpAddr, m_strMaxBlockHash);

	public:
		ACE_UINT16 m_nPeerID;
		ACE_UINT16 m_nCasPort; //代理服务侦听的端口号
		ACE_UINT64 m_nMaxBlockID; //peer当前最高区块高度
		CDscString m_strCasIpAddr; //代理服务侦听的IP地址
		CDscString m_strMaxBlockHash; //最高区块的Hash值
	};

	class CQueryMaxBlockInfoCpsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_MAX_BLOCK_INFO_CPS_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nOrderID);

	public:
		ACE_UINT16 m_nOrderID;
	};

	class CQueryMaxBlockInfoCsCpsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_MAX_BLOCK_INFO_CS_CPS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nPeerID, m_nCasPort, m_nMaxBlockID, m_strCasIpAddr, m_strMaxBlockHash);

	public:
		ACE_UINT16 m_nPeerID;
		ACE_UINT16 m_nCasPort; //代理服务侦听的端口号
		ACE_UINT64 m_nMaxBlockID; //peer当前最高区块高度
		CDscString m_strCasIpAddr; //代理服务侦听的IP地址
		CDscString m_strMaxBlockHash; //最高区块的Hash值
	};

	// order-cps -> cs //注册时，order命令cs执行同步
	class CSyncBlockOnRegistCpsCsNotify
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SYNC_BLOCK_ON_REGIST_CPS_CS_NOTIFY
		};

		enum
		{
			EN_HASH_INVALID = 0, //非法
			EN_HASH_NOT_VERIFY //未验证
		};

	public:
		DSC_BIND_ATTR(m_nMaxBlockHashIsValid, m_nSyncSrcPeerID, m_nSyncSrcPort, m_nRegisterBlockID, m_nKafkaBlockID, m_strSyncSrcIpAddr);

	public:
		ACE_UINT8 m_nMaxBlockHashIsValid; //最高区块的hash值是否合法
		ACE_UINT16 m_nSyncSrcPeerID; //可以进行区块同步的PeerID(同步源PeerID)
		ACE_UINT16 m_nSyncSrcPort;
		ACE_UINT64 m_nRegisterBlockID; //peer注册时发送的Block-id
		ACE_UINT64 m_nKafkaBlockID; //order端达成kafka共识的区块高度
		CDscString m_strSyncSrcIpAddr; //可以进行区块同步的地址
	};

	// order-cps -> cs //注册时，order命令cs执行同步
	class CSyncBlockOnRegistCpsCsOnOrderMasterStateNotify
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SYNC_BLOCK_ON_REGIST_CPS_CS_ON_ORDER_MASTER_STATE_NOTIFY
		};

	public:
		DSC_BIND_ATTR(m_nKafkaBlockID, m_strKafkaBlockHash, m_lstPeerAddr);

	public:
		ACE_UINT64 m_nKafkaBlockID; //order端达成kafka共识的区块高度
		CDscString m_strKafkaBlockHash; //达成kafka共识的区块hash
		DSC::CDscList<CSyncSourcePeerCasAddress> m_lstPeerAddr; //peer地址列表
	};

	//cps -> cs //非法peer通知 
	class CInvalidPeerCpsCsNotify
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_INVALID_PEER_CPS_CS_NOTIFY
		};

	public:
		enum
		{
			EN_REGIST_BLOCK_ID_INVALID //注册时上报的区块ID非法
		};
	public:
		DSC_BIND_ATTR(m_nInvalidReason);

	public:
		ACE_INT32 m_nInvalidReason; //通知peer非法的原因
	};

	// order-cps -> cs //order命令cs执行 区块退格
	class CBackspaceBlockCpsCsNotify
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_BACKSPACE_BLOCK_CPS_CS_NOTIFY
		};

	public:
		DSC_BIND_ATTR(m_nRegisterBlockID, m_nKafkaBlockID);

	public:
		ACE_UINT64 m_nRegisterBlockID; //peer注册时发送的Block-id
		ACE_UINT64 m_nKafkaBlockID; //order端达成kafka共识的区块高度
	};

	//order-cps -> peer-cs //master-order发送同步K.V请求到peer
	class CMasterSyncVersionTableCpsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_MASTER_SYNC_VERSION_TABLE_CPS_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nBlockID);

	public:
		ACE_UINT64 m_nBlockID; //order端要同步K.V的区块高度
	};

	//peer-cs -> order-cps //master-order请求K.V列表
	class CMasterSyncVersionTableCsCpsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_MASTER_SYNC_VERSION_TABLE_CS_CPS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nBlockID, m_strBlockHash, m_lstKv);

	public:
		ACE_UINT64 m_nBlockID; //区块ID
		CDscString m_strBlockHash; //当前区块的hash
		DSC::CDscShortList<VBH::CKeyVersion> m_lstKv; //块内包含的K.V列表 //为空表示Peer端不存在该块的K.V列表
	};

	//order-cps -> peer-cs //slave-order发送同步K.V请求到peer
	class CSlaveSyncVersionTableCpsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SLAVE_SYNC_VERSION_TABLE_CPS_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nBlockID);

	public:
		ACE_UINT64 m_nBlockID; //order端请求同步K.V的区块高度
	};

	//peer-cs -> order-cps //slave-order请求K.V列表
	class CSlaveSyncVersionTableCsCpsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_SLAVE_SYNC_VERSION_TABLE_CS_CPS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nBlockID, m_strBlockHash, m_lstKv);

	public:
		ACE_UINT64 m_nBlockID; //区块ID
		CDscString m_strBlockHash; //当前区块的hash
		DSC::CDscShortList<VBH::CKeyVersion> m_lstKv; //块内包含的K.V列表 //为空表示Peer端不存在该块的K.V列表
	};

	//===============================Peer同步区块=消息==================================
	//peer-cs -> order-cps //cs向Order查询需要从哪里同步 请求
	class CQuerySyncSourcePeerCsCpsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_SYNC_SOURCE_PEER_CS_CPS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nPeerID, m_nTargetBlockID);

	public:
		ACE_UINT16 m_nPeerID;
		ACE_UINT64 m_nTargetBlockID; //目标区块高度
	};

	//order-cps -> peer-cs //
	class CQuerySyncSourcePeerCpsCsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_SYNC_SOURCE_PEER_CPS_CS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nKafkaBlockID, m_strKafkaBlockHash, m_lstPeerAddr);

	public:
		ACE_UINT64 m_nKafkaBlockID; //order端达成kafka共识的区块高度
		CDscString m_strKafkaBlockHash; //达成kafka共识的区块hash
		DSC::CDscList<CSyncSourcePeerCasAddress> m_lstPeerAddr; //peer地址列表
	};

	// order-cps -> committer-cs
	class CDistributeBlockCpsCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_DISTRIBUTE_BLOCK_CPS_XCS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nBlockID, m_vbhBlock);

	public:
		ACE_UINT64 m_nBlockID; //区块ID
		DSC::CDscBlob m_vbhBlock; //区块数据
	};

	// committer-cs -> order-cps //committer存储成功才回应答，存储失败不回应答 //order会在超时后重发
	class CDistributeBlockXcsCpsRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_DISTRIBUTE_BLOCK_XCS_CPS_RSP
		};

	public:
		DSC_BIND_ATTR(m_nPeerID, m_nBlockID);

	public:
		ACE_UINT16 m_nPeerID;
		ACE_UINT64 m_nBlockID; //区块ID
	};


}

#endif