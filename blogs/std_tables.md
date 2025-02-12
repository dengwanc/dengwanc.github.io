1 分布式 VS 复制表

* 预估容量
	* < 1 million rows 小表
	* 1 million ~ 1 billion 中表
	* 大于 1 billion 大表
* 如果日增会超过大表，请建分布式表
	* 前端打点日志
	* 后端打点日志
	* nginx 日志 增量 8亿/天
	* 日志集群
    * 某业务表 存量 38亿, 增量 百万/天
	* 业务大表集群负责业务大表
	* 普通集群负责中小表
* 如果日增少且长时间内不会超过大表，请建复制表
	* 某业务表 1 亿，增量 十几万/天

2 分区设计

* 分区目的：1. 便于重入数据 2. 便于清理归档数据 3. 便于在节点间移动数据
* 如果表每天都有新的数据，应该按天分区
* 分区尽可能一级，如果有两级需求，按如下解决
	* 按主键 order by event_name 去重
	* 大表拆表，事件特别大单独抽出处理再 Merge，比如 PageView
* 分区有一定的查询性能提升，但不可依此无限制建分区

3 主键设计
* 主键应当基于查询场景设计
    * 顺序索引，枚举查询条件
	* events，order by event_name
	* users，order by uid
	* nxlog，order by host, path
* 是否选择去重
	* 和业务数据库一致
	* 逻辑需要去重，打点事件表 event_id

4 物化视图
* 落地和物理表一样
* 简单的数据清洗
* 简单的数据聚合

5 逻辑视图
* SQL 响应时间快
* 定义常量方便维护

6 作为业务后端表
* 只增数据(候选集/特征)
* 量特别大
* 使用聚合数据

7. 和范式的关系
1. 列的原子性
2. 有主键，尽量根据主键能定位到某条数据，其他数据依赖于主键