# Test Log

## Batch: 256
### cipher difference
Condition|Time| Space |
-|-|-|
Plain|0.36s|/|
enclave without cipher| 2.9s| 29008KB≈28.32MB|
enclave with rc4 cipher| 0.70s| 29088KB≈28.40MB|

第一层卷积中添加了batch normalization 后由3.4s变为 3.9s, 主要因为对l.x, l.x_norm 加解密耗时。

### Multi-thread difference
Condition|Time| Space |
-|-|-|
Plain OpenMP|2.7s|/|
enclave without cipher| 2.9s| 29008KB≈28.32MB|
enclave with rc4 cipher| 3.9s| 29088KB≈28.40MB|

