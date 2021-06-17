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

//CBlockChainTable不提供区块缓冲区，只提供区块的磁盘读写服务
class CBlockChainTable
{
private:
	//block chain storage 的配置
	class CBcTableConfig
	{
	public:
		ACE_UINT32 m_nCurFileID = 0;  //当前引用的文件
		ACE_UINT64 m_nCurOffset = 0; //当前记录文件写位置的偏移量
		ACE_UINT64 m_nLastBlockID = 0; //存储中所保存的最后1个区块的ID
	};

	//block chain storage 的日志，用于可靠存储
	class CBcTableLog
	{
	public:
		ACE_UINT32 m_nCurFileID = 0; //当前文件ID //文件ID从1开始
		ACE_UINT64 m_nCurOffset = 0; //记录文件写位置的偏移量
		ACE_UINT64 m_nLastBlockID = 0; //存储中所保存的最后1个区块的ID
	};

public:
	ACE_INT32 Open(VBFS::CVbfs* pVbfs, const ACE_UINT32 nChannelID);
	void Close(void);

	//读取区块，并进行hash校验和merkel检验，返回成功解码后的内存区块 //任何一步失败都返回nullptr
	VBH::CMemBcBlock* ReadDecodeBlock(const ACE_UINT64 nBlockID);

	//读取区块数据，校验，不解码
	char* ReadBlock(ACE_UINT32& nDataLen, const ACE_UINT64 nBlockID);

	//读取区块hash，如果存在下一个区块，直接从下一个区块中读取，如果不存在则通过区块计算
	ACE_INT32 ReadBlockHash(char* pBlockHash, const ACE_UINT64 nBlockID);
	
	//追加区块到存储
	ACE_INT32 AppendBlock(const ACE_UINT64 nBlockID, const DSC::CDscBlob& rBlockData, const VBH::CHashBuffer& rPreBlockHash);

	//把最后一个区块回退
	ACE_INT32 PopBack(const ACE_UINT64 nRecordID);

	//把变更保存到日志中，在ApplyModify之前被调用 
	ACE_INT32 SaveToLog(void);

	//应用所有变更
	ACE_INT32 Persistence(void);

	void CommitteTransaction(void);
	void RollbackCache(void);
	ACE_INT32 RollbackTransaction(void);
	ACE_INT32 RecoverFromLog(void);

private:
	//读取解码区块实现:读取，校验，解码校验, 
	//读取成功的区块数据位于 m_pAlignBlockDataBuf 中，在下一次读取或写入数据前有效
	VBH::CMemBcBlock* ReadDecodeBlockImpl(ACE_UINT32* pDataLen, const ACE_UINT64 nBlockID);

private:
	VBFS::CVbfs* m_pVbfs;
	ACE_UINT32 m_nChannelID;
	ACE_UINT64 m_nFileSize; //文件大小

	ACE_Shared_Memory_MM m_shmCfg; // 配置文件共享内存对象
	CBcTableConfig* m_pCfg = nullptr; // 配置文件共享内存指针
	ACE_Shared_Memory_MM m_shmLog; // 日志文件共享内存对象
	CBcTableLog* m_pLog = nullptr; //日志文件共享内存指针

	CBcTableConfig m_memCfg; //内存中存放的临时配置，

	char* m_pAlignBlockDataBuf = nullptr; //地址对齐的块数据缓冲区
	ACE_UINT32 m_nAlignBlockDataBufLen = 0; //缓冲区长度  //区块数据写入前，需要先放入内存对齐缓冲区； 读取时需要先读取到内存对齐缓冲区
	
	VBH::CMerkelTree m_merkelTree; //读取区块时，用于校验merkel树

	CVbhUpdateTable m_bcIndexTable; //区块链的索引数据库
};


#include "com_cs/block_chain_table/block_chain_table.inl"


#endif



