
inline ACE_UINT32 VBH::CTransactionSequenceNumber::CombineSequenceNumber(const ACE_UINT8 nType, const ACE_UINT32 nSerialNumber)
{
	ACE_UINT32 nSn = nType;

	nSn <<= (32 - EN_TYPE_BITS);
	//nSn |= DEF_TRANSACTION_SEQUENCE_NUMBER & nSerialNumber;
	nSn |= nSerialNumber;

	return nSn;
}

inline ACE_UINT8 VBH::CTransactionSequenceNumber::GetTransType(const ACE_UINT32 nSerialNumber)
{
	return nSerialNumber >> (32 - EN_TYPE_BITS);
}

inline void VBH::CTransactionSequenceNumber::SequenceNumberInc(ACE_UINT32& nSerialNumber)
{
	if (DSC_LIKELY(nSerialNumber < DEF_TRANSACTION_SEQUENCE_NUMBER))
	{
		++nSerialNumber;
	}
	else
	{
		nSerialNumber = 0;
	}
}



