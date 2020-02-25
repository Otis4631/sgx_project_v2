#include "gemm.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Enclave_u.h"
#include "sgx_err.h"
#include "sys/sysinfo.h"
int num_cpu; 

typedef struct gemm_thread_data {
            int TA,  TB,  M,  N,  K;
            float ALPHA; 
            float *A; int lda;
            float *B; int ldb;
            float BETA;
            float *C; int ldc;
}gemm_thread_data;
extern sgx_enclave_id_t EID;

void thread_gemm(void* ptr) {
    gemm_thread_data* data = (gemm_thread_data*)ptr;
    sgx_status_t ret = ecall_gemm(EID, data->TA, data->TB, data->M, data->N, data->K, data->ALPHA,
                &data->A, data->lda, &data->B, data->ldb, data->BETA, &data->C, data->ldc);
    if(ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

}

void gemm_segmentation(int TA, int TB, int M, int N, int K, float ALPHA, 
            float **A, int lda, 
            float **B, int ldb,
            float BETA,
            float **C, int ldc) 
{
    num_cpu = get_nprocs_conf();
    int offset_sum = 0;
    pthread_t *threads = calloc(num_cpu, sizeof(pthread_t));
    float *a, *b, *c;
    a = *A, b = *B, c = *C;
    for(int i = 0; i < num_cpu; i++) {
        pthread_t thread;
        int batch_offset = (i + 1) * M / num_cpu - i * M / num_cpu;
        gemm_thread_data args = {TA, TB, batch_offset, N, K, ALPHA, a + (offset_sum * K), lda, b, ldb, BETA, c, ldc};

        offset_sum += batch_offset;
        if(pthread_create(&thread, 0, thread_gemm, (void *)&args)) error("Thread creation failed");
        threads[i] = thread;
    }
        for(int i = 0; i < num_cpu; ++i){
            pthread_join(threads[i], 0);
    }
}


    
void gemm_bin(int M, int N, int K, float ALPHA, 
        char  *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    int i,j,k;
    for(i = 0; i < M; ++i){
        for(k = 0; k < K; ++k){
            char A_PART = A[i*lda+k];
            if(A_PART){
                for(j = 0; j < N; ++j){
                    C[i*ldc+j] += B[k*ldb+j];
                }
            } else {
                for(j = 0; j < N; ++j){
                    C[i*ldc+j] -= B[k*ldb+j];
                }
            }
        }
    }
}

float *random_matrix(int rows, int cols)
{
    int i;
    float *m = calloc(rows*cols, sizeof(float));
    for(i = 0; i < rows*cols; ++i){
        m[i] = (float)rand()/RAND_MAX;
    }
    return m;
}

void time_random_matrix(int TA, int TB, int m, int k, int n)
{
    float *a;
    if(!TA) a = random_matrix(m,k);
    else a = random_matrix(k,m);
    int lda = (!TA)?k:m;
    float *b;
    if(!TB) b = random_matrix(k,n);
    else b = random_matrix(n,k);
    int ldb = (!TB)?n:k;

    float *c = random_matrix(m,n);
    int i;
    clock_t start = clock(), end;
    for(i = 0; i<10; ++i){
        gemm_cpu(TA,TB,m,n,k,1,a,lda,b,ldb,1,c,n);
    }
    end = clock();
    printf("Matrix Multiplication %dx%d * %dx%d, TA=%d, TB=%d: %lf ms\n",m,k,k,n, TA, TB, (float)(end-start)/CLOCKS_PER_SEC);
    free(a);
    free(b);
    free(c);
}

/*
** 该函数只是调用了gemm_cpu()函数，并且将参数原封不动的传给gemm_cpu()
*/
void gemm(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc)
{
   num_cpu = get_nprocs_conf();
    gemm_cpu( TA,  TB,  M, N, K, ALPHA,A,lda, B, ldb,BETA,C,ldc);
}

/*
**  功能：被gemm_cpu()函数调用，实际完成C = ALPHA * A * B + C 矩阵计算，
**       输出的C也是按行存储（所有行并成一行）
**  输入： A,B,C   输入矩阵（一维数组格式）
**        ALPHA   系数
**        BETA    系数
**        M       A,C的行数（不做转置）或者A'的行数（做转置），此处A未转置，故为A的行数
**        N       B,C的列数（不做转置）或者B'的列数（做转置），此处B未转置，故为B的列数
**        K       A的列数（不做转置）或者A'的列数（做转置），B的行数（不做转置）或者B'的行数（做转置），此处A,B均未转置，故为A的列数、B的行数
**        lda     A的列数（不做转置）或者A'的行数（做转置），此处A未转置，故为A的列数
**        ldb     B的列数（不做转置）或者B'的行数（做转置），此处B未转置，故为B的列数
**        ldc     C的列数
**  说明1：此函数是用C实现矩阵乘法运算，这部分代码应该是模仿的Caffe中的math_functions.cpp的代码
**       参考博客：http://www.voidcn.com/blog/thy_2014/article/p-6149690.html
**       更为详细的注释参见：gemm_cpu()函数的注释
**  说明2：此函数在gemm_cpu()函数中调用，是其中四种情况之一，A,B都不进行转置
**       函数名称gemm_nn()中的两个nn分别表示not transpose， not transpose
*/
void gemm_nn(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    #pragma omp shedule(guided) parallel for  num_threads(omp_get_num_procs() / 2)
    for(int i = 0; i < M; ++i){
        for(int k = 0; k < K; ++k){
             float A_PART = ALPHA*A[i*lda+k];
            for(int j = 0; j < N; ++j){
                C[i*ldc+j] += A_PART*B[k*ldb+j];
            }
        }
    }
}

/*
**  功能：被gemm_cpu()函数调用，实际完成C = ALPHA * A * B' + C矩阵计算，
**       输出的C也是按行存储（所有行并成一行）
**  输入： A,B,C   输入矩阵（一维数组格式）
**        ALPHA   系数
**        BETA    系数
**        M       A,C的行数（不做转置）或者A'的行数（做转置），此处A未转置，故为A的行数
**        N       B,C的列数（不做转置）或者B'的列数（做转置），此处B转置，故为B’的列数
**        K       A的列数（不做转置）或者A'的列数（做转置），B的行数（不做转置）或者B'的行数（做转置），此处A不转置，B转置，故为A的列数、B'的行数
**        lda     A的列数（不做转置）或者A'的行数（做转置），此处A未转置，故为A的列数
**        ldb     B的列数（不做转置）或者B'的行数（做转置），此处B未转置，故为B'的行数
**        ldc     C的列数
**  说明：此函数是用C实现矩阵乘法运算，这部分代码应该是模仿的Caffe中的math_functions.cpp的代码
**       参考博客：http://www.voidcn.com/blog/thy_2014/article/p-6149690.html
**       更为详细的注释参见：gemm_cpu()函数的注释
**  说明2：此函数在gemm_cpu()函数中调用，是其中四种情况之一，A不进行转置,B转置
**       函数名称gemm_nt()中的nt分别表示not transpose， transpose
*/
void gemm_nt(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
   // int i,j,k;
    #pragma omp shedule(guided) parallel for  num_threads(omp_get_num_procs() / 2)
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
             float sum = 0;
            for(int k = 0; k < K; ++k){
                sum += ALPHA*A[i*lda+k]*B[j*ldb + k];
            }
            C[i*ldc+j] += sum;
        }
    }
}

/*
**  功能：矩阵计算，实际完成C = ALPHA * A' * B + BETA * C矩阵计算
**  输入： A,B,C   输入矩阵（一维数组格式）
**        ALPHA   系数
**        BETA    系数
**        M       A,C的行数（不做转置）或者A'的行数（做转置），此处A转置，故为A'的行数
**        N       B,C的列数（不做转置）或者B'的列数（做转置），此处B未转置，故为B的列数
**        K       A的列数（不做转置）或者A'的列数（做转置），B的行数（不做转置）或者B'的行数（做转置），此处A转置，B不转置，故为A'的列数、B的行数
**        lda     A的列数（不做转置）或者A'的行数（做转置），此处A转置，故为A'的行数  
**        ldb     B的列数（不做转置）或者B'的行数（做转置），此处B未转置，故为B的列数
**        ldc     C的列数
**  说明：此函数是用C实现矩阵乘法运算，这部分代码应该是模仿的Caffe中的math_functions.cpp的代码
**       参考博客：http://www.voidcn.com/blog/thy_2014/article/p-6149690.html
**       更为详细的注释参见：gemm_cpu()函数的注释
**  说明2：此函数在gemm_cpu()函数中调用，是其中四种情况之一，A进行转置,B不转置
**       函数名称gemm_tn()中的tn分别表示transpose， not transpose
*/
void gemm_tn(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
   // int i,j,k;
    #pragma omp shedule(guided) parallel for  num_threads(omp_get_num_procs() / 2)
    for(int i = 0; i < M; ++i){
        for(int k = 0; k < K; ++k){
             float A_PART = ALPHA*A[k*lda+i];
            for(int j = 0; j < N; ++j){
                C[i*ldc+j] += A_PART*B[k*ldb+j];
            }
        }
    }
}

/*
**  功能：矩阵计算，实际完成C = ALPHA * A' * B' + BETA * C矩阵计算
**  输入： A,B,C   输入矩阵（一维数组格式）
**        ALPHA   系数
**        BETA    系数
**        M       A,C的行数（不做转置）或者A'的行数（做转置），此处A转置，故为A'的行数
**        N       B,C的列数（不做转置）或者B'的列数（做转置），此处B转置，故为B'的列数
**        K       A'的列数，B'的行数
**        lda     A的列数（不做转置）或者A'的行数（做转置），此处A转置，故为A'的行数  
**        ldb     B的列数（不做转置）或者B'的行数（做转置），此处B转置，故为B'的行数
**        ldc     C的列数
**  说明：此函数是用C实现矩阵乘法运算，这部分代码应该是模仿的Caffe中的math_functions.cpp的代码
**       参考博客：http://www.voidcn.com/blog/thy_2014/article/p-6149690.html
**       更为详细的注释参见：gemm_cpu()函数的注释
**  说明2：此函数在gemm_cpu()函数中调用，是其中四种情况之一，A,B都进行转置
**       函数名称gemm_tt()中的tt分别表示transpose， transpose
*/
void gemm_tt(int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float *C, int ldc)
{
    //int i,j,k;
    #pragma omp shedule(guided) parallel for  num_threads(omp_get_num_procs() / 2)
    for(int i = 0; i < M; ++i){
        for(int j = 0; j < N; ++j){
             float sum = 0;
            for(int k = 0; k < K; ++k){
                sum += ALPHA*A[i+k*lda]*B[k+j*ldb];
            }
            C[i*ldc+j] += sum;
        }
    }
}

void gemm_cpu(int TA, int TB, int M, int N, int K, float ALPHA, 
        float *A, int lda, 
        float *B, int ldb,
        float BETA,
        float *C, int ldc)
{
    //printf("cpu: %d %d %d %d %d %f %d %d %f %d\n",TA, TB, M, N, K, ALPHA, lda, ldb, BETA, ldc);
   // int i, j;
    // 先把BETA * C计算完了，并将结果存在C中，得到的C将为M行，N列（按行存储在一维数组C中）
    #pragma omp shedule(guided) parallel for  num_threads(omp_get_num_procs() / 2)
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


