__不完全查询优化指南__

1. 过滤尽可能少的数据
2. 仅用需要的列
3. 更快的计算方式
4. SQL 越简单越容易优化
5. 优化可能会放弃一部分正确性或者简洁性

__常见问题__

1. read limit
2. memory limit
3. extremely slow but performed

1. 针对一必须优化读入数据/或者做中间表两次 Query
2. 针对二可以优化读入数据/单机已经不行考虑分布式表/或者做中间表两次 Query
3. 针对三有各种各样的方法,下面列举一些适用经验

1. Partition 尽可能命中分区列，分区列过滤数据有质的提升
2. Primay Index 过滤条件尽可能命中索引列，索引列过滤数据往往有质的提升
3. str like 'http://maimai.cn/jobs%' 字符串匹配尽量用前缀匹配
4. 过滤不要计算索引列 toDate(ts)=2019-01-01 => ts between '2019-01-01 00:00:00' and '2019-01-01 23:59:59'
5. 过滤条件要放在贴近源表

```sql
select uid, count(*) pv from pageview group by uid having uid>0
=> 
select uid, count(*) pv from pageview where uid> 0 group by uid -- better

select * from (
    select c1, c2 from A
    union all
    select c3 as c1, c4 as c2 from B
)
where c1 = 5 and c2 like 'http%'
=> 
select * from (
    select c1, c2 from A
    c1 = 5 and c2 like 'http%'
    union all
    select c3 as c1, c4 as c2 from B
    where c1 = 5 and c2 like 'http%'
) -- better
```

6. uniqExact 问题，uniqExact 是典型的耗内存应用，这时候要平衡，如果要求不精确可以使用模糊去重(uniq/uniqCombined/uniqHLL12)
7. JOIN 优化，由于 ClickHouse 的实现问题，右表的数据应该尽可能少，因为 ClickHouse 会先把右表读到内存里再进行计算
8. 使用高级函数，提高处理速度，减少"SQL 编程的现象"
```
windowFunnel
retention
roundDown
runningAccumulate
ArrayFunctoins
```


