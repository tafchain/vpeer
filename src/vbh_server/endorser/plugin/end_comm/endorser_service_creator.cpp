#include "ace/OS_NS_strings.h"
#include "ace/DLL_Manager.h"
#include "ace/OS_NS_dlfcn.h"

#include "dsc/service/dsc_service_container.h"
#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"
#include "dsc/dispatcher/dsc_dispatcher_center.h"

#include "end_comm/endorser_service_creator.h"


//创建inner-cc对象的函数指针
extern "C" typedef void* (*CreateInnerCC) (ACE_UINT32 nChannelID);

//对应于数据库中，用于es-cfg(非加密endorser)表 和ces-cfg(加密endorser表)
class CDBEsCfg
{
public:
	CDBEsCfg()
	: m_IpAddr("ES_IP_ADDR")
	, m_port("ES_PORT")
	, m_esID("ES_ID")
	{
	}

public:
	PER_BIND_ATTR(m_IpAddr, m_port, m_esID);

public:
	CColumnWrapper< CDscString > m_IpAddr;
	CColumnWrapper< ACE_INT32 > m_port;
	CColumnWrapper< ACE_INT32 > m_esID;
};

class CDBEsCfgCriterion : public CSelectCriterion
{
public:
	virtual void SetCriterion(CPerSelect& rPerSelect)
	{
		rPerSelect.Where(
			(rPerSelect["NODE_ID"] == CDscAppManager::Instance()->GetNodeID())
			|| (rPerSelect["NODE_ID"] == 0)
		);
	}
};

class CDBInnerCcCfg
{
public:
	CDBInnerCcCfg()
	: m_ccName("CC_NAME")
	, m_channelID("CHANNEL_ID")
	{
	}

public:
	PER_BIND_ATTR(m_ccName, m_channelID);

public:
	CColumnWrapper< CDscString > m_ccName;
	CColumnWrapper< ACE_INT32 > m_channelID;
};

class CDBInnerCcCfgCriterion : public CSelectCriterion
{
public:
	virtual void SetCriterion(CPerSelect& rPerSelect)
	{
		rPerSelect.Where(rPerSelect["INUSE"] == 1);
	}
};

ACE_INT32 CEndorserServiceCreator::ReadDBConfig(es_cfg_list_type& lstEsCfg, inner_cc_cfg_list_type& lstInnerCcCfg, const char* pEsCfgTableName, const char* pInnerCcCfgTableName)
{
	CDscDatabase database;
	CDBConnection dbConnection;

	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");
		return -1;
	}

	CTableWrapper< CCollectWrapper<CDBEsCfg> > lstDBEsCfg(pEsCfgTableName);
	CDBEsCfgCriterion dbEsCfgCriterion;
	CTableWrapper< CCollectWrapper<CDBInnerCcCfg> > lstDBInnerCcCfg(pInnerCcCfgTableName);
	CDBInnerCcCfgCriterion dbInnerCcCriterion;

	if (::PerSelect(lstDBEsCfg, database, dbConnection, &dbEsCfgCriterion))
	{
		DSC_RUN_LOG_ERROR("select from %s failed", pEsCfgTableName);
		return -1;
	}

	if (::PerSelect(lstDBInnerCcCfg, database, dbConnection, &dbInnerCcCriterion))
	{
		DSC_RUN_LOG_ERROR("select from %s failed", pInnerCcCfgTableName);
		return -1;
	}

	CEsCfg esCfg;
	for (auto it = lstDBEsCfg->begin(); it != lstDBEsCfg->end(); ++it)
	{
		esCfg.m_strIpAddr = *it->m_IpAddr;
		esCfg.m_nEsID = (ACE_UINT16)* it->m_esID;
		esCfg.m_nPort = (ACE_UINT16)* it->m_port;

		lstEsCfg.push_back(esCfg);
	}

	CInnerCcCfg ccCfg;
	for (auto it = lstDBInnerCcCfg->begin(); it != lstDBInnerCcCfg->end(); ++it)
	{
		ccCfg.m_strCcName = *it->m_ccName;
		ccCfg.m_nChannelID = (ACE_UINT16)* it->m_channelID;

		lstInnerCcCfg.push_back(ccCfg);
	}

	return 0;
}

ACE_INT32 CEndorserServiceCreator::LoadInnerCcInDLL(const CDscString& strCcName, CInnerCcBaseService** ppzService, ACE_UINT32 nChannelID, const ACE_UINT32 nServiceCount)
{
	const char* pszHomeDir = CDscAppManager::Instance()->GetWorkRoot();
	if (!pszHomeDir)
	{
		DSC_RUN_LOG_ERROR("can't get work directory");
		return -1;
	}

	CDscString strCcPathName(pszHomeDir);

	strCcPathName += DSC_FILE_PATH_SPLIT_CHAR;
	strCcPathName.append(DSC_STRING_TYPE_PARAM("lib"));

	strCcPathName += DSC_FILE_PATH_SPLIT_CHAR;
#if !( defined(ACE_WIN32) || defined(ACE_WIN64) )
	strCcPathName.append(DSC_STRING_TYPE_PARAM("lib"));
#endif
	strCcPathName += strCcName;
#if defined(ACE_WIN32) || defined(ACE_WIN64)
	strCcPathName.append(DSC_STRING_TYPE_PARAM(".dll"));
#else
	strCcPathName.append(DSC_STRING_TYPE_PARAM(".so"));
#endif

	ACE_DLL_Handle* handle = ACE_DLL_Manager::instance()->open_dll(ACE_TEXT(strCcPathName.c_str()), ACE_DEFAULT_SHLIB_MODE, 0);
	if (!handle)
	{
		DSC_RUN_LOG_ERROR("Can't open the plugin:%s, error info:%s.", strCcPathName.c_str(), ACE_OS::dlerror());
		return -1;
	}

	CreateInnerCC pCallBack = reinterpret_cast<CreateInnerCC>(handle->symbol(ACE_TEXT("CreateCommCC")));
	if (!pCallBack)
	{
		DSC_RUN_LOG_ERROR("Don't find the CreateInnerCC interface of plugin:%s.", strCcPathName.c_str());
		return -1;
	}

	for (ACE_UINT32 idx = 0; idx < nServiceCount; ++idx)
	{
		ppzService[idx] = reinterpret_cast<CInnerCcBaseService*>(pCallBack(nChannelID));

		if (!ppzService[idx])
		{
			DSC_RUN_LOG_ERROR("Can't create plugin:%s.", strCcPathName.c_str());
			return -1;
		}
	}

	return 0;
}
