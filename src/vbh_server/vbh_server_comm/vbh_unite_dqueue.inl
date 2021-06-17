#include "ace/Assert.h"

#include "dsc/mem_mng/dsc_allocator.h"

template<typename TYPE>
CVbhUniteDqueue<TYPE>::CVbhUniteDqueue()
: m_pFront(NULL)
, m_pBack(NULL)
{
}

//template<typename TYPE>
//void CDscDqueue<TYPE>::Assign(const CDscDqueue<TYPE>& rDqueue)
//{
//	CDscDqueue<TYPE>& r = const_cast< CDscDqueue<TYPE>& >(rDqueue);
//
//	m_pFront = NULL;
//	m_pBack = NULL;
//
//	TYPE* pNode = r.Front();
//	TYPE* pTempNode;
//
//	while(pNode)
//	{
//		pTempNode = pNode;
//		this->PushBack(pTempNode);
//		pNode = r.Next(pNode);
//	}
//}

//template<typename TYPE>
//CDscDqueue<TYPE>::CDscDqueue(const CDscDqueue<TYPE>& rDqueue)
//: m_pFront(NULL)
//, m_pBack(NULL)
//{
//	this->Assign(rDqueue);
//}

//template<typename TYPE>
//CDscDqueue<TYPE>& CDscDqueue<TYPE>::operator= (const CDscDqueue<TYPE>& rDqueue)
//{
//	if(this != &rDqueue)
//	{
//		this->Assign(rDqueue);
//	}
//
//	return *this;
//}


template<typename TYPE>
inline bool CVbhUniteDqueue<TYPE>::IsEmpty(void) const
{
#ifndef NDEBUG
	if(NULL == m_pFront)
	{
		ACE_ASSERT(NULL == m_pBack);
	}
#endif

	return NULL == m_pFront;
}

template<typename TYPE>
inline TYPE* CVbhUniteDqueue<TYPE>::Front(void)
{
	return m_pFront;
}

template<typename TYPE>
inline TYPE* CVbhUniteDqueue<TYPE>::Prev(TYPE* pNode)
{
	ACE_ASSERT(pNode);

	return pNode->m_pDqueuePrev;
}

template<typename TYPE>
inline TYPE* CVbhUniteDqueue<TYPE>::Next(TYPE* pNode)
{
	ACE_ASSERT(pNode);

	return pNode->m_pDqueueNext;
}

template<typename TYPE>
TYPE* CVbhUniteDqueue<TYPE>::GetAt(const ACE_UINT32 nIndex)
{
	TYPE* pNode = m_pFront;
	ACE_UINT32 i = 0;

	for(;;)
	{
		if(nIndex == i)
		{
			break;
		}
		++i;

		pNode = pNode->m_pDqueueNext;
		if(!pNode)
		{
			break;
		}
	}

	if(i == nIndex)
	{
		return pNode;
	}

	return NULL;
}

template<typename TYPE>
inline TYPE* CVbhUniteDqueue<TYPE>::Back(void)
{
	return m_pBack;
}

template<typename TYPE>
void CVbhUniteDqueue<TYPE>::PushFront(TYPE* pNode)
{
	ACE_ASSERT(pNode);

#ifndef NDEBUG
	TYPE* pDI = this->Front();
	while(pDI)
	{
		ACE_ASSERT(pDI != pNode);
		pDI = this->Next(pDI);
	}
#endif

	if(m_pFront)
	{
		ACE_ASSERT(NULL == m_pFront->m_pDqueuePrev);

		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = m_pFront;
		m_pFront->m_pDqueuePrev = pNode;
		m_pFront = pNode;
	}
	else
	{
		ACE_ASSERT(!m_pBack);
		m_pFront = pNode;
		m_pBack = pNode;
		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = NULL;
	}
}

template<typename TYPE>
TYPE* CVbhUniteDqueue<TYPE>::PopBack(void)
{
	if(m_pBack)
	{
		ACE_ASSERT(!m_pBack->m_pDqueueNext);

		TYPE* pNode = m_pBack;

		m_pBack = m_pBack->m_pDqueuePrev;
		if(m_pBack)
		{
			m_pBack->m_pDqueueNext = NULL;
		}
		else
		{
			ACE_ASSERT(m_pFront == pNode);
			m_pFront = NULL;
		}

		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = NULL;

		return pNode;
	}

	return NULL;
}

template<typename TYPE>
void CVbhUniteDqueue<TYPE>::PushBack(TYPE* pNode)
{
	ACE_ASSERT(pNode);

	if(m_pBack)
	{
		ACE_ASSERT(NULL == m_pBack->m_pDqueueNext);

		pNode->m_pDqueueNext = NULL;
		pNode->m_pDqueuePrev = m_pBack;
		m_pBack->m_pDqueueNext = pNode;
		m_pBack = pNode;
	}
	else
	{
		ACE_ASSERT(!m_pFront);
		m_pFront = pNode;
		m_pBack = pNode;
		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = NULL;
	}
}

template<typename TYPE>
TYPE* CVbhUniteDqueue<TYPE>::PopFront(void)
{
	if(m_pFront)
	{
		ACE_ASSERT(!m_pFront->m_pDqueuePrev);

		TYPE* pNode = m_pFront;

		m_pFront = m_pFront->m_pDqueueNext;
		if(m_pFront)
		{
			m_pFront->m_pDqueuePrev = NULL;
		}
		else
		{
			ACE_ASSERT(m_pBack == pNode);
			m_pBack = NULL;
		}

		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = NULL;

		return pNode;
	}

	return NULL;
}

template<typename TYPE>
inline bool CVbhUniteDqueue<TYPE>::IsNodeInQueue(TYPE* pNode)
{
	return pNode->m_pDqueuePrev || pNode->m_pDqueueNext || m_pFront == pNode;
}

template<typename TYPE>
void CVbhUniteDqueue<TYPE>::Insert(TYPE* pNewNode, TYPE* pOriNode)
{
	ACE_ASSERT(pNewNode);
	ACE_ASSERT(pOriNode);

	TYPE*& pFront = m_pFront;
	if (pFront == pOriNode)
	{
		this->PushFront(pNewNode);
	}
	else
	{
		pNewNode->m_pDqueueNext = pOriNode;
		pNewNode->m_pDqueuePrev = pOriNode->m_pDqueuePrev;
		pOriNode->m_pDqueuePrev->m_pDqueueNext = pNewNode;
		pOriNode->m_pDqueuePrev = pNewNode;
	}
}

template<typename TYPE>
TYPE* CVbhUniteDqueue<TYPE>::Erase(TYPE* pNode)
{
	TYPE* pRetNode;

	ACE_ASSERT(pNode);

	if(pNode->m_pDqueuePrev)
	{
		ACE_ASSERT(pNode->m_pDqueuePrev->m_pDqueueNext == pNode);
		if(pNode->m_pDqueueNext)
		{
			ACE_ASSERT(pNode->m_pDqueueNext->m_pDqueuePrev == pNode);
			pNode->m_pDqueuePrev->m_pDqueueNext = pNode->m_pDqueueNext;
			pNode->m_pDqueueNext->m_pDqueuePrev = pNode->m_pDqueuePrev;

			pRetNode = pNode->m_pDqueueNext;
		}
		else
		{
			//说明pNode是最后一个元素
			ACE_ASSERT(m_pBack == pNode);
			m_pBack = pNode->m_pDqueuePrev;
			m_pBack->m_pDqueueNext = NULL;
			pRetNode = NULL;
		}

		pNode->m_pDqueuePrev = NULL;
		pNode->m_pDqueueNext = NULL;
	}
	else
	{
		//说明pNode是第一个元素，需要调整m_pFront
		if(pNode->m_pDqueueNext)
		{
			m_pFront = pNode->m_pDqueueNext;
			m_pFront->m_pDqueuePrev = NULL;
			pRetNode = pNode->m_pDqueueNext;

			pNode->m_pDqueuePrev = NULL;
			pNode->m_pDqueueNext = NULL;
		}
		else
		{
			if(m_pFront == pNode)
			{
				m_pFront = NULL;
				m_pBack = NULL;
			}
			else
			{//不是本队列元素，不理
			}

			pRetNode = NULL;
		}
	}

	return pRetNode;
}

//绝大部分场合用不到
template<typename TYPE>
size_t CVbhUniteDqueue<TYPE>::Size(void) const
{
	size_t n = 0;

	TYPE* pNode = m_pFront;
	while(pNode)
	{
		++n;
		pNode = pNode->m_pDqueueNext;
	}

	return n;
}
