#include "dsc/dispatcher/dsc_dispatcher_center.h"

#include "cc_comm/inner_cc_service_factory.h"
#include "end_comm/endorser_service_common_factory.h"
#include "end_tas/transfer_agent_service.h"
#include "end_tas/transfer_agent_service_factory.h"

template <typename ES_TYPE>
ACE_INT32 CEndorserServiceCreator::CreateEs(void)
{
	//0. �����ݿ��������
	es_cfg_list_type lstEsCfg;
	inner_cc_cfg_list_type lstInnerCcCfg;	
	CDscString strEsConfigTableName;

	ES_TYPE::GetConfigTableName(strEsConfigTableName);
	if (ReadDBConfig(lstEsCfg, lstInnerCcCfg, strEsConfigTableName.c_str(), "INNER_CC_CFG"))
	{
		DSC_RUN_LOG_ERROR("read config from database failed.");
		return -1;
	}

	//1. �������е�inner-cc-service, ����endorser-service�ĸ�����ÿ��inner-cc-serviceҪ�������
	CInnerCcServiceInfo* pzInnerCcInfo = nullptr;

	//���� inner-cc-service-info ���飬������ÿ��Ԫ���е�inner-cc-serviceָ������
	if (lstInnerCcCfg.size() > 0)
	{
		DSC_THREAD_TYPE_ALLOCATE_ARRAY(pzInnerCcInfo, lstInnerCcCfg.size());
		for (ACE_UINT32 idx = 0; idx < lstInnerCcCfg.size(); ++idx)
		{
			pzInnerCcInfo[idx].m_nServiceCount = ACE_UINT32(lstEsCfg.size());
			DSC_THREAD_TYPE_ALLOCATE_ARRAY(pzInnerCcInfo[idx].m_ppzService, pzInnerCcInfo[idx].m_nServiceCount);
		}
	}

	ACE_UINT32 nCcIdx = 0;
	ACE_UINT32 nChannelID;
	for (auto it = lstInnerCcCfg.begin(); it != lstInnerCcCfg.end(); ++it, ++nCcIdx)
	{
		pzInnerCcInfo[nCcIdx].m_nCcType = it->m_nChannelID;
		nChannelID = it->m_nChannelID;
		if (LoadInnerCcInDLL(it->m_strCcName, pzInnerCcInfo[nCcIdx].m_ppzService, nChannelID, pzInnerCcInfo[nCcIdx].m_nServiceCount))
		{
			DSC_RUN_LOG_ERROR("load inner cc from dll failed, cc-name:%s, create-count:%d.", it->m_strCcName.c_str(), pzInnerCcInfo[nCcIdx].m_nServiceCount);
			return -1;
		}
	}

	CDscReactorServiceContainerFactory dscReactorServiceContainerFactory;
	IDscTask* pDscServiceContainer = nullptr;
	IEndorserCcService* pIInnerCcService;
	CInnerCCServiceFactory innerCcFactory;
	ACE_UINT32 nEsIdx = 0; //��������Ϊÿ��endorser-service����������inner-cc-service
	ACE_UINT16 nContainerID = 1;

	for (auto it = lstEsCfg.begin(); it != lstEsCfg.end(); ++it, ++nEsIdx)
	{
		//2.ע��container
		CDscDispatcherCenterDemon::instance()->AcquireWrite();
		pDscServiceContainer = CDscDispatcherCenterDemon::instance()->GetDscTask_i(ES_TYPE::EN_SERVICE_CONTAINER_TYPE, nContainerID);
		if (!pDscServiceContainer)
		{
			pDscServiceContainer = dscReactorServiceContainerFactory.CreateDscServiceContainer();
			if (CDscDispatcherCenterDemon::instance()->RegistDscTask_i(pDscServiceContainer, ES_TYPE::EN_SERVICE_CONTAINER_TYPE, nContainerID))
			{
				DSC_RUN_LOG_ERROR("regist endorser container error, type:%d, id:%d.", ES_TYPE::EN_SERVICE_CONTAINER_TYPE, nContainerID);
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


		//3.1.����endorser-service + tranfer-agent-service
		ES_TYPE* pEndorserService;
		CTransferAgentService* pTransferAgentService; 
		CEndorserServiceCommonFactory esFactory;
		CTransferAgentServiceFactory tasFactory;
		std::list< std::pair<ACE_UINT16, IEndorserCcService*> > lstInnerCcService; //endorser-srevice��Ҫ��inner-cc-service-map

		DSC_NEW(pEndorserService, ES_TYPE(it->m_strIpAddr, it->m_nPort));
		DSC_NEW(pTransferAgentService, CTransferAgentService);

		pEndorserService->SetType(ES_TYPE::EN_SERVICE_TYPE);
		pEndorserService->SetID(it->m_nEsID);
		pEndorserService->SetTransferAgentService(pTransferAgentService);
		esFactory.m_pEndorserService = pEndorserService;

		pTransferAgentService->SetType(ES_TYPE::EN_AGENT_SERVICE_TYPE);
		pTransferAgentService->SetID(it->m_nEsID);
		pTransferAgentService->SetEndorserService(pEndorserService);
		tasFactory.m_pTas = pTransferAgentService;

		//3.2 ���ÿ��endorser-service����Ϊ�䴴�����е�inner-cc-service, ͬʱע�����е� inner-cc-service
		for (nCcIdx = 0; nCcIdx < lstInnerCcCfg.size(); ++nCcIdx)
		{
			innerCcFactory.m_pInnerCCService = pzInnerCcInfo[nCcIdx].m_ppzService[nEsIdx];
			innerCcFactory.m_pInnerCCService->SetType(pzInnerCcInfo[nCcIdx].m_nCcType);
			//Ϊ�˵��ԣ�ΪCC��ʱ������һ��ֵ���Ժ����
			innerCcFactory.m_pInnerCCService->SetID(DSC::EN_INVALID_ID);
			//innerCcFactory.m_pInnerCCService->SetID(it->m_nEsID + 200);
			innerCcFactory.m_pInnerCCService->SetEndorserService(pEndorserService); //Ϊÿ��cc���ú��䱳������endorser

			pIInnerCcService = dynamic_cast<IEndorserCcService*>(innerCcFactory.m_pInnerCCService); //ǿ��ת���ӿ����͵�inner-cc-service
			if (pIInnerCcService == nullptr)
			{
				DSC_RUN_LOG_ERROR("dynamic_cast failed.");

				return -1;
			}

			lstInnerCcService.push_back(std::pair<ACE_UINT16, IEndorserCcService*>(pzInnerCcInfo[nCcIdx].m_nCcType, pIInnerCcService));

			CDscSynchCtrlMsg ctrlCcFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &innerCcFactory);

			if (pDscServiceContainer->PostDscMessage(&ctrlCcFactory))
			{
				DSC_RUN_LOG_ERROR("Failed to push queue at container:%d.", ES_TYPE::EN_SERVICE_CONTAINER_TYPE);

				return -1;
			}
		}
		pEndorserService->SetInnerCcServiceMap(lstInnerCcService);

		//4.ע��endorser-service + tranfer-agent-service
		CDscSynchCtrlMsg ctrlMsg4EsFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &esFactory);

		if (pDscServiceContainer->PostDscMessage(&ctrlMsg4EsFactory))
		{
			DSC_RUN_LOG_ERROR("When regist endoser-service-factory, Failed to push queue at container:%d.", ES_TYPE::EN_SERVICE_CONTAINER_TYPE);

			return -1;
		}

		CDscSynchCtrlMsg ctrlMsg4TasFactory(DSC::EN_REGIST_SERVICE_CONTAINER_MSG, &tasFactory);

		if (pDscServiceContainer->PostDscMessage(&ctrlMsg4TasFactory))
		{
			DSC_RUN_LOG_ERROR("When regist transfer-agent-service-factory, Failed to push queue at container:%d.", ES_TYPE::EN_SERVICE_CONTAINER_TYPE);

			return -1;
		}
	}

	//5. �ͷ���ʱ���ٵ��ڴ�
	if (pzInnerCcInfo)
	{
		for (ACE_UINT32 idx = 0; idx < lstInnerCcCfg.size(); ++idx)
		{
			DSC_THREAD_TYPE_DEALLOCATE_ARRAY(pzInnerCcInfo[idx].m_ppzService, pzInnerCcInfo[idx].m_nServiceCount);
		}
		DSC_THREAD_TYPE_DEALLOCATE_ARRAY(pzInnerCcInfo, lstInnerCcCfg.size());
	}

	DSC_RUN_LOG_FINE("regist endorser service succeed");

	return 0;
}

