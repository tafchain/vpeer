#include "dsc/dsc_log.h"

#include "vbh_server_comm/vbh_block_codec.h"


VBH::CMemBcBlock::CMemBcBlock(const ACE_UINT64 nBlockID, const ACE_UINT16 nTransCount)
	: m_nTransCount(nTransCount)
	, m_nBlockID(nBlockID)
{
	ACE_ASSERT(nTransCount);

	m_ppTransaction = (CMemTransactionPtr*)DSC_THREAD_SIZE_MALLOC(sizeof(CMemTransactionPtr) * m_nTransCount);
	
	//��ֹ�����ʼ����һ�룬��Ҫ����block��������
	memset(m_ppTransaction, 0, sizeof(CMemTransactionPtr) * m_nTransCount);
}

VBH::CMemBcBlock::~CMemBcBlock()
{
	if (DSC_LIKELY(m_ppTransaction))
	{
		for (ACE_UINT16 idx = 0; idx < m_nTransCount; ++idx)
		{
			if (DSC_LIKELY(m_ppTransaction[idx])) //��������ֹ����û�г�ʼ����Ҫ��������
			{
				m_ppTransaction[idx]->Release();
			}
		}

		DSC_THREAD_SIZE_FREE((char*)m_ppTransaction, sizeof(CMemTransactionPtr) * m_nTransCount);
	}
}

VBH::CMemBcBlock* VBH::vbhDecodeMemBcBloc(char* pPreBlockHash, const char* pBuf, const ACE_UINT32 nDataLen, VBH::CMerkelTree& merkelTree)
{
	//1.1. ��ʼ����������������� //��ʼ��У����merkel��
	DSC::CDscNetCodecDecoder decoder((char*)pBuf, nDataLen);
	VBH::CBcBlockHeader bcBlockHeader;

	merkelTree.Clear();

	//1.2. ��������ͷ //��������ͷ��ǰ1���hashֵ
	if (bcBlockHeader.Decode(decoder))
	{
		DSC_RUN_LOG_ERROR("decode block header error.");

		return nullptr;
	}
	memcpy(pPreBlockHash, bcBlockHeader.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);

	//1.3. �����������飬ѭ������ÿ������ //ͬʱ����merkel��
	ACE_ASSERT(bcBlockHeader.m_nTransCount > 0);

	VBH::CMemBcBlock* pMemBcBlock = DSC_THREAD_TYPE_NEW(VBH::CMemBcBlock) VBH::CMemBcBlock(bcBlockHeader.m_nBlockID, bcBlockHeader.m_nTransCount);//����mem-block�ṹ

	pMemBcBlock->m_nBlockTime = bcBlockHeader.m_nBlockTime;
	::memcpy(pMemBcBlock->m_preBlockHash, bcBlockHeader.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);
	::memcpy(pMemBcBlock->m_merkelTreeRootHash, bcBlockHeader.m_merkelTreeRootHash.data(), VBH_BLOCK_DIGEST_LENGTH);

	//1.4 ��������,���н��� //����������ʹ�õı���
	ACE_UINT32 nTransSequenceNumber; //�������ˮ��
	VBH::CMemRegistUserTransaction* pMemRegistTrans; //ע����������
	VBH::CMemProposeTransaction* pMemPropTrans; //�᰸��������

	char* pTransStartPtr = decoder.GetCurBufPtr(); //����Ŀ�ʼλ��ָ��
	char* pTransEndPtr = decoder.GetCurBufPtr(); //����Ľ���λ��ָ��

	for (ACE_UINT16 nTransIdx = 0; nTransIdx < bcBlockHeader.m_nTransCount; ++nTransIdx)
	{
		//��¼����ʼλ�� //�ϴεĽ���������εĿ�ʼ
		pTransStartPtr = pTransEndPtr;

		//�����������к�
		if (decoder.Decode(nTransSequenceNumber))
		{
			DSC_THREAD_TYPE_DELETE(pMemBcBlock);
			DSC_RUN_LOG_ERROR("decode sequence number failed, blcok-id:%lld, transaction-idx:%d.", bcBlockHeader.m_nBlockID, nTransIdx);

			return nullptr;
		}

		//�ж��������ͣ���ͬ������������ͬ�Ľ�������
		switch (VBH::CTransactionSequenceNumber::GetTransType(nTransSequenceNumber))
		{
		case VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE: //�����᰸��������
		{
			//���룬����VBH::CProposeTransaction �ṹ
			pMemPropTrans = DSC_THREAD_TYPE_NEW(VBH::CMemProposeTransaction) VBH::CMemProposeTransaction;
			pMemBcBlock->m_ppTransaction[nTransIdx] = pMemPropTrans;
			pMemPropTrans->m_nTransSequenceNumber = nTransSequenceNumber;

			if (pMemPropTrans->Decode(decoder)) //���������᰸����
			{
				DSC_RUN_LOG_ERROR("decode CProposeTransaction failed, blcok-id:%lld, transaction-idx:%d.", bcBlockHeader.m_nBlockID, nTransIdx);
				DSC_THREAD_TYPE_DELETE(pMemBcBlock);

				return nullptr;
			}
		}
		break;
		case VBH::CTransactionSequenceNumber::EN_REGIST_USER_TRANSACTION_TYPE: //����ע����������
		{
			//���� ���� VBH::CRegistUserTransaction �ṹ
			pMemRegistTrans = DSC_THREAD_TYPE_NEW(VBH::CMemRegistUserTransaction) VBH::CMemRegistUserTransaction;
			pMemBcBlock->m_ppTransaction[nTransIdx] = pMemRegistTrans;
			pMemRegistTrans->m_nTransSequenceNumber = nTransSequenceNumber;

			if (pMemRegistTrans->Decode(decoder)) //
			{
				DSC_RUN_LOG_ERROR("decode CRegistUserTransaction failed, blcok-id:%lld, transaction-idx:%d.", bcBlockHeader.m_nBlockID, nTransIdx);
				DSC_THREAD_TYPE_DELETE(pMemBcBlock);

				return nullptr;
			}
		}
		break;
		default:
		{
			DSC_RUN_LOG_ERROR("unknown transaction-type:%d, blcok-id:%lld, transaction-idx:%d.",
				VBH::CTransactionSequenceNumber::GetTransType(nTransSequenceNumber), bcBlockHeader.m_nBlockID, nTransIdx);
			DSC_THREAD_TYPE_DELETE(pMemBcBlock);

			return nullptr;
		}
		}

		//��¼�������λ��
		pTransEndPtr = decoder.GetCurBufPtr();

		//����merkel���ڵ�
		merkelTree.AddLeafNode(pTransStartPtr, (ACE_UINT32)(pTransEndPtr - pTransStartPtr));
	}

	//1.5. У��merkel���Ƿ���ȷ
	VBH::CHashBuffer merkelRoot;

	merkelTree.BuildMerkleTree();
	merkelTree.GetRoot(merkelRoot);
	if (DSC_UNLIKELY(!VBH::IsEqual(merkelRoot, bcBlockHeader.m_merkelTreeRootHash)))
	{
		DSC_THREAD_TYPE_DELETE(pMemBcBlock);
		DSC_RUN_LOG_ERROR("check merkel-tree failed, blcok-id:%lld", bcBlockHeader.m_nBlockID);

		return nullptr;
	}

	return pMemBcBlock;
}
