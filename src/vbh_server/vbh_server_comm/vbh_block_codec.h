#ifndef VBH_BLOCK_CODEC_H_B087D2B2BF0611E9B4F760F18A3A20D1
#define VBH_BLOCK_CODEC_H_B087D2B2BF0611E9B4F760F18A3A20D1


#include "vbh_server_comm/merkel_tree/merkel_tree.h"
#include "vbh_server_comm/vbh_transaction_def.h"

/* *********************************************************************
 * ��������(ע��/�����᰸) �ϲ��� ������ˮ���У������ CTransactionSequenceNumber
 * ********************************************************************* */

namespace VBH
{
	//����ͷ�Ľṹ���� //����ͷ�����Ƕ���(���ܰ���blob�ȱ߳�����)
	class CBcBlockHeader
	{
	public:
		enum
		{
			EN_SIZE = sizeof(ACE_UINT16)*2 + sizeof(ACE_UINT64)*2 + VBH_BLOCK_DIGEST_LENGTH*2,
			EN_MAX_TRANSACTION_COUNT_IN_BLOCK = ((1 << 16) - 1), //1��������������ͬʱ������
		};

	public:
		DSC_BIND_ATTR(m_nOrderID, m_nTransCount, m_nBlockID, m_nBlockTime, m_preBlockHash, m_merkelTreeRootHash);

	public:
		ACE_UINT16 m_nOrderID; //�������ʱ��orderID��У��ʱҲҪ��
		ACE_UINT16 m_nTransCount; //����������е�������� //��������಻��
		ACE_UINT64 m_nBlockID; //����ID����1��ʼ����
		ACE_UINT64 m_nBlockTime; //��������ʱ��
		CHashBuffer m_preBlockHash; //ǰһ�����hashֵ
		CHashBuffer m_merkelTreeRootHash; //������merkel������hashֵ
	};

	//================================================================================
	//�������������� //ʹ�÷�ʽ����CPS�ж���һ������Ķ���ѭ��ʹ�ã����ڱ���
	class VBH_SERVER_COMM_DEF_EXPORT CBlockEncoder
	{
	public:
		//��ʼ���ô�ű�������ݵ�buffer
		void InitSetEncodeBuffer(char* pBuf);

		//�������ô����ݵı��뻺������������������ nDataLen������
		void ResetEncodeBuffer(char* pBuf, size_t nDataLen);

		//��ʼ�������񣬷��ر���״̬���� ��¼��ʼ�����ָ��
		DSC::CDscNetCodecEncoder& BeginEncodeTransaction(void);

		//�����������񣬽��ӿ�ʼ������������������ݶ���ӵ�merkel����
		void EndEncodeTransaction(void);

		//��ȡ��ǰ�Ѿ�������������ݴ�С,��������ͷ�ĳ���
		size_t GetEncodeDataSize(void);

		//��������ͷ,��д�뻺���� //��������ͷǰ��������merkel����֮��������ֶ�
		ACE_INT32 EncodeBlockHeader(void);

	public:
		CBcBlockHeader m_bcBlockHeader; //����ͷ

	protected:
		char* m_pTransBeginPtr = nullptr; //����ʼ��ָ�룬�ڳ�������������ݽ���������ȷ������Ŀ�ʼλ��
		DSC::CDscNetCodecEncoder m_encodeState; //������ͷ֮��� ����״̬
		VBH::CMerkelTree m_merkelTree; //У����merkel�� //����
	};

	//================================================================================

	//����������ڴ��е�BcBlock
	class VBH_SERVER_COMM_DEF_EXPORT CMemBcBlock
	{
	public:
		CMemBcBlock(const ACE_UINT64 nBlockID, const ACE_UINT16 nTransCount);
		~CMemBcBlock();

	public:
		const ACE_UINT16 m_nTransCount; //����������е�������� //��������಻��
		const ACE_UINT64 m_nBlockID; //����ID
		ACE_UINT64 m_nBlockTime; //��������ʱ��
		CMemTransactionPtr* m_ppTransaction = nullptr; //��������(�б�) //�����б�ĳ���������ͷ��
		char m_preBlockHash[VBH_BLOCK_DIGEST_LENGTH]; //ǰһ�����hashֵ
		char m_merkelTreeRootHash[VBH_BLOCK_DIGEST_LENGTH]; //������merkel������hashֵ
	};

	/** �������飬��������䵽 CMemBcBlock �� //�������ݷֳ������֣���������|ʧ�������б�
	 * @ pPreBlockHash: �����д洢��ǰһ�����hashֵ //�������
	 * @ nBlockDataLen: �������ݳ��� //�������
	 * @ pBuf: ���������ݻ�����
	 * @ nTotalBufLen: �����������ܳ���
	 * @ merkelTree: ����У���merkel���ṹ�����ǵ��ڴ�����ԭ��merkel���ṹ���ⲿ�ṩ����������ʱ����
	 *
	 * */
	VBH_SERVER_COMM_DEF_EXPORT VBH::CMemBcBlock* vbhDecodeMemBcBloc(char* pPreBlockHash, const char* pBuf, const ACE_UINT32 nDataLen, VBH::CMerkelTree& merkelTree);
}

#include "vbh_server_comm/vbh_block_codec.inl"

#endif
