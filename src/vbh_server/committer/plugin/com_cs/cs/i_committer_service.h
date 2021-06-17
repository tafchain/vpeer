#ifndef I_COMMITTER_SERVICE_H_4328792341231123643217234834127
#define I_COMMITTER_SERVICE_H_4328792341231123643217234834127


class ICommitterService
{
public:
	virtual char* ReadBlock(ACE_UINT32& nBlockBufLen, const ACE_UINT64 nBlockID) = 0;
	virtual ACE_INT32 ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID) = 0;
};

#endif
