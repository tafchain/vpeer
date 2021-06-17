#ifndef VBH_ENCRYPT_LIB_H_7FAC6382B1DD11E9B5D660F18A3A20D1
#define VBH_ENCRYPT_LIB_H_7FAC6382B1DD11E9B5D660F18A3A20D1

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/sha.h>

#include "dsc/mem_mng/dsc_allocator.h"

#include "vbh_comm/vbh_comm_def_export.h"


//哈希摘要的长度
#ifdef VBH_USE_SMX
#define VBH_BLOCK_DIGEST_LENGTH 32  // crypto/include/internal/sm3.h 中定义 SM3_DIGEST_LENGTH 值为32
#define VBH_ENVELOPE_KEY_LENGTH 32  //信封秘钥长度, 长度为定长的
#define VBH_PUBLIC_KEY_LENGTH 33 //公钥最大长度 //openssl1.1.1.d时，sm2计算公钥长度  BIN:33, HEX:66
#define VBH_PRIVATE_KEY_LENGTH 32 //私钥最大长度 //openssl1.1.1.d时，sm2计算私钥长度  BIN:32, HEX:64
#else
#define VBH_BLOCK_DIGEST_LENGTH SHA256_DIGEST_LENGTH 
#define VBH_ENVELOPE_KEY_LENGTH 32  //信封秘钥长度, 长度为定长的
#define VBH_PUBLIC_KEY_LENGTH 33 //公钥最大长度 //openssl1.1.1.d时，计算公钥长度  BIN:33, HEX:66
#define VBH_PRIVATE_KEY_LENGTH 32 //私钥最大长度 //openssl1.1.1.d时，计算私钥长度  BIN:32, HEX:64
#endif

namespace VBH
{
	//初始化openssl库
	VBH_COMM_DEF_EXPORT void InitOpenSsl(void);

	VBH_COMM_DEF_EXPORT EC_GROUP* vbhEcGroup(void);
	VBH_COMM_DEF_EXPORT EC_KEY* vbhCreateEcKey(const EC_GROUP* pGroup);
	
	VBH_COMM_DEF_EXPORT char* vbhGetBinPublicKeyFromEcKey(int& nBinKeyLen, const EC_KEY* key);
	VBH_COMM_DEF_EXPORT char* vbhGetBinPrivateKeyFromEcKey(int& nBinKeyLen, const EC_KEY* key);

	VBH_COMM_DEF_EXPORT EC_KEY* vbhCreateEcKeyFromBinPublicKey(const char* pBinKeyBuf, const int nBinKeyLen, const EC_GROUP* pGroup);
	VBH_COMM_DEF_EXPORT EC_KEY* vbhCreateEcKeyFromBinPrivateKey(const char* pBinKeyBuf, const int nBinKeyLen, const EC_GROUP* pGroup);

	//计算ECDH算法中的对称秘钥（信封秘钥）：使用远程公钥，本地EcKey计算信封秘钥
	//pszRemoteHexKey 必须是0结尾字符串
	//返回的字符串使用 DSC_THREAD_SIZE_FREE 释放
	VBH_COMM_DEF_EXPORT char* vbhGenEcdhEnvKey(int& nEnvKeyLen, const char* pszRemoteBinKey, const int nRemoteBinKeyLen, const EC_KEY* pLocalEcKey);

	//对称加密函数; 返回的指针在使用完毕后需要调用 DSC_THREAD_FREE 进行释放
	VBH_COMM_DEF_EXPORT char* vbhEncrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen);

	//对称解密函数; 返回的指针在使用完毕后需要调用 DSC_THREAD_FREE 进行释放
	VBH_COMM_DEF_EXPORT char* vbhDecrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen);

#ifdef VBH_USE_SMX //使用国密时需要导出符号
	//签名函数；//pEcKey 在 vbh_sign 中会被释放
	VBH_COMM_DEF_EXPORT char* vbhSign(EC_KEY* pEcKey, char* buff, int buff_len, unsigned int& nSignature);

	//验签函数；//pEcKey 在 vbh_verify_sign 中会被释放
	VBH_COMM_DEF_EXPORT bool vbhVerifySign(EC_KEY* pEcKey, char* buff, size_t buff_len, char* pSignature, const int nSignature);

	//计算哈希摘要函数; 内联，不用导出
	VBH_COMM_DEF_EXPORT void vbhDigest(const char* data, size_t n, char* digest);

#else //使用aes-gcm时，需要内联
	//签名函数；//pEcKey 在 vbh_sign 中会被释放
	char* vbhSign(EC_KEY* pEcKey, char* buff, int buff_len, unsigned int& nSignature);

	//验签函数；//pEcKey 在 vbh_verify_sign 中会被释放
	bool vbhVerifySign(EC_KEY* pEcKey, char* buff, size_t buff_len, char* pSignature, const int nSignature);

	//计算哈希摘要函数; 内联，不用导出
	void vbhDigest(const char* data, size_t n, char* digest);

#endif

	//直接使用字符串形式公钥和私钥的签名验签函数 //方便直接调用
	char* vbhSign(char* pPriKeyBuf, int nPriKeyLen, EC_GROUP* pGroup, char* pDataBuf, size_t nDataLen, unsigned int& nSignatureLen);
	bool vbhVerifySign(char* pPubKeyBuf, int nPubKeyLen, EC_GROUP* pGroup, char* pDataBuf, size_t nDataLen, char* pSignatureBuf, size_t nSignatureLen);
}

#include "vbh_comm/vbh_encrypt_lib.inl"

#endif
