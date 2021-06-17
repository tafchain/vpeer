inline VBH::CMemBcBlock* CBlockChainTable::ReadDecodeBlock(const ACE_UINT64 nBlockID)
{
	return ReadDecodeBlockImpl(nullptr, nBlockID);
}

inline char* CBlockChainTable::ReadBlock(ACE_UINT32& nDataLen, const ACE_UINT64 nBlockID)
{
	VBH::CMemBcBlock* pMemBlock = ReadDecodeBlockImpl(&nDataLen, nBlockID);

	if (pMemBlock)
	{
		DSC_THREAD_TYPE_DELETE(pMemBlock);

		return m_pAlignBlockDataBuf;
	}
	else
	{
		return nullptr;
	}
}

inline ACE_INT32 CBlockChainTable::SaveToLog(void)
{
	m_pLog->m_nCurFileID = m_pCfg->m_nCurFileID;
	m_pLog->m_nCurOffset = m_pCfg->m_nCurOffset;
	m_pLog->m_nLastBlockID = m_pCfg->m_nLastBlockID;

	return m_bcIndexTable.SaveToLog();
}

inline ACE_INT32 CBlockChainTable::Persistence(void)
{
	*m_pCfg = m_memCfg;

	return m_bcIndexTable.Persistence();
}

inline void CBlockChainTable::CommitteTransaction(void)
{
	m_bcIndexTable.CommitteTransaction();
}

inline void CBlockChainTable::RollbackCache()
{
	m_memCfg = *m_pCfg;

	m_bcIndexTable.RollbackCache();
}

inline ACE_INT32 CBlockChainTable::RollbackTransaction(void)
{
	m_pCfg->m_nCurFileID = m_pLog->m_nCurFileID;
	m_pCfg->m_nCurOffset = m_pLog->m_nCurOffset;
	m_pCfg->m_nLastBlockID = m_pLog->m_nLastBlockID;

	m_memCfg = *m_pCfg;

	return m_bcIndexTable.RollbackTransaction();
}

inline ACE_INT32 CBlockChainTable::RecoverFromLog(void)
{
	m_pCfg->m_nCurFileID = m_pLog->m_nCurFileID;
	m_pCfg->m_nCurOffset = m_pLog->m_nCurOffset;
	m_pCfg->m_nLastBlockID = m_pLog->m_nLastBlockID;

	m_memCfg = *m_pCfg;

	return m_bcIndexTable.RecoverFromLog();
}


