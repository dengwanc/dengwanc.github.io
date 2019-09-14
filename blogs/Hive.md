### 数据定义
```sql
show databases like 'dw*';
desc database extended dwd;
show tables '*_history';
desc formatted dwd.dw_pagev;
desc formatted dwd.dw_pagev partition(p_date='2019-09-01');
```

### 数据加载
```sql
-- 扫描一次，多方划分
from dwd.dw_frontend_events fe
insert overwrite table dwd.dw_general_events_ partition(p_date='2019-09-02', event_name='feed')
select * where fe.p_date='2019-09-02' and fe.event_name='feed'
insert overwrite table dwd.dw_general_events_ partition(p_date='2019-09-02', event_name='PN')
select * where fe.p_date='2019-09-02' and fe.event_name='PN'
```

### 数据查询
```sql
-- Map-side Aggregation
set hive.map.aggr=true;
-- table function
select explode(array(1,2,4));
select explode(map('k1',2,'k2',4, 'k3', 'x'));
-- 嵌套 select, 更简洁, 逻辑更清晰
from (
    select uid, timestamp from dwd.dw_frontend_events 
    where p_date='2019-09-02' and to_date(timestamp/1000)=p_date
) fe
select fe.uid, cast(fe.timestamp/1000 as timestamp) as ts

-- 本地模式
set hive.exec.mode.local.auto=true

-- Hive 先执行 JOIN 语句，再将结果通过 WHERE 语句过滤
-- 部分 IN 语句可以用 LEFT SEMI JOIN 代替
-- Map-side JOIN 
set hive.mapjoin.smalltable.filesize;
select /*+ MAPJOIN(p) */ from dim.dim_page p inner join dwd.dw_page_session s on s.page_name = p.page_name 
-- DISTRIBUTE BY 控制 map 的输出再 reducer 中是如何划分的
-- CLUSTER BY 可以实现输出文件的数据是全局有序的
```

### 视图
```sql
-- 使用视图降低查询复杂度
-- 使用视图限制条件过滤的数据
-- 使用视图对外提供统一的数据接口，内部实现可以更换
-- 限制：JOIN/窗口函数 没办法视图化
-- 限制：某些的谓词下推不支持 LIMIT WHERE 等
-- 使用视图需要经过反复的 trade-off
```

### Hive 模式设计
1. 同一份数据多种处理，从同一个数据源产生多个数据处理，INSERT 到不同表中，无需重新扫描
2. 几乎总是使用压缩

### 输出结果使用压缩
```
set hive.exec.compress.output=true; -- trun on
set mapred.output.compression.codec; -- control format
-- 压缩率 BZip2 > GZip > LZO > Snappy 
-- 注意 Gzip 和 Snappy 是不可分割的

-- 如果 Map 输出结果集很大可以开启中间压缩
set hive.exec.compress.intermediate;
```

### 调优

1. 尝试调优时，观察 EXPLAIN 的输出也许会有帮助
2. 严格模式有三种限制 1. 必须写分区 2. ORDER BY 必须 LIMIT 3. 禁止笛卡尔积
3. 受外部因素的影响，像 reducer 个数这种标杆值估计十分复杂
    3.1 因为 map 任务输出有可能多，有可能少
    3.2 一个快速验证的办法就是：设置一个固定的值 set mapred.reduce.tasks=3;
4. 虚拟列
```sql
set hive.exec.rowoffset=true;
select page_name, INPUT__FILE__NAME, BLOCK__OFFSET__INSIDE__FILE from dwd.dw_pagev where p_date='2019-09-01' limit 2;
```

### 函数

```
show functions like 'a*';
desc function extended concat;
```

### 宏命令

```sql
-- 简单的抽象可以用宏命令
create temporary macro SIGMOID (x double) 1.0 / (1.0 + exp(-x));
select SIGMOID(2);
```

### Hive 使用问题

1. 每次加载 UDF 函数很麻烦 => .hiverc
2. 分桶的作用 map-join/抽样
3. 该用哪种压缩模式 开启压缩 or 使用 ORC

### 数据分析

1. 排序/TOP-N 问题
2. Page Session 分割/时序分析(行间计算)
3. 有序漏斗/windowFunnel

### 几个瓶颈

1. 没有认识的同道友人打比赛
2. 如何学习到先进的大数据技术方法