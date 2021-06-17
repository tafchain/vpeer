#ifndef I_INNER_CC_SERVICE_H_846413165746413213244643123216567484
#define I_INNER_CC_SERVICE_H_846413165746413213244643123216567484

#include "vbh_comm/vbh_comm_msg_def.h"

//cc����ʽ�Ϸ�Ϊinner-cc��outer-cc
//inner-cc����˾���б�д���̳���inner-service���ࣻ
//inner-cc service����õ���so�ļ��ᱻendorser-app���أ�����endoser������һ�������У���˿ɺ�endorserֱ��ͨ���������ã�
//outer-cc�����ɵ�������д�������ϼ̳���outer-service��
//outer-cc���Ὺ��api��֧��python,js��дcc��
//----------------------------------------
//�ͻ����᰸��ָ����cc-id, 
// ��ָ��inner-ccʱ����ָCC��service-id��inner-cc��endorser���˱�������������ʱָ����ͳһ��service-type, cc-id��������service-id;
// endorser������Ϣ��ccʱ������dsc���ߴ�����Ϣ�����Ǻ������ã�cc-id�������������ĸ�inner-cc��;
// ��ָ��outter-ccʱ����ָCC��service-type.
//inner-cc��endorserһ�����˶Եȼ�Ⱥ��outer-cc�ڽ���Ҳ�����Եȼ�Ⱥ����ˣ�service-type��service-id�ܶ�λcc

class IInnerCcService
{
public:
	virtual ACE_INT32 SendRegistUserEsCcReq(VBH::CRegistUserEsCcReq& req) = 0;
	virtual ACE_INT32 SendProposeEsCcReq(VBH::CProposeEsCcReq& req) = 0;
	virtual ACE_INT32 SendQueryEsCcReq(VBH::CQueryEsCcReq& req) = 0;
};

#endif