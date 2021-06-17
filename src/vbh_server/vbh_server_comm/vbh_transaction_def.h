#ifndef VBH_TRANSACTION_DEF_H_890790SDFSD7F89DS7F89DS7F8976DF
#define VBH_TRANSACTION_DEF_H_890790SDFSD7F89DS7F89DS7F8976DF

#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"
#include "vbh_comm/vbh_comm_id_def.h"

//����ʱ��DSC::CDscShortBlob <==> CDscString
namespace VBH
{
	//�����ڴ�ṹ������
	class VBH_SERVER_COMM_DEF_EXPORT CMemTransaction
	{
	public:
		//�ͷ��ڴ溯��
		virtual void Release(void) = 0;

	public:
		ACE_UINT32 m_nTransSequenceNumber; //�������ˮ��
	};
	using CMemTransactionPtr = CMemTransaction*;


	//ע�������񣬴��ʱʹ��
	class CRegistUserTransaction
	{
	public:
		DSC_BIND_ATTR(m_nUserID, m_cltPubKey, m_svrPriKey, m_envelopeKey);

	public:
		ACE_UINT64 m_nUserID; //Ϊ�û�����Ψһ����ˮ�ţ�����ƾ��ú���У��λ�����û���Ϣ //������ˮ�� + m_nUserID == user-key
		DSC::CDscShortBlob m_cltPubKey;
		DSC::CDscShortBlob m_svrPriKey;
		DSC::CDscShortBlob m_envelopeKey; //ע���������Ҫ��������Կ
	};

	//�ڴ�ע���û����� //���ڽ���ʱʹ�� //��CRegistUserTransaction�ȼۣ���CDscString����DSC::CDscShortBlob
	class CMemRegistUserTransaction final : public CMemTransaction
	{
	public:
		//�ͷź���
		inline virtual void Release(void) override
		{
			DSC_THREAD_TYPE_DELETE(this);
		}

		//��֧�ֽ������
		template <typename DECODER_TYPE>
		ACE_INT32 Decode(DECODER_TYPE& decoder)
		{
			DSC_DECODE_FOREACH(decoder, m_nUserID, m_cltPubKey, m_svrPriKey, m_envelopeKey);
			return 0; 
		}

	public:
		ACE_UINT64 m_nUserID; //Ϊ�û�����Ψһ����ˮ�ţ�����ƾ��ú���У��λ�����û���Ϣ //������ˮ�� + m_nUserID == user-key
		CDscString m_cltPubKey;
		CDscString m_svrPriKey;
		CDscString m_envelopeKey; //ע���������Ҫ��������Կ
	};


	//�᰸�����񣬴��ʱʹ��
	class CProposeTransaction
	{
	public:
		DSC_BIND_ATTR(m_vecWsItem, m_nActionID, m_nUserKeyID, m_signature, m_proposal);

	public:
		VBH::CSimpleVector<VBH::CVbhWriteSetItem> m_vecWsItem; //д���������ڵ�һ��
		ACE_UINT32 m_nActionID = DEF_INVALID_CC_ACTION_ID;
		ACE_UINT64 m_nUserKeyID = DEF_INVALID_WRITE_SET_ID; //ϵͳ����� �û�ID //��������
		DSC::CDscShortBlob m_signature;
		DSC::CDscShortBlob m_proposal;
	};

	//�ڴ��᰸������ //���ڽ���ʱʹ�� //��CProposeTransaction�ȼۣ���CDscString����DSC::CDscShortBlob
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
			ACE_UINT32 m_nVersion; //value�İ汾�� //д���б���İ汾����ʵ����һ�εİ汾�ţ�ʵ�ʴ洢ʱҪ+1�洢
			ACE_UINT64 m_nAllocatedID; //ϵͳ����� �û�ID �� information-ID //key
			CDscString m_value;
		};

	public:
		//�ͷź���
		inline virtual void Release(void) override
		{
			DSC_THREAD_TYPE_DELETE(this);
		}

		//��֧�ֽ���
		template <typename DECODER_TYPE>
		ACE_INT32 Decode(DECODER_TYPE& decoder)
		{
			DSC_DECODE_FOREACH(decoder, m_vecWsItem, m_nActionID, m_nUserKeyID, m_signature, m_proposal);
			return 0;
		}

	public:
		VBH::CSimpleVector<CWs> m_vecWsItem; //д���������ڵ�һ��
		ACE_UINT32 m_nActionID;
		ACE_UINT64 m_nUserKeyID; //ϵͳ����� �û�ID //��������
		CDscString m_signature;
		CDscString m_proposal;
	};

	//transaction����ˮ�ţ����λ������������
#define DEF_TRANSACTION_SEQUENCE_NUMBER 0x0FFFFFFF
	class CTransactionSequenceNumber
	{
	public:
		enum
		{
			EN_TYPE_BITS = 4, //type����ռ�ݵ�bitλ
			EN_REGIST_USER_TRANSACTION_TYPE = 1, //ע���û�����������
			EN_PROPOSE_TRANSACTION_TYPE = 2, //�����᰸����������
		};

	public:
		//�ϳ����յ�SerialNumber
		static ACE_UINT32 CombineSequenceNumber(const ACE_UINT8 nType, const ACE_UINT32 nSerialNumber);

		//��ȡ�ϳ���ˮ���е���������
		static ACE_UINT8 GetTransType(const ACE_UINT32 nSerialNumber);

		//serial-number����
		static void SequenceNumberInc(ACE_UINT32& nSerialNumber);
	};

}

#include "vbh_server_comm/vbh_transaction_def.inl"


#endif
