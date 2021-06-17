#ifndef I_COMM_CC_SERVICE_H_785642144234654
#define I_COMM_CC_SERVICE_H_785642144234654

#include "vbh_comm/cc_comm_def.h"

//IOuterCcService用于外部的合约交流
class ICommCcService
{
public:
	//验证参数所指用户是否是提案发起用户 //参数所传递用户一定是按 提案用户格式进行的编码
	virtual ACE_INT32 VerifyProposeUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey) = 0;

	//验证参数是否是发起查询的用户
	virtual ACE_INT32 VerifyQueryUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey) = 0;

	//留给上层逻辑使用： 获取用户信息
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//留给上层逻辑使用： 获取信息
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//留给上层逻辑使用： 获取用户信息
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//留给上层逻辑使用： 获取信息
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//留给上层逻辑使用： 获取用户交易信息
	virtual ACE_INT32 GetTransAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;

	virtual ACE_INT32 GetInformationHistoryAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;

	//留给上层逻辑使用： 注册请求处理完毕
	virtual ACE_INT32 RegistUserRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, char* pUserInitInfo, const size_t nUserInitInfo) = 0;

	//留给上层逻辑使用： 提案处理完毕 //TODO: 中台CC可以通过此函数修改最终打包到区块中的提案内容
	virtual ACE_INT32 ProposalRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID,
		const CProposeSimpleUser* pUserArry, const ACE_UINT16 nUserArrayLen,
		const CProposeSimpleInformation* pInfoArry = nullptr, const ACE_UINT16 nInfoArrayLen = 0,
		const DSC::CDscShortBlob& receipt = DSC::CDscShortBlob(nullptr, 0)) = 0;

	//查询应答
	virtual ACE_INT32 QueryRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const DSC::CDscBlob& info) = 0;

};



#endif