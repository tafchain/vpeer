#include "dsc/dsc_log.h"

#include "vbh_comm/vbh_encrypt_lib.h"
#include "vbh_comm/vbh_key_codec.h"



VBH::CVbhCryptKeyWrapper::CVbhCryptKeyWrapper(CDscString& strSvrPubKey, VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl)
	: m_strSvrPubKey(strSvrPubKey)
	, m_userKey(userKey)
	, m_transUrl(transUrl)
{
}



VBH::CUserCryptKeyWrapper::CUserCryptKeyWrapper(CDscString& strCltPriKey, CDscString& strSvrPubKey, CDscString& strEnvelopeKey,
	VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl, ACE_UINT32& nChannelID)
	: m_strCltPriKey(strCltPriKey)
	, m_strSvrPubKey(strSvrPubKey)
	, m_strEnvelopeKey(strEnvelopeKey)
	, m_userKey(userKey)
	, m_transUrl(transUrl)
	, m_nChannelID(nChannelID)
{
}

ACE_INT32 VBH::CreateBinCryptKeyPair(CDscString& strPubKey, CDscString& strPriKey)
{
	ACE_INT32 nReturnCode = 0;
	EC_GROUP* pGroup = VBH::vbhEcGroup();

	if (pGroup)
	{
		EC_KEY* pEcKey = VBH::vbhCreateEcKey(pGroup);

		if (pEcKey)
		{
			int nPublicKeyLen;
			char* pPublicKey = VBH::vbhGetBinPublicKeyFromEcKey(nPublicKeyLen, pEcKey);

			if (pPublicKey)
			{
				strPubKey.assign(pPublicKey, nPublicKeyLen);
				DSC_THREAD_SIZE_FREE(pPublicKey, nPublicKeyLen);
			}
			else
			{
				DSC_RUN_LOG_ERROR("vbhGetBinPublicKeyFromEcKey failed.");
				nReturnCode = -1;
			}

			int nPrivateKeyLen;
			char* pPrivateKey = VBH::vbhGetBinPrivateKeyFromEcKey(nPrivateKeyLen, pEcKey);

			if (pPrivateKey)
			{
				strPriKey.assign(pPrivateKey, nPrivateKeyLen);
				DSC_THREAD_SIZE_FREE(pPrivateKey, nPrivateKeyLen);
			}
			else
			{
				DSC_RUN_LOG_ERROR("vbhGetBinPrivateKeyFromEcKey failed.");
				nReturnCode = -1;
			}

			EC_KEY_free(pEcKey);
		}
		else
		{
			DSC_RUN_LOG_ERROR("vbhCreateEcKey failed.");
			nReturnCode = -1;
		}

		EC_GROUP_free(pGroup);
	}
	else
	{
		DSC_RUN_LOG_ERROR("vbhEcGroup failed.");
		nReturnCode = -1;
	}

	return nReturnCode;
}

ACE_INT32 VBH::CreateHexCryptKeyPair(CDscString& strPubKey, CDscString& strPriKey)
{
	if (VBH::CreateBinCryptKeyPair(strPubKey, strPriKey))
	{
		DSC_RUN_LOG_ERROR("CreateBinCryptKeyPair failed.");
		
		return -1;
	}

	char* pTmpKey;
	size_t nTmpLen = VBH::HexEncode(pTmpKey, strPubKey.c_str(), strPubKey.size());
	strPubKey.assign(pTmpKey, nTmpLen);
	DSC_THREAD_SIZE_FREE(pTmpKey, nTmpLen);

	nTmpLen = VBH::HexEncode(pTmpKey, strPriKey.c_str(), strPriKey.size());
	strPriKey.assign(pTmpKey, nTmpLen);
	DSC_THREAD_SIZE_FREE(pTmpKey, nTmpLen);

	return 0;
}

ACE_INT32 VBH::GenerateUserCryptKey(CDscString& strUserCryptKey, const char* pPriKey, const size_t nPriKeyLen, const char* pVbhCryptKey, const size_t nVbhCryptKeyLen, ACE_UINT32 nChannelID)
{
	ACE_INT32 nReturnCode = 0;
	CDscString strSvrPubKey;
	VBH::CVbhAllocatedKey userKey;
	VBH::CTransactionUrl transUrl;

	VBH::CVbhCryptKeyWrapper vbhCryptKeyWrapper(strSvrPubKey, userKey, transUrl);

	if (VBH::HexDecode(vbhCryptKeyWrapper, pVbhCryptKey, nVbhCryptKeyLen))
	{
		DSC_RUN_LOG_ERROR("decode user-sequence failed.");
		nReturnCode = -1;
	}
	else
	{
		char* pBinPriKey;
		size_t nBinPriKeyLen;
		if (VBH::HexDecode(pBinPriKey, nBinPriKeyLen, pPriKey, nPriKeyLen))
		{
			DSC_RUN_LOG_ERROR("decode private-key failed.");
			nReturnCode = -1;
		}
		else
		{
			EC_GROUP* pGroup = VBH::vbhEcGroup();
			if (pGroup)
			{
				EC_KEY* pEcKey = VBH::vbhCreateEcKeyFromBinPrivateKey(pBinPriKey, nBinPriKeyLen, pGroup);

				if (pEcKey)
				{
					int nEnvelopeKeyLen;
					char* pEnvelopeKey = VBH::vbhGenEcdhEnvKey(nEnvelopeKeyLen, strSvrPubKey.c_str(), strSvrPubKey.size(), pEcKey);

					if (pEnvelopeKey)
					{
						CDscString envelopeKey(pEnvelopeKey, nEnvelopeKeyLen);
						CDscString strBinPriKey(pBinPriKey, nBinPriKeyLen);
						VBH::CUserCryptKeyWrapper userCryptKeyWrapper(strBinPriKey, strSvrPubKey, envelopeKey, userKey, transUrl, nChannelID);
						
						char* pTmpBuf;
						size_t nTmpLen = VBH::Base64Encode(pTmpBuf, userCryptKeyWrapper);
						
						strUserCryptKey.assign(pTmpBuf, nTmpLen);
						DSC_THREAD_FREE(pTmpBuf);

						DSC_THREAD_SIZE_FREE(pEnvelopeKey, nEnvelopeKeyLen);
					}
					else
					{
						DSC_RUN_LOG_ERROR("vbhGenEcdhEnvKey failed.");
						nReturnCode = -1;
					}

					EC_KEY_free(pEcKey);
				}
				else
				{
					DSC_RUN_LOG_ERROR("vbhCreateEcKeyFromBinPrivateKey failed.");
					nReturnCode = -1;
				}

				EC_GROUP_free(pGroup);
			}
			else
			{
				DSC_RUN_LOG_ERROR("vbhEcGroup failed.");
				nReturnCode = -1;
			}

			DSC_THREAD_SIZE_FREE(pBinPriKey, nBinPriKeyLen);
		}
	}

	return nReturnCode;
}

ACE_INT32 VBH::HexDecode(char*& pOutBuf, size_t& nOutLen, const char* pszIn, const size_t nInLen)
{
	if ((nInLen > 0) && (nInLen % 2 == 0))
	{
		nOutLen = nInLen >> 1;

		pOutBuf = DSC_THREAD_SIZE_MALLOC(nOutLen);

		if (DSC::ToBin(pOutBuf, pszIn, nInLen)) 
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

//解码后长度肯定不会超过输入长度
ACE_INT32 VBH::Base64Decode(char*& pOutBuf, size_t& nOutLen, const char* pszIn, const size_t nInLen)
{
	if (nInLen > 0)
	{
		pOutBuf = DSC_THREAD_MALLOC(nInLen);
		nOutLen = DSC::Base64decode(pOutBuf, pszIn);

		return 0;
	}
	else
	{
		return -1;
	}
}

