
inline bool VBH::IsEqual(const CDscString& str, const DSC::CDscBlob& blob)
{
	if (str.size() == blob.GetSize())
	{
		return 0 == ::memcmp(str.c_str(), blob.GetBuffer(), blob.GetSize());
	}

	return false;
}

inline void VBH::Assign(CDscString& str, const DSC::CDscBlob& blob)
{
	str.assign(blob.GetBuffer(), blob.GetSize());
}

inline void VBH::Assign(DSC::CDscBlob& blob, CDscString& str)
{
	blob.Set(str.data(), str.size());
}

inline bool VBH::IsEqual(const CDscString& str, const DSC::CDscShortBlob& blob)
{
	if (str.size() == blob.GetSize())
	{
		return 0 == ::memcmp(str.c_str(), blob.GetBuffer(), blob.GetSize());
	}

	return false;
}

inline void VBH::Assign(CDscString& str, const DSC::CDscShortBlob& blob)
{
	str.assign(blob.GetBuffer(), blob.GetSize());
}

inline void VBH::Assign(DSC::CDscShortBlob& blob, CDscString& str)
{
	blob.Set(str.data(), str.size());
}

template<typename ITEM_TYPE>
void VBH::ClearDscUnboundQueue(CDscUnboundQueue<ITEM_TYPE>& queue)
{
	ITEM_TYPE* pItem = queue.PopFront();

	while (pItem)
	{
		DSC_THREAD_TYPE_DELETE(pItem);
		pItem = queue.PopFront();
	}
}

template<typename ITEM_TYPE>
void VBH::MergeDscUnboundQueue(CDscUnboundQueue<ITEM_TYPE>& queueTo, CDscUnboundQueue<ITEM_TYPE>& queueFrom)
{
	ITEM_TYPE* pItem = queueFrom.PopFront();

	while (pItem)
	{
		queueTo.PushBack(pItem);
		pItem = queueFrom.PopFront();
	}
}


inline ACE_INT32 VBH::GetVbhProfileInt(const char* pszKeyName, ACE_INT32& nValue)
{
	return CDscConfigureDemon::instance()->GetDscProfileInt("VBH_SYS_CFG", pszKeyName, nValue);
}

inline ACE_INT32 VBH::GetVbhProfileString(const char* pszKeyName, CDscString& strValue)
{
	return CDscConfigureDemon::instance()->GetDscProfileString("VBH_SYS_CFG", pszKeyName, strValue);
}


inline ACE_UINT16 VBH::GetDataHashValue(const DSC::CDscBlob& data, const ACE_UINT32 nHashBits)
{
	if (nHashBits)
	{
		ACE_UINT64 h = DSC::Hash(data.GetBuffer(), data.GetSize());

		h *= DSC_HASH_PRIME;

		return (h >> (64 - nHashBits)) + 1; //committerID分配从1开始连续分配
	}
	else
	{
		return 1;
	}
}

template <typename CFG_TYPE>
ACE_INT32 VBH::CreateCfgFile(const CDscString& strFilePath, CFG_TYPE& initCfg)
{
	ACE_HANDLE handle = ACE_OS::open(strFilePath.c_str(), O_RDWR | O_BINARY | O_CREAT);

	if (ACE_INVALID_HANDLE == handle)
	{
		int nLastError = ACE_OS::last_error();
		DSC_RUN_LOG_ERROR("cann't open file:%s, last error code:%d, last error reason:%s.", strFilePath.c_str(), nLastError, ACE_OS::strerror(nLastError));
		return -1;
	}

	if (-1 == ACE_OS::write(handle, &initCfg, sizeof(initCfg)))
	{
		int nLastError = ACE_OS::last_error();
		DSC_RUN_LOG_ERROR("write file failed, file name:%s, last error code:%d, last error reason:%s.", strFilePath.c_str(), nLastError, ACE_OS::strerror(nLastError));
		ACE_OS::close(handle);

		return -1;
	}

	ACE_OS::close(handle);

	return 0;
}

template<typename CFG_TYPE>
ACE_INT32 VBH::LoadMmapCfgFile(const CDscString& strFilePath, ACE_Shared_Memory_MM& shmHandle, CFG_TYPE*& pCfg)
{
	//打开按共享内存打开文件
	if (-1 == shmHandle.open(strFilePath.c_str(), sizeof(CFG_TYPE)))
	{
		int nLastError = ACE_OS::last_error();
		DSC_RUN_LOG_ERROR("open share memory failed, file name:%s, last error code:%d, last error reason:%s.", strFilePath.c_str(), nLastError, ACE_OS::strerror(nLastError));

		return -1;
	}

	pCfg = (CFG_TYPE*)shmHandle.malloc();
	if (NULL == pCfg)
	{
		DSC_RUN_LOG_ERROR("alloc memory for memory map file failed, file name:%s.", strFilePath.c_str());
		shmHandle.close();

		return -1;
	}

	return 0;
}

