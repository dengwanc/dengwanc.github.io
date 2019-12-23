
```
price = face_value / (1 + r)^T
```

R1 is interest rate between today and next_year
R3 is interest rate between year_2 and year_3
Rt is interest rate between year_t-1 and year_t

```
price = face_value / ( (1 + R1) * (1 + R2) * (1 + R3) )
```

Supoose we observe serveral discont bond prices Today
```
{P0~1, P0~2, ..., P0~T} -> {r0~1, r0~2, ..., r0~T}
```

today's forward rates between dates T-1 and T
```
P0-t~1/P0~t = 1 + forward_rate
```
