inline void CTransferAgentService::CSubmitRegistUserTransactionSession::OnTimer(void)
{
	m_rTas.OnTimeOut(this);
}

inline void CTransferAgentService::CSubmitRegistUserTransactionSession::OnRelease(void)
{
	DSC_THREAD_TYPE_DELETE(this);
}

inline void CTransferAgentService::CSubmitProposalTransactionSession::OnTimer(void)
{
	m_rTas.OnTimeOut(this);
}

inline void CTransferAgentService::CSubmitProposalTransactionSession::OnRelease(void)
{
	DSC_THREAD_TYPE_DELETE(this);
}



inline void CTransferAgentService::CQueryLeaderCpsSession::OnTimer(void)
{
	m_rTas.OnTimeOut(this);
}


inline void CTransferAgentService::SetEndorserService(IEndorserService* pEs)
{
	m_pEs = pEs;
}
