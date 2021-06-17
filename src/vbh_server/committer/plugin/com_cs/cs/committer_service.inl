
inline void CCommitterService::CQuerySyncSourcePeerSession::OnTimer(void)
{
	m_rXCommitterService.OnTimeOut(this);
}

inline void CCommitterService::CVerifyPeerStateSession::OnTimer(void)
{
	m_rXCommitterService.OnTimeOut(this);
}

inline void CCommitterService::CSyncBlockSession::OnTimer(void)
{
	m_rXCommitterService.OnTimeOut(this);
}

inline char* CCommitterService::ReadBlock(ACE_UINT32& nBlockBufLen, const ACE_UINT64 nBlockID)
{
	return m_bcTable.ReadBlock(nBlockBufLen, nBlockID);
}

inline ACE_INT32 CCommitterService::ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID)
{
	return m_bcTable.ReadBlockHash(pBlockHash, nBlockID);
}

inline void CCommitterService::SetCommitterAgentService(ICommitterAgentService* pCommitterAgentService)
{
	m_pCommitterAgentService = pCommitterAgentService;
}

inline ACE_INT32 CCommitterService::GetSequenceNumber(ACE_UINT32& nSeq, const ACE_UINT32 nAlocID)
{
	VBH_CLS::CIndexTableItem indexTableItem;

	if (m_wsIndexTable.Read(indexTableItem, nAlocID))
	{
		DSC_RUN_LOG_ERROR("read write-set-index-table failed.");
		return -1;
	}
	else
	{
		nSeq = indexTableItem.m_nSequenceNumber4Verify;
		return 0;
	}
}

inline void CCommitterService::InsertBlockIntoCache(VBH::CMemBcBlock* pBlock)
{
	CMemBlockPtrInfo* pBlockPtrInfo = DSC_THREAD_TYPE_NEW(CMemBlockPtrInfo) CMemBlockPtrInfo;

	//²åÈë¶ÓÁÐ
	pBlockPtrInfo->m_pBcBlock = pBlock;
	m_mapBlockCache.DirectInsert(pBlock->m_nBlockID, pBlockPtrInfo);
	++m_nCurBlockCacheSize;
	m_dequeBlockCache.PushBack(pBlockPtrInfo);
}

