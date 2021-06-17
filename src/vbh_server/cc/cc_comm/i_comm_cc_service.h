#ifndef I_COMM_CC_SERVICE_H_785642144234654
#define I_COMM_CC_SERVICE_H_785642144234654

#include "vbh_comm/cc_comm_def.h"

//IOuterCcService�����ⲿ�ĺ�Լ����
class ICommCcService
{
public:
	//��֤������ָ�û��Ƿ����᰸�����û� //�����������û�һ���ǰ� �᰸�û���ʽ���еı���
	virtual ACE_INT32 VerifyProposeUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey) = 0;

	//��֤�����Ƿ��Ƿ����ѯ���û�
	virtual ACE_INT32 VerifyQueryUser(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob& userKey) = 0;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û���Ϣ
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetUserAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//�����ϲ��߼�ʹ�ã� ��ȡ��Ϣ
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetInformationAtPropose(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û���Ϣ
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetUserAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//�����ϲ��߼�ʹ�ã� ��ȡ��Ϣ
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;
	virtual ACE_INT32 GetInformationAtQuery(const ACE_UINT32 nCcSessionID, const DSC::CDscShortBlob* pKeyVec, const ACE_UINT16 nVecLen) = 0;

	//�����ϲ��߼�ʹ�ã� ��ȡ�û�������Ϣ
	virtual ACE_INT32 GetTransAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;

	virtual ACE_INT32 GetInformationHistoryAtQuery(const ACE_UINT32 nCcSessionID, DSC::CDscShortBlob& key) = 0;

	//�����ϲ��߼�ʹ�ã� ע�����������
	virtual ACE_INT32 RegistUserRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, char* pUserInitInfo, const size_t nUserInitInfo) = 0;

	//�����ϲ��߼�ʹ�ã� �᰸������� //TODO: ��̨CC����ͨ���˺����޸����մ���������е��᰸����
	virtual ACE_INT32 ProposalRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID,
		const CProposeSimpleUser* pUserArry, const ACE_UINT16 nUserArrayLen,
		const CProposeSimpleInformation* pInfoArry = nullptr, const ACE_UINT16 nInfoArrayLen = 0,
		const DSC::CDscShortBlob& receipt = DSC::CDscShortBlob(nullptr, 0)) = 0;

	//��ѯӦ��
	virtual ACE_INT32 QueryRsp(const ACE_INT32 nReturnCode, const ACE_UINT32 nCcSessionID, const DSC::CDscBlob& info) = 0;

};



#endif