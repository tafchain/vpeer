#ifndef TEST_DEF_H_2438934279342324838
#define TEST_DEF_H_2438934279342324838

#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "vbh_comm/cc_comm_def.h"

//#include "vbh_comm/vbh_comm_msg_def.h"

namespace TEST_CC
{
	enum
	{
		EN_CREATE_INFORMATION_ACTION_ID = 1,
		EN_ALTER_INFORMATION_ACTION_ID,
		EN_TEST_TRADE_ACTION_ID,
		EN_COMMIT_INFORMATION_ACTION_ID ,
	};

	enum
	{
		EN_QUERY_ACTION_QUERY_USER = EN_USER_MIN_QUERY_ACTION_ID + 1,
		EN_QUERY_ACTION_QUERY_TRANS,
		EN_QUERY_ACTION_QUERY_INFOMATION_HISTORY_LIST
	};


	//注册时，提交到cc,cc审核使用的结构
	class CCcCheckInfoOnUserRegist
	{
	public:
		DSC_BIND_ATTR(m_nAsset, m_userName, m_idCardNo)

	public:
		ACE_UINT32 m_nAsset;
		DSC::CDscShortBlob m_userName;
		DSC::CDscShortBlob m_idCardNo;
	};


	//用户存储在链上的真正信息
	class CUserInfo
	{
	public:
		DSC_BIND_ATTR(m_nAsset)

	public:
		ACE_UINT32 m_nAsset = 0;
	};

	class CCreateInformationAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_CREATE_INFORMATION_ACTION_ID
		};

	public:
		DSC_BIND_ATTR(m_lstInitInfo);

	public:
		DSC::CDscShortList<DSC::CDscShortBlob> m_lstInitInfo;
	};
	class CCommitInformationAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_COMMIT_INFORMATION_ACTION_ID
		};

	public:
		DSC_BIND_ATTR(m_InformationKey, m_InformationValue);

	public:
		DSC::CDscShortBlob m_InformationKey;
		DSC::CDscShortBlob m_InformationValue;
	};

	class CAlterInformationAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_ALTER_INFORMATION_ACTION_ID
		};

	public:
		DSC_BIND_ATTR(m_nAsset, m_nPhoneNo, m_userName, m_address, m_userKey)

	public:
		ACE_UINT32 m_nAsset;
		ACE_UINT64 m_nPhoneNo;
		DSC::CDscShortBlob m_userName;
		DSC::CDscShortBlob m_address;
		DSC::CDscShortBlob m_userKey;
	};

	
	class CTradeAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_TEST_TRADE_ACTION_ID
		};

	public:
		DSC_BIND_ATTR(m_nAsset, m_fromUserKey, m_toUserKey)

	public:
		ACE_UINT32 m_nAsset;
		DSC::CDscShortBlob m_fromUserKey;
		DSC::CDscShortBlob m_toUserKey;
	};

	class CQueryUserAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_QUERY_ACTION_QUERY_USER
		};

	public:
		DSC_BIND_ATTR(m_userID)

	public:
		DSC::CDscShortBlob m_userID;
	};

	class CQueryUserRsp
	{
	public:
		DSC_BIND_ATTR(m_nVersion, m_key, m_value);

	public:
		ACE_UINT32 m_nVersion;
		CDscString m_key;
		CDscString m_value;
	};

	class CQueryTransAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_QUERY_ACTION_QUERY_TRANS
		};

	public:
		DSC_BIND_ATTR(m_transID)

	public:
		DSC::CDscShortBlob m_transID;
	};

	class CQueryTransRsp
	{
	public:
		class CWs
		{
		public:
			DSC_BIND_ATTR(m_nVersion, m_key, m_value);

		public:
			ACE_UINT32 m_nVersion;
			CDscString m_key;
			CDscString m_value;
		};

	public:
		DSC_BIND_ATTR(m_vecWs, m_transKey, m_userKey, m_signature, m_proposal);

	public:
		VBH::CSimpleVector<CWs> m_vecWs; //写集
		CDscString m_transKey; //transaction-key
		CDscString m_userKey;  //事务发起者
		CDscString m_signature;
		CDscString m_proposal;
	};

	class CQueryInfoHistoryListAction
	{
	public:
		enum
		{
			EN_ACTION_ID = EN_QUERY_ACTION_QUERY_INFOMATION_HISTORY_LIST
		};

	public:
		DSC_BIND_ATTR(m_informationID)

	public:
		DSC::CDscShortBlob m_informationID;
	};
}



#endif
