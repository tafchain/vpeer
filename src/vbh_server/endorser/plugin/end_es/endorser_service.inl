
inline void CEndorserService::CRegistUserSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CEndorserService::CRegistUserSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline void CEndorserService::CCreateInformationSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CEndorserService::CCreateInformationSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline void CEndorserService::CProposeSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CEndorserService::CProposeSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline CMcpServerHandler* CEndorserService::AllocMcpHandler(ACE_HANDLE handle)
{
	return DSC_THREAD_DYNAMIC_TYPE_NEW(CEndorserServiceHandler) CEndorserServiceHandler(*this, handle, this->AllocHandleID());
}

inline void CEndorserService::CQueryUserInfoSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CEndorserService::CQueryUserInfoSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline void CEndorserService::CQueryTransactionSession::OnTimer(void)
{
	m_rEndorserService.OnTimeOut(this);
}

inline void CEndorserService::CQueryTransactionSession::OnNetError(void)
{
	m_rEndorserService.OnNetError(this);
}

inline void CEndorserService::SetTransferAgentService(ITransferAgentService* pTas)
{
	m_pTas = pTas;
}

template <typename SESSION_TYPE>
inline void CEndorserService::OnRelease(SESSION_TYPE* pSession)
{
	pSession->m_pEndorserServiceHandler->m_arrUserSession.Erase(pSession);
	DSC_THREAD_TYPE_DELETE(pSession);
}

template<typename SESSION_TYPE>
void CEndorserService::SetMcpHandleSession(SESSION_TYPE* pSession, CMcpHandler* pMcpHandler)
{
	CEndorserServiceHandler* pEndorserServiceHandler = (CEndorserServiceHandler* )pMcpHandler;

	pSession->m_pEndorserServiceHandler = pEndorserServiceHandler;
	pEndorserServiceHandler->m_arrUserSession.Insert(pSession);
}
