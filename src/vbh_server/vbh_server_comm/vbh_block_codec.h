#ifndef VBH_BLOCK_CODEC_H_B087D2B2BF0611E9B4F760F18A3A20D1
#define VBH_BLOCK_CODEC_H_B087D2B2BF0611E9B4F760F18A3A20D1


#include "vbh_server_comm/merkel_tree/merkel_tree.h"
#include "vbh_server_comm/vbh_transaction_def.h"

/* *********************************************************************
 * 事务类型(注册/发起提案) 合并在 事务流水号中，具体见 CTransactionSequenceNumber
 * ********************************************************************* */

namespace VBH
{
	//区块头的结构定义 //区块头必须是定长(不能包含blob等边长数据)
	class CBcBlockHeader
	{
	public:
		enum
		{
			EN_SIZE = sizeof(ACE_UINT16)*2 + sizeof(ACE_UINT64)*2 + VBH_BLOCK_DIGEST_LENGTH*2,
			EN_MAX_TRANSACTION_COUNT_IN_BLOCK = ((1 << 16) - 1), //1个区块中最多可以同时包含的
		};

	public:
		DSC_BIND_ATTR(m_nOrderID, m_nTransCount, m_nBlockID, m_nBlockTime, m_preBlockHash, m_merkelTreeRootHash);

	public:
		ACE_UINT16 m_nOrderID; //打包区块时的orderID，校验时也要用
		ACE_UINT16 m_nTransCount; //打包在区块中的事务个数 //区块数最多不能
		ACE_UINT64 m_nBlockID; //区块ID，从1开始编码
		ACE_UINT64 m_nBlockTime; //区块生成时间
		CHashBuffer m_preBlockHash; //前一区块的hash值
		CHashBuffer m_merkelTreeRootHash; //本区块merkel树根的hash值
	};

	//================================================================================
	//用于区块编码的类 //使用方式：在CPS中定义一个该类的对象，循环使用，用于编码
	class VBH_SERVER_COMM_DEF_EXPORT CBlockEncoder
	{
	public:
		//初始设置存放编码后数据的buffer
		void InitSetEncodeBuffer(char* pBuf);

		//重新设置带数据的编码缓冲区，缓冲区中已有 nDataLen的数据
		void ResetEncodeBuffer(char* pBuf, size_t nDataLen);

		//开始编码事务，返回编码状态对象， 记录开始编码的指针
		DSC::CDscNetCodecEncoder& BeginEncodeTransaction(void);

		//结束编码事务，将从开始到结束编码进来的数据都添加到merkel树种
		void EndEncodeTransaction(void);

		//获取当前已经编码的区块数据大小,包括区块头的长度
		size_t GetEncodeDataSize(void);

		//编码区块头,并写入缓冲区 //编码区块头前，先填充除merkel树根之外的所有字段
		ACE_INT32 EncodeBlockHeader(void);

	public:
		CBcBlockHeader m_bcBlockHeader; //区块头

	protected:
		char* m_pTransBeginPtr = nullptr; //事务开始的指针，在持续添加事务内容结束后，用于确定事务的开始位置
		DSC::CDscNetCodecEncoder m_encodeState; //除区块头之外的 编码状态
		VBH::CMerkelTree m_merkelTree; //校验用merkel树 //生成
	};

	//================================================================================

	//解码后存放于内存中的BcBlock
	class VBH_SERVER_COMM_DEF_EXPORT CMemBcBlock
	{
	public:
		CMemBcBlock(const ACE_UINT64 nBlockID, const ACE_UINT16 nTransCount);
		~CMemBcBlock();

	public:
		const ACE_UINT16 m_nTransCount; //打包在区块中的事务个数 //区块数最多不能
		const ACE_UINT64 m_nBlockID; //区块ID
		ACE_UINT64 m_nBlockTime; //区块生成时间
		CMemTransactionPtr* m_ppTransaction = nullptr; //事务数组(列表) //事务列表的长度在区块头中
		char m_preBlockHash[VBH_BLOCK_DIGEST_LENGTH]; //前一区块的hash值
		char m_merkelTreeRootHash[VBH_BLOCK_DIGEST_LENGTH]; //本区块merkel树根的hash值
	};

	/** 解码区块，将数据填充到 CMemBcBlock 中 //区块数据分成两部分，区块数据|失败事务列表
	 * @ pPreBlockHash: 本块中存储的前一区块的hash值 //输出参数
	 * @ nBlockDataLen: 区块数据长度 //输出参数
	 * @ pBuf: 待解码数据缓冲区
	 * @ nTotalBufLen: 待解码数据总长度
	 * @ merkelTree: 用于校验的merkel树结构，考虑到内存管理的原因，merkel树结构由外部提供，而不是临时创建
	 *
	 * */
	VBH_SERVER_COMM_DEF_EXPORT VBH::CMemBcBlock* vbhDecodeMemBcBloc(char* pPreBlockHash, const char* pBuf, const ACE_UINT32 nDataLen, VBH::CMerkelTree& merkelTree);
}

#include "vbh_server_comm/vbh_block_codec.inl"

#endif
