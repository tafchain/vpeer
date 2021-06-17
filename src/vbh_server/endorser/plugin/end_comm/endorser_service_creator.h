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
		ACE_UINT16 m_nChannelID; //channel-id就是inner-cc-type
		CDscString m_strCcName;
	};

	//cc service 信息
	class CInnerCcServiceInfo
	{
	public:
		ACE_UINT16 m_nCcType = 0; //TODO: 目前暂时用cc-type作为cc所处的channel，后面会增加配置表，指定cc-type所属的channel
		ACE_UINT32 m_nServiceCount = 0; //service的个数
		CInnerCcBaseService** m_ppzService = NULL; //CCcService的指针数组
	};

public: 
	template <typename ES_TYPE>
	ACE_INT32 CreateEs(void); 

	//TODO: 如果要支持动态卸载，需要在退出时，释放创建的所有cc类	

private:
	using es_cfg_list_type = dsc_list_type(CEsCfg);
	using inner_cc_cfg_list_type = dsc_list_type(CInnerCcCfg);

private:
	ACE_INT32 ReadDBConfig(es_cfg_list_type& lstEsCfg, inner_cc_cfg_list_type& lstInnerCcCfg, const char* pEsCfgTableName, const char* pInnerCcCfgTableName);

	//从动态库中加载inner-cc-service服务，并指定，一次创建多少个服务
	ACE_INT32 LoadInnerCcInDLL(const CDscString& strCcName, CInnerCcBaseService** ppzService, ACE_UINT32 nChannelID, const ACE_UINT32 nServiceCount);
}; 

#include "end_comm/endorser_service_creator.inl"

#endif