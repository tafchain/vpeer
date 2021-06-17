#ifndef VBH_KEY_CODEC_H_LKJERLJEW5435L43J6543J643JL4KHJK34H5
#define VBH_KEY_CODEC_H_LKJERLJEW5435L43J6543J643JL4KHJK34H5

#include "dsc/dsc_base64.h"

#include "vbh_comm/vbh_comm_def_export.h"
#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

namespace VBH
{
	//注册时得到的 vbh秘钥 //wrapper对象
	class VBH_COMM_DEF_EXPORT CVbhCryptKeyWrapper
	{
	public:
		CVbhCryptKeyWrapper(CDscString& strSvrPubKey, VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl);

	public:
		DSC_BIND_ATTR(m_strSvrPubKey, m_userKey, m_transUrl);

	public:
		CDscString& m_strSvrPubKey;
		VBH::CVbhAllocatedKey& m_userKey;
		VBH::CTransactionUrl& m_transUrl;
	};

	//用户秘钥 //
	class VBH_COMM_DEF_EXPORT CUserCryptKeyWrapper
	{
	public:
		CUserCryptKeyWrapper(CDscString& strCltPriKey, CDscString& strSvrPubKey, CDscString& strEnvelopeKey,
			VBH::CVbhAllocatedKey& userKey, VBH::CTransactionUrl& transUrl, ACE_UINT32& nChannelID);

	public:
		DSC_BIND_ATTR(m_strCltPriKey, m_strSvrPubKey, m_strEnvelopeKey, m_userKey, m_transUrl, m_nChannelID);

	public:
		CDscString& m_strCltPriKey;
		CDscString& m_strSvrPubKey;
		CDscString& m_strEnvelopeKey;
		VBH::CVbhAllocatedKey& m_userKey;
		VBH::CTransactionUrl& m_transUrl;
		ACE_UINT32& m_nChannelID;
	};

	//用户的短UUID
	using CUserShortUuidWrapper = CVbhAllocatedKey;

	//生成1对Bin格式的公钥和私钥(不可见字符)
	VBH_COMM_DEF_EXPORT ACE_INT32 CreateBinCryptKeyPair(CDscString& strPubKey, CDscString& strPriKey);

	//生成1对Hex编码的公钥和私钥
	VBH_COMM_DEF_EXPORT ACE_INT32 CreateHexCryptKeyPair(CDscString& strPubKey, CDscString& strPriKey);

	//生成base64编码的 user-crypt-key
	VBH_COMM_DEF_EXPORT ACE_INT32 GenerateUserCryptKey(CDscString& strUserCryptKey, 
		const char* pPriKey, const size_t nPriKeyLen, const char* pVbhCryptKey, const size_t nVbhCryptKeyLen, ACE_UINT32 nChannelID);

	//对输入的字符串进行hex编码 //返回的pOutBuf要使用 DSC_THREAD_SIZE_FREE 释放
	size_t HexEncode(char*& pOutBuf, const char* pszIn, const size_t nInLen);

	//返回的pOutBuf要使用 DSC_THREAD_SIZE_FREE 释放
	//TYPE中不能包含blob类型的成员变量
	template<typename TYPE>
	size_t HexEncode(char*& pOutBuf, TYPE& t);

	//对输入的字符串进行Base64解码 //返回的pOutBuf要使用 DSC_THREAD_FREE 释放
	VBH_COMM_DEF_EXPORT ACE_INT32 HexDecode(char*& pOutBuf, size_t& nOutLen, const char* pszIn, const size_t nInLen);

	//TYPE中不能包含blob类型的成员变量
	template<typename TYPE>
	ACE_INT32 HexDecode(TYPE& t, const char* pszIn, const size_t nInLen);

	//----------------------------------

	//对输入的字符串进行Base64编码 //返回的pOutBuf要使用 DSC_THREAD_FREE 释放
	size_t Base64Encode(char*& pOutBuf, const char* pszIn, const size_t nInLen);

	//返回的pOutBuf要使用 DSC_THREAD_FREE 释放
	//TYPE中不能包含blob类型的成员变量
	template<typename TYPE>
	size_t Base64Encode(char*& pOutBuf, TYPE& t);

	//对输入的字符串进行Base64解码 //返回的pOutBuf要使用 DSC_THREAD_FREE 释放
	VBH_COMM_DEF_EXPORT ACE_INT32 Base64Decode(char*& pOutBuf, size_t& nOutLen, const char* pszIn, const size_t nInLen);

	//TYPE中不能包含blob类型的成员变量
	template<typename TYPE>
	ACE_INT32 Base64Decode(TYPE& t, const char* pszIn, const size_t nInLen);

}

#include "vbh_comm/vbh_key_codec.inl"

#endif // !VBH_KEY_CODEC_H_LKJERLJEW5435L43J6543J643JL4KHJK34H5
