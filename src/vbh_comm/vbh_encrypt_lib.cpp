#include <openssl/err.h>
#include <openssl/hmac.h>

#include "dsc/dsc_log.h"

#include "vbh_comm/vbh_encrypt_lib.h"
#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_wrap_msg_def.h"


//使用国密（SM2、SM3、SM4）方式时，需要在编译时增加宏 VBH_USE_SMX
#define VBH_CIPHER_IV "jfkngkghjfdumgldjvm" //对称加密时的初始化向量

#ifdef VBH_USE_SMX
#define VBH_CURVE NID_sm2
#define VBH_CIPHER EVP_sm4_cbc()
#define VBH_CIPHER_TAG_SIZE 32
#define VBH_HASHER EVP_sm3()
#define VBH_SM2_ID "VBCHAINBLOC@YONGQI.COM"
#define VBH_SM2_ID_LEN (sizeof(VBH_SM2_ID)-1)
#define VBH_SM_HMAC_KEY "ufjnblkhgodhfn"
#define VBH_SM_HMAC_KEY_LEN (sizeof(VBH_SM_HMAC_KEY)-1)
#else
#define VBH_CURVE NID_secp256k1
#define VBH_CIPHER EVP_aes_256_gcm()
#define VBH_CIPHER_TAG_SIZE 16
#define VBH_HASHER EVP_sha256()
#endif


#ifndef DONNT_USING_MEMORY_POOL
void* openssl_malloc(size_t nSize, const char*, int)
{
	return dsc_safe_malloc_impl(nSize);
}

void* openssl_realloc(void* p, size_t nSize, const char*, int)
{
	return dsc_safe_realloc_impl((char*)p, nSize);
}

void openssl_free(void* p, const char*, int)
{
	dsc_safe_free_impl((char*)p);
}
#endif

//#ifdef DONNT_USING_MEMORY_POOL
//void* openssl_malloc(size_t nSize)
//{
//	return ACE_OS::malloc(nSize);
//}
//
//void* openssl_realloc(void* p, size_t nSize)
//{
//	return ACE_OS::realloc(p, nSize);
//}
//
//void openssl_free(void* p)
//{
//	ACE_OS::free(p);
//}
//#else
//void* openssl_malloc(size_t nSize)
//{
//	return dsc_malloc_impl(nSize);
//}
//
//void* openssl_realloc(void* p, size_t nSize)
//{
//	return dsc_realloc_impl((char*)p, nSize);
//}
//
//void openssl_free(void* p)
//{
//	dsc_free_impl((char*)p);
//}
//#endif


void VBH::InitOpenSsl(void)
{
#ifndef DONNT_USING_MEMORY_POOL
	CRYPTO_set_mem_functions(openssl_malloc, openssl_realloc, openssl_free);
#endif
}

EC_GROUP* VBH::vbhEcGroup(void)
{
	EC_GROUP* group = EC_GROUP_new_by_curve_name(VBH_CURVE);

	if (!group)
	{
		DSC_RUN_LOG_INFO("EC_GROUP_new_by_curve_name failed. {error = %s}",
			ERR_error_string(ERR_get_error(), NULL));

		return NULL;
	}

	if (EC_GROUP_precompute_mult(group, NULL) != 1)
	{
		DSC_RUN_LOG_INFO("EC_GROUP_precompute_mult failed. {error = %s}",
			ERR_error_string(ERR_get_error(), NULL));
		EC_GROUP_free(group);

		return NULL;
	}

	EC_GROUP_set_point_conversion_form(group, POINT_CONVERSION_COMPRESSED);

	return group;
}

EC_KEY* VBH::vbhCreateEcKey(const EC_GROUP* pGroup)
{
	EC_KEY* key = EC_KEY_new();

	if (EC_KEY_set_group(key, pGroup) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_set_group failed. {error = %s}", errMsgBuf);

		EC_KEY_free(key);
		return NULL;
	}

	if (EC_KEY_generate_key(key) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_generate_key failed. {error = %s}", errMsgBuf);
		EC_KEY_free(key);
		return NULL;
	}

	return key;
}

char* VBH::vbhGetBinPublicKeyFromEcKey(int& nBinKeyLen, const EC_KEY* key)
{
	const EC_POINT* point = EC_KEY_get0_public_key(key);
	if (NULL == point)
	{
		char errMsgBuf[1024];

		nBinKeyLen = 0;
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_get0_public_key failed. {error = %s}", errMsgBuf);
		
		return NULL;
	}

	const EC_GROUP* group = EC_KEY_get0_group(key);
	if (NULL == group)
	{
		char errMsgBuf[1024];

		nBinKeyLen = 0;
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_get0_group failed. {error = %s}", errMsgBuf);
		
		return NULL;
	}

	//没有直接把point转换为bin的函数，只有先把point转换为big-num，再转换为bin
	BIGNUM* bn = EC_POINT_point2bn(group, point, POINT_CONVERSION_COMPRESSED, NULL, NULL);

	if (bn)
	{
		nBinKeyLen = BN_num_bytes(bn);
		char* pBinKeyBuf = DSC_THREAD_SIZE_MALLOC(nBinKeyLen);

		if (BN_bn2bin(bn, (unsigned char*)pBinKeyBuf) < nBinKeyLen)
		{
			DSC_THREAD_SIZE_FREE(pBinKeyBuf, nBinKeyLen);
			nBinKeyLen = 0;
			pBinKeyBuf = NULL;

			char errMsgBuf[1024];
			ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
			DSC_RUN_LOG_ERROR("BN_bn2bin failed. {error = %s}", errMsgBuf);
		}

		BN_free(bn);

		return pBinKeyBuf;
	}
	else
	{
		char errMsgBuf[1024];

		nBinKeyLen = 0;
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_POINT_point2bn failed. {error = %s}", errMsgBuf);

		return NULL;
	}
}

char* VBH::vbhGetBinPrivateKeyFromEcKey(int& nBinKeyLen, const EC_KEY* key)
{
	const BIGNUM* bn = EC_KEY_get0_private_key(key);
	if (NULL == bn)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_get0_private_key failed. {error = %s}", errMsgBuf);
		return NULL;
	}

	nBinKeyLen = BN_num_bytes(bn);
	char* pBinKeyBuf = DSC_THREAD_SIZE_MALLOC(nBinKeyLen);

	if (BN_bn2bin(bn, (unsigned char*)pBinKeyBuf) < nBinKeyLen)
	{
		DSC_THREAD_SIZE_FREE(pBinKeyBuf, nBinKeyLen);
		nBinKeyLen = 0;
		pBinKeyBuf = NULL;

		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("BN_bn2bin failed. {error = %s}", errMsgBuf);
	}

	return pBinKeyBuf;
}

//EC_POINT_hex2point调用时，输入的字符串必须是以0结尾的
EC_KEY* VBH::vbhCreateEcKeyFromBinPublicKey(const char* pBinKeyBuf, const int nBinKeyLen, const EC_GROUP* pGroup)
{
	EC_KEY* key = EC_KEY_new();

	if (EC_KEY_set_group(key, pGroup) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_set_group failed. {error = %s}", errMsgBuf);

		EC_KEY_free(key);

		return NULL;
	}

	BIGNUM* bn = BN_bin2bn((const unsigned char*)pBinKeyBuf, nBinKeyLen, NULL);
	if (bn)
	{
		EC_POINT* point = EC_POINT_bn2point(pGroup, bn, NULL, NULL);
		if (NULL == point)
		{
			char errMsgBuf[1024];

			BN_free(bn);
			EC_KEY_free(key);
			ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
			DSC_RUN_LOG_ERROR("EC_POINT_bn2point failed. {error = %s}", errMsgBuf);

			return NULL;
		}

		if (EC_KEY_set_public_key(key, point) != 1)
		{
			char errMsgBuf[1024];

			EC_POINT_free(point);
			BN_free(bn);
			EC_KEY_free(key);
			ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
			DSC_RUN_LOG_ERROR("EC_KEY_set_public_key failed. {error = %s}", errMsgBuf);

			return NULL;
		}

		EC_POINT_free(point);
		BN_free(bn);

		return key;
	}
	else
	{
		char errMsgBuf[1024];

		EC_KEY_free(key);
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("BN_bin2bn failed. {error = %s}", errMsgBuf);

		return NULL;
	}
}

//BN_hex2bn 调用时，输入的字符串必须是以0结尾的
EC_KEY* VBH::vbhCreateEcKeyFromBinPrivateKey(const char* pBinKeyBuf, const int nBinKeyLen, const EC_GROUP* pGroup)
{
	EC_KEY* key = EC_KEY_new();

	if (EC_KEY_set_group(key, pGroup) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_set_group failed. {error = %s}", errMsgBuf);

		EC_KEY_free(key);

		return NULL;
	}

	BIGNUM* bn = BN_bin2bn((const unsigned char*)pBinKeyBuf, nBinKeyLen, NULL);
	if (bn == NULL)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("BN_bin2bn failed. {error = %s}", errMsgBuf);
		EC_KEY_free(key);

		return NULL;
	}

	if (EC_KEY_set_private_key(key, bn) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_set_private_key failed. {error = %s}", errMsgBuf);
		EC_KEY_free(key);
		BN_free(bn);

		return NULL;
	}

	BN_free(bn);

#ifdef VBH_USE_SMX
	//在使用国密时，需要这一步调用，使用私钥在key中生成公钥
	if (EC_KEY_generate_key(key) != 1)
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_generate_key failed. {error = %s}", errMsgBuf);
		EC_KEY_free(key);
		return NULL;
	}
#endif

	return key;
}



char* VBH::vbhGenEcdhEnvKey(int& nEnvKeyLen, const char* pszRemoteBinKey, const int nRemoteBinKeyLen, const EC_KEY* pLocalEcKey)
{
	char* pEnvKeyBuf = nullptr;
	const EC_GROUP* group = EC_KEY_get0_group(pLocalEcKey);

	if (group)
	{
		BIGNUM* bn = BN_bin2bn((const unsigned char*)pszRemoteBinKey, nRemoteBinKeyLen, NULL);

		if (bn)
		{
			EC_POINT* point = EC_POINT_bn2point(group, bn, NULL, NULL);

			if (point)
			{
				char arrTempEnvKey[VBH_ENVELOPE_KEY_LENGTH * 2];

				nEnvKeyLen = ECDH_compute_key(arrTempEnvKey, sizeof(arrTempEnvKey), point, pLocalEcKey, NULL);
				if (nEnvKeyLen > 0)
				{
					pEnvKeyBuf = DSC_THREAD_SIZE_MALLOC(nEnvKeyLen);
					::memcpy(pEnvKeyBuf, arrTempEnvKey, nEnvKeyLen);
				}
				else
				{
					char errMsgBuf[1024];
					ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
					DSC_RUN_LOG_ERROR("ECDH_compute_key failed. {error = %s}", errMsgBuf);
				}

				EC_POINT_free(point);
			}
			else
			{
				char errMsgBuf[1024];
				ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
				DSC_RUN_LOG_ERROR("EC_POINT_bn2point failed. {error = %s}", errMsgBuf);
			}

			BN_free(bn);
		}
		else
		{
			char errMsgBuf[1024];
			ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
			DSC_RUN_LOG_ERROR("BN_bin2bn failed. {error = %s}", errMsgBuf);
		}

		//group 不用释放
	}
	else
	{
		char errMsgBuf[1024];
		ERR_error_string_n(ERR_get_error(), errMsgBuf, sizeof(errMsgBuf));
		DSC_RUN_LOG_ERROR("EC_KEY_get0_group failed. {error = %s}", errMsgBuf);
	}

	return pEnvKeyBuf;
}

#ifdef VBH_USE_SMX //使用国密

void VBH::vbhDigest(const char* data, size_t n, char* digest)
{
	EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
	const EVP_MD* md = EVP_sm3();
	unsigned int mdLen = VBH_BLOCK_DIGEST_LENGTH;

	EVP_DigestInit_ex(mdCtx, md, NULL);
	EVP_DigestUpdate(mdCtx, data, n);
	EVP_DigestFinal_ex(mdCtx, (unsigned char*)digest, &mdLen);
	EVP_MD_CTX_free(mdCtx);
}

char* VBH::vbhEncrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen)
{
	unsigned char* outbuf = (unsigned char*)DSC_THREAD_MALLOC(nDataLen + EVP_MAX_BLOCK_LENGTH * 2 + sizeof(ACE_UINT16));
	ACE_UINT16 nMacLen = 0; //消息后面mac数据的长度，mac数据长度放在缓冲区最前面，占2字节
	unsigned char* encDataBuf = outbuf + sizeof(ACE_UINT16);
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

	if (EVP_CipherInit_ex(ctx, VBH_CIPHER, NULL, envelope_key, (const unsigned char*)VBH_CIPHER_IV, 1) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	if (EVP_EncryptUpdate(ctx, encDataBuf, &nOutLen, (unsigned char*)data, nDataLen) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	int tmplen = 0;
	if (EVP_EncryptFinal_ex(ctx, encDataBuf + nOutLen, &tmplen) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	EVP_CIPHER_CTX_free(ctx);
	nOutLen += tmplen;

	//生成加密数据后，再将未加密数据的MAC放到加密数据后
	HMAC_CTX* mac_ctx = HMAC_CTX_new();

	if (HMAC_Init_ex(mac_ctx, VBH_SM_HMAC_KEY, VBH_SM_HMAC_KEY_LEN, VBH_HASHER, NULL) != 1)
	{
		HMAC_CTX_free(mac_ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	if (HMAC_Update(mac_ctx, (unsigned char*)data, nDataLen) != 1)
	{
		HMAC_CTX_free(mac_ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	tmplen = 0;
	if (HMAC_Final(mac_ctx, (unsigned char*)encDataBuf + nOutLen, (unsigned int*)& tmplen) != 1)
	{
		HMAC_CTX_free(mac_ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	HMAC_CTX_free(mac_ctx);
	nOutLen += tmplen;
	nMacLen = (ACE_UINT16)tmplen;

	DSC::EncodeItemNoOff(nMacLen, outbuf);
	nOutLen += sizeof(ACE_UINT16);

	return (char*)outbuf;
}

char* VBH::vbhDecrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen)
{
	char* encData = data;
	ACE_UINT16 nMacLen;

	DSC::DscDecodeItem(nMacLen, encData);

	if (nDataLen > (nMacLen + sizeof(ACE_UINT16)))
	{
		int nEncDataLen = nDataLen - sizeof(ACE_UINT16) - nMacLen;
		char* macData = encData + nEncDataLen;
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
		unsigned char* outbuf = (unsigned char*)DSC_THREAD_MALLOC(nDataLen + EVP_MAX_BLOCK_LENGTH * 2);

		if (EVP_CipherInit_ex(ctx, VBH_CIPHER, NULL, envelope_key, (const unsigned char*)VBH_CIPHER_IV, 0) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		if (EVP_DecryptUpdate(ctx, outbuf, &nOutLen, (unsigned char*)encData, nEncDataLen) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		int tmplen = 0;
		if (EVP_DecryptFinal_ex(ctx, outbuf + nOutLen, &tmplen) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		EVP_CIPHER_CTX_free(ctx);
		nOutLen += tmplen;

		//数据解密后，再对解密的数据生成MAC，和加密数据后携带的MAC比较，一致，则解密成功
		HMAC_CTX* mac_ctx = HMAC_CTX_new();

		if (HMAC_Init_ex(mac_ctx, VBH_SM_HMAC_KEY, VBH_SM_HMAC_KEY_LEN, VBH_HASHER, NULL) != 1)
		{
			HMAC_CTX_free(mac_ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		if (HMAC_Update(mac_ctx, (unsigned char*)outbuf, (unsigned long)nOutLen) != 1)
		{
			HMAC_CTX_free(mac_ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		tmplen = 0;
		unsigned char newMacDataBuf[EVP_MAX_MD_SIZE];
		if (HMAC_Final(mac_ctx, newMacDataBuf, (unsigned int*)& tmplen) != 1)
		{
			HMAC_CTX_free(mac_ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		if (::memcmp(newMacDataBuf, macData, nMacLen) != 0)
		{
			HMAC_CTX_free(mac_ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		HMAC_CTX_free(mac_ctx);

		return (char*)outbuf;
	}
	else
	{
		return NULL;
	}
}

char* VBH::vbhSign(EC_KEY* pEcKey, char* buff, int buff_len, unsigned int& nSignature)
{
	EVP_PKEY* pkey = EVP_PKEY_new();

	if (EVP_PKEY_assign_EC_KEY(pkey, pEcKey) != 1)
	{
		EVP_PKEY_free(pkey);

		return NULL;
	}

	if (EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2) != 1)
	{
		EVP_PKEY_free(pkey);

		return NULL;
	}

	EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
	if (EVP_PKEY_CTX_set1_id(pkey_ctx, VBH_SM2_ID, VBH_SM2_ID_LEN) <= 0)
	{
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);
		return NULL;
	}

	EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
	EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

	//准备摘要签名; 必须在每次签名前都进行调用
	if (EVP_DigestSignInit(md_ctx, NULL, VBH_HASHER, NULL, pkey) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return NULL;
	}

	//对数据进行摘要
	if (EVP_DigestSignUpdate(md_ctx, buff, buff_len) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return NULL;
	}

	//计算最终签名数据需要的长度，
	size_t nTempLen = 0; //需要的sig-buf长度
	if (EVP_DigestSignFinal(md_ctx, NULL, &nTempLen) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return NULL;
	}

	unsigned char* pSignature = (unsigned char*)DSC_THREAD_MALLOC(nTempLen);

	//执行真正的签名
	if (EVP_DigestSignFinal(md_ctx, pSignature, &nTempLen) != 1)
	{
		DSC_THREAD_FREE((char*)pSignature);
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return NULL;
	}
	nSignature = (unsigned int)nTempLen;

	return (char*)pSignature;
}

bool VBH::vbhVerifySign(EC_KEY* pEcKey, char* buff, size_t buff_len, char* pSignature, const int nSignature)
{
	EVP_PKEY* pkey = EVP_PKEY_new();

	if (EVP_PKEY_assign_EC_KEY(pkey, pEcKey) != 1)
	{
		EVP_PKEY_free(pkey);

		return false;
	}

	if (EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2) != 1)
	{
		EVP_PKEY_free(pkey);

		return false;
	}

	EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pkey, NULL);
	if (EVP_PKEY_CTX_set1_id(pkey_ctx, VBH_SM2_ID, VBH_SM2_ID_LEN) <= 0)
	{
		EVP_PKEY_free(pkey);
		return false;
	}

	EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
	EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

	//准备验签; 必须在每次验签前都调用
	if (EVP_DigestVerifyInit(md_ctx, NULL, VBH_HASHER, NULL, pkey) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return false;
	}

	//对数据进行摘要
	if (EVP_DigestVerifyUpdate(md_ctx, buff, buff_len) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return false;
	}

	//执行验签
	if (EVP_DigestVerifyFinal(md_ctx, (unsigned char*)pSignature, nSignature) != 1)
	{
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_CTX_free(pkey_ctx);
		EVP_PKEY_free(pkey);

		return false;
	}

	EVP_MD_CTX_free(md_ctx);
	EVP_PKEY_CTX_free(pkey_ctx);
	EVP_PKEY_free(pkey);

	return true;
}

#else //使用aes-gcm; ecdsa
char* VBH::vbhEncrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen)
{
	unsigned char* outbuf = (unsigned char*)DSC_THREAD_MALLOC(nDataLen + EVP_MAX_BLOCK_LENGTH * 2);
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

	//EVP_CipherInit_ex 最后1个参数为1表示加密
	if (EVP_CipherInit_ex(ctx, VBH_CIPHER, NULL, envelope_key, (const unsigned char*)VBH_CIPHER_IV, 1) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	if (EVP_EncryptUpdate(ctx, outbuf, &nOutLen, (unsigned char*)data, nDataLen) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}

	int tmplen = 0;
	if (EVP_EncryptFinal_ex(ctx, outbuf + nOutLen, &tmplen) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}
	nOutLen += tmplen;

	//在加密结束后，将tag数据填充到加密数据的末尾
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, VBH_CIPHER_TAG_SIZE, outbuf + nOutLen) != 1)
	{
		EVP_CIPHER_CTX_free(ctx);
		DSC_THREAD_FREE((char*)outbuf);

		return NULL;
	}
	nOutLen += VBH_CIPHER_TAG_SIZE;

	EVP_CIPHER_CTX_free(ctx);

	return (char*)outbuf;
}

char* VBH::vbhDecrypt(const unsigned char* envelope_key, int& nOutLen, char* data, const int nDataLen)
{
	if (nDataLen > VBH_CIPHER_TAG_SIZE)
	{
		unsigned char* outbuf = (unsigned char*)DSC_THREAD_MALLOC(nDataLen + EVP_MAX_BLOCK_LENGTH * 2);
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

		//EVP_CipherInit_ex 最后1个参数为0表示解密
		if (EVP_CipherInit_ex(ctx, VBH_CIPHER, NULL, envelope_key, (const unsigned char*)VBH_CIPHER_IV, 0) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		//在解密开始前，首先获取加密后填充的tag数据，在解密过程中用作校验
		if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, VBH_CIPHER_TAG_SIZE, data + nDataLen - VBH_CIPHER_TAG_SIZE) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		if (EVP_DecryptUpdate(ctx, outbuf, &nOutLen, (unsigned char*)data, nDataLen - VBH_CIPHER_TAG_SIZE) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		int tmplen = 0;
		if (EVP_DecryptFinal_ex(ctx, outbuf + nOutLen, &tmplen) != 1)
		{
			EVP_CIPHER_CTX_free(ctx);
			DSC_THREAD_FREE((char*)outbuf);

			return NULL;
		}

		nOutLen += tmplen;
		EVP_CIPHER_CTX_free(ctx);

		return (char*)outbuf;
	}
	else
	{
		return NULL;
	}
}
#endif

