#include "dsc/dsc_comm_def.h"

#include "vbh_comm/vbh_comm_error_code.h"

const char* VBH::GetErrorString(int nErrorCode)
{
	static const char* pOkString = "ok";

	//common 错误码
	static const char* pArrCommonErrorString[EN_COMMON_ERROR_CODE_NUM] = {
		"common error begin value", //对应 EN_MAX_COMMON_ERROR_VALUE
		"not ready error", //未准备就绪
		"reentry error", //重入错误，重复提交错误
		"timeout error", //超时错误
		"network error", //网络错误
		"vbh system inner error", //服务器内部错误
		"decode error", //解码错误
		"cannot find session in memory, most because of timeout",
		"invalid input param, please check your input carefully", //系统使用者输入的参数不合法
		"there is logic failed, please wait a moment and try agin, or check your input param", //逻辑失败错误
		"order service is not ready, please try again later", //order端未准备就绪
		"this transaction not support query", //不支持查询的事务类型
		nullptr
	};

	//config 错误码
	static const char* pArrConfigErrorString[EN_CONFIG_ERROR_CODE_NUM] = {
		"config error begin value", //对应 EN_MAX_COMMON_ERROR_VALUE
		"cannot find committer belong to this channel", //未找到指定channel的committer
		"cannot find order-cps belong to this channel", //未找到指定channel的order-cps地址
		nullptr
	};

	//crypt 错误码
	static const char* pArrCryptErrorString[EN_CRYPT_ERROR_CODE_NUM] = {
		"crypt error begin value", //对应 EN_MAX_COMMON_ERROR_VALUE
		"encrypt error, please check your encrypt key.", //加密错误
		"decrypt error, please check your decrypt key.", //解密错误
		"verify signature failed, this is a invalid access.", //验证签名错误
		"nonce verify faild, this must be a hostile attack", //随机数校验失败错误
		nullptr
	};

	//persistence 错误码
	static const char* pArrPersistenceErrorString[EN_PERSISTENCE_ERROR_CODE_NUM] = {
		"persistence error begin value", //对应 EN_MAX_COMMON_ERROR_VALUE
		"block cache in order-cps is full, may be distribute block encoter problem.", // order-cps的 block cache已满
		"version is conflic when update world state", //流水号不一致，发生冲突 错误
		"write-set system id used up", //用户的系统ID耗尽
		"read const length table failed", //读取定长存储表失败
		"modify(append/update) const length table failed", //修改定长存储表失败
		"write disk failed",//写磁盘失败
		nullptr
	};

	//cc 错误码
	static const char* pArrCcErrorString[EN_CC_ERROR_CODE_NUM] = {
		"cc error begin value", //对应 EN_MAX_COMMON_ERROR_VALUE
		"cc common error", //cc侧的通用错误
		"cc refused regist user", //cc拒绝用户注册
		nullptr
	};


	if (nErrorCode == VBH::EN_OK_TYPE)
	{
		return pOkString;
	}
	else if ((nErrorCode <= EN_MAX_COMMON_ERROR_VALUE) && (nErrorCode > (EN_MAX_COMMON_ERROR_VALUE - EN_COMMON_ERROR_CODE_NUM)))
	{
		nErrorCode -= EN_MAX_COMMON_ERROR_VALUE;
		DSC_ABS(nErrorCode); //将nErrorCode转变为数组下标

		return pArrCommonErrorString[nErrorCode];
	}
	else if ((nErrorCode <= EN_MAX_CONFIG_ERROR_VALUE) && (nErrorCode > (EN_MAX_CONFIG_ERROR_VALUE - EN_CONFIG_ERROR_CODE_NUM)))
	{
		nErrorCode -= EN_MAX_CONFIG_ERROR_VALUE;
		DSC_ABS(nErrorCode); //将nErrorCode转变为数组下标

		return pArrConfigErrorString[nErrorCode];
	}
	else if ((nErrorCode <= EN_MAX_CRYPT_ERROR_VALUE) && (nErrorCode > (EN_MAX_CRYPT_ERROR_VALUE - EN_CRYPT_ERROR_CODE_NUM)))
	{
		nErrorCode -= EN_MAX_CRYPT_ERROR_VALUE;
		DSC_ABS(nErrorCode); //将nErrorCode转变为数组下标

		return pArrCryptErrorString[nErrorCode];
	}
	else if ((nErrorCode <= EN_MAX_PERSISTENCE_ERROR_VALUE) && (nErrorCode > (EN_MAX_PERSISTENCE_ERROR_VALUE - EN_PERSISTENCE_ERROR_CODE_NUM)))
	{
		nErrorCode -= EN_MAX_PERSISTENCE_ERROR_VALUE;
		DSC_ABS(nErrorCode); //将nErrorCode转变为数组下标

		return pArrPersistenceErrorString[nErrorCode];
	}
	else if ((nErrorCode <= EN_MAX_CC_ERROR_VALUE) && (nErrorCode > (EN_MAX_CC_ERROR_VALUE - EN_CC_ERROR_CODE_NUM)))
	{
		nErrorCode -= EN_MAX_CC_ERROR_VALUE;
		DSC_ABS(nErrorCode); //将nErrorCode转变为数组下标

		return pArrCcErrorString[nErrorCode];
	}
	else
	{
		return nullptr;
	}
}