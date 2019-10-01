
### 测试思路
1. 先用 100 万数据集测试
2. 再用 1亿数据 验证结果, 使用 ClickHouse 计算正确结果
3. 监测内存使用量


### 算法思路
* 100GB 数据, 假设一条记录 192 Bytes, 总共约 100 * 1024 * 1024 * 1024 / 200 = 536870912 rows(5亿多条数据)
* 1GB 内存能装大概 5368709 条数据
* 分治
    1. 让一样的 url 都分配到同一个文件
    2. 让我们考虑一种情况，假如某一个 hash 过后的文件超过 5368709 条记录，那么我们的内存是装不下的
        2.1 先尝试使用 HashMap 统计(因为很有可能某一个 url 出现频次较高，如果 url 频次分布均匀那么每个文件分配到的数量相差不多)
        2.2 如果超额则再分治
* 归并
    1. 对每个文件求 topK 数据(topK 数据由最小堆维护)
    2. 把 X 个文件的 topK 再归并一次得到最终结果


### 优化思路
1. 如果文件个数少于模数，那么一定有大量重复，直接用 HashMap 即可
2. 减少 IO 次数


### 复杂度分析
* 分治扫描一次 O(N)
* 对每个文件求 topK = O(N) * log(K)
* 归并 X * K * log(K)
* 总体负杂度 O(N) + O(N)*log(K) + X * K * log(K) = O(N) * log(K)
* IO 次数
    1. 分治扫描一次
    2. 对每个文件求 topK



### 扩展性
1. 如果数据分布不确定怎么办, 考虑两个极端 
    1. 所有数据都是一样的
    2. 所有数据都不一样
2. K 为任意数怎么办
3. 考虑对任意类型 topK 比如, 整型/IP/查询关键字


### 运行
```
clang++ -std=c++11 main.cpp && ./a.out
```

### 问题记录

1. C++ 读文件 to int https://stackoverflow.com/questions/7868936/read-file-line-by-line-using-ifstream-in-c
2. C++ STL 容器，在函数结束时会释放内存吗
3. C++ 字符串格式化有库吗
4. C++11 iterate Map https://stackoverflow.com/questions/6963894/how-to-use-range-based-for-loop-with-stdmap
5. C++ 引用怎么用