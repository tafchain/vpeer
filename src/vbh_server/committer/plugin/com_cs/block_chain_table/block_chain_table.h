#ifndef BLOCK_CHAIN_STORAGE_H_LKJOIUOIPJLKJLKJ09879078UYHN
#define BLOCK_CHAIN_STORAGE_H_LKJOIUOIPJLKJLKJ09879078UYHN

#include "ace/Shared_Memory_MM.h"

#include "dsc/container/dsc_string.h"
#include "dsc/mem_mng/dsc_stl_type.h"
#include "dsc/container/dsc_dqueue.h"
#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/container/dsc_type_array.h"

#include "vbh_server_comm/merkel_tree/merkel_tree.h"
#include "vbh_server_comm/vbh_block_codec.h"
#include "vbh_server_comm/vbh_const_length_storage/vbh_update_table.h"
#include "vbh_server_comm/vbh_const_length_storage/vbfs/vbfs.h"

//CBlockChainTable���ṩ���黺������ֻ�ṩ����Ĵ��̶�д����
class CBlockChainTable
{
private:
	//block chain storage ������
	class CBcTableConfig
	{
	public:
		ACE_UINT32 m_nCurFileID = 0;  //��ǰ���õ��ļ�
		ACE_UINT64 m_nCurOffset = 0; //��ǰ��¼�ļ�дλ�õ�ƫ����
		ACE_UINT64 m_nLastBlockID = 0; //�洢������������1�������ID
	};

	//block chain storage ����־�����ڿɿ��洢
	class CBcTableLog
	{
	public:
		ACE_UINT32 m_nCurFileID = 0; //��ǰ�ļ�ID //�ļ�ID��1��ʼ
		ACE_UINT64 m_nCurOffset = 0; //��¼�ļ�дλ�õ�ƫ����
		ACE_UINT64 m_nLastBlockID = 0; //�洢������������1�������ID
	};

public:
	ACE_INT32 Open(VBFS::CVbfs* pVbfs, const ACE_UINT32 nChannelID);
	void Close(void);

	//��ȡ���飬������hashУ���merkel���飬���سɹ��������ڴ����� //�κ�һ��ʧ�ܶ�����nullptr
	VBH::CMemBcBlock* ReadDecodeBlock(const ACE_UINT64 nBlockID);

	//��ȡ�������ݣ�У�飬������
	char* ReadBlock(ACE_UINT32& nDataLen, const ACE_UINT64 nBlockID);

	//��ȡ����hash�����������һ�����飬ֱ�Ӵ���һ�������ж�ȡ�������������ͨ���������
	ACE_INT32 ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID);
	
	//׷�����鵽�洢
	ACE_INT32 AppendBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& rBlockData, const VBH::CHashBuffer& rPreBlockHash);

	//�����һ���������
	ACE_INT32 PopBack(const ACE_UINT64 nRecordID);

	//�ѱ�����浽��־�У���ApplyModify֮ǰ������ 
	ACE_INT32 SaveToLog(void);

	//Ӧ�����б��
	ACE_INT32 Persistence(void);

	void CommitteTransaction(void);
	void RollbackCache(void);
	ACE_INT32 RollbackTransaction(void);
	ACE_INT32 RecoverFromLog(void);

private:
	//��ȡ��������ʵ��:��ȡ��У�飬����У��, 
	//��ȡ�ɹ�����������λ�� m_pAlignBlockDataBuf �У�����һ�ζ�ȡ��д������ǰ��Ч
	VBH::CMemBcBlock* ReadDecodeBlockImpl(ACE_UINT32* pDataLen, const ACE_UINT64 nBlockID);

private:
	VBFS::CVbfs* m_pVbfs;
	ACE_UINT32 m_nChannelID;
	ACE_UINT64 m_nFileSize; //�ļ���С

	ACE_Shared_Memory_MM m_shmCfg; // �����ļ������ڴ����
	CBcTableConfig* m_pCfg = nullptr; // �����ļ������ڴ�ָ��
	ACE_Shared_Memory_MM m_shmLog; // ��־�ļ������ڴ����
	CBcTableLog* m_pLog = nullptr; //��־�ļ������ڴ�ָ��

	CBcTableConfig m_memCfg; //�ڴ��д�ŵ���ʱ���ã�

	char* m_pAlignBlockDataBuf = nullptr; //��ַ����Ŀ����ݻ�����
	ACE_UINT32 m_nAlignBlockDataBufLen = 0; //����������  //��������д��ǰ����Ҫ�ȷ����ڴ���뻺������ ��ȡʱ��Ҫ�ȶ�ȡ���ڴ���뻺����
	
	VBH::CMerkelTree m_merkelTree; //��ȡ����ʱ������У��merkel��

	CVbhUpdateTable m_bcIndexTable; //���������������ݿ�
};


#include "com_cs/block_chain_table/block_chain_table.inl"


#endif



