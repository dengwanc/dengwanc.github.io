### 输出结果使用压缩
```
set hive.exec.compress.output=true; -- trun on
set mapred.output.compression.codec=; -- control format
-- 注意 Gzip 是不可分割的
```