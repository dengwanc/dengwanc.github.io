# clickhouse 相关问题

## `union all` Cannot convert column v because it is non constant in source stream
https://github.com/yandex/ClickHouse/issues/2507

## clickhouse 导入 csv
```
sudo scp tmp.csv ubuntu@0.0.0.0:/tmp
cat /tmp/tmp.csv | clickhouse-client --host clickhouse6 -u dengwancheng --password ******* --query="INSERT INTO profile.tagmap FORMAT CSVWithNames"
```

## 外部字典使用
```
select * from system.dictionaries where name in ('uri', 'users') format Vertical
select dictGetString('uri', 'behavior', tuple('/contact/visit_history'))
```

## redash 踩坑指南
* 结果集不能出现同样的列名，否则不可用 Visualization(Angular 会报错)
* redash 可以改列数据类型 [Visualization Editor] ⇒ [Type format]

## clickhouse 小表 join 大表优化
```sql
-- userevent is small table like 100,000 rows
-- user_status is big table like 100,000,000 rows
select d,
    uniqExact(uid) newu,
    uniqExactIf(uid, com) comu,
    comu / newu
from userevent any left join (
    select uid, day d, 1 com from profile.user_status prewhere today()-3<=d
) using(uid, d) prewhere event='upload' and today()-3<=d
group by d

-- this sql fast than above 30 times
select d,
    uniqExact(uid) newu, 
    uniqExactIf(uid, com) comu,
    comu / newu
from userevent any left join (
    select uid, day d, 1 com from profile.user_status prewhere uid in (
        select uid from userevent prewhere event='upload' and today()-3<=d
    ) and today()-3<=d
) using(uid, d) prewhere event='upload' and today()-3<=d
group by d

-- run in ch9
-- mysqldump.usess is big table
-- mysqldump.work_tags1  is small table
-- take 30s
select count() from mysqldump.work_tags1 any inner join (
    select uid from mysqldump.users
) using uid

-- this sql fast than above 30 times
select count() from mysqldump.users any inner join (
    select uid from mysqldump.work_tags1
) using uid
```

## groupUniqArrayArray
```sql
SELECT 
    key, 
    groupUniqArrayArray(arrayValue)
FROM table 
GROUP BY key
```

## clickhouse sharding and replicated
> reference https://zhuanlan.zhihu.com/p/37173180
```
<shard>
    <replica>
        <host>10.9.0.12</host>
    </replica>
    <replica>
        <host>10.9.0.11</host>
    </replica>
</shard>
<shard>
    <replica>
        <host>10.9.0.13</host>
    </replica>
    <replica>
        <host>10.9.0.14</host>
    </replica>
</shard>
<shard>
    <replica>
        <host>10.9.0.15</host>
    </replica>
    <replica>
        <host>10.9.0.16</host>
    </replica>
</shard>
```
* `<replica>` define 11 and 12 is backup each other(11 crash 12 will work, 12 crash 11 will work)
* ClickHouse sharding is for Distributed table query(dispatch to three Shard for querying, 12, 13, 15 will work)
* ClikkHouse Replicaed table data only depend on ZooKeeper(path, specifically)

## 建表时如何指定 partition

苏联专家有过讨论 https://github.com/yandex/ClickHouse/issues/2378
1. 建议按照时间纬度做 partition
2. 主键和 partition 没有关系
3. 一张表的 partition 个数建议不要超过 1000，主要是partition 太多会引起文件太多的问题
以下是我总结的经验，单位是年
    * [1-3)      => 使用 d 级别
    * [3-10)     => 使用 week 级别
    * [10-20)    => 使用 month 级别
    * [20, 100)  => 使用 quarter 级别

## clickhouse create materialized view

### 删除
- 如果 ''create materialized view db.view to db.table''，先删除 view 再删除table
- 如果 ''show tables'' 出错了，使用 ''select * from system.columns where database='tmp''' 去删表
- 如果 **.inner.table doesn't exist**，建一张名字叫 .inner.table 的表

### query
- ''create materialized view as select from table''，view 严格依赖于 table，所以不能用 subquery，因为clickhouse 只会对能写入数据的 table 触发 insert data to materialized view
- backfill 数据的时候注意要把表名 nxloa_shard.table -> nxlog.table 否则只有部分数据
- 不能用 join
- 只能用最简单的 select columns from table where 的形式
- 改表字段，1. 删除所有的 view 2. 该表 3 重新建 view。materialized view 会实时写入，因此可能导致写入失败

## Key Properties of OLAP Scenario (ClickHouse)

* The vast majority of requests are for read access.(主要是读)
* Data is ingested in fairly large batches (> 1000 rows), not by single rows;(数据批量入库)
* Data is added to the DB but is not modified.(很少更新数据)
* For reads, quite a large number of rows are extracted from the DB, but only a small subset of columns.(只选取一部分列查询分析)
* For simple queries, latencies around 50 ms are allowed.(要求很快)
* Column values are fairly small: numbers and short strings (for example, 60 bytes per URL).(列存的数据size不大)
* Requires high throughput when processing a single query (up to billions of rows per second per server).(高吞吐量)
* Transactions are not necessary.(对事物没有要求)
* Low requirements for data consistency.(对数据库一致性要求低)
* A query result is significantly smaller than the source data. In other words, data is filtered or aggregated, so the result fits in a single server's RAM.(查询时把数据放在内存里)


> __已知问题__
* MySQL Engine 有泄漏数据库密码的风险 https://github.com/yandex/ClickHouse/issues/3311
