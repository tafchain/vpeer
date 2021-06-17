#include "dsc/dsc_log.h"

#include "vbh_server_comm/vbh_block_codec.h"


VBH::CMemBcBlock::CMemBcBlock(const ACE_UINT64 nBlockID, const ACE_UINT16 nTransCount)
	: m_nTransCount(nTransCount)
	, m_nBlockID(nBlockID)
{
	ACE_ASSERT(nTransCount);

	m_ppTransaction = (CMemTransactionPtr*)DSC_THREAD_SIZE_MALLOC(sizeof(CMemTransactionPtr) * m_nTransCount);
	
	//防止事务初始化到一半，又要销毁block对象的情况
	memset(m_ppTransaction, 0, sizeof(CMemTransactionPtr) * m_nTransCount);
}

VBH::CMemBcBlock::~CMemBcBlock()
{
	if (DSC_LIKELY(m_ppTransaction))
	{
		for (ACE_UINT16 idx = 0; idx < m_nTransCount; ++idx)
		{
			if (DSC_LIKELY(m_ppTransaction[idx])) //保护，防止事务没有初始化就要销毁区块
			{
				m_ppTransaction[idx]->Release();
			}
		}

		DSC_THREAD_SIZE_FREE((char*)m_ppTransaction, sizeof(CMemTransactionPtr) * m_nTransCount);
	}
}

VBH::CMemBcBlock* VBH::vbhDecodeMemBcBloc(char* pPreBlockHash, const char* pBuf, const ACE_UINT32 nDataLen, VBH::CMerkelTree& merkelTree)
{
	//1.1. 初始化，创建解码控制器 //初始化校验用merkel树
	DSC::CDscNetCodecDecoder decoder((char*)pBuf, nDataLen);
	VBH::CBcBlockHeader bcBlockHeader;

	merkelTree.Clear();

	//1.2. 解码区块头 //拷贝区块头中前1块的hash值
	if (bcBlockHeader.Decode(decoder))
	{
		DSC_RUN_LOG_ERROR("decode block header error.");

		return nullptr;
	}
	memcpy(pPreBlockHash, bcBlockHeader.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);

	//1.3. 开辟事务数组，循环解码每个事务 //同时计算merkel树
	ACE_ASSERT(bcBlockHeader.m_nTransCount > 0);

	VBH::CMemBcBlock* pMemBcBlock = DSC_THREAD_TYPE_NEW(VBH::CMemBcBlock) VBH::CMemBcBlock(bcBlockHeader.m_nBlockID, bcBlockHeader.m_nTransCount);//开辟mem-block结构

	pMemBcBlock->m_nBlockTime = bcBlockHeader.m_nBlockTime;
	::memcpy(pMemBcBlock->m_preBlockHash, bcBlockHeader.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);
	::memcpy(pMemBcBlock->m_merkelTreeRootHash, bcBlockHeader.m_merkelTreeRootHash.data(), VBH_BLOCK_DIGEST_LENGTH);

	//1.4 遍历事务,进行解码 //遍历过程中使用的变量
	ACE_UINT32 nTransSequenceNumber; //事务的流水号
	VBH::CMemRegistUserTransaction* pMemRegistTrans; //注册类型事务
	VBH::CMemProposeTransaction* pMemPropTrans; //提案类型事务

	char* pTransStartPtr = decoder.GetCurBufPtr(); //事务的开始位置指针
	char* pTransEndPtr = decoder.GetCurBufPtr(); //事务的结束位置指针

	for (ACE_UINT16 nTransIdx = 0; nTransIdx < bcBlockHeader.m_nTransCount; ++nTransIdx)
	{
		//记录事务开始位置 //上次的结束就是这次的开始
		pTransStartPtr = pTransEndPtr;

		//解码事务序列号
		if (decoder.Decode(nTransSequenceNumber))
		{
			DSC_THREAD_TYPE_DELETE(pMemBcBlock);
			DSC_RUN_LOG_ERROR("decode sequence number failed, blcok-id:%lld, transaction-idx:%d.", bcBlockHeader.m_nBlockID, nTransIdx);

			return nullptr;
		}

		//判断事务类型，不同事务类型做不同的解析处理
		switch (VBH::CTransactionSequenceNumber::GetTransType(nTransSequenceNumber))
		{
		case VBH::CTransactionSequenceNumber::EN_PROPOSE_TRANSACTION_TYPE: //解码提案类型事务
		{
			//解码，对照VBH::CProposeTransaction 结构
			pMemPropTrans = DSC_THREAD_TYPE_NEW(VBH::CMemProposeTransaction) VBH::CMemProposeTransaction;
			pMemBcBlock->m_ppTransaction[nTransIdx] = pMemPropTrans;
			pMemPropTrans->m_nTransSequenceNumber = nTransSequenceNumber;

			if (pMemPropTrans->Decode(decoder)) //解码整个提案事务
			{
				DSC_RUN_LOG_ERROR("decode CProposeTransaction failed, blcok-id:%lld, transaction-idx:%d.", bcBlockHeader.m_nBlockID, nTransIdx);
				DSC_THREAD_TYPE_DELETE(pMemBcBlock);

				return nullptr;
			}
		}
		break;
		case VBH::CTransactionSequenceNumber::EN_REGIST_USER_TRANSACTION_TYPE: //解码注册类型事务
		{
			//解码 对照 VBH::CRegistUserTransaction 结构
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

		//记录事务结束位置
		pTransEndPtr = decoder.GetCurBufPtr();

		//加入merkel树节点
		merkelTree.AddLeafNode(pTransStartPtr, (ACE_UINT32)(pTransEndPtr - pTransStartPtr));
	}

	//1.5. 校验merkel树是否正确
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
