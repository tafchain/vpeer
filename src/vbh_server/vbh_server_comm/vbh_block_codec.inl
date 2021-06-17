
inline void VBH::CBlockEncoder::InitSetEncodeBuffer(char* pBuf)
{
	m_encodeState.SetBuffer(pBuf + VBH::CBcBlockHeader::EN_SIZE);
	m_bcBlockHeader.m_nTransCount = 0;
	m_merkelTree.Clear();
}

inline void VBH::CBlockEncoder::ResetEncodeBuffer(char* pBuf, size_t nDataLen)
{
	m_encodeState.SetBuffer(pBuf + VBH::CBcBlockHeader::EN_SIZE);
	m_encodeState.AddOffset(nDataLen - VBH::CBcBlockHeader::EN_SIZE);
}

inline DSC::CDscNetCodecEncoder& VBH::CBlockEncoder::BeginEncodeTransaction()
{
	m_pTransBeginPtr = m_encodeState.GetCurBufPtr();

	return m_encodeState;
}

inline void VBH::CBlockEncoder::EndEncodeTransaction()
{
	m_merkelTree.AddLeafNode(m_pTransBeginPtr, (ACE_UINT32)(m_encodeState.GetCurBufPtr() - m_pTransBeginPtr));
	++m_bcBlockHeader.m_nTransCount;
}

inline size_t VBH::CBlockEncoder::GetEncodeDataSize()
{
	return m_encodeState.GetOffset() + VBH::CBcBlockHeader::EN_SIZE;
}

inline ACE_INT32 VBH::CBlockEncoder::EncodeBlockHeader()
{
	if (m_merkelTree.BuildMerkleTree())
	{
		return -1;
	}

	m_merkelTree.GetRoot(m_bcBlockHeader.m_merkelTreeRootHash);
	DSC::Encode(m_bcBlockHeader, m_encodeState.Begin() - VBH::CBcBlockHeader::EN_SIZE);

	return 0;
}

//===========================================================================


