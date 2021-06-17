#ifndef VBH_UNITE_DQUEUE_H_287340832904832903290732094780329748
#define VBH_UNITE_DQUEUE_H_287340832904832903290732094780329748

#include "ace/Basic_Types.h"

template<typename TYPE>
class CVbhUniteDqueue
{
public:
	CVbhUniteDqueue();

public:
	TYPE* Front(void);
	TYPE* Back(void);

	TYPE* Prev(TYPE* pNode);
	TYPE* Next(TYPE* pNode);
	bool IsEmpty(void) const;
	size_t Size(void) const;
	TYPE* GetAt(const ACE_UINT32 nIndex);

public:
	void PushFront(TYPE* pNode);
	TYPE* PopBack(void);

	void PushBack(TYPE* pNode);
	TYPE* PopFront(void);

	void Insert(TYPE* pNewNode, TYPE* pOriNode);
	TYPE* Erase(TYPE* pNode);
	bool IsNodeInQueue(TYPE* pNode);

private:
	//void Assign(const CDscDqueue<TYPE>& rDqueue);
	CVbhUniteDqueue(const CVbhUniteDqueue<TYPE>& rDqueue) = delete;
	CVbhUniteDqueue<TYPE>& operator= (const CVbhUniteDqueue<TYPE>& rDqueue) = delete;

private:
	TYPE* m_pFront = NULL;
	TYPE* m_pBack = NULL;
};

#include "vbh_server_comm/vbh_unite_dqueue.inl"

#endif

