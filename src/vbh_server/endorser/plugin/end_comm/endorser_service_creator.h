#ifndef ENDORSER_SERVICE_CREATOR_H_9879646541354984351325645648
#define ENDORSER_SERVICE_CREATOR_H_9879646541354984351325645648

#include "dsc/plugin/i_dsc_plugin.h"
#include "dsc/container/dsc_string.h"
#include "dsc/mem_mng/dsc_stl_type.h"

#include "end_comm/endorser_common_export.h"
#include "cc/cc_comm/cc_base_service.h"


class ENDORSER_COMMON_EXPORT CEndorserServiceCreator
{ 
private:
	class CEsCfg
	{
	public:
		ACE_UINT16 m_nPort;
		ACE_UINT16 m_nEsID;
		CDscString m_strIpAddr;
	};

	class CInnerCcCfg
	{
	public:
		ACE_UINT16 m_nChannelID; //channel-id����inner-cc-type
		CDscString m_strCcName;
	};

	//cc service ��Ϣ
	class CInnerCcServiceInfo
	{
	public:
		ACE_UINT16 m_nCcType = 0; //TODO: Ŀǰ��ʱ��cc-type��Ϊcc������channel��������������ñ�ָ��cc-type������channel
		ACE_UINT32 m_nServiceCount = 0; //service�ĸ���
		CInnerCcBaseService** m_ppzService = NULL; //CCcService��ָ������
	};

public: 
	template <typename ES_TYPE>
	ACE_INT32 CreateEs(void); 

	//TODO: ���Ҫ֧�ֶ�̬ж�أ���Ҫ���˳�ʱ���ͷŴ���������cc��	

private:
	using es_cfg_list_type = dsc_list_type(CEsCfg);
	using inner_cc_cfg_list_type = dsc_list_type(CInnerCcCfg);

private:
	ACE_INT32 ReadDBConfig(es_cfg_list_type& lstEsCfg, inner_cc_cfg_list_type& lstInnerCcCfg, const char* pEsCfgTableName, const char* pInnerCcCfgTableName);

	//�Ӷ�̬���м���inner-cc-service���񣬲�ָ����һ�δ������ٸ�����
	ACE_INT32 LoadInnerCcInDLL(const CDscString& strCcName, CInnerCcBaseService** ppzService, ACE_UINT32 nChannelID, const ACE_UINT32 nServiceCount);
}; 

#include "end_comm/endorser_service_creator.inl"

#endif