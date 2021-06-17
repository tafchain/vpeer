#ifndef VBH_MERKEL_TREE_H_27C4328CA76D11E9B55260F18A3A20D1
#define VBH_MERKEL_TREE_H_27C4328CA76D11E9B55260F18A3A20D1

#include "ace/Basic_Types.h"

#include "dsc/container/dsc_array.h"

#include "vbh_comm/vbh_encrypt_lib.h"
#include "vbh_server_comm/vbh_server_comm_def_export.h"

namespace VBH
{
	using CHashBuffer = CDscArray<char, VBH_BLOCK_DIGEST_LENGTH>;

	class VBH_SERVER_COMM_DEF_EXPORT CMerkelNode
	{
	public:
		char m_valueBuffer[VBH_BLOCK_DIGEST_LENGTH * 2]; //前半段用于存放自己的hash值，后半段用于计算上层节点时的临时缓冲区

	public:
		bool operator == (const CMerkelNode& value)const; //两个hash值是否相等
		bool IsEqual(const VBH::CHashBuffer& value) const;
		void operator = (const CMerkelNode& value); //两个hash值是否相等

		void SetHashValue(const char* pValue); //设置计算好的hash值
		void SetHashValue(const VBH::CHashBuffer& hashBlock); //设置计算好的hash值

		void SetOriginalData(const char* pData, const ACE_UINT32 nLen); //设置原始数据，自行计算hash值

		void GetHashValue(VBH::CHashBuffer& hashBlock) const; //获取存储的hash值

		void GetHashValue(char* pValue); //获取存储的hash值
	};

#define  MERKEL_TREE_INIT_NODE_COUNT 1024 //初始值
#define  MERKEL_TREE_NODE_INCREASE_STEP 256 //步进值

	class VBH_SERVER_COMM_DEF_EXPORT CMerkelTree
	{
	public:
		CMerkelTree();
		virtual ~CMerkelTree();

		//清空当前树
		void Clear();

		//获取树根的hash值，hash值长度是固定的
		void GetRoot(VBH::CHashBuffer& hashBlock) const;

		//添加1个叶子节点，参数为属于叶子节点的数据，需要为该数据计算hash值
		ACE_INT32 AddLeafNode(const char* pOriginalData, const ACE_UINT32 nLen);

		//构建树，构建之后可以得到根节点的值
		ACE_INT32 BuildMerkleTree();

	protected:
		//增加节点
		void InceaseNode();

		//根据左右叶子节点，计算父节点的hash值
		void CalculateHashValue(CMerkelNode* pLeft, CMerkelNode* pRight, CMerkelNode* pParent);

	private:
		CMerkelNode* m_pNodes = NULL;
		ACE_UINT32 m_nAllocNum = MERKEL_TREE_INIT_NODE_COUNT; //已经开辟的节点个数
		ACE_UINT32 m_nCurNum = 0; //已经添加的节点个数
	};

	//比较两个CHashBuffer是否一致
	bool IsEqual(VBH::CHashBuffer& left, VBH::CHashBuffer& right);

}

#include "vbh_server_comm/merkel_tree/merkel_tree.inl"

#endif // vbh_merkle_tree_h__
