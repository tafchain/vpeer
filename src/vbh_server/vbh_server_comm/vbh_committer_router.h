#ifndef VBH_COMMITTER_ROUTER_H_4CA973A4879311E9875660F18A3A20D1
#define VBH_COMMITTER_ROUTER_H_4CA973A4879311E9875660F18A3A20D1

#include "dsc/dsc_msg.h"
#include "dsc/configure/dsc_configure.h"
#include "dsc/container/bare_hash_map.h"

#include "vbh_server_comm/vbh_server_comm_def_export.h"

namespace VBH
{
	//committer sevice ��·����
	class VBH_SERVER_COMM_DEF_EXPORT CCommitterServiceRouter
	{
	private:
		enum
		{
			EN_HASH_MAP_BITES = 16
		};

		class CDscAddr //��DSCϵͳ�� ��ַ �� ID����
		{
		public:
			ACE_INT16 m_nNodeID;
			ACE_INT16 m_nServiceID;

		public:
			ACE_UINT32 m_nKey = 0;
			CDscAddr* m_pPrev = NULL;
			CDscAddr* m_pNext = NULL;
		};

	public:
		//��, ��ʼ��
		ACE_INT32 Open(void);

		//�ر�
		void Close(void);

		//��ȡ�ض�channel �û��� xcs��ַ
		ACE_INT32 GetCsAddr(CDscMsg::CDscMsgAddr& addr, const ACE_UINT32 nChannelID);

	private:
		//���� channel �� tcs addr ��ӳ���ϵ
		ACE_INT32 LoadChannelMapCsAddr();

	private:
		using channel_map_dsc_addr_type = CBareHashMap<ACE_UINT32, CDscAddr, EN_HASH_MAP_BITES>; //channel -> xcs addr

	private:
		channel_map_dsc_addr_type m_mapChannelMapCsAddr;
	};

}

#include "vbh_server_comm/vbh_committer_router.inl"


#endif // VBH_COMMITER_PARAMETER_H
