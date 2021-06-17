-- -----------------------------------------------------------------
-- 配置说明: 所有的表名必须大写，windows不区分大小写，但是在linux下区分大小写
-- --------------------------
-- order manager app 的NODE_TYPE = 1
-- order service app 的NODE_TYPE = 2
-- node-id: 所有的node-id都取1（各种类型都只有1个节点）
-- cps端口号：channel x 对应的 cps 的 cps-id 为x, cps 的 端口号为 13000 + x
-- cms端口号固定为：14000

-- --------------------------
-- peer endorser app 的NODE_TYPE = 11
-- peer anchor service app 的NODE_TYPE = 21
-- peer x committer service app 的NODE_TYPE = 31
-- peer cc app 的NODE_TYPE = 41
-- node-id: 所有的node-id都取1（各种类型都只有1个节点）
-- peer 所有node节点的dsc侦听端口从8000开始，后续依次+1
-- endorser端口号：每个Node上endorser的端口号为 11000+ES_ID
-- xcs端口号: channel x 对应的 service-id 为x, xcs 的 端口号为 12000 + x


-- -----------------------------------------------------------------

-- --------------------------
-- 调试时，修改默认的日志数据库的配置
-- --------------------------
DELETE FROM `T_LOG_CONFIG` WHERE 1=1;
INSERT INTO `T_LOG_CONFIG` VALUES (0, 0, 'RUN', 10, 10, 1, 4, 5);
INSERT INTO `T_LOG_CONFIG` VALUES (0, 0, 'INTERFACE', 10, 10, 1, 4, 5);
INSERT INTO `T_LOG_CONFIG` VALUES (0, 0, 'STATISTIC', 10, 10, 1, 4, 5);

-- --------------------------
-- ORDER的配置
-- --------------------------
-- CPS的配置
INSERT INTO `CPS_CFG`(`NODE_ID`,`CPS_ID`,`CPS_IP_ADDR`,`CPS_PORT`,`CH_ID`) VALUES (1, 1, '127.0.0.1', 13001, 1);
-- VBH系统基础配置
INSERT INTO `VBH_SYS_CFG` VALUES ('CMS_IP_ADDR', '127.0.0.1', 0, 0, 'channel-manage-service的地址，全order唯一');
INSERT INTO `VBH_SYS_CFG` VALUES ('CMS_PORT', '14000', 0, 0, 'channel-manage-service的端口号');
INSERT INTO `VBH_SYS_CFG` VALUES ('ORDER_ID', '1', 0, 0, '指定node节点的order-id');
INSERT INTO `VBH_SYS_CFG` VALUES ('PEER_COUNT', '1', 0, 0, '系统中peer的个数');
INSERT INTO `VBH_SYS_CFG` VALUES ('MIN_AGREED_PEER_COUNT', '1', 0, 0, '达成共识的最小节点数');
INSERT INTO `VBH_SYS_CFG` VALUES ('PEER_ENVELOPE_KEY', '12345678123456781234567812345678', 0, 0, '对称密钥');
-- 插件数据库的配置
INSERT INTO `T_PLUGIN` VALUES (1, 0, 'ord_cms', 1, 'ord_cms', 1);
INSERT INTO `T_PLUGIN` VALUES (2, 0, 'ord_cps', 1, 'ord_cps', 1);

-- --------------------------
-- PEER的配置
-- --------------------------
-- peer端dsc网络配置
-- 连接: es<-->xcs 
INSERT INTO `T_LISTEN` VALUES (31, 1, '127.0.0.1', 8000); -- xcs 侦听
INSERT INTO `T_CONNECT` VALUES (11, 1, '127.0.0.1', 8000, '127.0.0.1', -1); -- endorser -> xcs

-- order上CMS地址配置
INSERT INTO `CMS_ADDR`(`ORD_ID`,`IP_ADDR`,`PORT`) VALUES (1, '127.0.0.1', 14000);
-- endorser 配置
INSERT INTO `ES_CFG`(`NODE_ID`,`ES_ID`,`ES_IP_ADDR`,`ES_PORT`) VALUES (1, 1, '127.0.0.1', 11001);
INSERT INTO `CES_CFG`(`NODE_ID`,`ES_ID`,`ES_IP_ADDR`,`ES_PORT`) VALUES (1, 1, '127.0.0.1', 11002);
-- xcs 配置
INSERT INTO `XCS_CFG`(`NODE_ID`,`XCS_ID`,`XAS_ID`,`CH_ID`) VALUES (1, 1, 1, 1);
-- xcs cache配置
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 1, 8, 4, 10, 8); -- block-index-table
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 2, 8, 4, 10, 8); -- user-key-table
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 3, 8, 4, 10, 8); -- user-inex-table
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 4, 8, 4, 10, 8); -- user-history-table
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 5, 8, 4, 10, 8); -- information-index-table
INSERT INTO `XCS_CACHE_CFG`(`NODE_ID`,`XCS_ID`,`CACHE_TYPE`,`PAGE_RECORD_BITS`,`MAX_OPEN_FILE_NUM`,`FILE_PAGE_BITS`,`CACHED_PAGE_NUM`) VALUES (0, 0, 6, 8, 4, 10, 8); -- information-history-table
-- 内部CC的配置
INSERT INTO `INNER_CC_CFG`(`CC_NAME`,`CC_TYPE`,`INUSE`) VALUES ('vbh_func_test_cc', 123, 1); -- 不加密SDK测试用CC
-- VBH系统基础配置
INSERT INTO `VBH_SYS_CFG` VALUES ('PEER_ID','1',0,0,'当前peer的 PEER ID值');
-- 插件数据库的配置
INSERT INTO `T_PLUGIN` VALUES (11, 0, 'end_es', 1, 'end_es', 1); -- 加载不加密的endorser-service
INSERT INTO `T_PLUGIN` VALUES (31, 0, 'xcm_xas', 1, 'xcm_xas', 1); -- 加载x committer agent service 
INSERT INTO `T_PLUGIN` VALUES (31, 0, 'xcm_xcs', 1, 'xcm_xcs', 1); -- 加载x committer service 
-- --------------------------
-- SDK端的配置
-- --------------------------
-- endorser 地址配置
INSERT INTO `ES_ADDR_CFG`(`PEER_ID`,`ES_IP_ADDR`,`ES_PORT`) VALUES (1, '127.0.0.1', 11001);
INSERT INTO `CRYPT_ES_ADDR_CFG`(`PEER_ID`,`CRYPT_ES_IP_ADDR`,`CRYPT_ES_PORT`) VALUES (1, '127.0.0.1', 11002);
-- VBH系统基础配置
INSERT INTO `VBH_SYS_CFG` VALUES ('ENDORSE_PEER_COUNT', '1', 0, 0, '背书节点个数'); 
-- 插件数据库的配置
INSERT INTO `T_PLUGIN` VALUES (501, 0, 'vbh_client_service', 1, 'vbh_client_service', 1); -- 非加密注册测试程序 sdk-client-service 
INSERT INTO `T_PLUGIN` VALUES (502, 0, 'vbh_client_service', 1, 'vbh_client_service', 1); -- 非加密提案测试程序 sdk-client-service 


