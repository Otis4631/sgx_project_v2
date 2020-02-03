#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_ecall_normalize_array_t {
	float* ms_array;
	size_t ms_arr_len;
	size_t ms_batch;
} ms_ecall_normalize_array_t;

typedef struct ms_ecall_gemm_t {
	int ms_TA;
	int ms_TB;
	int ms_M;
	int ms_N;
	int ms_K;
	float ms_ALPHA;
	float* ms_A;
	int ms_lda;
	float* ms_B;
	int ms_ldb;
	float ms_BETA;
	float* ms_C;
	int ms_ldc;
	int ms_a_size;
	int ms_b_size;
	int ms_c_size;
} ms_ecall_gemm_t;

typedef struct ms_ecall_activate_array_t {
	float* ms_x;
	int ms_n;
	ACTIVATION ms_a;
} ms_ecall_activate_array_t;

typedef struct ms_ecall_avgpool_forward_t {
	int ms_batch;
	int ms_c;
	int ms_fig_size;
	float* ms_input;
	int ms_input_len;
	float* ms_output;
	int ms_output_len;
} ms_ecall_avgpool_forward_t;

typedef struct ms_ecall_forward_connected_layer_t {
	int ms_TA;
	int ms_TB;
	int ms_M;
	int ms_outputs;
	int ms_K;
	int ms_BN;
	int ms_train;
	float* ms_rolling_mean;
	float* ms_rolling_variance;
	float* ms_scales;
	float* ms_x;
	float* ms_x_norm;
	float* ms_A;
	int ms_lda;
	float* ms_B;
	int ms_ldb;
	float* ms_C;
	int ms_ldc;
	long int ms_a_size;
	long int ms_b_size;
	long int ms_c_size;
	float* ms_bias;
	ACTIVATION ms_a;
} ms_ecall_forward_connected_layer_t;

typedef struct ms_ecall_forward_maxpool_layer_t {
	int ms_pad;
	int ms_h;
	int ms_w;
	int ms_out_h;
	int ms_out_w;
	int ms_c;
	int ms_batch;
	int ms_size;
	int ms_stride;
	float* ms_input;
	int ms_input_len;
	float* ms_output;
	int ms_out_len;
	int* ms_indcies;
} ms_ecall_forward_maxpool_layer_t;

typedef struct ms_ecall_forward_convolutional_layer_t {
	int ms_batch;
	int ms_ic;
	int ms_h;
	int ms_w;
	int ms_size;
	int ms_stride;
	int ms_pad;
	int ms_n_filters;
	int ms_out_h;
	int ms_out_w;
	float* ms_weights;
	int ms_weight_len;
	float* ms_input;
	int ms_in_len;
	float* ms_output;
	int ms_out_len;
	float* ms_biases;
	int ms_bias_len;
	ACTIVATION ms_activation;
} ms_ecall_forward_convolutional_layer_t;

typedef struct ms_ecall_rc4_crypt_t {
	unsigned char* ms_key;
	unsigned long int ms_key_len;
	unsigned char* ms_Data;
	unsigned long int ms_Len;
} ms_ecall_rc4_crypt_t;

typedef struct ms_ecall_forward_cost_layer_t {
	COST_TYPE ms_cost_type;
	int ms_batch;
	int ms_in_len;
	float* ms_input;
	size_t ms_input_size;
	float* ms_truth;
	float* ms_delta;
	float* ms_output;
	float* ms_cost;
} ms_ecall_forward_cost_layer_t;

typedef struct ms_ecall_backward_convolutional_layer_t {
	size_t ms_batch;
	size_t ms_m;
	size_t ms_size;
	size_t ms_ic;
	size_t ms_out_h;
	size_t ms_out_w;
	size_t ms_h;
	size_t ms_w;
	size_t ms_stride;
	size_t ms_pad;
	size_t ms_bias_len;
	size_t ms_output_len;
	size_t ms_input_len;
	size_t ms_weight_len;
	ACTIVATION ms_activation;
	float* ms_output;
	float* ms_input;
	float* ms_delta;
	float* ms_ndelta;
	float* ms_weight;
	float* ms_bias_updates;
	float* ms_weight_updates;
} ms_ecall_backward_convolutional_layer_t;

typedef struct ms_ecall_backward_cost_layer_t {
	size_t ms_input_size;
	int ms_scale;
	float* ms_delta;
	float* ms_n_delta;
} ms_ecall_backward_cost_layer_t;

typedef struct ms_ecall_backward_connected_layer_t {
	int ms_batch;
	int ms_outputs;
	int ms_inputs;
	ACTIVATION ms_a;
	size_t ms_a_len;
	size_t ms_b_len;
	size_t ms_c_len;
	size_t ms_nd_len;
	float* ms_output;
	float* ms_input;
	float* ms_delta;
	float* ms_n_delta;
	float* ms_weights;
	float* ms_bias_updates;
	float* ms_weight_updates;
} ms_ecall_backward_connected_layer_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_str;
} ms_ocall_print_string_t;

static sgx_status_t SGX_CDECL Enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string(ms->ms_str);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[1];
} ocall_table_Enclave = {
	1,
	{
		(void*)Enclave_ocall_print_string,
	}
};
sgx_status_t ecall_normalize_array(sgx_enclave_id_t eid, float* array, size_t arr_len, size_t batch)
{
	sgx_status_t status;
	ms_ecall_normalize_array_t ms;
	ms.ms_array = array;
	ms.ms_arr_len = arr_len;
	ms.ms_batch = batch;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_gemm(sgx_enclave_id_t eid, int TA, int TB, int M, int N, int K, float ALPHA, float* A, int lda, float* B, int ldb, float BETA, float* C, int ldc, int a_size, int b_size, int c_size)
{
	sgx_status_t status;
	ms_ecall_gemm_t ms;
	ms.ms_TA = TA;
	ms.ms_TB = TB;
	ms.ms_M = M;
	ms.ms_N = N;
	ms.ms_K = K;
	ms.ms_ALPHA = ALPHA;
	ms.ms_A = A;
	ms.ms_lda = lda;
	ms.ms_B = B;
	ms.ms_ldb = ldb;
	ms.ms_BETA = BETA;
	ms.ms_C = C;
	ms.ms_ldc = ldc;
	ms.ms_a_size = a_size;
	ms.ms_b_size = b_size;
	ms.ms_c_size = c_size;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_activate_array(sgx_enclave_id_t eid, float* x, int n, ACTIVATION a)
{
	sgx_status_t status;
	ms_ecall_activate_array_t ms;
	ms.ms_x = x;
	ms.ms_n = n;
	ms.ms_a = a;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_avgpool_forward(sgx_enclave_id_t eid, int batch, int c, int fig_size, float* input, int input_len, float* output, int output_len)
{
	sgx_status_t status;
	ms_ecall_avgpool_forward_t ms;
	ms.ms_batch = batch;
	ms.ms_c = c;
	ms.ms_fig_size = fig_size;
	ms.ms_input = input;
	ms.ms_input_len = input_len;
	ms.ms_output = output;
	ms.ms_output_len = output_len;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_forward_connected_layer(sgx_enclave_id_t eid, int TA, int TB, int M, int outputs, int K, int BN, int train, float* rolling_mean, float* rolling_variance, float* scales, float* x, float* x_norm, float* A, int lda, float* B, int ldb, float* C, int ldc, long int a_size, long int b_size, long int c_size, float* bias, ACTIVATION a)
{
	sgx_status_t status;
	ms_ecall_forward_connected_layer_t ms;
	ms.ms_TA = TA;
	ms.ms_TB = TB;
	ms.ms_M = M;
	ms.ms_outputs = outputs;
	ms.ms_K = K;
	ms.ms_BN = BN;
	ms.ms_train = train;
	ms.ms_rolling_mean = rolling_mean;
	ms.ms_rolling_variance = rolling_variance;
	ms.ms_scales = scales;
	ms.ms_x = x;
	ms.ms_x_norm = x_norm;
	ms.ms_A = A;
	ms.ms_lda = lda;
	ms.ms_B = B;
	ms.ms_ldb = ldb;
	ms.ms_C = C;
	ms.ms_ldc = ldc;
	ms.ms_a_size = a_size;
	ms.ms_b_size = b_size;
	ms.ms_c_size = c_size;
	ms.ms_bias = bias;
	ms.ms_a = a;
	status = sgx_ecall(eid, 4, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_forward_maxpool_layer(sgx_enclave_id_t eid, int pad, int h, int w, int out_h, int out_w, int c, int batch, int size, int stride, float* input, int input_len, float* output, int out_len, int* indcies)
{
	sgx_status_t status;
	ms_ecall_forward_maxpool_layer_t ms;
	ms.ms_pad = pad;
	ms.ms_h = h;
	ms.ms_w = w;
	ms.ms_out_h = out_h;
	ms.ms_out_w = out_w;
	ms.ms_c = c;
	ms.ms_batch = batch;
	ms.ms_size = size;
	ms.ms_stride = stride;
	ms.ms_input = input;
	ms.ms_input_len = input_len;
	ms.ms_output = output;
	ms.ms_out_len = out_len;
	ms.ms_indcies = indcies;
	status = sgx_ecall(eid, 5, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_forward_convolutional_layer(sgx_enclave_id_t eid, int batch, int ic, int h, int w, int size, int stride, int pad, int n_filters, int out_h, int out_w, float* weights, int weight_len, float* input, int in_len, float* output, int out_len, float* biases, int bias_len, ACTIVATION activation)
{
	sgx_status_t status;
	ms_ecall_forward_convolutional_layer_t ms;
	ms.ms_batch = batch;
	ms.ms_ic = ic;
	ms.ms_h = h;
	ms.ms_w = w;
	ms.ms_size = size;
	ms.ms_stride = stride;
	ms.ms_pad = pad;
	ms.ms_n_filters = n_filters;
	ms.ms_out_h = out_h;
	ms.ms_out_w = out_w;
	ms.ms_weights = weights;
	ms.ms_weight_len = weight_len;
	ms.ms_input = input;
	ms.ms_in_len = in_len;
	ms.ms_output = output;
	ms.ms_out_len = out_len;
	ms.ms_biases = biases;
	ms.ms_bias_len = bias_len;
	ms.ms_activation = activation;
	status = sgx_ecall(eid, 6, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_rc4_crypt(sgx_enclave_id_t eid, unsigned char* key, unsigned long int key_len, unsigned char* Data, unsigned long int Len)
{
	sgx_status_t status;
	ms_ecall_rc4_crypt_t ms;
	ms.ms_key = key;
	ms.ms_key_len = key_len;
	ms.ms_Data = Data;
	ms.ms_Len = Len;
	status = sgx_ecall(eid, 7, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_forward_cost_layer(sgx_enclave_id_t eid, COST_TYPE cost_type, int batch, int in_len, float* input, size_t input_size, float* truth, float* delta, float* output, float* cost)
{
	sgx_status_t status;
	ms_ecall_forward_cost_layer_t ms;
	ms.ms_cost_type = cost_type;
	ms.ms_batch = batch;
	ms.ms_in_len = in_len;
	ms.ms_input = input;
	ms.ms_input_size = input_size;
	ms.ms_truth = truth;
	ms.ms_delta = delta;
	ms.ms_output = output;
	ms.ms_cost = cost;
	status = sgx_ecall(eid, 8, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_backward_convolutional_layer(sgx_enclave_id_t eid, size_t batch, size_t m, size_t size, size_t ic, size_t out_h, size_t out_w, size_t h, size_t w, size_t stride, size_t pad, size_t bias_len, size_t output_len, size_t input_len, size_t weight_len, ACTIVATION activation, float* output, float* input, float* delta, float* ndelta, float* weight, float* bias_updates, float* weight_updates)
{
	sgx_status_t status;
	ms_ecall_backward_convolutional_layer_t ms;
	ms.ms_batch = batch;
	ms.ms_m = m;
	ms.ms_size = size;
	ms.ms_ic = ic;
	ms.ms_out_h = out_h;
	ms.ms_out_w = out_w;
	ms.ms_h = h;
	ms.ms_w = w;
	ms.ms_stride = stride;
	ms.ms_pad = pad;
	ms.ms_bias_len = bias_len;
	ms.ms_output_len = output_len;
	ms.ms_input_len = input_len;
	ms.ms_weight_len = weight_len;
	ms.ms_activation = activation;
	ms.ms_output = output;
	ms.ms_input = input;
	ms.ms_delta = delta;
	ms.ms_ndelta = ndelta;
	ms.ms_weight = weight;
	ms.ms_bias_updates = bias_updates;
	ms.ms_weight_updates = weight_updates;
	status = sgx_ecall(eid, 9, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_backward_cost_layer(sgx_enclave_id_t eid, size_t input_size, int scale, float* delta, float* n_delta)
{
	sgx_status_t status;
	ms_ecall_backward_cost_layer_t ms;
	ms.ms_input_size = input_size;
	ms.ms_scale = scale;
	ms.ms_delta = delta;
	ms.ms_n_delta = n_delta;
	status = sgx_ecall(eid, 10, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t ecall_backward_connected_layer(sgx_enclave_id_t eid, int batch, int outputs, int inputs, ACTIVATION a, size_t a_len, size_t b_len, size_t c_len, size_t nd_len, float* output, float* input, float* delta, float* n_delta, float* weights, float* bias_updates, float* weight_updates)
{
	sgx_status_t status;
	ms_ecall_backward_connected_layer_t ms;
	ms.ms_batch = batch;
	ms.ms_outputs = outputs;
	ms.ms_inputs = inputs;
	ms.ms_a = a;
	ms.ms_a_len = a_len;
	ms.ms_b_len = b_len;
	ms.ms_c_len = c_len;
	ms.ms_nd_len = nd_len;
	ms.ms_output = output;
	ms.ms_input = input;
	ms.ms_delta = delta;
	ms.ms_n_delta = n_delta;
	ms.ms_weights = weights;
	ms.ms_bias_updates = bias_updates;
	ms.ms_weight_updates = weight_updates;
	status = sgx_ecall(eid, 11, &ocall_table_Enclave, &ms);
	return status;
}

