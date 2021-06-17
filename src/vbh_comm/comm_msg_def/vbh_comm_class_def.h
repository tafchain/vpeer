#ifndef COMM_MSG_DEF_H_423809423798432798479843286468432849894865484684386456
#define COMM_MSG_DEF_H_423809423798432798479843286468432849894865484684386456

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "vbh_comm/vbh_comm_def_export.h"

namespace VBH
{
	//表示在同步过程中peer Cas的地址信息
	class CSyncSourcePeerCasAddress
	{
	public:
		DSC_BIND_ATTR(m_nPeerID, m_nPort, m_strIpAddr);

	public:
		ACE_UINT16 m_nPeerID;
		ACE_UINT16 m_nPort;
		CDscString m_strIpAddr; //可以进行区块同步的地址

	};

	//vbh系统为用户或information分配的key值
	class CVbhAllocatedKey
	{
	public:
		enum
		{
			EN_SIZE = sizeof(ACE_UINT32) + sizeof(ACE_UINT64)
		};
	public:
		DSC_BIND_ATTR(m_nSequenceNumber, m_nAllocatedID);

	public:
		bool operator==(const CVbhAllocatedKey& key) const;

	public:
		ACE_UINT32 m_nSequenceNumber = 0; //校验用流水号
		ACE_UINT64 m_nAllocatedID = 0; //系统分配的 用户ID 或 information-ID
	};

	//表示系统中 version > 0 的 写集item
	//k.v(version).v(value)
	// key = m_nAllocatedID
	// v = m_nVersio
	// v = m_value
	class CVbhWriteSetItem
	{
	public:
		DSC_BIND_ATTR(m_nVersion, m_nAllocatedID, m_value);

	public:
		ACE_UINT32 m_nVersion; //value的版本号 //写集中保存的版本号其实是上一次的版本号，实际存储时要+1存储
		ACE_UINT64 m_nAllocatedID; //系统分配的 用户ID 或 information-ID //key
		DSC::CDscShortBlob m_value;
	};

	//事务的url
	class CTransactionUrl
	{
	public:
		DSC_BIND_ATTR(m_nTransIdx, m_nBlockID);

	public:
		ACE_UINT16 m_nTransIdx; //事务在区块中的序号， 从0开始
		ACE_UINT64 m_nBlockID; //区块ID
	};

	//事务中写集的url
	class CWriteSetUrl
	{
	public:
		enum
		{
			EN_SIZE = sizeof(ACE_UINT16) * 2 + sizeof(ACE_UINT64)
		};

	public:
		DSC_BIND_ATTR(m_nWsIdx, m_nTransIdx, m_nBlockID);

	public:
		ACE_UINT16 m_nWsIdx; //写集在写集列表中序号，从0开始
		ACE_UINT16 m_nTransIdx; //事务在区块中的序号， 从0开始
		ACE_UINT64 m_nBlockID; //区块ID
	};



	//表示系统中 verion>0 的user
	using CVbhUser = CVbhWriteSetItem;
	//表示系统中 version > 0 的information
	using CVbhInformation = CVbhWriteSetItem;

	class CCcEndorsePolicy //CC 的背书策略
	{
	public:
		DSC_BIND_ATTR(m_lstPeerID);

	public:
		DSC::CDscShortList<ACE_INT16> m_lstPeerID;
	};

	//简单vector,提供类似数组的使用体验，一次性开辟所用空间，//可编码 //可自动释放空间
	template <typename TYPE>
	class CSimpleVector
	{
	public:
		~CSimpleVector(); //定义析构函数释放内存

	public: //普通访问函数
		//重置存储空间大小
		void Open(const ACE_UINT16 nItemCount);
		void Close(void);

	public: //普通访问函数
		//获取元素个数
		ACE_UINT16 Size(void) const;

		//重载[]运算符
		TYPE& operator[](const ACE_UINT16 idx);

		//重载赋值运算符
		CSimpleVector<TYPE>& operator= (const CSimpleVector<TYPE>& vec);

	public:
		void GetSize(DSC::CDscCodecGetSizer& rDscCodecGetSizer);

		template<typename ENCODER_TYPE>
		void Encode(ENCODER_TYPE& rDscCodecEncoder);

		template<typename DECODER_TYPE>
		ACE_INT32 Decode(DECODER_TYPE& rDscCodecDecoder);

	private:
		ACE_UINT16 m_nItemCount = 0; //元素个数
		TYPE* m_pItemBuf = NULL; //元素缓冲区
	};

	class CKeyVersion //k.v数据结构
	{
	public:
		DSC_BIND_ATTR(m_nAllocatedID, m_nVersion);

	public:
		ACE_UINT64 m_nAllocatedID; //write-set-id
		ACE_UINT32 m_nVersion; //对应的版本号
	};

	//===============================Transaction相关消息定义==================================

	//vbh系统为transaction生成的key定义 //order生成
	class CVbhAllocatedTransactionKey
	{
	public:
		DSC_BIND_ATTR(m_nTransIdx, m_nSequenceNumber, m_nBlockID);

	public:
		ACE_UINT16 m_nTransIdx; //事务在区块中的序号， 从0开始 //一个区块中事务的个数不超过2^16 //===//真正的用户ID
		ACE_UINT32 m_nSequenceNumber; //打包事务时的流水号 //===//校验用的ID
		ACE_UINT64 m_nBlockID; //区块ID //===//真正的用户ID
	};

	//查询得到的提案事务结构 //查询不支持注册事务的查询
	class CProposeTransactionAtQuery
	{
	public:
		class CWs
		{
		public:
			DSC_BIND_ATTR(m_nVersion, m_key, m_value);

		public:
			ACE_UINT32 m_nVersion; //value的版本号 //写集中保存的版本号其实是上一次的版本号，实际存储时要+1存储
			CVbhAllocatedKey m_key; //系统分配的 用户ID 或 information-ID //key
			DSC::CDscShortBlob m_value;
		};

	public:
		DSC_BIND_ATTR(m_vecWs, m_nActionID, m_userKey, m_signature, m_proposal);

	public:
		VBH::CSimpleVector<CWs> m_vecWs; //写集
		ACE_UINT32 m_nActionID;
		CVbhAllocatedKey m_userKey;  //事务发起者
		DSC::CDscShortBlob m_signature;
		DSC::CDscShortBlob m_proposal;
	};

	//查询得到的提案事务结构(浏览器时) //查询不支持注册事务的查询
	class CProposeTransactionAtExplorer
	{
	public:
		DSC_BIND_ATTR(m_nActionID, m_userKey, m_signature, m_proposal);

	public:
		ACE_UINT32 m_nActionID;
		CVbhAllocatedKey m_userKey;  //事务发起者
		DSC::CDscShortBlob m_signature;
		DSC::CDscShortBlob m_proposal; //提案为空时，表示事务不可浏览； 正常情况下，提案不会为空
	};


}

#include "vbh_comm/comm_msg_def/vbh_comm_class_def.inl"

#endif
