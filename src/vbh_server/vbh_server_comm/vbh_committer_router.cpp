#include "ace/OS_NS_strings.h"

#include "vbh_comm/vbh_comm_func.h"
#include "vbh_comm/vbh_comm_id_def.h"

#include "vbh_server_comm/vbh_committer_router.h"


ACE_INT32 VBH::CCommitterServiceRouter::Open()
{
	if (LoadChannelMapCsAddr())
	{
		DSC_RUN_LOG_ERROR("load channel map x committer serivce addr failed.");

		return -1;
	}

	return 0;
}

void VBH::CCommitterServiceRouter::Close(void)
{
	CDscAddr* pAddr;
	for (auto it = m_mapChannelMapCsAddr.begin(); it != m_mapChannelMapCsAddr.end();)
	{
		pAddr = it.second;
		++it;
		DSC_THREAD_TYPE_DELETE(pAddr);
	}
}

ACE_INT32 VBH::CCommitterServiceRouter::GetCsAddr(CDscMsg::CDscMsgAddr& addr, const ACE_UINT32 nChannelID)
{
	//查找用户对应的tcs地址
	CDscAddr* pAddr = m_mapChannelMapCsAddr.Find(nChannelID);

	if (pAddr)
	{
		addr.SetNodeType(VBH::EN_COMMITTER_APP_TYPE);
		addr.SetNodeID(pAddr->m_nNodeID);
		addr.SetServiceType(VBH::EN_COMMITTER_SERVICE_TYPE);
		addr.SetServiceID(pAddr->m_nServiceID);

		return 0;
	}
	else
	{
		return -1;
	}
}

class CVbhCommonCsConfig
{
public:
	CVbhCommonCsConfig()
	: m_nodeID("NODE_ID")
	, m_csID("CS_ID")
	, m_channelID("CH_ID")
	{
	}

public:
	PER_BIND_ATTR(m_nodeID, m_csID, m_channelID);

public:
	CColumnWrapper< ACE_INT32 > m_nodeID;
	CColumnWrapper< ACE_INT32 > m_csID;
	CColumnWrapper< ACE_UINT32 > m_channelID;
};

ACE_INT32 VBH::CCommitterServiceRouter::LoadChannelMapCsAddr()
{
	CDscDatabase database;
	CDBConnection dbConnection;
	ACE_INT32 nRet = CDscDatabaseFactoryDemon::instance()->CreateDatabase(database, dbConnection);

	if (nRet)
	{
		DSC_RUN_LOG_ERROR("connect database failed.");

		return -1;
	}

	CTableWrapper< CCollectWrapper<CVbhCommonCsConfig> > lstCfg("CS_CFG");

	nRet = ::PerSelect(lstCfg, database, dbConnection);
	if (nRet)
	{
		DSC_RUN_LOG_ERROR("select from CS_CFG failed");

		return -1;
	}
	else
	{
		CDscAddr* pAddr;
		ACE_UINT32 nChannelID;

		for (auto it = lstCfg->begin(); it != lstCfg->end(); ++it)
		{
			pAddr = DSC_THREAD_TYPE_NEW(CDscAddr) CDscAddr;

			pAddr->m_nNodeID = *it->m_nodeID;
			pAddr->m_nServiceID = *it->m_csID;
			nChannelID = *it->m_channelID;

			m_mapChannelMapCsAddr.DirectInsert(nChannelID, pAddr);
		}
	}

	return 0;
}
