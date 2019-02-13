> https://www.bilibili.com/video/av3781284 缪柏其教授
> http://staff.ustc.edu.cn/~zwp/teach/Prob-Stat/probstat.htm 讲义

## ch1 随机事件与概率

* 仔细考虑样本空间
* 提供的信息=条件 => 条件概率
* 公理3 => 全概率公式
* 贝叶斯公式 => 因果关系互换
* 独立性 P(AB) = P(A)*P(B) 
    * P(ABC) = P(A)P(B)P(C)
    * P(AB) = P(A)*P(B)
    * P(AC) = P(A)*P(C)
    * P(BC) = P(B)*P(C)

## ch2 随机变量及其分布

* 分布 
* 分布函数 F(x) = P(X <= x)
* 密度函数 ∫ f(x)dx = 1
* 目的都是为了算概率!

* 离散型: 二项分布
    * X ~ B(n, p)
    * 小概率事件在试验次数足够多时必然发生(命中率低的人打靶)
* 离散型: 泊松分布
    * X ~ Poisson(λ) // λ=n*p
    * 二项分布在一定条件下可以用泊松分布近似代替(非常接近)
        * n > 30
        * n*p <= 5
* 连续型: 指数分布
    * X ~ exp(λ)
    * 无后效性 P(X > t+s | X > t) = P(X > s)
* 连续型: 正态分布
    * X ~ normal(μ, σ^2)
    * f(x) 关于 x=μ 对称
    * σ 越小, f(x) 峰越陡峭, σ 越大, f(x) 峰越平缓
    * 3σ 原则 => 0.9974 