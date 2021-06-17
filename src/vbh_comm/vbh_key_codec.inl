

inline size_t VBH::HexEncode(char*& pOutBuf, const char* pszIn, const size_t nInLen)
{
	pOutBuf = DSC_THREAD_SIZE_MALLOC(nInLen << 1);

	return DSC::ToHex(pOutBuf, pszIn, nInLen);
}

template<typename TYPE>
size_t VBH::HexEncode(char*& pOutBuf, TYPE& t)
{
	char* pBuf;
	size_t nBufLen;

	DSC::Encode(t, pBuf, nBufLen);

	size_t nOutLen = HexEncode(pOutBuf, pBuf, nBufLen);

	DSC_THREAD_SIZE_FREE(pBuf, nBufLen);

	return nOutLen;
}

template<typename TYPE>
ACE_INT32 VBH::HexDecode(TYPE& t, const char* pszIn, const size_t nInLen)
{
	char* pOutBuf;
	size_t nOutLen;

	if (HexDecode(pOutBuf, nOutLen, pszIn, nInLen) == 0)
	{
		if (DSC::Decode(t, pOutBuf, nOutLen))
		{
			DSC_THREAD_SIZE_FREE(pOutBuf, nOutLen);
			return -1;
		}
		else
		{
			DSC_THREAD_SIZE_FREE(pOutBuf, nOutLen);
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

//base64±àÂë
inline size_t VBH::Base64Encode(char*& pOutBuf, const char* pszIn, const size_t nInLen)
{
	size_t nOutLen = DSC::Base64encode_len(nInLen);

	pOutBuf = DSC_THREAD_MALLOC(nOutLen);

	return DSC::Base64encode(pOutBuf, pszIn, nInLen);
}

template<typename TYPE>
size_t VBH::Base64Encode(char*& pOutBuf, TYPE& t)
{
	char* pBuf;
	size_t nBufLen;

	DSC::Encode(t, pBuf, nBufLen);

	size_t nOutLen = Base64Encode(pOutBuf, pBuf, nBufLen);

	DSC_THREAD_SIZE_FREE(pBuf, nBufLen);

	return nOutLen;
}

template<typename TYPE>
ACE_INT32 VBH::Base64Decode(TYPE& t, const char* pszIn, const size_t nInLen)
{
	char* pOutBuf;
	size_t nOutLen;

	if (Base64Decode(pOutBuf, nOutLen, pszIn, nInLen) == 0)
	{
		if (DSC::Decode(t, pOutBuf, nOutLen))
		{
			DSC_THREAD_FREE(pOutBuf);
			return -1;
		}
		else
		{
			DSC_THREAD_FREE(pOutBuf);
			return 0;
		}		
	}
	else
	{
		return -1;
	}
}