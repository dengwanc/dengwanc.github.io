
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

create table tmp.session_sliceby_30min
(
  uid Int64,
  timestamp_ms UInt64,
  url StringWithDictionary,
  platform StringWithDictionary,
  source StringWithDictionary,
  city StringWithDictionary,
  brand StringWithDictionary,
  purchase_quantity UInt16,
  price Float64,
  session_id UInt16,
  ts MATERIALIZED toDateTime(timestamp_ms/1000),
  d MATERIALIZED toDate(ts)
)
Engine=MergeTree partition by d order by (uid, session_id)
-- 按 uid 为窗口，进行 session 划分，得到 session_sliceby_30min 表
-- 关于跨天的思考：用 uid, session_id 分组，天取 min(d) 即可
-- 关于跨天的思考：现实中如果支持跨天，也是只支持到第二天前一个小时左右，因为 ETL 时间很可能是夜里 2 点以后
-- 关于指定开始事件的思考：需要阅读论文，指定开始事件是否在生产环境中有大量应用，研究开始事件具体是怎么运作的
-- 注意：在生产环境中产生 session_id 应该用 date-tag 的形式，这里跑全量数据所以可以直接用 tag=session_id
-- 注意：生成 session_id 后，需要把原先的各种维度恢复，即 Array Join 回去
-- 注意：此表转化需要较长事件，需要优化，并发读写，更好的写法
insert into tmp.session_sliceby_30min
select uid,
  item.1.1 as timestamp_ms,
  item.1.2 as url,
  item.1.3 as platform,
  item.1.4 as source,
  item.1.5 as city,
  item.1.6 as brand,
  item.1.7 as purchase_quantity,
  item.1.8 as price,
  item.2 as session_id
from (
  with
      groupArray(timestamp_ms) as timeseries,
      arrayDifference(timeseries) as diff,
      arrayMap(x -> x >= 1800000, diff) as segments,
      arrayCumSum(segments) as session_tag
  select
      groupArray((timestamp_ms, url, platform, source, city, brand, purchase_quantity, price)) as dimensions,
      arrayMap(i -> (dimensions[i], session_tag[i]), arrayEnumerate(session_tag)) as mixed,
      uid, segments, session_tag, diff
  from tmp.session_orderby_ms
  group by uid
) array join mixed as item

-- 支持跨天
-- 每天的会话次数
select dt, uniqExact(uid, session_id) 
from (select uid, session_id, min(d) dt from tmp.session_sliceby_30min group by uid, session_id)
group by dt
┌─────────dt─┬─uniqExact(uid, session_id)─┐
│ 2019-01-01 │                     415983 │
│ 2019-01-02 │                     396952 │
│ 2019-01-03 │                     389109 │
│ 2019-01-04 │                    4632336 │
│ 2019-01-05 │                    4527106 │
│ 2019-01-06 │                    4561181 │
│ 2019-01-07 │                    4683356 │
└────────────┴────────────────────────────┘

-- 跳出率
select dt,
    count() as session_cnt,
    countIf(pv=1) as jumper_cnt,
    jumper_cnt/session_cnt as jumper_ratio
from (
    select uid, min(d) dt, session_id, uniqExact(url) as pv 
    from tmp.session_sliceby_30min group by uid, session_id
) group by dt
┌─────────dt─┬─session_cnt─┬─jumper_cnt─┬────jumper_ratio─┐
│ 2019-01-01 │      415983 │       1298 │ 0.0031203198207619062 │
│ 2019-01-02 │      396952 │       1194 │   0.00300792035309055 │
│ 2019-01-03 │      389109 │       1163 │ 0.0029888797226484097 │
│ 2019-01-04 │     4632336 │    2233339 │    0.4821193885763036 │
│ 2019-01-05 │     4527106 │    2161022 │    0.4773517562875709 │
│ 2019-01-06 │     4561181 │    2177910 │   0.47748817685595024 │
│ 2019-01-07 │     4683356 │    2268837 │   0.48444683684093204 │
└─────────┴─────────────┴────────────┴────────────┘

-- 以着陆页进行分组,每天的会话次数
select dt, landing_page, uniqExact(uid, session_id) as session_cnt
from (select uid, session_id, min(d) dt, argMin(url, timestamp_ms) landing_page from tmp.session_sliceby_30min group by uid, session_id)
group by dt, landing_page
order by dt, session_cnt desc
┌─────────dt─┬─landing_page─────────────────────────────┬─session_cnt─┐
│ 2019-01-01 │ https://ark.analysys.cn/index            │      415671 │
│ 2019-01-01 │ https://ark.analysys.cn/browseGoods      │         105 │
│ 2019-01-01 │ https://ark.analysys.cn/searchGoods      │          47 │
│ 2019-01-01 │ https://ark.analysys.cn/addCart          │          28 │
│ 2019-01-01 │ https://ark.analysys.cn/orderPayment     │          23 │
│ 2019-01-01 │ https://ark.analysys.cn/order            │          21 │
│ 2019-01-01 │ https://ark.analysys.cn/reminding        │          16 │
│ 2019-01-01 │ https://ark.analysys.cn/shareGoods       │          14 │
│ 2019-01-01 │ https://ark.analysys.cn/evaluationGoods  │          12 │
│ 2019-01-01 │ https://ark.analysys.cn/consultGoods     │          12 │
│ 2019-01-01 │ https://ark.analysys.cn/confirm          │          12 │
│ 2019-01-01 │ https://ark.analysys.cn/startUp          │          10 │
│ 2019-01-01 │ https://ark.analysys.cn/collectionGoods  │           7 │
│ 2019-01-01 │ https://ark.analysys.cn/login            │           5 │
│ 2019-01-02 │ https://ark.analysys.cn/index            │      396559 │
│ 2019-01-02 │ https://ark.analysys.cn/browseGoods      │         114 │
│ 2019-01-02 │ https://ark.analysys.cn/searchGoods      │          54 │
│ 2019-01-02 │ https://ark.analysys.cn/addCart          │          33 │
│ 2019-01-02 │ https://ark.analysys.cn/order            │          30 │
│ 2019-01-02 │ https://ark.analysys.cn/orderPayment     │          26 │
│ 2019-01-02 │ https://ark.analysys.cn/startUp          │          21 │
│ 2019-01-02 │ https://ark.analysys.cn/collectionGoods  │          20 │
│ 2019-01-02 │ https://ark.analysys.cn/reminding        │          17 │
│ 2019-01-02 │ https://ark.analysys.cn/evaluationGoods  │          17 │
│ 2019-01-02 │ https://ark.analysys.cn/login            │          16 │
│ 2019-01-02 │ https://ark.analysys.cn/consultGoods     │          16 │
│ 2019-01-02 │ https://ark.analysys.cn/confirm          │          16 │
│ 2019-01-02 │ https://ark.analysys.cn/shareGoods       │          11 │
│ 2019-01-02 │ https://ark.analysys.cn/unsubscribeGoods │           2 │
│ 2019-01-03 │ https://ark.analysys.cn/index            │      388763 │
│ 2019-01-03 │ https://ark.analysys.cn/browseGoods      │         110 │
│ 2019-01-03 │ https://ark.analysys.cn/searchGoods      │          63 │
│ 2019-01-03 │ https://ark.analysys.cn/addCart          │          31 │
│ 2019-01-03 │ https://ark.analysys.cn/orderPayment     │          22 │
│ 2019-01-03 │ https://ark.analysys.cn/order            │          22 │
│ 2019-01-03 │ https://ark.analysys.cn/confirm          │          17 │
│ 2019-01-03 │ https://ark.analysys.cn/evaluationGoods  │          16 │
│ 2019-01-03 │ https://ark.analysys.cn/startUp          │          14 │
│ 2019-01-03 │ https://ark.analysys.cn/login            │          13 │
│ 2019-01-03 │ https://ark.analysys.cn/reminding        │          12 │
│ 2019-01-03 │ https://ark.analysys.cn/consultGoods     │          10 │
│ 2019-01-03 │ https://ark.analysys.cn/collectionGoods  │           9 │
│ 2019-01-03 │ https://ark.analysys.cn/shareGoods       │           6 │
│ 2019-01-03 │ https://ark.analysys.cn/unsubscribeGoods │           1 │
│ 2019-01-04 │ https://ark.analysys.cn/browseGoods      │     1457123 │
│ 2019-01-04 │ https://ark.analysys.cn/searchGoods      │      729058 │
│ 2019-01-04 │ https://ark.analysys.cn/addCart          │      327921 │
│ 2019-01-04 │ https://ark.analysys.cn/order            │      273250 │
│ 2019-01-04 │ https://ark.analysys.cn/orderPayment     │      253782 │
│ 2019-01-04 │ https://ark.analysys.cn/index            │      227253 │
│ 2019-01-04 │ https://ark.analysys.cn/confirm          │      182189 │
│ 2019-01-04 │ https://ark.analysys.cn/reminding        │      181823 │
│ 2019-01-04 │ https://ark.analysys.cn/evaluationGoods  │      181774 │
│ 2019-01-04 │ https://ark.analysys.cn/consultGoods     │      181614 │
│ 2019-01-04 │ https://ark.analysys.cn/collectionGoods  │      181562 │
│ 2019-01-04 │ https://ark.analysys.cn/startUp          │      172515 │
│ 2019-01-04 │ https://ark.analysys.cn/login            │      163913 │
│ 2019-01-04 │ https://ark.analysys.cn/shareGoods       │       91208 │
│ 2019-01-04 │ https://ark.analysys.cn/unsubscribeGoods │       27351 │
│ 2019-01-05 │ https://ark.analysys.cn/browseGoods      │     1422225 │
│ 2019-01-05 │ https://ark.analysys.cn/searchGoods      │      711403 │
│ 2019-01-05 │ https://ark.analysys.cn/addCart          │      321021 │
│ 2019-01-05 │ https://ark.analysys.cn/order            │      266315 │
│ 2019-01-05 │ https://ark.analysys.cn/orderPayment     │      249193 │
│ 2019-01-05 │ https://ark.analysys.cn/index            │      221975 │
│ 2019-01-05 │ https://ark.analysys.cn/consultGoods     │      178707 │
│ 2019-01-05 │ https://ark.analysys.cn/reminding        │      178245 │
│ 2019-01-05 │ https://ark.analysys.cn/confirm          │      177722 │
│ 2019-01-05 │ https://ark.analysys.cn/collectionGoods  │      177720 │
│ 2019-01-05 │ https://ark.analysys.cn/evaluationGoods  │      176845 │
│ 2019-01-05 │ https://ark.analysys.cn/startUp          │      168809 │
│ 2019-01-05 │ https://ark.analysys.cn/login            │      161120 │
│ 2019-01-05 │ https://ark.analysys.cn/shareGoods       │       89079 │
│ 2019-01-05 │ https://ark.analysys.cn/unsubscribeGoods │       26727 │
│ 2019-01-06 │ https://ark.analysys.cn/browseGoods      │     1433425 │
│ 2019-01-06 │ https://ark.analysys.cn/searchGoods      │      716952 │
│ 2019-01-06 │ https://ark.analysys.cn/addCart          │      322556 │
│ 2019-01-06 │ https://ark.analysys.cn/order            │      268309 │
│ 2019-01-06 │ https://ark.analysys.cn/orderPayment     │      250709 │
│ 2019-01-06 │ https://ark.analysys.cn/index            │      224168 │
│ 2019-01-06 │ https://ark.analysys.cn/evaluationGoods  │      179775 │
│ 2019-01-06 │ https://ark.analysys.cn/reminding        │      179635 │
│ 2019-01-06 │ https://ark.analysys.cn/consultGoods     │      179330 │
│ 2019-01-06 │ https://ark.analysys.cn/collectionGoods  │      179196 │
│ 2019-01-06 │ https://ark.analysys.cn/confirm          │      179153 │
│ 2019-01-06 │ https://ark.analysys.cn/startUp          │      170488 │
│ 2019-01-06 │ https://ark.analysys.cn/login            │      161456 │
│ 2019-01-06 │ https://ark.analysys.cn/shareGoods       │       89306 │
│ 2019-01-06 │ https://ark.analysys.cn/unsubscribeGoods │       26723 │
│ 2019-01-07 │ https://ark.analysys.cn/browseGoods      │     1473235 │
│ 2019-01-07 │ https://ark.analysys.cn/searchGoods      │      737367 │
│ 2019-01-07 │ https://ark.analysys.cn/addCart          │      330965 │
│ 2019-01-07 │ https://ark.analysys.cn/order            │      276100 │
│ 2019-01-07 │ https://ark.analysys.cn/orderPayment     │      256613 │
│ 2019-01-07 │ https://ark.analysys.cn/index            │      230105 │
│ 2019-01-07 │ https://ark.analysys.cn/confirm          │      184464 │
│ 2019-01-07 │ https://ark.analysys.cn/reminding        │      183986 │
│ 2019-01-07 │ https://ark.analysys.cn/collectionGoods  │      183969 │
│ 2019-01-07 │ https://ark.analysys.cn/consultGoods     │      183916 │
│ 2019-01-07 │ https://ark.analysys.cn/evaluationGoods  │      183300 │
│ 2019-01-07 │ https://ark.analysys.cn/startUp          │      174994 │
│ 2019-01-07 │ https://ark.analysys.cn/login            │      164949 │
│ 2019-01-07 │ https://ark.analysys.cn/shareGoods       │       91672 │
│ 2019-01-07 │ https://ark.analysys.cn/unsubscribeGoods │       27721 │
└────────────┴──────────────────────────────────────────┴─────────────┘

-- 以着陆页进行分组,每天的跳出率
select dt, landing_page,
    count() as session_cnt,
    countIf(pv=1) as jumper_cnt,
    jumper_cnt/session_cnt as jumper_ratio
from (
    select uid, min(d) dt, session_id, argMin(url, timestamp_ms) landing_page, uniqExact(url) as pv
    from tmp.session_sliceby_30min group by uid, session_id
) 
group by dt, landing_page
order by dt, jumper_ratio desc
┌─────────dt─┬─landing_page─────────────────────────────┬─session_cnt─┬─jumper_cnt─┬──────────jumper_ratio─┐
│ 2019-01-01 │ https://ark.analysys.cn/evaluationGoods  │          12 │          1 │   0.08333333333333333 │
│ 2019-01-01 │ https://ark.analysys.cn/shareGoods       │          14 │          1 │   0.07142857142857142 │
│ 2019-01-01 │ https://ark.analysys.cn/browseGoods      │         105 │          7 │   0.06666666666666667 │
│ 2019-01-01 │ https://ark.analysys.cn/reminding        │          16 │          1 │                0.0625 │
│ 2019-01-01 │ https://ark.analysys.cn/addCart          │          28 │          1 │   0.03571428571428571 │
│ 2019-01-01 │ https://ark.analysys.cn/index            │      415671 │       1287 │ 0.0030961986763570226 │
│ 2019-01-01 │ https://ark.analysys.cn/startUp          │          10 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/collectionGoods  │           7 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/consultGoods     │          12 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/confirm          │          12 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/order            │          21 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/login            │           5 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/searchGoods      │          47 │          0 │                     0 │
│ 2019-01-01 │ https://ark.analysys.cn/orderPayment     │          23 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/unsubscribeGoods │           2 │          1 │                   0.5 │
│ 2019-01-02 │ https://ark.analysys.cn/orderPayment     │          26 │          3 │   0.11538461538461539 │
│ 2019-01-02 │ https://ark.analysys.cn/confirm          │          16 │          1 │                0.0625 │
│ 2019-01-02 │ https://ark.analysys.cn/searchGoods      │          54 │          2 │  0.037037037037037035 │
│ 2019-01-02 │ https://ark.analysys.cn/browseGoods      │         114 │          3 │   0.02631578947368421 │
│ 2019-01-02 │ https://ark.analysys.cn/index            │      396559 │       1184 │ 0.0029856843496175855 │
│ 2019-01-02 │ https://ark.analysys.cn/reminding        │          17 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/shareGoods       │          11 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/collectionGoods  │          20 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/order            │          30 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/startUp          │          21 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/login            │          16 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/consultGoods     │          16 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/evaluationGoods  │          17 │          0 │                     0 │
│ 2019-01-02 │ https://ark.analysys.cn/addCart          │          33 │          0 │                     0 │
│ 2019-01-03 │ https://ark.analysys.cn/confirm          │          17 │          3 │   0.17647058823529413 │
│ 2019-01-03 │ https://ark.analysys.cn/startUp          │          14 │          2 │   0.14285714285714285 │
│ 2019-01-03 │ https://ark.analysys.cn/collectionGoods  │           9 │          1 │    0.1111111111111111 │
│ 2019-01-03 │ https://ark.analysys.cn/consultGoods     │          10 │          1 │                   0.1 │
│ 2019-01-03 │ https://ark.analysys.cn/reminding        │          12 │          1 │   0.08333333333333333 │
│ 2019-01-03 │ https://ark.analysys.cn/addCart          │          31 │          2 │   0.06451612903225806 │
│ 2019-01-03 │ https://ark.analysys.cn/orderPayment     │          22 │          1 │  0.045454545454545456 │
│ 2019-01-03 │ https://ark.analysys.cn/searchGoods      │          63 │          2 │  0.031746031746031744 │
│ 2019-01-03 │ https://ark.analysys.cn/browseGoods      │         110 │          3 │   0.02727272727272727 │
│ 2019-01-03 │ https://ark.analysys.cn/index            │      388763 │       1147 │ 0.0029503836527653093 │
│ 2019-01-03 │ https://ark.analysys.cn/evaluationGoods  │          16 │          0 │                     0 │
│ 2019-01-03 │ https://ark.analysys.cn/shareGoods       │           6 │          0 │                     0 │
│ 2019-01-03 │ https://ark.analysys.cn/login            │          13 │          0 │                     0 │
│ 2019-01-03 │ https://ark.analysys.cn/unsubscribeGoods │           1 │          0 │                     0 │
│ 2019-01-03 │ https://ark.analysys.cn/order            │          22 │          0 │                     0 │
│ 2019-01-04 │ https://ark.analysys.cn/index            │      227253 │     122194 │    0.5377002723836429 │
│ 2019-01-04 │ https://ark.analysys.cn/browseGoods      │     1457123 │     762196 │    0.5230828145599239 │
│ 2019-01-04 │ https://ark.analysys.cn/searchGoods      │      729058 │     348311 │    0.4777548562665796 │
│ 2019-01-04 │ https://ark.analysys.cn/addCart          │      327921 │     149654 │    0.4563721140152659 │
│ 2019-01-04 │ https://ark.analysys.cn/order            │      273250 │     123769 │   0.45295150960658737 │
│ 2019-01-04 │ https://ark.analysys.cn/orderPayment     │      253782 │     114897 │    0.4527389649384117 │
│ 2019-01-04 │ https://ark.analysys.cn/evaluationGoods  │      181774 │      81855 │    0.4503119257979689 │
│ 2019-01-04 │ https://ark.analysys.cn/reminding        │      181823 │      81866 │    0.4502510683466888 │
│ 2019-01-04 │ https://ark.analysys.cn/collectionGoods  │      181562 │      81703 │   0.45000055077604345 │
│ 2019-01-04 │ https://ark.analysys.cn/consultGoods     │      181614 │      81658 │    0.4496239276707743 │
│ 2019-01-04 │ https://ark.analysys.cn/confirm          │      182189 │      81738 │   0.44864399058120963 │
│ 2019-01-04 │ https://ark.analysys.cn/startUp          │      172515 │      77389 │    0.4485928759817987 │
│ 2019-01-04 │ https://ark.analysys.cn/login            │      163913 │      73321 │   0.44731656427495076 │
│ 2019-01-04 │ https://ark.analysys.cn/shareGoods       │       91208 │      40662 │   0.44581615647750195 │
│ 2019-01-04 │ https://ark.analysys.cn/unsubscribeGoods │       27351 │      12126 │   0.44334759240978394 │
│ 2019-01-05 │ https://ark.analysys.cn/index            │      221975 │     118546 │    0.5340511318842212 │
│ 2019-01-05 │ https://ark.analysys.cn/browseGoods      │     1422225 │     737273 │    0.5183940656365905 │
│ 2019-01-05 │ https://ark.analysys.cn/searchGoods      │      711403 │     336426 │   0.47290494979638825 │
│ 2019-01-05 │ https://ark.analysys.cn/addCart          │      321021 │     145084 │   0.45194551135283983 │
│ 2019-01-05 │ https://ark.analysys.cn/order            │      266315 │     119635 │   0.44922366370651295 │
│ 2019-01-05 │ https://ark.analysys.cn/orderPayment     │      249193 │     111493 │   0.44741625968626725 │
│ 2019-01-05 │ https://ark.analysys.cn/reminding        │      178245 │      79412 │    0.4455216135094954 │
│ 2019-01-05 │ https://ark.analysys.cn/startUp          │      168809 │      75190 │    0.4454146402146805 │
│ 2019-01-05 │ https://ark.analysys.cn/evaluationGoods  │      176845 │      78712 │   0.44509033334275777 │
│ 2019-01-05 │ https://ark.analysys.cn/confirm          │      177722 │      79076 │   0.44494210058405825 │
│ 2019-01-05 │ https://ark.analysys.cn/collectionGoods  │      177720 │      78985 │   0.44443506639657887 │
│ 2019-01-05 │ https://ark.analysys.cn/consultGoods     │      178707 │      79370 │    0.4441348128500842 │
│ 2019-01-05 │ https://ark.analysys.cn/login            │      161120 │      71071 │   0.44110600794438926 │
│ 2019-01-05 │ https://ark.analysys.cn/shareGoods       │       89079 │      39222 │    0.4403057959788502 │
│ 2019-01-05 │ https://ark.analysys.cn/unsubscribeGoods │       26727 │      11527 │    0.4312867138100049 │
│ 2019-01-06 │ https://ark.analysys.cn/index            │      224168 │     119622 │    0.5336265657899433 │
│ 2019-01-06 │ https://ark.analysys.cn/browseGoods      │     1433425 │     742965 │    0.5183145263965676 │
│ 2019-01-06 │ https://ark.analysys.cn/searchGoods      │      716952 │     339252 │   0.47318648947209857 │
│ 2019-01-06 │ https://ark.analysys.cn/addCart          │      322556 │     145590 │    0.4513634841701906 │
│ 2019-01-06 │ https://ark.analysys.cn/orderPayment     │      250709 │     112420 │    0.4484083140214352 │
│ 2019-01-06 │ https://ark.analysys.cn/order            │      268309 │     120154 │     0.447819491705459 │
│ 2019-01-06 │ https://ark.analysys.cn/collectionGoods  │      179196 │      80168 │    0.4473760575012835 │
│ 2019-01-06 │ https://ark.analysys.cn/consultGoods     │      179330 │      79956 │   0.44585958846818713 │
│ 2019-01-06 │ https://ark.analysys.cn/startUp          │      170488 │      75841 │    0.4448465581155272 │
│ 2019-01-06 │ https://ark.analysys.cn/evaluationGoods  │      179775 │      79927 │   0.44459463217911277 │
│ 2019-01-06 │ https://ark.analysys.cn/login            │      161456 │      71680 │   0.44395996432464574 │
│ 2019-01-06 │ https://ark.analysys.cn/reminding        │      179635 │      79730 │    0.4438444623820525 │
│ 2019-01-06 │ https://ark.analysys.cn/confirm          │      179153 │      79407 │    0.4432356700697169 │
│ 2019-01-06 │ https://ark.analysys.cn/shareGoods       │       89306 │      39492 │    0.4422099299039258 │
│ 2019-01-06 │ https://ark.analysys.cn/unsubscribeGoods │       26723 │      11706 │    0.4380496201773753 │
│ 2019-01-07 │ https://ark.analysys.cn/index            │      230105 │     124489 │    0.5410095391234436 │
│ 2019-01-07 │ https://ark.analysys.cn/browseGoods      │     1473235 │     773779 │    0.5252244210869278 │
│ 2019-01-07 │ https://ark.analysys.cn/searchGoods      │      737367 │     354578 │   0.48087044850122124 │
│ 2019-01-07 │ https://ark.analysys.cn/addCart          │      330965 │     152371 │    0.4603840285226535 │
│ 2019-01-07 │ https://ark.analysys.cn/orderPayment     │      256613 │     116570 │   0.45426381360258444 │
│ 2019-01-07 │ https://ark.analysys.cn/order            │      276100 │     125236 │   0.45358927924664977 │
│ 2019-01-07 │ https://ark.analysys.cn/collectionGoods  │      183969 │      83233 │   0.45242948540243194 │
│ 2019-01-07 │ https://ark.analysys.cn/confirm          │      184464 │      83352 │    0.4518605256310174 │
│ 2019-01-07 │ https://ark.analysys.cn/reminding        │      183986 │      83084 │   0.45157783744415336 │
│ 2019-01-07 │ https://ark.analysys.cn/consultGoods     │      183916 │      82989 │   0.45123317166532545 │
│ 2019-01-07 │ https://ark.analysys.cn/login            │      164949 │      74427 │   0.45121219285961117 │
│ 2019-01-07 │ https://ark.analysys.cn/evaluationGoods  │      183300 │      82706 │    0.4512056737588652 │
│ 2019-01-07 │ https://ark.analysys.cn/startUp          │      174994 │      78770 │    0.4501297187332137 │
│ 2019-01-07 │ https://ark.analysys.cn/shareGoods       │       91672 │      41107 │   0.44841391046339124 │
│ 2019-01-07 │ https://ark.analysys.cn/unsubscribeGoods │       27721 │      12146 │    0.4381515818332672 │
└────────────┴──────────────────────────────────────────┴─────────────┴────────────┴───────────────────────┘
```

### 超时时间30分钟+跨天的session切割规则+指定开始事件
```sql
create table tmp.session_with_start
(
  uid Int64,
  timestamp_ms UInt64,
  url StringWithDictionary,
  platform StringWithDictionary,
  source StringWithDictionary,
  city StringWithDictionary,
  brand StringWithDictionary,
  purchase_quantity UInt16,
  price Float64,
  session_id UInt16,
  ts MATERIALIZED toDateTime(timestamp_ms/1000),
  d MATERIALIZED toDate(ts)
)
Engine=MergeTree partition by d order by (uid, session_id)

insert into tmp.session_with_start
select uid,
  item.1.1 as timestamp_ms,
  item.1.2 as url,
  item.1.3 as platform,
  item.1.4 as source,
  item.1.5 as city,
  item.1.6 as brand,
  item.1.7 as purchase_quantity,
  item.1.8 as price,
  item.2 as session_id
from (
  with
      groupArray(timestamp_ms) as timeseries,
      groupArray(url) as eventseries,
      arrayDifference(timeseries) as diff,
      arrayMap(i -> diff[i] >= 1800000 or eventseries[i] in ('https://ark.analysys.cn/login'), arrayEnumerate(diff)) as segments,
      arrayCumSum(segments) as session_tag
  select
      groupArray((timestamp_ms, url, platform, source, city, brand, purchase_quantity, price)) as dimensions,
      arrayMap(i -> (dimensions[i], session_tag[i]), arrayEnumerate(session_tag)) as mixed,
      uid, segments, session_tag, diff
  from tmp.session_orderby_ms
  group by uid
) array join mixed as item

-- 包含购物车行为的会话总数
select dt, uniqExactIf(uid, session_id, hasAddCart) session_cnt
from (select uid, session_id, min(d) dt, anyIf(1, url='https://ark.analysys.cn/addCart') as hasAddCart from tmp.session_with_start group by uid, session_id)
group by dt
┌─────────dt─┬─session_cnt─┐
│ 2019-01-01 │      470197 │
│ 2019-01-02 │      466860 │
│ 2019-01-03 │      461421 │
│ 2019-01-04 │      805353 │
│ 2019-01-05 │      798422 │
│ 2019-01-06 │      802223 │
│ 2019-01-07 │      807185 │
└────────────┴─────────────┘

-- 人均访问深度
select dt,
    uniqExact(uid, session_id) as uv,
    sum(pv) / uv as avg_visit_depth
from (
    select uid, min(d) dt, session_id, uniqExact(url) as pv
    from tmp.session_sliceby_30min group by uid, session_id
) 
group by dt
┌─────────dt─┬──────uv─┬────avg_visit_depth─┐
│ 2019-01-01 │  415983 │   9.87817771399312 │
│ 2019-01-02 │  396952 │ 10.060012293677826 │
│ 2019-01-03 │  389109 │ 10.108779288065811 │
│ 2019-01-04 │ 4632336 │  2.175055522742737 │
│ 2019-01-05 │ 4527106 │ 2.2012550622848237 │
│ 2019-01-06 │ 4561181 │  2.199155438032387 │
│ 2019-01-07 │ 4683356 │ 2.1623598547708096 │
└────────────┴─────────┴────────────────────┘
```