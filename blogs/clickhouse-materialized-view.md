# clickhouse create materialized view

## 删除
- 如果 ''create materialized view db.view to db.table''，先删除 view 再删除table
- 如果 ''show tables'' 出错了，使用 ''select * from system.columns where database='tmp''' 去删表
- 如果 **.inner.table doesn't exist**，建一张名字叫 .inner.table 的表

## query
- ''create materialized view as select from table''，view 严格依赖于 table，所以不能用 subquery，因为clickhouse 只会对能写入数据的 table 触发 insert data to materialized view
- backfill 数据的时候注意要把表名 nxloa_shard.table -> nxlog.table 否则只有部分数据
- 不能用 join
- 只能用最简单的 select columns from table where 的形式
- 改表字段，1. 删除所有的 view 2. 该表 3 重新建 view。materialized view 会实时写入，因此可能导致写入失败