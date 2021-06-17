#ifndef I_INNER_CC_SERVICE_H_846413165746413213244643123216567484
#define I_INNER_CC_SERVICE_H_846413165746413213244643123216567484

#include "vbh_comm/vbh_comm_msg_def.h"

//cc在形式上分为inner-cc和outer-cc
//inner-cc由我司自行编写，继承自inner-service基类；
//inner-cc service编译得到的so文件会被endorser-app加载，并和endoser运行于一个容器中，因此可和endorser直接通过函数调用；
//outer-cc可以由第三方编写，总体上继承自outer-service；
//outer-cc将会开放api，支持python,js等写cc；
//----------------------------------------
//客户在提案中指定的cc-id, 
// 在指定inner-cc时，是指CC的service-id（inner-cc和endorser做了背靠背处理，创建时指定了统一的service-type, cc-id被用作了service-id;
// endorser发送消息到cc时，不走dsc总线传输消息，而是函数调用，cc-id仅用来区分是哪个inner-cc）;
// 在指定outter-cc时，是指CC的service-type.
//inner-cc和endorser一起做了对等集群，outer-cc在将来也会做对等集群，因此，service-type比service-id能定位cc

class IInnerCcService
{
public:
	virtual ACE_INT32 SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req) = 0;
	virtual ACE_INT32 SendProposeEsCcReq(VBH::CProposeEsCcReq& req) = 0;
	virtual ACE_INT32 SendQueryEsCcReq(VBH::CQueryEsCcReq& req) = 0;
};

#endif