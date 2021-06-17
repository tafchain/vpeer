#ifndef VBH_COMMON_MACRO_DEF_H_076E0F629CAF11E9BDAA60F18A3A20D1
#define VBH_COMMON_MACRO_DEF_H_076E0F629CAF11E9BDAA60F18A3A20D1

#include "dsc/dsc_log.h"

//---------------工具宏
#ifdef NDEBUG
	#define VBH_MESSAGE_ENTER_TRACE(service_name, message_name)
	#define VBH_MESSAGE_LEAVE_TRACE(service_name, message_name)
	#define VBH_TRACE_MESSAGE(FORMAT_STRING, ...)
#else
	#define VBH_TO_STR(s) #s  //将任意参数转换为字符串的方法
	//#define VBH_MESSAGE_ENTER_TRACE(service_name, message_name) DSC_INTF_LOG_FINE("func:$s, In service:%s, begin process message:%s.", __FUNC__, VBH_TO_STR(service_name), VBH_TO_STR(message_name))
	#define VBH_MESSAGE_ENTER_TRACE(service_name, message_name) DSC_INTF_LOG_FINE("In service:%s, begin process message:%s.", VBH_TO_STR(service_name), VBH_TO_STR(message_name))
#define VBH_MESSAGE_LEAVE_TRACE(service_name, message_name) DSC_INTF_LOG_FINE("In service:%s, process message:%s end.", VBH_TO_STR(service_name), VBH_TO_STR(message_name))

	//用于trace的打印宏
	#define VBH_TRACE_MESSAGE(FORMAT_STRING, ...) DSC_RUN_LOG_FINE(FORMAT_STRING, ##__VA_ARGS__)
#endif
//---------------字符串宏

#define DEF_STORAGE_DIR_NAME "storage"

#define DEF_ORDER_CPS_CONFIG_FILE_NAME "cps.cfg"
#define DEF_ORDER_CPS_LOG_FILE_NAME "cps.log"

#define DEF_ANCHOR_CONFIG_FILE_NAME "anchor.cfg"
#define DEF_ANCHOR_LOG_FILE_NAME "anchor.log"

#define DEF_USER_HISTORY_DB_PATH_NAME "U_HIST_DB"
#define DEF_INFORMATION_HISTORY_DB_PATH_NAME "I_HIST_DB"
#define DEF_HISTORY_DB_HEAD_FILE_NAME ".hist_db_head"
#define DEF_HISTORY_DB_FILE_SUBFIX ".hdb"
#define DEF_WRITE_CACHE_HISTORY_DB_FILE_NAME "write_cache.hdb"


#define DEF_TRANSACTION_INDEX_PATH_NAME "T_IDX"
#define DEF_BLOCKCHAIN_STORAGE_PATH_NAME "BLOCK_CHAIN"
#define DEF_BLOCKCHAIN_HEAD_FILE_NAME ".bc_head"
#define DEF_BLOCKCHAIN_FILE_SUBFIX ".bc"


// 区块存储位置
#define DEF_CFG_BLOCK_DATA_PATH												"VBH_BLOCK_DATA_PATH"

// 区块数据缓存大小，单位MB
#define DEF_CFG_BLOCK_DATA_CACHE_SIZE_MB						"VBH_BLOCK_DATA_CACHE_SIZE_MB"

#endif // VBH_COMMON_MACRO_DEF_H_076E0F629CAF11E9BDAA60F18A3A20D1
