### 题目一
```sql
select dt,
  uniqExact(uid, session_id) as session_cnt,
  uniqExact(uid) as uv,
  sum(duration) as sum_duration
from (
  select uid, session_id, date_str as dt, max(timestamp_ms)-min(timestamp_ms) as duration
  from tmp.bak group by uid, session_id, date_str
) group by dt;
```

结果集
10 rows in set. Elapsed: 18.230 sec. Processed 289.98 million rows, 21.46 GB (15.91 million rows/s., 1.18 GB/s.) 
┌─dt───────┬─session_cnt─┬─────uv─┬───sum_duration─┐
│ 20190906 │     1103792 │ 552406 │ 51856059188127 │
│ 20190902 │     1104104 │ 552503 │ 51853238199372 │
│ 20190907 │     1104113 │ 552555 │ 51849301432585 │
│ 20190903 │     1103896 │ 552355 │ 51850710422041 │
│ 20190910 │     1104031 │ 552546 │ 51879137284872 │
│ 20190904 │     1103908 │ 552434 │ 51862416309852 │
│ 20190909 │     1104167 │ 552417 │ 51863692749891 │
│ 20190905 │     1103814 │ 552287 │ 51847682571815 │
│ 20190908 │     1104220 │ 552563 │ 51857512039286 │
│ 20190901 │     1057131 │ 529195 │ 48398297909315 │
└──────────┴─────────────┴────────┴────────────────┘

### 题目二 会话次数、用户次数、跳出次数
```sql
select dt, landing_page, 
    uniqExact(uid, session_id) as session_cnt,
    uniqExact(uid) as uv,
    sum(pv = 1) as jumper_times
from (
    select uid, session_id, date_str dt, 
        argMin(url, timestamp_ms) landing_page,
        uniqExact(url) as pv
    from tmp.session_raw group by uid, session_id, date_str
)
group by dt, landing_page
order by dt, session_cnt desc;
```
Code: 241. DB::Exception: Received from localhost:9000. DB::Exception: Memory limit (for query) exceeded:




