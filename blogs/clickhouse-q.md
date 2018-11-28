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
以下是我总结的经验，单位是年（year）
[1-3)      => 使用 d 级别
[3-10)     => 使用 week 级别
[10-20)    => 使用 month 级别
[20, 100)  => 使用 quarter 级别