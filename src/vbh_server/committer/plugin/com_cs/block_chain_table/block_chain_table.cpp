#include "ace/OS_NS_fcntl.h"
#include "ace/OS_NS_sys_stat.h"

#include "dsc/mem_mng/dsc_allocator.h"
#include "dsc/dsc_log.h"

#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_error_code.h"
#include "vbh_comm/vbh_comm_macro_def.h"
#include "vbh_comm/vbh_encrypt_lib.h"
#include "vbh_server_comm/vbh_const_length_storage/vbh_cls_config_param.h"
#include "vbh_server_comm/vbh_const_length_storage/vbfs/vbfs_config_select_def.h"

#include "com_cs/block_chain_table/block_chain_table.h"

ACE_INT32 CBlockChainTable::Open(VBFS::CVbfs* pVbfs, const ACE_UINT32 nChannelID)
{
	m_pVbfs = pVbfs;
	m_nChannelID = nChannelID;

	//1. ��ȡ�ļ�size
	CDscDatabase database;
	CDBConnection dbConnection;

	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");

		return -1;
	}
	else
	{
		CTableWrapper< CVbfsConfigOnlyFileSize > vbfsCfg("VBFS_CONFIG");
		CVbfsCriterion criterion(nChannelID);

		if (::PerSelect(vbfsCfg, database, dbConnection, &criterion))
		{
			DSC_RUN_LOG_ERROR("select from VBFS_CONFIG failed.");

			return -1;
		}
		else
		{
			m_nFileSize = *vbfsCfg->m_fileSize;
			m_nFileSize <<= 30; //��ȡ��������GBΪ��λ
		}
	}

	//2. �������ļ�
	CDscString strBasePath(CDscAppManager::Instance()->GetWorkRoot());

	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += DEF_STORAGE_DIR_NAME;
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += "channel_";
	strBasePath += nChannelID;
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;
	strBasePath += VBH_CLS::GetClsTableName(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE);
	strBasePath += DSC_FILE_PATH_SPLIT_CHAR;

	CDscString strCfgFilePath(strBasePath);
	CDscString strLogFilePath(strBasePath);
	ACE_stat stat;

	strCfgFilePath += DEF_CLS_CONFIG_FILE_NAME;
	strLogFilePath += DEF_CLS_LOG_FILE_NAME;

	if (-1 == ACE_OS::stat(strBasePath.c_str(), &stat))
	{
		if (DSC::DscRecurMkdir(strBasePath.c_str(), strBasePath.size()))
		{
			DSC_RUN_LOG_ERROR("make-dir:%s failed.", strBasePath.c_str());
			return -1;
		}

		CBcTableConfig cfg;
		CBcTableLog log;

		//���������ļ�
		if (VBH::CreateCfgFile(strCfgFilePath, cfg))
		{
			return -1;
		}

		//������־�ļ�
		if (VBH::CreateCfgFile(strLogFilePath, log))
		{
			return -1;
		}
	}

	//��������
	if (VBH::LoadMmapCfgFile(strCfgFilePath, m_shmCfg, m_pCfg))
	{
		return -1;
	}
	m_memCfg = *m_pCfg;

	//������־
	if (VBH::LoadMmapCfgFile(strLogFilePath, m_shmLog, m_pLog))
	{
		return -1;
	}

	//3. ��index table
	if (m_bcIndexTable.Open(pVbfs, nChannelID, VBH_CLS::EN_BLOCK_INDEX_TABLE_TYPE))
	{
		DSC_RUN_LOG_ERROR("open block-index table failed, channel:%u.", nChannelID);

		return -1;
	}

	return 0;
}

void CBlockChainTable::Close(void)
{
	m_bcIndexTable.Close();

	m_shmCfg.close();
	m_shmLog.close();

	if (m_pAlignBlockDataBuf)
	{
		DSC_FREE(m_pAlignBlockDataBuf);
		m_pAlignBlockDataBuf = nullptr;
		m_nAlignBlockDataBufLen = 0;
	}
}


ACE_INT32 CBlockChainTable::ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID)
{
	if (nBlockID < m_pCfg->m_nLastBlockID)
	{
		VBH_CLS::CBcIndexTableItem indexTableItem;

		if (DSC_UNLIKELY(m_bcIndexTable.Read(indexTableItem, nBlockID + 1)))
		{
			DSC_RUN_LOG_ERROR("read block index %llu failed", nBlockID + 1);
			return -1;
		}
		else
		{
			memcpy(pBlockHash, indexTableItem.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH);
		}
	}
	else if (nBlockID == m_pCfg->m_nLastBlockID)
	{
		ACE_UINT32 nBlockDataLen;
		char* pBlockData = ReadBlock(nBlockDataLen, nBlockID);

		if (nullptr == pBlockData)
		{
			DSC_RUN_LOG_ERROR("read block failed block-id :lld%", nBlockID);

			return -1;
		}
		else
		{
			VBH::vbhDigest(pBlockData, nBlockDataLen, pBlockHash);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("read-block-hash, error block-id:%llu, last-block-id:%llu", nBlockID, m_pCfg->m_nLastBlockID);
		return -1;
	}

	return 0;
}

ACE_INT32 CBlockChainTable::AppendBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& rBlockData, const VBH::CHashBuffer& rPreBlockHash)
{
	//1. ���������ݶ��룬�Է�������direct-ioд��
	ACE_UINT32 nTotalDataBufLen = rBlockData.GetSize();
	
	nTotalDataBufLen = DEF_GET_MEM_ALIGN_SIZE(nTotalDataBufLen); //���ݳ���ҲҪ����
	
	if (m_nAlignBlockDataBufLen < nTotalDataBufLen) //���������������ȿ��ٻ�����
	{
		if (m_pAlignBlockDataBuf)
		{
			DSC_FREE(m_pAlignBlockDataBuf);
		}
		
		m_nAlignBlockDataBufLen = nTotalDataBufLen; //�����������������¿��ٴ���
		DSC_MEM_ALIGN(m_pAlignBlockDataBuf, m_nAlignBlockDataBufLen);

		if (!m_pAlignBlockDataBuf)
		{
			DSC_RUN_LOG_ERROR("DSC_MEM_ALIGN failed, memory size:%lld.", m_nAlignBlockDataBufLen);
			m_nAlignBlockDataBufLen = 0;

			return -1;
		}
	}

	memcpy(m_pAlignBlockDataBuf, rBlockData.GetBuffer(), rBlockData.GetSize());

	//2. ȷ����ǰ�ļ��Ƿ�д��
	if (m_memCfg.m_nCurOffset + nTotalDataBufLen > m_nFileSize) //��ǰ�ļ��Ѿ�����д��Ҫ�����µ��ļ�
	{
		if (m_pVbfs->AllocatFile(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE, m_pCfg->m_nCurFileID+1))
		{
			DSC_RUN_LOG_ERROR("allocate block file-id failed, block-id:%llu, channel-id:%u", nBlockID, m_nChannelID);

			return -1;
		}
		m_pCfg->m_nCurOffset = 0; //allocat�����Գ����������ڴ�ӳ���ļ��ļ�¼ҲҪһ���
		m_pCfg->m_nCurFileID++;
		m_memCfg.m_nCurFileID = m_pCfg->m_nCurFileID;
		m_memCfg.m_nCurOffset = 0;
	}

	if (m_pVbfs->Write(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE, m_memCfg.m_nCurFileID, m_pAlignBlockDataBuf, nTotalDataBufLen, m_memCfg.m_nCurOffset))
	{
		DSC_RUN_LOG_ERROR("write block into file failed, block-id:%lld.", nBlockID);
		
		return -1;
	}

	//3. д��index
	VBH_CLS::CBcIndexTableItem index;

	index.m_nBlockDataLength = rBlockData.GetSize();
	index.m_nTotalDataLength = nTotalDataBufLen;
	index.m_nFileID = m_memCfg.m_nCurFileID;
	index.m_nOffset = m_memCfg.m_nCurOffset;
	memcpy(index.m_preBlockHash.data(), rPreBlockHash.c_array(), VBH_BLOCK_DIGEST_LENGTH);

	if (m_bcIndexTable.Append(nBlockID, index))
	{
		DSC_RUN_LOG_ERROR("write block index failed.");

		return -1;
	}
	
	m_memCfg.m_nCurOffset += nTotalDataBufLen; //��ת�����ĳ���, ���鰴����洢
	m_memCfg.m_nLastBlockID = nBlockID;

	return 0;
}

ACE_INT32 CBlockChainTable::PopBack(const ACE_UINT64 nBlockID)
{
	ACE_ASSERT(nBlockID == m_memCfg.m_nLastBlockID);

	VBH_CLS::CBcIndexTableItem indexTableItem;

	if (m_bcIndexTable.Read(indexTableItem, nBlockID))
	{
		//ֻ�����黹�ڵ�ǰ�ļ�ʱ���ſ����˳������Ѿ�ռ�ݵĿռ�
		if (m_memCfg.m_nCurOffset >= indexTableItem.m_nTotalDataLength)
		{
			m_memCfg.m_nCurOffset -= indexTableItem.m_nTotalDataLength;
		}

		--m_memCfg.m_nLastBlockID;
	}
	else
	{
		return -1;
	}
	
	return 0;
}

VBH::CMemBcBlock* CBlockChainTable::ReadDecodeBlockImpl(ACE_UINT32* pDataLen, const ACE_UINT64 nBlockID)
{
	//1. ��ȡĿ�������Լ���һ�������index //��ȡ��һ������index��Ϊ��У�鱾�����hash
	VBH_CLS::CBcIndexTableItem curBlockIndex; //�ڶ��ζ�ȡʱ�����ܱ�֤��һ�ζ�ȡ�����ݱ����������Ե�һ�ζ�ȡ������Ҫ��������
	VBH_CLS::CBcIndexTableItem nextBlockIndex; //�����һ�������������ڣ����ȡ

	if (DSC_UNLIKELY(m_bcIndexTable.Read(curBlockIndex, nBlockID)))
	{
		DSC_RUN_LOG_ERROR("read block index %llu failed", nBlockID);
		return nullptr;
	}

	if (nBlockID < m_pCfg->m_nLastBlockID)
	{
		if (DSC_UNLIKELY(m_bcIndexTable.Read(nextBlockIndex, nBlockID + 1)))
		{
			DSC_RUN_LOG_ERROR("read block index %llu failed", nBlockID + 1);
			return nullptr;
		}
	}
	else
	{
		if (nBlockID > m_pCfg->m_nLastBlockID)
		{
			DSC_RUN_LOG_ERROR("error block-id:%llu, cur-last-block-id:%llu, mem-last-blcok-id:%llu", nBlockID, m_pCfg->m_nLastBlockID, m_memCfg.m_nLastBlockID);
			return nullptr;
		}
	}

	//2. ���ļ���ȡ��ǰ���鵽�ڴ���뻺������ //��ȡǰ��֤�ڴ���뻺����
	if (m_nAlignBlockDataBufLen < curBlockIndex.m_nTotalDataLength) //���������������ȿ��ٻ�����
	{
		if (m_pAlignBlockDataBuf)
		{
			DSC_FREE(m_pAlignBlockDataBuf);
		}

		m_nAlignBlockDataBufLen = curBlockIndex.m_nTotalDataLength; //curBlockIndex.m_nTotalDataLength ֵ���Ѿ����й����ȶ����ֵ
		DSC_MEM_ALIGN(m_pAlignBlockDataBuf, m_nAlignBlockDataBufLen);

		if (!m_pAlignBlockDataBuf)
		{
			DSC_RUN_LOG_ERROR("DSC_MEM_ALIGN failed, memory size:%lld.", m_nAlignBlockDataBufLen);
			m_nAlignBlockDataBufLen = 0;

			return nullptr;
		}
	}

	if (m_pVbfs->Read(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE, curBlockIndex.m_nFileID, m_pAlignBlockDataBuf,
		curBlockIndex.m_nTotalDataLength, curBlockIndex.m_nOffset))
	{
		DSC_RUN_LOG_ERROR("read block from file failed, block-id:%llu", nBlockID);
		return nullptr;
	}

	char blockHash[VBH_BLOCK_DIGEST_LENGTH]; //У��ʱ����ŵ�ǰ���hash�������ʱ����ſ��д�ŵ�ǰ1���hash

	//3. �����ȡ�����飬�����ͬʱУ��merkel���� //�����ͬʱ���������д洢����һ���hash�ͱ����鳤��
	VBH::CMemBcBlock* pMemBlock = VBH::vbhDecodeMemBcBloc(blockHash, m_pAlignBlockDataBuf, curBlockIndex.m_nBlockDataLength, m_merkelTree);

	if (pMemBlock)
	{
		if (DSC_LIKELY(!memcmp(blockHash, curBlockIndex.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH)))
		{
			//4. ���������hashУ�� //�������һ���飨��ǰ���鲻�����1����
			if (DSC_LIKELY(nBlockID < m_pCfg->m_nLastBlockID))
			{
				VBH::vbhDigest(m_pAlignBlockDataBuf, curBlockIndex.m_nBlockDataLength, blockHash);

				if (DSC_UNLIKELY(memcmp(blockHash, nextBlockIndex.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH))) //�����
				{
					DSC_THREAD_TYPE_DELETE(pMemBlock);
					pMemBlock = nullptr;

					DSC_RUN_LOG_ERROR("check hash failed(%llu)", nBlockID);
				}
			}

			//5. �������鳤��
			if (DSC_UNLIKELY(pDataLen)) //��Ҫʱ���ŷ��ظó���, ���������в���Ҫ�ó���
			{
				*pDataLen = curBlockIndex.m_nBlockDataLength;
			}
		}
		else
		{
			DSC_THREAD_TYPE_DELETE(pMemBlock);
			pMemBlock = nullptr;

			DSC_RUN_LOG_ERROR("preBlockHash in block is not equal to that in index-db, block-id:%lld", nBlockID);
		}
	}
	else
	{
		DSC_RUN_LOG_ERROR("decode block failed, block-id:%lld", nBlockID);
	}

	return pMemBlock;
}

