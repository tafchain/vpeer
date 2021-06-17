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

	//1. 读取文件size
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
			m_nFileSize <<= 30; //读取的数据以GB为单位
		}
	}

	//2. 打开配置文件
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

		//创建配置文件
		if (VBH::CreateCfgFile(strCfgFilePath, cfg))
		{
			return -1;
		}

		//创建日志文件
		if (VBH::CreateCfgFile(strLogFilePath, log))
		{
			return -1;
		}
	}

	//加载配置
	if (VBH::LoadMmapCfgFile(strCfgFilePath, m_shmCfg, m_pCfg))
	{
		return -1;
	}
	m_memCfg = *m_pCfg;

	//加载日志
	if (VBH::LoadMmapCfgFile(strLogFilePath, m_shmLog, m_pLog))
	{
		return -1;
	}

	//3. 打开index table
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
	//1. 将区块数据对齐，以方便后面的direct-io写入
	ACE_UINT32 nTotalDataBufLen = rBlockData.GetSize();
	
	nTotalDataBufLen = DEF_GET_MEM_ALIGN_SIZE(nTotalDataBufLen); //数据长度也要对齐
	
	if (m_nAlignBlockDataBufLen < nTotalDataBufLen) //缓冲区不够，则先开辟缓冲区
	{
		if (m_pAlignBlockDataBuf)
		{
			DSC_FREE(m_pAlignBlockDataBuf);
		}
		
		m_nAlignBlockDataBufLen = nTotalDataBufLen; //快速增长，减少重新开辟次数
		DSC_MEM_ALIGN(m_pAlignBlockDataBuf, m_nAlignBlockDataBufLen);

		if (!m_pAlignBlockDataBuf)
		{
			DSC_RUN_LOG_ERROR("DSC_MEM_ALIGN failed, memory size:%lld.", m_nAlignBlockDataBufLen);
			m_nAlignBlockDataBufLen = 0;

			return -1;
		}
	}

	memcpy(m_pAlignBlockDataBuf, rBlockData.GetBuffer(), rBlockData.GetSize());

	//2. 确定当前文件是否够写入
	if (m_memCfg.m_nCurOffset + nTotalDataBufLen > m_nFileSize) //当前文件已经不够写，要开辟新的文件
	{
		if (m_pVbfs->AllocatFile(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE, m_pCfg->m_nCurFileID+1))
		{
			DSC_RUN_LOG_ERROR("allocate block file-id failed, block-id:%llu, channel-id:%u", nBlockID, m_nChannelID);

			return -1;
		}
		m_pCfg->m_nCurOffset = 0; //allocat不可以撤销，所以内存映射文件的记录也要一起变
		m_pCfg->m_nCurFileID++;
		m_memCfg.m_nCurFileID = m_pCfg->m_nCurFileID;
		m_memCfg.m_nCurOffset = 0;
	}

	if (m_pVbfs->Write(VBH_CLS::EN_BLOCK_CHAIN_TABLE_TYPE, m_memCfg.m_nCurFileID, m_pAlignBlockDataBuf, nTotalDataBufLen, m_memCfg.m_nCurOffset))
	{
		DSC_RUN_LOG_ERROR("write block into file failed, block-id:%lld.", nBlockID);
		
		return -1;
	}

	//3. 写入index
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
	
	m_memCfg.m_nCurOffset += nTotalDataBufLen; //跳转对齐后的长度, 区块按对齐存储
	m_memCfg.m_nLastBlockID = nBlockID;

	return 0;
}

ACE_INT32 CBlockChainTable::PopBack(const ACE_UINT64 nBlockID)
{
	ACE_ASSERT(nBlockID == m_memCfg.m_nLastBlockID);

	VBH_CLS::CBcIndexTableItem indexTableItem;

	if (m_bcIndexTable.Read(indexTableItem, nBlockID))
	{
		//只有区块还在当前文件时，才可以退出区块已经占据的空间
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
	//1. 读取目标区块以及下一个区块的index //读取下一个区块index是为了校验本区块的hash
	VBH_CLS::CBcIndexTableItem curBlockIndex; //第二次读取时，不能保证第一次读取的数据被换出，所以第一次读取的数据要拷贝出来
	VBH_CLS::CBcIndexTableItem nextBlockIndex; //如果下一个区块索引存在，则读取

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

	//2. 从文件读取当前区块到内存对齐缓冲区中 //读取前保证内存对齐缓冲区
	if (m_nAlignBlockDataBufLen < curBlockIndex.m_nTotalDataLength) //缓冲区不够，则先开辟缓冲区
	{
		if (m_pAlignBlockDataBuf)
		{
			DSC_FREE(m_pAlignBlockDataBuf);
		}

		m_nAlignBlockDataBufLen = curBlockIndex.m_nTotalDataLength; //curBlockIndex.m_nTotalDataLength 值是已经进行过长度对齐的值
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

	char blockHash[VBH_BLOCK_DIGEST_LENGTH]; //校验时，存放当前块的hash；解码块时，存放块中存放的前1块的hash

	//3. 解码读取的区块，解码的同时校验merkel树根 //解码的同时返回区块中存储的上一块的hash和本区块长度
	VBH::CMemBcBlock* pMemBlock = VBH::vbhDecodeMemBcBloc(blockHash, m_pAlignBlockDataBuf, curBlockIndex.m_nBlockDataLength, m_merkelTree);

	if (pMemBlock)
	{
		if (DSC_LIKELY(!memcmp(blockHash, curBlockIndex.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH)))
		{
			//4. 对区块进行hash校验 //如果有下一区块（当前区块不是最后1个）
			if (DSC_LIKELY(nBlockID < m_pCfg->m_nLastBlockID))
			{
				VBH::vbhDigest(m_pAlignBlockDataBuf, curBlockIndex.m_nBlockDataLength, blockHash);

				if (DSC_UNLIKELY(memcmp(blockHash, nextBlockIndex.m_preBlockHash.data(), VBH_BLOCK_DIGEST_LENGTH))) //不相等
				{
					DSC_THREAD_TYPE_DELETE(pMemBlock);
					pMemBlock = nullptr;

					DSC_RUN_LOG_ERROR("check hash failed(%llu)", nBlockID);
				}
			}

			//5. 返回区块长度
			if (DSC_UNLIKELY(pDataLen)) //需要时，才返回该长度, 正常流程中不需要该长度
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

