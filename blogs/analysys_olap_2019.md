
### 部署 ClickHouse
1. https://clickhouse.yandex/docs/en/getting_started/#from-rpm-packages 注意网络问题
2. 安装 Zookeeper
3. config.xml
  3.1 集群配置
  3.2 TCP/HTTP 端口监听
  3.3 配置 ZK

### Define schema

```sql
-- 此表两个作用
-- 1. 导入原始数据，中间表皆从此表生成
-- 2. 计算默认 session_id 相关统计
-- 注意：正式比赛时需要做成分布式表，按 uid 分片
drop table if exists tmp.session_raw_local on cluster ch_cluster2;
create table if not exists tmp.session_raw_local on cluster ch_cluster2
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

create table if not exists tmp.session_raw AS tmp.session_raw_local Engine=Distributed(ch_cluster2, tmp, session_raw_local, uid);
```

### Load data
```bash
cat session.dat | clickhouse-client --host clickhouse15 -u $USER --password $PWD --query="INSERT INTO tmp.session_raw FORMAT TabSeparated"
cat demo_olap.dat | clickhouse-client --query="INSERT INTO tmp.session_raw FORMAT TabSeparated"
```

### 默认 Session
```sql
-- 支持跨天
-- 每天的会话次数
select dt, uniqExact(uid, session_id) 
from (select uid, session_id, min(d) dt from tmp.session_raw group by uid, session_id)
group by dt;
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
) group by dt;
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
) group by dt;
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
drop table if exists tmp.session_sliceby_30min_local on cluster ch_cluster2;
create table tmp.session_sliceby_30min_local on cluster ch_cluster2
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
Engine=MergeTree partition by d order by (uid, session_id);

create table if not exists tmp.session_sliceby_30min AS tmp.session_sliceby_30min_local Engine=Distributed(ch_cluster2, tmp, session_sliceby_30min_local, uid);
-- 按 uid 为窗口，进行 session 划分，得到 session_sliceby_30min 表
-- 关于跨天的思考：用 uid, session_id 分组，天取 min(d) 即可
-- 关于跨天的思考：现实中如果支持跨天，也是只支持到第二天前一个小时左右，因为 ETL 时间很可能是夜里 2 点以后
-- 关于指定开始事件的思考：需要阅读论文，指定开始事件是否在生产环境中有大量应用，研究开始事件具体是怎么运作的
-- 注意：在生产环境中产生 session_id 应该用 date-tag 的形式，这里跑全量数据所以可以直接用 tag=session_id
-- 注意：生成 session_id 后，需要把原先的各种维度恢复，即 Array Join 回去
-- 注意：此表转化需要较长事件，需要优化，并发读写，更好的写法
-- TEST
-- select uid, url, ts, session_id from tmp.session_sliceby_30min where uid=-4863761402216521686;
insert into tmp.session_sliceby_30min_local
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
  select uid,
    groupArray((timestamp_ms, url, platform, source, city, brand, purchase_quantity, price)) as dimensions,
    arraySort(x -> x.1, dimensions) as sorted,
    arrayDifference(arrayMap(x -> x.1, sorted)) as diff,
    arrayCumSum(arrayMap(x -> x >= 1800000, diff)) as session_tag,
    arrayMap(i -> (sorted[i], session_tag[i]), arrayEnumerate(session_tag)) as mixed
  from tmp.session_raw_local
  group by uid
) array join mixed as item;

-- 支持跨天
-- 每天的会话次数
select dt, uniqExact(uid, session_id) 
from (select uid, session_id, min(d) dt from tmp.session_sliceby_30min group by uid, session_id)
group by dt;
┌─────────dt─┬─uniqExact(uid, session_id)─┐
│ 2019-01-01 │                     434553 │
│ 2019-01-02 │                     436231 │
│ 2019-01-03 │                     433288 │
│ 2019-01-04 │                    3712253 │
│ 2019-01-05 │                    3698444 │
│ 2019-01-06 │                    3712765 │
│ 2019-01-07 │                    3721564 │
└────────────┴────────────────────────────┘


-- 跳出率
select dt,
    count() as session_cnt,
    countIf(pv=1) as jumper_cnt,
    jumper_cnt/session_cnt as jumper_ratio
from (
    select uid, min(d) dt, session_id, uniqExact(url) as pv 
    from tmp.session_sliceby_30min group by uid, session_id
) group by dt;
┌─────────dt─┬─session_cnt─┬─jumper_cnt─┬──────────jumper_ratio─┐
│ 2019-01-01 │      434553 │       1471 │ 0.0033850876647957785 │
│ 2019-01-02 │      436231 │       1517 │  0.003477515353104204 │
│ 2019-01-03 │      433288 │       1514 │ 0.0034942117021473018 │
│ 2019-01-04 │     3712253 │    1558555 │   0.41984072745041895 │
│ 2019-01-05 │     3698444 │    1551907 │   0.41961078767178844 │
│ 2019-01-06 │     3712765 │    1556616 │    0.4192605780328138 │
│ 2019-01-07 │     3721564 │    1563005 │   0.41998605962439445 │
└────────────┴─────────────┴────────────┴───────────────────────┘

-- 以着陆页进行分组,每天的会话次数
select dt, landing_page, uniqExact(uid, session_id) as session_cnt
from (select uid, session_id, min(d) dt, argMin(url, timestamp_ms) landing_page from tmp.session_sliceby_30min group by uid, session_id)
group by dt, landing_page
order by dt, session_cnt desc;

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
order by dt, jumper_ratio desc;
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
Engine=MergeTree partition by d order by (uid, session_id);

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
  select uid,
    groupArray((timestamp_ms, url, platform, source, city, brand, purchase_quantity, price)) as dimensions,
    arraySort(x -> x.1, dimensions) as sorted,
    arrayDifference(arrayMap(x -> x.1, sorted)) as diff,
    arrayCumSum(arrayMap(i -> diff[i] >= 1800000 or sorted[i].2 in ('https://ark.analysys.cn/login'), arrayEnumerate(diff))) as session_tag,
    arrayMap(i -> (sorted[i], session_tag[i]), arrayEnumerate(session_tag)) as mixed
  from tmp.session_raw
  group by uid
) array join mixed as item;

-- 包含购物车行为的会话总数
select dt, uniqExactIf(uid, session_id, hasAddCart) session_cnt
from (select uid, session_id, min(d) dt, anyIf(1, url='https://ark.analysys.cn/addCart') as hasAddCart from tmp.session_with_start group by uid, session_id)
group by dt;


-- 人均访问深度
select dt,
    uniqExact(uid, session_id) as uv,
    sum(pv) / uv as avg_visit_depth
from (
    select uid, min(d) dt, session_id, uniqExact(url) as pv
    from tmp.session_sliceby_30min group by uid, session_id
)
group by dt;
```