#include <stdlib.h>
#include <math.h>
#include "e_gemm.h"
#include "Enclave_t.h"
#include "enclave.h"
/*
** 该函数只是调用了gemm_cpu()函数，并且将参数原封不动的传给gemm_cpu()
*/
void ecall_gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float **A, int lda, 
        float **B, int ldb,
        float BETA,
        float **C, int ldc)
{
    gemm_cpu(TA,  TB,  M, N, K, ALPHA , *A, lda, *B, ldb,BETA,*C,ldc);
}

void gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc)
{
    gemm_cpu(TA,  TB,  M, N, K, ALPHA ,A, lda, B, ldb,BETA,C,ldc);
}

void gemm_nn(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    #pragma omp parallel for
    for(int i = 0; i < M; ++i){
        printf("%d\n", omp_get_num_threads());
        for(int k = 0; k < K; ++k){
            register float A_PART = ALPHA*A[i*lda+k];
            for(int j = 0; j < N; ++j){
                C[i*ldc+j] += A_PART*B[k*ldb+j];
            }
        }
    }
}

void gemm_nt(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    #pragma omp parallel for
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
            register float sum = 0;
            for(int k = 0; k < K; ++k){
                sum += ALPHA*A[i*lda+k]*B[j*ldb + k];
            }
            C[i*ldc+j] += sum;
        }
    }
}

void gemm_tn(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    #pragma omp parallel for
    for(int i = 0; i < M; ++i){
        for(int k = 0; k < K; ++k){
            register float A_PART = ALPHA*A[k*lda+i];
            for(int j = 0; j < N; ++j){
                C[i*ldc+j] += A_PART*B[k*ldb+j];
            }
        }
    }
}

void gemm_tt(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    #pragma omp parallel for
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
            register float sum = 0;
            for(int k = 0; k < K; ++k){
                sum += ALPHA*A[i+k*lda]*B[k+j*ldb];
            }
            C[i*ldc+j] += sum;
        }
    }
}


/*
**  功能：矩阵计算，完成C = ALPHA * A * B + BETA * C矩阵计算，最后的输出为C
**  输入： 
**        TA,TB   是否需要对A,B做转置操作，是为1,否为0（要不要转置取决于A,B之间维度是否匹配，比如A:3*2,B:4*2，则需要对B转置，才满足矩阵乘法维度匹配规则）
**        M       A,C的行数（若A需要转置，则此处给出转置后的A即A'的行数，而不是转置前的）
**        N       B,C的列数（若B需要转置，则此处给出转置后的B即B'的列数，而不是转置前的）
**        K       A的列数，B的行数（同样，若A与B中的二者或者其中一个需要转置，则不管怎样，转置后的A，B必须行列能够匹配，符合矩阵乘法规则，K也是转置后的值，不是转置前的）
**        A,B,C   输入矩阵（一维数组格式）
**        ALPHA   系数
**        BETA    系数
**        lda     A的列数（不做转置）或者行数（做转置，且给的是转置后A即A'的行数）
**        ldb     B的列数（不做转置）或者行数（做转置，且给的是转置后B即B'的行数）
**        ldc     C的列数
**  说明：如果TA = 0, TB = 0，那么计算的是C = ALPHA * A * B + BETA * C,此时M是A,C的行数，N是B,C的列数，K是A的列数、B的行数，lda是A的列数，ldb是B的列数；
**       如果TA = 1, TB = 0，那么计算的是C = ALPHA * A' * B + BETA * C,此时M是A’,C的行数，N是B,C的列数，K是A'的列数、B的行数，lda是A'的行数，ldb是B的列数；
**       如果TA = 0, TB = 1，那么计算的是C = ALPHA * A * B' + BETA * C,此时M是A,C的行数，N是B',C的列数，K是A的列数、B'的行数，lda是A的列数，ldb是B'的行数；
**       如果TA = 1, TB = 1，那么计算的是C = ALPHA * A' * B' + BETA * C,此时M是A’,C的行数，N是B',C的列数，K是A'的列数、B'的行数，lda是A'的行数，ldb是B'的行数；
**       总之，参与计算的矩阵必须满足矩阵行列匹配规则。比如A为2*3，B为3*2，C为2*2，那么就是第一种情况；而如果A为3*2，B为3*2，C为2*2,
**       那么就是第二种情况；如果A为2*3，B为2*3，C为2*2,对应第三种情况；如果A为2*3，B为2*3，C为2*2,对应第四种情况。
**  链接：此函数是用C实现矩阵乘法运算，这部分代码应该是模仿的Caffe中的math_functions.cpp的代码
**       参考博客：http://www.voidcn.com/blog/thy_2014/article/p-6149690.html
**  举例说明： 这个函数比较难以理解的地方在于A,B有没有转置这个问题上。首先要清楚，虽然这里A,B,C都是矩阵，但其实都是用一维数组按行保存的，
**           举个例子，假设： A = [1, 2, 3, 2, 2, 1], B = [2, 0, 1, 1, 2, 1], C = [3, 0, 1, 2] （这些输入是打死不变的，
**           都是一维数组格式），且C为2*2的矩阵，即C = [3, 0; 1, 2]，那么要进行C = ALPHA * A * B + BETA * C的计算，
**           必须满足矩阵乘法行列匹配规则，则参与运算的第一个矩阵只能为2*3，第二个只能为3*2，因为A,B的元素个数已经固定为6个。
**           下面分别说明gemm_nn(),gemm_tn(),gemm_nt,gemm_tt()四个函数对该例子的计算。
**           诚如上所述，不管A, B有没有转置，反正最后参与计算的两个矩阵必须前者为2*3,后者为3*2。如果使用gemm_nn()，A,B都没有转置，
**           那么就要求没有转置的A,B分别为2*3,3*2矩阵，则 A = [ 1, 2, 3; 2, 2, 1], B = [2, 0; 1, 1; 2, 1], 
**           调用gemm_nn(2, 2, 3, 1, A, 3, B, 2, C, 2)计算得到 C = [13, 5; 9, 5]（其中ALPHA = BETA = 1，下同）；

**           如果要用gemm_tn()函数，即A需要进行转置之后才能计算，也即转置之后的维度为2*3,而转置之前的维度为3*2，B没有转置，
**           本身就是3*2的矩阵，这样，A = [ 1, 2; 3, 2; 2, 1], A' = [1, 3, 2; 2, 2, 1], B = [2, 0; 1, 1; 2, 1]，
**           gemm_tn(2, 2, 3, 1, A, 2, B, 2, C, 2)函数实际计算的是A'*B+C的值，注意此时的A与gemm_nn()中的A有什么不同，
**           输入的一维数组还是[1, 2, 3, 2, 2, 1]，如前所述，A是按行保存的，因为此时的A本身是一个3*2的矩阵，按照按行保存规则，
**           就是A = [ 1, 2; 3, 2; 2, 1]，调用gemm_tn()的时候，M, N, K分别为2, 2, 3,都是最终参与计算的矩阵的行列数，
**           因为此处真正参与计算的是A'与B，所以M为A'的行数，即为2,N为B的列数，即为2,K为A'与B的列数，即为3，而此时lda=2，
**           是因为A进行了转置，因此输入的是A'的行数，而不是列数3,ldb=2，为B的列数，最终计算得到C=[12, 5; 9, 5]。
**           对于gemm_nt()与gemm_tt()，与上分析一样，不再赘述了。此部分注释进行了测试，对应测试文件darknet_test_gemm.c。
**  强调： 这一系列的gemm()函数，都带有叠加效果，也即最终的值是保存在C中，但这种保存并不是擦除式的保存，而是叠加式的保存，也就是说，
**        如果进入gemm()函数之前，如果C的元素已经有值了，那么这些值不会被擦除掉，而是会将其叠加，
**        其实看式子就可以看出来：此函数完成的是C = ALPHA * A * B + BETA * C矩阵运算。
**          
*/
void gemm_cpu(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc)
{
    #pragma omp parallel for
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
            C[i*ldc + j] *= BETA;
        }
    }
    // 根据需要，调用下面四种函数之一
    if(!TA && !TB)
        gemm_nn(M, N, K, ALPHA,A,lda, B, ldb,C,ldc);
    else if(TA && !TB)
        gemm_tn(M, N, K, ALPHA,A,lda, B, ldb,C,ldc);
    else if(!TA && TB)
        gemm_nt(M, N, K, ALPHA,A,lda, B, ldb,C,ldc);
    else
        gemm_tt(M, N, K, ALPHA,A,lda, B, ldb,C,ldc);
}