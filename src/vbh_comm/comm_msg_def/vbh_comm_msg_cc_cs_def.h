#ifndef COMM_MSG_CC_CS_DEF_H_455455645646465465478987979765465
#define COMM_MSG_CC_CS_DEF_H_455455645646465465478987979765465
#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_id_def.h"
#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{
	class CQueryWriteSetListQueryCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_LIST_QUERY_CC_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID, m_vecKey);

	public:
		ACE_UINT64 m_nCcSessionID; //合成的cc-id
		VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecKey;
	};

	class CQueryWriteSetListQueryCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_LIST_QUERY_CS_CC_RSP
		};

	public:
		class CItem
		{
		public:
			DSC_BIND_ATTR(m_nVersion, m_value);

		public:
			ACE_UINT32 m_nVersion; //user 数据的版本号
			DSC::CDscShortBlob m_value;
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_vecValue);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT64 m_nCcSessionID; //合成的cc-id
		VBH::CSimpleVector<CItem> m_vecValue;
	};

	//查询交易信息
	class CQueryTransactionQueryCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANSACTION_QUERY_CC_CS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID, m_transKey);

	public:
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedTransactionKey m_transKey;
	};

	class CQueryTransactionQueryCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANSACTION_QUERY_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_transKey, m_transContent);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedTransactionKey m_transKey;
		VBH::CProposeTransactionAtQuery m_transContent;
	};

	class CQueryInformationHistoryCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_INFORMATION_HISTORY_QUERY_CC_CS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nPageSize, m_nPageIndex, m_nCcSessionID, m_InfoKey);

	public:
		ACE_UINT16 m_nPageSize;
		ACE_UINT16 m_nPageIndex; //在事务比较多时，按页查询：页大小，页序号
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedKey m_InfoKey;
	};

	class CQueryInformationHistoryCsCcRsp
	{
	public:
		class CTrans
		{
		public:
			DSC_BIND_ATTR(m_key, m_content);

		public:
			VBH::CVbhAllocatedTransactionKey m_key; //事务ID
			VBH::CProposeTransactionAtExplorer m_content;
		};

	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_INFORMATION_HISTORY__QUERY_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_InfoKey, m_TransList);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedKey m_InfoKey;
		VBH::CSimpleVector<CTrans> m_TransList;
	};

	//===============================浏览器查询流程=消息==================================

	//浏览器专用：查询区块头信息
	class CQueryBlockHeaderInfoExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_BLOCK_HEADER_INFO_EXPLORER_CC_CS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nBlockID, m_nCcSessionID);

	public:
		ACE_UINT64 m_nBlockID;
		ACE_UINT32 m_nCcSessionID;
	};

	class CQueryBlockHeaderInfoExplorerCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_BLOCK_HEADER_INFO_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nTransCount, m_nReturnCode, m_nCcSessionID, m_nBlockID, m_nBlockTime, m_preBlockHash, m_merkelTreeRootHash);

	public:
		ACE_UINT16 m_nTransCount;
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockID;
		ACE_UINT64 m_nBlockTime; //区块生成时间
		DSC::CDscShortBlob m_preBlockHash; //前一区块的hash值
		DSC::CDscShortBlob m_merkelTreeRootHash; //本区块merkel树根的hash值
	};


	//浏览器专用：查询区块数量
	class CQueryBlockCountExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_BLOCK_COUNT_EXPLORER_CC_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID);

	public:
		ACE_UINT32 m_nCcSessionID;
	};

	class CQueryBlockCountExplorerCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_BLOCK_COUNT_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_nBlockCount);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockCount;
	};

	//查询写集信息
	class CQueryWriteSetExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_EXPLORER_CC_XCS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID, m_alocKey);

	public:
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedKey m_alocKey;
	};

	class CQueryWriteSetExplorerCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_userInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		DSC::CDscShortBlob m_userInfo;
	};

	//查询交易信息
	class CQueryTransInfoExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_INFO_EXPLORER_CC_CS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID, m_transKey);

	public:
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedTransactionKey m_transKey;
	};

	class CQueryTransInfoExplorerCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_INFO_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_transKey, m_transInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		VBH::CVbhAllocatedTransactionKey m_transKey;
		VBH::CProposeTransactionAtQuery m_transInfo;
	};

	class CQueryTransListExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_LIST_EXPLORER_CC_CS_REQ,
		};

	public:
		DSC_BIND_ATTR(m_nPageSize, m_nPageIndex, m_nCcSessionID, m_nBlockID);

	public:
		ACE_UINT16 m_nPageSize;
		ACE_UINT16 m_nPageIndex; //在事务比较多时，按页查询：页大小，页序号
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockID;
	};

	class CQueryTransListExplorerCsCcRsp
	{
	public:
		class CTrans
		{
		public:
			DSC_BIND_ATTR(m_key, m_content);

		public:
			VBH::CVbhAllocatedTransactionKey m_key; //事务ID
			VBH::CProposeTransactionAtExplorer m_content;
		};

	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_LIST_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_nBlockID, m_nBlockTime, m_blockHash, m_preBlockHash, m_vecTrans);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockID;
		ACE_UINT64 m_nBlockTime; //区块生成时间
		DSC::CDscShortBlob m_blockHash; //区块的hash值
		DSC::CDscShortBlob m_preBlockHash; //前一个区块的hash值
		VBH::CSimpleVector<CTrans> m_vecTrans;
	};

	//查询交易数量
	class CQueryTransCountExplorerCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_COUNT_EXPLORER_CC_CS_REQ
		};

	public:
		DSC_BIND_ATTR(m_nCcSessionID, m_nBlockID);

	public:
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockID;
	};

	class CQueryTransCountExplorerCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_TRANS_COUNT_EXPLORER_CS_CC_RSP
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_nBlockID, m_nTransCount);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID;
		ACE_UINT64 m_nBlockID;
		ACE_UINT64 m_nTransCount;
	};

	//cc -> committer service,  //query-user-info和query-information 统一的父类
	class CQueryWriteSetListProposeCcCsReq
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_LIST_PROPOSE_CC_CS_REQ
		};

	public:
		class CGroup //1组
		{
		public:
			DSC_BIND_ATTR(m_vecKey);

		public:
			VBH::CSimpleVector<VBH::CVbhAllocatedKey> m_vecKey; //注册成功时带回的，系统为该注册用户分配的user-key
		};

	public:
		DSC_BIND_ATTR(m_vecGroupWsKey, m_nCcSessionID);

	public:
		VBH::CSimpleVector<CGroup> m_vecGroupWsKey; //要查询的user-key列表 //一批包含多组，1组可以包含1个或多个
		ACE_UINT32 m_nCcSessionID; //这个CC-SessionID并非真正的ID，而是一个合成的句柄，返回到CC时，要处理后才能使用
	};

	//committer service -> cc
	class CQueryWriteSetListProposeCsCcRsp
	{
	public:
		enum
		{
			EN_MSG_ID = VBH::EN_QUERY_WRITE_SET_LIST_PROPOSE_CS_CC_RSP
		};

	public:
		class CItem
		{
		public:
			DSC_BIND_ATTR(m_nVersion, m_value);

		public:
			ACE_UINT32 m_nVersion; //user 数据的版本号
			DSC::CDscShortBlob m_value;
		};

		class CGroup
		{
		public:
			DSC_BIND_ATTR(m_vecValue);

		public:
			VBH::CSimpleVector<CItem> m_vecValue;
		};

	public:
		DSC_BIND_ATTR(m_nReturnCode, m_nCcSessionID, m_vecGroupWsInfo);

	public:
		ACE_INT32 m_nReturnCode;
		ACE_UINT32 m_nCcSessionID; //这个CC-SessionID并非真正的ID，而是一个合成的句柄，返回到CC时，要处理后才能使用
		VBH::CSimpleVector<CGroup> m_vecGroupWsInfo; //查询到的用户信息列表
	};

}
#endif
