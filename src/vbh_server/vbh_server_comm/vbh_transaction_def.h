#ifndef VBH_TRANSACTION_DEF_H_890790SDFSD7F89DS7F89DS7F8976DF
#define VBH_TRANSACTION_DEF_H_890790SDFSD7F89DS7F89DS7F8976DF

#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"
#include "vbh_comm/vbh_comm_id_def.h"

//解码时，DSC::CDscShortBlob <==> CDscString
namespace VBH
{
	//事务内存结构，父类
	class VBH_SERVER_COMM_DEF_EXPORT CMemTransaction
	{
	public:
		//释放内存函数
		virtual void Release(void) = 0;

	public:
		ACE_UINT32 m_nTransSequenceNumber; //事务的流水号
	};
	using CMemTransactionPtr = CMemTransaction*;


	//注册型事务，打包时使用
	class CRegistUserTransaction
	{
	public:
		DSC_BIND_ATTR(m_nUserID, m_cltPubKey, m_svrPriKey, m_envelopeKey);

	public:
		ACE_UINT64 m_nUserID; //为用户分配唯一的流水号，后续凭借该号与校验位访问用户信息 //事务流水号 + m_nUserID == user-key
		DSC::CDscShortBlob m_cltPubKey;
		DSC::CDscShortBlob m_svrPriKey;
		DSC::CDscShortBlob m_envelopeKey; //注册后服务端需要保留的秘钥
	};

	//内存注册用户事务 //仅在解码时使用 //和CRegistUserTransaction等价，用CDscString代替DSC::CDscShortBlob
	class CMemRegistUserTransaction final : public CMemTransaction
	{
	public:
		//释放函数
		inline virtual void Release(void) override
		{
			DSC_THREAD_TYPE_DELETE(this);
		}

		//仅支持解码操作
		template <typename DECODER_TYPE>
		ACE_INT32 Decode(DECODER_TYPE& decoder)
		{
			DSC_DECODE_FOREACH(decoder, m_nUserID, m_cltPubKey, m_svrPriKey, m_envelopeKey);
			return 0; 
		}

	public:
		ACE_UINT64 m_nUserID; //为用户分配唯一的流水号，后续凭借该号与校验位访问用户信息 //事务流水号 + m_nUserID == user-key
		CDscString m_cltPubKey;
		CDscString m_svrPriKey;
		CDscString m_envelopeKey; //注册后服务端需要保留的秘钥
	};


	//提案型事务，打包时使用
	class CProposeTransaction
	{
	public:
		DSC_BIND_ATTR(m_vecWsItem, m_nActionID, m_nUserKeyID, m_signature, m_proposal);

	public:
		VBH::CSimpleVector<VBH::CVbhWriteSetItem> m_vecWsItem; //写集必须拍在第一个
		ACE_UINT32 m_nActionID = DEF_INVALID_CC_ACTION_ID;
		ACE_UINT64 m_nUserKeyID = DEF_INVALID_WRITE_SET_ID; //系统分配的 用户ID //事务发起者
		DSC::CDscShortBlob m_signature;
		DSC::CDscShortBlob m_proposal;
	};

	//内存提案类事务 //仅在解码时使用 //和CProposeTransaction等价，用CDscString代替DSC::CDscShortBlob
	class CMemProposeTransaction final : public CMemTransaction
	{
	public:
		class CWs
		{
		public:
			template <typename DECODER_TYPE>
			ACE_INT32 Decode(DECODER_TYPE& decoder)
			{
				DSC_DECODE_FOREACH(decoder, m_nVersion, m_nAllocatedID, m_value);
				return 0;
			}

		public:
			ACE_UINT32 m_nVersion; //value的版本号 //写集中保存的版本号其实是上一次的版本号，实际存储时要+1存储
			ACE_UINT64 m_nAllocatedID; //系统分配的 用户ID 或 information-ID //key
			CDscString m_value;
		};

	public:
		//释放函数
		inline virtual void Release(void) override
		{
			DSC_THREAD_TYPE_DELETE(this);
		}

		//仅支持解码
		template <typename DECODER_TYPE>
		ACE_INT32 Decode(DECODER_TYPE& decoder)
		{
			DSC_DECODE_FOREACH(decoder, m_vecWsItem, m_nActionID, m_nUserKeyID, m_signature, m_proposal);
			return 0;
		}

	public:
		VBH::CSimpleVector<CWs> m_vecWsItem; //写集必须拍在第一个
		ACE_UINT32 m_nActionID;
		ACE_UINT64 m_nUserKeyID; //系统分配的 用户ID //事务发起者
		CDscString m_signature;
		CDscString m_proposal;
	};

	//transaction的流水号，其高位含有事务类型
#define DEF_TRANSACTION_SEQUENCE_NUMBER 0x0FFFFFFF
	class CTransactionSequenceNumber
	{
	public:
		enum
		{
			EN_TYPE_BITS = 4, //type类型占据的bit位
			EN_REGIST_USER_TRANSACTION_TYPE = 1, //注册用户的事务类型
			EN_PROPOSE_TRANSACTION_TYPE = 2, //发起提案的事务类型
		};

	public:
		//合成最终的SerialNumber
		static ACE_UINT32 CombineSequenceNumber(const ACE_UINT8 nType, const ACE_UINT32 nSerialNumber);

		//获取合成流水号中的事务类型
		static ACE_UINT8 GetTransType(const ACE_UINT32 nSerialNumber);

		//serial-number自增
		static void SequenceNumberInc(ACE_UINT32& nSerialNumber);
	};

}

#include "vbh_server_comm/vbh_transaction_def.inl"


#endif
