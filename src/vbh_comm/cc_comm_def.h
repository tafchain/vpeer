#ifndef CC_EXPLORER_QUERY_DEF_H_9874654351889451684986158548
#define CC_EXPLORER_QUERY_DEF_H_9874654351889451684986158548

#include "vbh_comm/comm_msg_def/vbh_comm_class_def.h"

enum
{
	EN_QUERY_ACTION_QUERY_BLOCK_COUNT = 1,
	EN_QUERY_ACTION_QUERY_BLOCK_HEADER_INFO, //
	EN_QUERY_ACTION_QUERY_BLOCK_TRANS_LIST,		


	EN_QUERY_ACTION_QUERY_WRITE_SET,
	EN_QUERY_ACTION_QUERY_TRANS_INFO,
	EN_QUERY_ACTION_QUERY_BLOCK_TRANS_COUNT,


	EN_USER_MIN_QUERY_ACTION_ID = 64 //用户可用的最小 查询action-id
};


enum
{
	EN_HASH_MAP_BITES = 16,
	EN_PROPOSE_BATCH_QUERY_HASH_MAP_BITS = 10,
	EN_SESSION_TIMEOUT_VALUE = 60
};

enum //写集的表现类型
{
	EN_WS_USER_TYPE = 1,
	EN_WS_INFORMATION_TYPE
};

//不会在session中使用，仅用作临时变量，所以成员使用Blob类型 //提案时使用
class CProposeSimpleWriteSet
{
public:
	DSC::CDscShortBlob m_key;
	DSC::CDscShortBlob m_value;
};
using CProposeSimpleUser = CProposeSimpleWriteSet;
using CProposeSimpleInformation = CProposeSimpleWriteSet;

//不会在session中使用，仅用作临时变量，所以成员使用Blob类型 //查询时使用
class CQuerySimpleWriteSet
{
public:
	ACE_UINT32 m_nVersion = 0;
	DSC::CDscShortBlob m_key;
	DSC::CDscShortBlob m_value;
};
using CQuerySimpleUser = CQuerySimpleWriteSet;
using CQuerySimpleInformation = CQuerySimpleWriteSet;

//不会在session中使用，仅用作临时变量，所以成员使用Blob类型 //查询时使用
class CQuerySimpleTransaction
{
public:
	VBH::CSimpleVector<CQuerySimpleWriteSet> m_vecWs; //写集
	ACE_UINT32 m_nActionID;
	DSC::CDscShortBlob m_transKey; //transaction-key
	DSC::CDscShortBlob m_userKey;  //事务发起者
	DSC::CDscShortBlob m_signature;
	DSC::CDscShortBlob m_proposal;
};

class CCcWsKV
{
public:
	ACE_UINT32 m_nVersion = 0; //写集数据的版本号；0表示初始版本，每次设置+1
	ACE_UINT64 m_nAllocatedID = 0; //系统分配的 用户ID 或 information-ID

public: //使用 CDscUnboundQueue 容器时，必须具备的模板接口
	CCcWsKV* m_pNext;
};


#endif
