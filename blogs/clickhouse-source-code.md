> https://zhuanlan.zhihu.com/p/30823204

### 看了下 ClickHouse 源码实现 windowFunnel 两点感受
1. windowFunnel 算法实现的很 brilliant
2. ClickHouse 源码封装的非常简洁，可读性非常高，在保证高性能的同时源码能做到这点，可见苏联专家代码功底有多深厚

### 怎么看源码

### 下面着重介绍源码相关细节

### 备注问题
1. 分布式表怎么实现分发数据的
2. 分布式表在节点变更时如何减少数据迁移
3. 向量化引擎到底好处是什么, 如何实现的
4. 复制表是如何实现的
5. 列式存储是如何转换成行的
6. Replacing 引擎如何实现的

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
    
