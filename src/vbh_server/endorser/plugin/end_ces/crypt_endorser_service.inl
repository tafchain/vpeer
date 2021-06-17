inline void CCryptEndorserService::CRegistUserSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CCryptEndorserService::CRegistUserSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}


inline void CCryptEndorserService::CProposeSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CCryptEndorserService::CProposeSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline CMcpServerHandler* CCryptEndorserService::AllocMcpHandler(ACE_HANDLE handle)
{
	return DSC_THREAD_DYNAMIC_TYPE_NEW(CEndorserServiceHandler) CEndorserServiceHandler(*this, handle, this->AllocHandleID());
}

inline void CCryptEndorserService::CQuerySession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CCryptEndorserService::CQuerySession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline void CCryptEndorserService::CLoadCcSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CCryptEndorserService::CLoadCcSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}


inline void CCryptEndorserService::SetTransferAgentService(ITransferAgentService* pTas)
{
	m_pTas = pTas;
}

template <typename SESSION_TYPE>
inline void CCryptEndorserService::OnRelease(SESSION_TYPE* pSession)
{
	pSession->m_pEndorserServiceHandler->m_arrUserSession.Erase(pSession);
	DSC_THREAD_TYPE_DELETE(pSession);
}

template<typename SESSION_TYPE>
inline void CCryptEndorserService::SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler)
{
	CEndorserServiceHandler* pEndorserServiceHandler = (CEndorserServiceHandler* )pMcpHandler;

	pSession->m_pEndorserServiceHandler = pEndorserServiceHandler;
	pEndorserServiceHandler->m_arrUserSession.Insert(pSession);
}

inline ACE_UINT32 CCryptEndorserService::AllocSessionID(void)
{//¹æ±Ü0
	return ++m_nSessionID ? m_nSessionID : ++m_nSessionID;
}

inline bool CCryptEndorserService::CUserKeyAsMapKey::operator< (const CUserKeyAsMapKey& key) const
{
	if (m_nChannelID == key.m_nChannelID)
	{
		return m_nUserAllocatedID < key.m_nUserAllocatedID;
	}
	else
	{
		return m_nChannelID < key.m_nChannelID;
	}
}
