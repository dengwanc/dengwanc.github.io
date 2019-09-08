### 输出结果使用压缩
```
set hive.exec.compress.output=true; -- trun on
set mapred.output.compression.codec=; -- control format
-- 注意 Gzip 是不可分割的
```

### Hive 使用问题

1. 每次加载 UDF 函数很麻烦 => .hiverc
2. 分桶的作用 map-join/抽样
3. 该用哪种压缩模式 开启压缩 or 使用 ORC

### 数据分析

1. 排序/TOP-N 问题
2. Page Session 分割/时序分析(行间计算)
3. 有序漏斗/windowFunnel

### 几个瓶颈

1. 没有认识的同道友人打比赛
2. 如何学习到先进的大数据技术方法