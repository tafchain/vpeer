#ifndef VBH_USE_SMX //  π”√ecdsa
inline char* VBH::vbhSign(EC_KEY* pEcKey, char* buff, int buff_len, unsigned int& nSignature)
{
	unsigned char* pSignature = (unsigned char*)DSC_THREAD_MALLOC(ECDSA_size(pEcKey));

	if (ECDSA_sign(0, (unsigned char*)buff, buff_len, pSignature, &nSignature, pEcKey) != 1)
	{
		DSC_THREAD_FREE((char*)pSignature);
		pSignature = NULL;
	}

	EC_KEY_free(pEcKey);

	return (char*)pSignature;
}

inline bool VBH::vbhVerifySign(EC_KEY* pEcKey, char* buff, size_t buff_len, char* pSignature, const int nSignature)
{
	int nRet = ECDSA_verify(0, (unsigned char*)buff, buff_len, (unsigned char*)pSignature, nSignature, pEcKey);

	EC_KEY_free(pEcKey);

	return (1 == nRet);
}

inline void VBH::vbhDigest(const char* data, size_t n, char* digest)
{
	SHA256((const unsigned char*)data, n, (unsigned char*)digest);
}

#endif

inline char* VBH::vbhSign(char* pPriKeyBuf, int nPriKeyLen, EC_GROUP* pGroup, char* pDataBuf, size_t nDataLen, unsigned int& nSignatureLen)
{
	EC_KEY* pKey = vbhCreateEcKeyFromBinPrivateKey(pPriKeyBuf, nPriKeyLen, pGroup);

	if (pKey)
	{
		return vbhSign(pKey, pDataBuf, nDataLen, nSignatureLen);
	}

	return nullptr;
}

inline bool VBH::vbhVerifySign(char* pPubKeyBuf, int nPubKeyLen, EC_GROUP* pGroup, char* pDataBuf, size_t nDataLen, char* pSignatureBuf, size_t nSignatureLen)
{
	EC_KEY* pKey = vbhCreateEcKeyFromBinPublicKey(pPubKeyBuf, nPubKeyLen, pGroup);

	if (pKey)
	{
		return vbhVerifySign(pKey, pDataBuf, nDataLen, pSignatureBuf, nSignatureLen);
	}

	return false;
}
