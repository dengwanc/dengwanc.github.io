## dig命令的+trace参数可以显示DNS的整个分级查询过程
```
dig +trace maimai.cn
```

## Key Properties of OLAP Scenario

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
