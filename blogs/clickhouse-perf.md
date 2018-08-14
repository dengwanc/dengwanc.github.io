# clickhouse 性能相关问题

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
