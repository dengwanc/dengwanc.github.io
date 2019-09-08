
### Define schema

```sql
-- 此表两个作用
-- 1. 导入原始数据，中间表皆从此表生成
-- 2. 计算默认 session_id 相关统计
-- 注意：正式比赛时需要做成分布式表，按 uid 分片
create table tmp.session_raw
(
  uid Int64,
  timestamp_ms UInt64,
  event_code StringWithDictionary,
  session_id String,
  url String,
  platform StringWithDictionary,
  source StringWithDictionary,
  city StringWithDictionary,
  brand StringWithDictionary,
  purchase_quantity UInt16,
  price Float64,
  date_str String,
  ts MATERIALIZED toDateTime(timestamp_ms/1000),
  d MATERIALIZED toDate(ts)
)
Engine=MergeTree partition by d order by (uid, session_id);
```

### Load data
```bash
cat session.dat | clickhouse-client --host clickhouse15 -u $USER --password $PWD --query="INSERT INTO tmp.session_raw FORMAT TabSeparated"
```

### 默认 Session
```sql
-- 支持跨天
-- 每天的会话次数
select dt, uniqExact(uid, session_id) 
from (select uid, session_id, min(d) dt from tmp.session_raw group by uid, session_id)
group by dt
┌─uniqExact(uid, session_id)─┬──────────d─┐
│                     498691 │ 2019-01-01 │
│                     501484 │ 2019-01-02 │
│                     497576 │ 2019-01-03 │
│                     499975 │ 2019-01-04 │
│                     498580 │ 2019-01-05 │
│                     500903 │ 2019-01-06 │
│                     501345 │ 2019-01-07 │
└────────────────────────────┴────────────┘

-- 人均访问时长
select dt,
  sum(duration) as sum_duration,
  uniqExact(uid) as uv,
  sum_duration/1000/60/uv as avg_usage_minute
from (
  select uid, min(d) dt, max(timestamp_ms)-min(timestamp_ms) as duration
  from tmp.session_raw group by uid, session_id
) group by dt
┌──────────d─┬───sum_duration─┬─────uv─┬───avg_usage_minute─┐
│ 2019-01-01 │  3520397138635 │ 249651 │    235.02123221584 │
│ 2019-01-02 │  3540018446183 │ 250493 │  235.5367512720249 │
│ 2019-01-03 │  3515254610792 │ 248289 │  235.9652535816461 │
│ 2019-01-04 │ 37553546439867 │ 250160 │ 2501.9685028159975 │
│ 2019-01-05 │ 37459627148993 │ 249201 │ 2505.3154648251143 │
│ 2019-01-06 │ 37648597035632 │ 250374 │  2506.157257784488 │
│ 2019-01-07 │ 37645293839956 │ 250788 │ 2501.8005805671196 │
└────────────┴────────────────┴────────┴────────────────────┘

-- 退出率
select dt,
    count() as session_cnt,
    countIf(pv=1) as jumper_cnt,
    jumper_cnt/session_cnt as jumper_ratio
from (
    select uid, min(d) dt, session_id, uniqExact(url) as pv 
    from tmp.session_raw group by uid, session_id
) group by dt
┌──────────d─┬─session_cnt─┬─jumper_cnt─┬──────────jumper_ratio─┐
│ 2019-01-01 │      498691 │       1698 │ 0.0034049140650222283 │
│ 2019-01-02 │      501484 │       1771 │  0.003531518453230811 │
│ 2019-01-03 │      497576 │       1759 │ 0.0035351383507243113 │
│ 2019-01-04 │      499975 │       1793 │ 0.0035861793089654484 │
│ 2019-01-05 │      498580 │       1775 │ 0.0035601107144289782 │
│ 2019-01-06 │      500903 │       1795 │  0.003583528148164415 │
│ 2019-01-07 │      501345 │       1790 │ 0.0035703956357398597 │
└────────────┴─────────────┴────────────┴───────────────────────┘
```

### 超时时间30分钟+跨天的session切割规则

```sql
-- 1. 为了更快的 ETL 按 uid, timestamp_ms 排序
-- 2. 生产环境中可直接使用 session_raw 表跑每天的数据
create table tmp.session_orderby_ms
(
  uid Int64,
  timestamp_ms UInt64,
  event_code StringWithDictionary,
  session_id String,
  url String,
  platform StringWithDictionary,
  source StringWithDictionary,
  city StringWithDictionary,
  brand StringWithDictionary,
  purchase_quantity UInt16,
  price Float64,
  date_str String,
  ts MATERIALIZED toDateTime(timestamp_ms/1000),
  d MATERIALIZED toDate(ts)
)
Engine=MergeTree order by (uid, timestamp_ms);

-- 按 uid 为窗口，进行 session 划分，得到 session_sliceby_30min 表
-- 关于跨天的思考：用 uid, session_id 分组，天取 min(d) 即可
-- 关于指定开始事件的思考：需要阅读论文，指定开始事件是否在生产环境中有大量应用，研究开始事件具体是怎么运作的
-- 注意：在生产环境中产生 session_id 应该用 date-tag 的形式，这里跑全量数据所以可以直接用 tag=session_id
-- 注意：生成 session_id 后，需要把原先的各种维度恢复，即 Array Join 回去
with
    groupArray(timestamp_ms) as timeseries,
    arrayDifference(timeseries) as diff,
    arrayMap(x -> x >= 1800000, diff) as segments,
    arrayCumSum(segments) as session_tag
select
    uid, segments, session_tag, diff, groupArray(ts)
from tmp.session_orderby_ms
group by uid
having uid = 14230773428449917
```
