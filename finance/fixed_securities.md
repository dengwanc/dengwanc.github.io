### Pure Discount Bond
```
price = face_value / (1 + r)^T
```

R1 is interest rate between today and next_year
R3 is interest rate between year_2 and year_3
Rt is interest rate between year_t-1 and year_t

```
price = face_value / ( (1 + R1) * (1 + R2) * ... (1 + Rt) )
      = face_value / (1 + r_0t) ^ t # r_0t means Today's T-year Spot Rate
```

Supoose we observe serveral discont bond prices Today
```
{P0~1, P0~2, ..., P0~T} -> {r0~1, r0~2, ..., r0~T}
```

today's forward rates between dates T-1 and T
```
P_0t-1/P_0t = 1 + forward_rate = (1 + r_0t)^t/(1 + r_0t-1)^(t-1) # it's a forecast of the future spot rate between T-1 and T
```

### Coupon Bonds
```
P = coupon/(1 + y) + coupon/(1 + y)^2 + ... + (coupon + face_value)/(1+y)^T
```