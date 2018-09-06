# clickhouse 相关问题

## `all left join` better than `any left join`
```
select d,
    uniqExact(uid) as dauu,
    uniqExactIf(uid, day=d and status=1) s1u,
    s1u/dauu `status=1占日活比`
from fact.dau all left join (
    select uid, day, status from profile.user_status prewhere today()-15<=day
)  using(uid) prewhere today()-15<=d group by d order by d desc

-- 64 sec
```
VS
```
select d,
    uniqExact(uid) as dauu,
    uniqExactIf(uid, status=1) s1u,
    s1u/dauu `status=1占比`
from fact.dau any left join (
    select uid, day d, status from profile.user_status prewhere today()-15<=d
)  using(uid, d) prewhere today()-15<=d group by d order by d desc
-- 302 sec
```

## `union all` Cannot convert column v because it is non constant in source stream
https://github.com/yandex/ClickHouse/issues/2507

## clickhouse 导入 csv
```
sudo scp tmp.csv ubuntu@0.0.0.0:/tmp
cat /tmp/tmp.csv | clickhouse-client --host clickhouse6 -u dengwancheng --password ******* --query="INSERT INTO profile.tagmap FORMAT CSVWithNames"
```

## 外部字典使用
```
select * from system.dictionaries where name = 'uri'
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