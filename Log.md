# Test Log

## Batch Size: 512
### cipher difference
Condition|Time| Space |
-|-|-|
without enclave|0.772s|/|
enclave without cipher| 0.650s| 58096KB≈56.734MB|
enclave with rc4 cipher| 1.462s| 29088KB≈28.40MB|

*在不加密的情况下，使用enclave耗时比未使用enclave耗时短的原因（猜测）*
+ 在模拟模式下，模拟器可能只是做了简单的地址映射，未做真正的内存拷贝。
+ 代码有细微差距，enclave外的代码在函数调用时传递的是整个结构体，可能更加耗时。

第一层卷积中添加了batch normalization 后由3.4s变为 3.9s, 主要因为对l.x, l.x_norm 加解密耗时。

### Multi-thread difference
Condition|Time| Space |
-|-|-|
Plain OpenMP|0.528s|/|
enclave(DNNL) without cipher| 0.327s| N/A|
enclave with rc4 cipher| 3.9s| N/A|


batch size: 3072
    openMP(num_cpu / 2): 17.9s
    single thread: 20.5s