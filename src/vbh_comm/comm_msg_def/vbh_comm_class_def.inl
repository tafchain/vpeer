
inline bool VBH::CVbhAllocatedKey::operator==(const CVbhAllocatedKey& key) const
{
	return (this->m_nSequenceNumber == key.m_nSequenceNumber)
		&& (this->m_nAllocatedID == key.m_nAllocatedID);
}

template <typename TYPE>
VBH::CSimpleVector<TYPE>::~CSimpleVector()
{
	this->Close();
}

template <typename TYPE>
void VBH::CSimpleVector<TYPE>::Open(const ACE_UINT16 nItemCount)
{
	//保证只被打开一次 //再次打开时会出断言错误
	ACE_ASSERT(m_pItemBuf == NULL);
	ACE_ASSERT(nItemCount > 0);

	//允许打开时，指定的长度为0，以保证编解码过程中支持0长数组
	m_nItemCount = nItemCount;
	DSC_THREAD_TYPE_ALLOCATE_ARRAY(m_pItemBuf, m_nItemCount);
}

template <typename TYPE>
void VBH::CSimpleVector<TYPE>::Close(void)
{
	if (m_pItemBuf)
	{
		ACE_ASSERT(m_nItemCount);

		DSC_THREAD_TYPE_DEALLOCATE_ARRAY(m_pItemBuf, m_nItemCount);
		m_pItemBuf = NULL;
		m_nItemCount = 0;
	}
}

template <typename TYPE>
inline ACE_UINT16 VBH::CSimpleVector<TYPE>::Size(void) const
{
	return m_nItemCount;
}

template <typename TYPE>
inline TYPE& VBH::CSimpleVector<TYPE>::operator[](const ACE_UINT16 idx)
{
	return m_pItemBuf[idx];
}

template <typename TYPE>
VBH::CSimpleVector<TYPE>& VBH::CSimpleVector<TYPE>::operator=(const CSimpleVector<TYPE>& vec)
{
	this->Close();

	if (vec.m_nItemCount)
	{
		this->Open(vec.m_nItemCount);
		for (ACE_UINT16 idx = 0; idx < vec.m_nItemCount; ++idx)
		{
			this->m_pItemBuf[idx] = vec.m_pItemBuf[idx];
		}
	}
	
	return *this;
}

template <typename TYPE>
inline void VBH::CSimpleVector<TYPE>::GetSize(DSC::CDscCodecGetSizer& rDscCodecGetSizer)
{
	rDscCodecGetSizer.GetSize(m_nItemCount);

	for (ACE_UINT16 idx = 0; idx < m_nItemCount; ++idx) //支持0长数组的编码解码
	{
		rDscCodecGetSizer.GetSize(m_pItemBuf[idx]);
	}
}

template <typename TYPE>
template<typename ENCODER_TYPE>
inline void VBH::CSimpleVector<TYPE>::Encode(ENCODER_TYPE& rDscCodecEncoder)
{
	rDscCodecEncoder.Encode(m_nItemCount);

	for (ACE_UINT16 idx = 0; idx < m_nItemCount; ++idx) //支持0长数组的编码解码
	{
		rDscCodecEncoder.Encode(m_pItemBuf[idx]);
	}
}

template <typename TYPE>
template<typename DECODER_TYPE>
ACE_INT32 VBH::CSimpleVector<TYPE>::Decode(DECODER_TYPE& rDscCodecDecoder)
{
	ACE_UINT16 nSize;

	if (rDscCodecDecoder.Decode(nSize))
	{
		return -1;
	}
	else
	{
		this->Close();
		if (nSize)
		{
			this->Open(nSize);

			for (ACE_UINT16 i = 0; i < nSize; ++i)
			{
				if (rDscCodecDecoder.Decode(m_pItemBuf[i]))
				{
					return -1;
				}
			}
		}

		return 0;
	}
}
