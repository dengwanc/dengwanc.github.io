> https://zhuanlan.zhihu.com/p/30823204

### 看了下 ClickHouse 源码实现 windowFunnel 两点感受
1. windowFunnel 算法实现的很 brilliant
2. ClickHouse 源码封装的非常简洁，可读性非常高，在保证高性能的同时源码能做到这点，可见苏联专家代码功底有多深厚

### 怎么看源码
0. C++11 的知识要掌握, 源码里有大量运用 
    0.1 rhs means right-hand-side
1. 列是怎么实现的
    1.0 COW is intended for the cases when you need to share states of large objects(Copy on Write)
    1.1 接口 IColumn::filter/IColumn::permute/IColumn::cut
    1.2 ColumnVector ColumnString 的实现
    1.3 getData() = 实际数据用 PODArray 装载, PODArray 就是内存里的一片连续区域, 拷贝/向量化计算非常高效
    1.4 `Field operator[](size_t n)` PODArray<T> 是怎么变成 Field 的 ?
2. 块是数据处理的基本单位
    2.1 Container = std::vector<ColumnWithTypeAndName>



### 下面着重介绍源码相关细节

### 备注问题
1. 分布式表怎么表示中间结果的,怎么做 Merge 的
2. 分布式表在节点变更时如何减少数据迁移(未来要做 auto-blance)
3. 向量化引擎到底好处是什么, 如何实现的 (更多的是以列为计算对象) __SSE2__
4. 复制表是如何实现的
5. 列式存储是如何转换成行的 
6. Replacing 引擎如何实现的
3. 为什么 IO 模块比 原生 iostream 模块高效
4. 聚合函数的中间状态到底是怎么表示的
5. 聚合函数的中间状态是怎么实现累加的
6. 重点对标 Impala 的定位去看 ClickHouse 的实现思路/看一下同行对源码的解读

### C++ 编码习惯
0. CodeStyle 保持一致性有利于可读性/并且有利于搜索
1. const 永远在类型左边
2. 用 static_cast 做类型转换
3. 写注释有利于反思这段代码的正确性/是否需要/设计错误
4. English Only
5. `///` and `/**  */` are considered "documentation"
6. 大多数情况推荐使用 const 引用
7. 内存管理
    7.1 Library Code can ONLY USE DELETE
    7.2 Application Code DO NOT USE DELETE
    7.3 place an object on the stack
    7.4 make object as a member of another class
    7.5 For a large number of small objects, use containers
    7.6 For small number of objects that reside in the heap, use shared_ptr/unique_ptr
8. 如果要做 C++ 开发，重中之重是 C++ 内存模型
