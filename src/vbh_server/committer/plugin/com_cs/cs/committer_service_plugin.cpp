#include "dsc/dispatcher/dsc_dispatcher_center.h"
#include "dsc/service/dsc_service_container.h"
#include "dsc/db/per/persistence.h"
#include "dsc/dsc_database_factory.h"

#include "com_cs/cs/committer_service.h"
#include "com_cs/cs/committer_service_factory.h"
#include "com_cs/cs/committer_service_plugin.h"
#include "com_cs/cas/committer_agent_service.h"
#include "com_cs/cas/committer_agent_service_factory.h"

class CCsConfig
{
public:
	CCsConfig()
		: m_csID("CS_ID")
		, m_casIpAddr("CAS_IP_ADDR")
		, m_casPort("CAS_PORT")
		, m_channelID("CH_ID")
	{
	}

public:
	PER_BIND_ATTR(m_csID, m_casIpAddr, m_casPort, m_channelID);

public:
	CColumnWrapper< ACE_INT32 > m_csID;
	CColumnWrapper< CDscString > m_casIpAddr;
	CColumnWrapper< ACE_INT32 > m_casPort;
	CColumnWrapper< ACE_INT32 > m_channelID;
};

class CxcsCriterion : public CSelectCriterion
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

ACE_INT32 CCommitterServicePlugin::OnInit(void)
{ 
	CDscDatabase database;
	CDBConnection dbConnection;

	if (CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection))
	{
		DSC_RUN_LOG_ERROR("connect database failed.");

		return -1;
	}
	else
	{
		CTableWrapper< CCollectWrapper<CCsConfig> > lstCfg("CS_CFG");
		CxcsCriterion criterion;

		if (::PerSelect(lstCfg, database, dbConnection, &criterion))
		{
			DSC_RUN_LOG_ERROR("select from XCS_CFG failed.");

			return -1;
		}
		else
		{
			CCommitterService* pCommitterService;
			CCommitterAgentService* pCommitterAgentService;
			
			CDscReactorServiceContainerFactory dscReactorServiceContainerFactory;
			ACE_UINT16 nContainerID = 1;

			for (auto it = lstCfg->begin(); it != lstCfg->end(); ++it)
			{
				//2.×¢²ácontainer
				CDscDispatcherCenterDemon::instance()->AcquireWrite();
				IDscTask* pDscServiceContainer = CDscDispatcherCenterDemon::instance()->GetDscTask_i(VBH::EN_COMMITTER_SERVICE_CONTAINER_TYPE, nContainerID);
				if (!pDscServiceContainer)
				{
					pDscServiceContainer = dscReactorServiceContainerFactory.CreateDscServiceContainer();
					if (CDscDispatcherCenterDemon::instance()->RegistDscTask_i(pDscServiceContainer, VBH::EN_COMMITTER_SERVICE_CONTAINER_TYPE, nContainerID))
					{
						DSC_RUN_LOG_ERROR("regist endorser container error, type:%d, id:%d.", VBH::EN_COMMITTER_SERVICE_CONTAINER_TYPE, nContainerID);
						CDscDispatcherCenterDemon::instance()->Release();

						return -1;
					}
				}
				if (!pDscServiceContainer)
				{
					DSC_RUN_LOG_ERROR("cann't create container.");
					CDscDispatcherCenterDemon::instance()->Release();

					return -1;
				}
				CDscDispatcherCenterDemon::instance()->Release();
				++nContainerID;

				const ACE_UINT16 nCsID = (ACE_UINT16)*it->m_csID;

				DSC_NEW(pCommitterService, CCommitterService(*it->m_channelID, *it->m_casIpAddr, *it->m_casPort));
				pCommitterService->SetType(CCommitterService::EN_SERVICE_TYPE);
				pCommitterService->SetID(nCsID);

				DSC_NEW(pCommitterAgentService, CCommitterAgentService(*it->m_channelID, *it->m_casIpAddr, *it->m_casPort));
				pCommitterAgentService->SetType(CCommitterAgentService::EN_SERVICE_TYPE);
				pCommitterAgentService->SetID(nCsID);

				pCommitterService->SetCommitterAgentService(pCommitterAgentService);
				pCommitterAgentService->SetCommitterService(pCommitterService);

				CCommitterServiceFactory csFactory(pCommitterService);
				CDscSynchCtrlMsg ctrlCcFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &csFactory);

				if (pDscServiceContainer->PostDscMessage(&ctrlCcFactory))
				{
					DSC_RUN_LOG_ERROR("Failed to push queue at container:%d.", VBH::EN_COMMITTER_SERVICE_CONTAINER_TYPE);

					return -1;
				}

				CCommitterAgentServiceFactory agentServiceFactory(pCommitterAgentService);
				CDscSynchCtrlMsg ctrlCcAgentFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &agentServiceFactory);

				if (pDscServiceContainer->PostDscMessage(&ctrlCcAgentFactory))
				{
					DSC_RUN_LOG_ERROR("Failed to push queue at container:%d.", VBH::EN_COMMITTER_SERVICE_CONTAINER_TYPE);

					return -1;
				}
			}
		}

		DSC_RUN_LOG_INFO("regist committer service succeed");
		return 0;
	}
} 

#ifndef DSC_TEST
extern "C" PLUGIN_EXPORT void* CreateDscPlugin(void)
{
	CCommitterServicePlugin* pPlugIn = NULL;
	
	DSC_NEW(pPlugIn, CCommitterServicePlugin);

	return pPlugIn;
}
#endif
