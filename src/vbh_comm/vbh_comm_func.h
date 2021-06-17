#ifndef VBH_COMM_FUNC_H_432843298432658431469719764325685
#define VBH_COMM_FUNC_H_432843298432658431469719764325685

#include "ace/OS_NS_fcntl.h"
#include "ace/Shared_Memory_MM.h"

#include "dsc/container/dsc_string.h"
#include "dsc/container/dsc_queue.h"
#include "dsc/codec/dsc_codec/dsc_codec.h"
#include "dsc/configure/dsc_configure.h"

#include "vbh_comm/vbh_comm_def_export.h"

namespace VBH
{
	//blob, string的赋值和比对
	bool IsEqual(const CDscString& str, const DSC::CDscBlob& blob); //string和blob是否相等
	void Assign(CDscString& str, const DSC::CDscBlob& blob); //将blob内容赋给string
	void Assign(DSC::CDscBlob& blob, CDscString& str); //将stirng内容赋给blb

	//short-blob, string的赋值和比对
	bool IsEqual(const CDscString& str, const DSC::CDscShortBlob& blob); //string和blob是否相等
	void Assign(CDscString& str, const DSC::CDscShortBlob& blob); //将blob内容赋给string
	void Assign(DSC::CDscShortBlob& blob, CDscString& str); //将stirng内容赋给blb

	//清空 CDscUnboundQueue 
	template<typename ITEM_TYPE>
	void ClearDscUnboundQueue(CDscUnboundQueue<ITEM_TYPE>& queue);

	//合并两个 CDscUnboundQueue from->to
	template<typename ITEM_TYPE>
	void MergeDscUnboundQueue(CDscUnboundQueue<ITEM_TYPE>& queueTo, CDscUnboundQueue<ITEM_TYPE>& queueFrom);


	//读取通用配置
	ACE_INT32 GetVbhProfileInt(const char* pszKeyName, ACE_INT32& nValue); //读取整数类型的配置值
	ACE_INT32 GetVbhProfileString(const char* pszKeyName, CDscString& strValue); //读取字符串类型的配置值

	//计算数据的hash值
	ACE_UINT16 GetDataHashValue(const DSC::CDscBlob& data, const ACE_UINT32 nHashBits);

	//内存映射配置文件的创建和加载，通用函数 (配置，日志，等均可使用)
	template <typename CFG_TYPE>
	ACE_INT32 CreateCfgFile(const CDscString& strFilePath, CFG_TYPE& initCfg); //initCfg是初始写入文件的配置
	template<typename CFG_TYPE>
	ACE_INT32 LoadMmapCfgFile(const CDscString& strFilePath, ACE_Shared_Memory_MM& shmHandle, CFG_TYPE*& pCfg);

	//创建指定大小的mmap文件
	VBH_COMM_DEF_EXPORT ACE_INT32 CreateMmapFile(const CDscString& strFilePath, const ACE_UINT32 nSize);
	//加载指定的mmap文件，按文件大小进行内存映射 //通过nSize返回文件的大小
	VBH_COMM_DEF_EXPORT ACE_INT32 LoadMmapFile(const CDscString& strFilePath, ACE_Shared_Memory_MM& shmHandle, char*& pContent, ACE_UINT32& nSize);
	//调整指定mmap文件的大小，并重新加载mmap文件 //nSize为指定的size
	VBH_COMM_DEF_EXPORT ACE_INT32 ResizeMmapFile(const CDscString& strFilePath, ACE_Shared_Memory_MM& shmHandle, char*& pContent, const ACE_UINT32 nSize);
}

#include "vbh_comm/vbh_comm_func.inl"
#endif
