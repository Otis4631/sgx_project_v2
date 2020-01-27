#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


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
	long int ms_a_size;
	long int ms_b_size;
	long int ms_c_size;
	float* ms_bias;
	int ms_bias_len;
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

static sgx_status_t SGX_CDECL sgx_hello(void* pms)
{
	sgx_status_t status = SGX_SUCCESS;
	if (pms != NULL) return SGX_ERROR_INVALID_PARAMETER;
	hello();
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_normalize_array(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_normalize_array_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_normalize_array_t* ms = SGX_CAST(ms_ecall_normalize_array_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_array = ms->ms_array;
	size_t _tmp_arr_len = ms->ms_arr_len;
	size_t _len_array = _tmp_arr_len * sizeof(float);
	float* _in_array = NULL;

	if (sizeof(*_tmp_array) != 0 &&
		(size_t)_tmp_arr_len > (SIZE_MAX / sizeof(*_tmp_array))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_array, _len_array);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_array != NULL && _len_array != 0) {
		if ( _len_array % sizeof(*_tmp_array) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_array = (float*)malloc(_len_array);
		if (_in_array == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_array, _len_array, _tmp_array, _len_array)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_normalize_array(_in_array, _tmp_arr_len, ms->ms_batch);
	if (_in_array) {
		if (memcpy_s(_tmp_array, _len_array, _in_array, _len_array)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_array) free(_in_array);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_gemm(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_gemm_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_gemm_t* ms = SGX_CAST(ms_ecall_gemm_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_A = ms->ms_A;
	int _tmp_a_size = ms->ms_a_size;
	size_t _len_A = _tmp_a_size * sizeof(float);
	float* _in_A = NULL;
	float* _tmp_B = ms->ms_B;
	int _tmp_b_size = ms->ms_b_size;
	size_t _len_B = _tmp_b_size * sizeof(float);
	float* _in_B = NULL;
	float* _tmp_C = ms->ms_C;
	int _tmp_c_size = ms->ms_c_size;
	size_t _len_C = _tmp_c_size * sizeof(float);
	float* _in_C = NULL;

	if (sizeof(*_tmp_A) != 0 &&
		(size_t)_tmp_a_size > (SIZE_MAX / sizeof(*_tmp_A))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_B) != 0 &&
		(size_t)_tmp_b_size > (SIZE_MAX / sizeof(*_tmp_B))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_C) != 0 &&
		(size_t)_tmp_c_size > (SIZE_MAX / sizeof(*_tmp_C))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_A, _len_A);
	CHECK_UNIQUE_POINTER(_tmp_B, _len_B);
	CHECK_UNIQUE_POINTER(_tmp_C, _len_C);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_A != NULL && _len_A != 0) {
		if ( _len_A % sizeof(*_tmp_A) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_A = (float*)malloc(_len_A);
		if (_in_A == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_A, _len_A, _tmp_A, _len_A)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_B != NULL && _len_B != 0) {
		if ( _len_B % sizeof(*_tmp_B) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_B = (float*)malloc(_len_B);
		if (_in_B == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_B, _len_B, _tmp_B, _len_B)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_C != NULL && _len_C != 0) {
		if ( _len_C % sizeof(*_tmp_C) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_C = (float*)malloc(_len_C);
		if (_in_C == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_C, _len_C, _tmp_C, _len_C)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_gemm(ms->ms_TA, ms->ms_TB, ms->ms_M, ms->ms_N, ms->ms_K, ms->ms_ALPHA, _in_A, ms->ms_lda, _in_B, ms->ms_ldb, ms->ms_BETA, _in_C, ms->ms_ldc, _tmp_a_size, _tmp_b_size, _tmp_c_size);
	if (_in_C) {
		if (memcpy_s(_tmp_C, _len_C, _in_C, _len_C)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_A) free(_in_A);
	if (_in_B) free(_in_B);
	if (_in_C) free(_in_C);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_activate_array(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_activate_array_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_activate_array_t* ms = SGX_CAST(ms_ecall_activate_array_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_x = ms->ms_x;
	int _tmp_n = ms->ms_n;
	size_t _len_x = _tmp_n * sizeof(float);
	float* _in_x = NULL;

	if (sizeof(*_tmp_x) != 0 &&
		(size_t)_tmp_n > (SIZE_MAX / sizeof(*_tmp_x))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_x, _len_x);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_x != NULL && _len_x != 0) {
		if ( _len_x % sizeof(*_tmp_x) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_x = (float*)malloc(_len_x);
		if (_in_x == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_x, _len_x, _tmp_x, _len_x)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_activate_array(_in_x, _tmp_n, ms->ms_a);
	if (_in_x) {
		if (memcpy_s(_tmp_x, _len_x, _in_x, _len_x)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_x) free(_in_x);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_avgpool_forward(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_avgpool_forward_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_avgpool_forward_t* ms = SGX_CAST(ms_ecall_avgpool_forward_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_input = ms->ms_input;
	int _tmp_input_len = ms->ms_input_len;
	size_t _len_input = _tmp_input_len * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_output = ms->ms_output;
	int _tmp_output_len = ms->ms_output_len;
	size_t _len_output = _tmp_output_len * sizeof(float);
	float* _in_output = NULL;

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_input_len > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_output_len > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_avgpool_forward(ms->ms_batch, ms->ms_c, ms->ms_fig_size, _in_input, _tmp_input_len, _in_output, _tmp_output_len);
	if (_in_output) {
		if (memcpy_s(_tmp_output, _len_output, _in_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_input) free(_in_input);
	if (_in_output) free(_in_output);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_forward_connected_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_forward_connected_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_forward_connected_layer_t* ms = SGX_CAST(ms_ecall_forward_connected_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_A = ms->ms_A;
	long int _tmp_a_size = ms->ms_a_size;
	size_t _len_A = _tmp_a_size * sizeof(float);
	float* _in_A = NULL;
	float* _tmp_B = ms->ms_B;
	long int _tmp_b_size = ms->ms_b_size;
	size_t _len_B = _tmp_b_size * sizeof(float);
	float* _in_B = NULL;
	float* _tmp_C = ms->ms_C;
	long int _tmp_c_size = ms->ms_c_size;
	size_t _len_C = _tmp_c_size * sizeof(float);
	float* _in_C = NULL;
	float* _tmp_bias = ms->ms_bias;
	int _tmp_bias_len = ms->ms_bias_len;
	size_t _len_bias = _tmp_bias_len * sizeof(float);
	float* _in_bias = NULL;

	if (sizeof(*_tmp_A) != 0 &&
		(size_t)_tmp_a_size > (SIZE_MAX / sizeof(*_tmp_A))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_B) != 0 &&
		(size_t)_tmp_b_size > (SIZE_MAX / sizeof(*_tmp_B))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_C) != 0 &&
		(size_t)_tmp_c_size > (SIZE_MAX / sizeof(*_tmp_C))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_bias) != 0 &&
		(size_t)_tmp_bias_len > (SIZE_MAX / sizeof(*_tmp_bias))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_A, _len_A);
	CHECK_UNIQUE_POINTER(_tmp_B, _len_B);
	CHECK_UNIQUE_POINTER(_tmp_C, _len_C);
	CHECK_UNIQUE_POINTER(_tmp_bias, _len_bias);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_A != NULL && _len_A != 0) {
		if ( _len_A % sizeof(*_tmp_A) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_A = (float*)malloc(_len_A);
		if (_in_A == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_A, _len_A, _tmp_A, _len_A)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_B != NULL && _len_B != 0) {
		if ( _len_B % sizeof(*_tmp_B) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_B = (float*)malloc(_len_B);
		if (_in_B == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_B, _len_B, _tmp_B, _len_B)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_C != NULL && _len_C != 0) {
		if ( _len_C % sizeof(*_tmp_C) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_C = (float*)malloc(_len_C);
		if (_in_C == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_C, _len_C, _tmp_C, _len_C)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_bias != NULL && _len_bias != 0) {
		if ( _len_bias % sizeof(*_tmp_bias) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_bias = (float*)malloc(_len_bias);
		if (_in_bias == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_bias, _len_bias, _tmp_bias, _len_bias)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_forward_connected_layer(ms->ms_TA, ms->ms_TB, ms->ms_M, ms->ms_N, ms->ms_K, ms->ms_ALPHA, _in_A, ms->ms_lda, _in_B, ms->ms_ldb, ms->ms_BETA, _in_C, ms->ms_ldc, _tmp_a_size, _tmp_b_size, _tmp_c_size, _in_bias, _tmp_bias_len, ms->ms_a);
	if (_in_C) {
		if (memcpy_s(_tmp_C, _len_C, _in_C, _len_C)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_A) free(_in_A);
	if (_in_B) free(_in_B);
	if (_in_C) free(_in_C);
	if (_in_bias) free(_in_bias);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_forward_maxpool_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_forward_maxpool_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_forward_maxpool_layer_t* ms = SGX_CAST(ms_ecall_forward_maxpool_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_input = ms->ms_input;
	int _tmp_input_len = ms->ms_input_len;
	size_t _len_input = _tmp_input_len * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_output = ms->ms_output;
	int _tmp_out_len = ms->ms_out_len;
	size_t _len_output = _tmp_out_len * sizeof(float);
	float* _in_output = NULL;
	int* _tmp_indcies = ms->ms_indcies;
	size_t _len_indcies = _tmp_out_len * sizeof(int);
	int* _in_indcies = NULL;

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_input_len > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_out_len > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_indcies) != 0 &&
		(size_t)_tmp_out_len > (SIZE_MAX / sizeof(*_tmp_indcies))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_indcies, _len_indcies);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_indcies != NULL && _len_indcies != 0) {
		if ( _len_indcies % sizeof(*_tmp_indcies) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_indcies = (int*)malloc(_len_indcies);
		if (_in_indcies == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_indcies, _len_indcies, _tmp_indcies, _len_indcies)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_forward_maxpool_layer(ms->ms_pad, ms->ms_h, ms->ms_w, ms->ms_out_h, ms->ms_out_w, ms->ms_c, ms->ms_batch, ms->ms_size, ms->ms_stride, _in_input, _tmp_input_len, _in_output, _tmp_out_len, _in_indcies);
	if (_in_output) {
		if (memcpy_s(_tmp_output, _len_output, _in_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_indcies) {
		if (memcpy_s(_tmp_indcies, _len_indcies, _in_indcies, _len_indcies)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_input) free(_in_input);
	if (_in_output) free(_in_output);
	if (_in_indcies) free(_in_indcies);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_forward_convolutional_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_forward_convolutional_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_forward_convolutional_layer_t* ms = SGX_CAST(ms_ecall_forward_convolutional_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_weights = ms->ms_weights;
	int _tmp_weight_len = ms->ms_weight_len;
	size_t _len_weights = _tmp_weight_len * sizeof(float);
	float* _in_weights = NULL;
	float* _tmp_input = ms->ms_input;
	int _tmp_in_len = ms->ms_in_len;
	size_t _len_input = _tmp_in_len * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_output = ms->ms_output;
	int _tmp_out_len = ms->ms_out_len;
	size_t _len_output = _tmp_out_len * sizeof(float);
	float* _in_output = NULL;
	float* _tmp_biases = ms->ms_biases;
	int _tmp_bias_len = ms->ms_bias_len;
	size_t _len_biases = _tmp_bias_len * sizeof(float);
	float* _in_biases = NULL;

	if (sizeof(*_tmp_weights) != 0 &&
		(size_t)_tmp_weight_len > (SIZE_MAX / sizeof(*_tmp_weights))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_in_len > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_out_len > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_biases) != 0 &&
		(size_t)_tmp_bias_len > (SIZE_MAX / sizeof(*_tmp_biases))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_weights, _len_weights);
	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_biases, _len_biases);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_weights != NULL && _len_weights != 0) {
		if ( _len_weights % sizeof(*_tmp_weights) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_weights = (float*)malloc(_len_weights);
		if (_in_weights == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_weights, _len_weights, _tmp_weights, _len_weights)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_biases != NULL && _len_biases != 0) {
		if ( _len_biases % sizeof(*_tmp_biases) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_biases = (float*)malloc(_len_biases);
		if (_in_biases == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_biases, _len_biases, _tmp_biases, _len_biases)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_forward_convolutional_layer(ms->ms_batch, ms->ms_ic, ms->ms_h, ms->ms_w, ms->ms_size, ms->ms_stride, ms->ms_pad, ms->ms_n_filters, ms->ms_out_h, ms->ms_out_w, _in_weights, _tmp_weight_len, _in_input, _tmp_in_len, _in_output, _tmp_out_len, _in_biases, _tmp_bias_len, ms->ms_activation);
	if (_in_output) {
		if (memcpy_s(_tmp_output, _len_output, _in_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_weights) free(_in_weights);
	if (_in_input) free(_in_input);
	if (_in_output) free(_in_output);
	if (_in_biases) free(_in_biases);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_rc4_crypt(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_rc4_crypt_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_rc4_crypt_t* ms = SGX_CAST(ms_ecall_rc4_crypt_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	unsigned char* _tmp_key = ms->ms_key;
	unsigned long int _tmp_key_len = ms->ms_key_len;
	size_t _len_key = _tmp_key_len * sizeof(unsigned char);
	unsigned char* _in_key = NULL;
	unsigned char* _tmp_Data = ms->ms_Data;
	unsigned long int _tmp_Len = ms->ms_Len;
	size_t _len_Data = _tmp_Len * sizeof(unsigned char);
	unsigned char* _in_Data = NULL;

	if (sizeof(*_tmp_key) != 0 &&
		(size_t)_tmp_key_len > (SIZE_MAX / sizeof(*_tmp_key))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_Data) != 0 &&
		(size_t)_tmp_Len > (SIZE_MAX / sizeof(*_tmp_Data))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_key, _len_key);
	CHECK_UNIQUE_POINTER(_tmp_Data, _len_Data);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_key != NULL && _len_key != 0) {
		if ( _len_key % sizeof(*_tmp_key) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_key = (unsigned char*)malloc(_len_key);
		if (_in_key == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_key, _len_key, _tmp_key, _len_key)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_Data != NULL && _len_Data != 0) {
		if ( _len_Data % sizeof(*_tmp_Data) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_Data = (unsigned char*)malloc(_len_Data);
		if (_in_Data == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_Data, _len_Data, _tmp_Data, _len_Data)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_rc4_crypt(_in_key, _tmp_key_len, _in_Data, _tmp_Len);
	if (_in_Data) {
		if (memcpy_s(_tmp_Data, _len_Data, _in_Data, _len_Data)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_key) free(_in_key);
	if (_in_Data) free(_in_Data);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_forward_cost_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_forward_cost_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_forward_cost_layer_t* ms = SGX_CAST(ms_ecall_forward_cost_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_input = ms->ms_input;
	size_t _tmp_input_size = ms->ms_input_size;
	size_t _len_input = _tmp_input_size * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_truth = ms->ms_truth;
	size_t _len_truth = _tmp_input_size * sizeof(float);
	float* _in_truth = NULL;
	float* _tmp_delta = ms->ms_delta;
	size_t _len_delta = _tmp_input_size * sizeof(float);
	float* _in_delta = NULL;
	float* _tmp_output = ms->ms_output;
	size_t _len_output = _tmp_input_size * sizeof(float);
	float* _in_output = NULL;
	float* _tmp_cost = ms->ms_cost;
	size_t _len_cost = 1 * sizeof(float);
	float* _in_cost = NULL;

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_truth) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_truth))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_delta) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_cost) != 0 &&
		1 > (SIZE_MAX / sizeof(*_tmp_cost))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_truth, _len_truth);
	CHECK_UNIQUE_POINTER(_tmp_delta, _len_delta);
	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_cost, _len_cost);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_truth != NULL && _len_truth != 0) {
		if ( _len_truth % sizeof(*_tmp_truth) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_truth = (float*)malloc(_len_truth);
		if (_in_truth == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_truth, _len_truth, _tmp_truth, _len_truth)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_delta != NULL && _len_delta != 0) {
		if ( _len_delta % sizeof(*_tmp_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_delta = (float*)malloc(_len_delta);
		if (_in_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_delta, _len_delta, _tmp_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_cost != NULL && _len_cost != 0) {
		if ( _len_cost % sizeof(*_tmp_cost) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_cost = (float*)malloc(_len_cost);
		if (_in_cost == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_cost, _len_cost, _tmp_cost, _len_cost)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_forward_cost_layer(ms->ms_cost_type, ms->ms_batch, ms->ms_in_len, _in_input, _tmp_input_size, _in_truth, _in_delta, _in_output, _in_cost);
	if (_in_delta) {
		if (memcpy_s(_tmp_delta, _len_delta, _in_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_output) {
		if (memcpy_s(_tmp_output, _len_output, _in_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_cost) {
		if (memcpy_s(_tmp_cost, _len_cost, _in_cost, _len_cost)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_input) free(_in_input);
	if (_in_truth) free(_in_truth);
	if (_in_delta) free(_in_delta);
	if (_in_output) free(_in_output);
	if (_in_cost) free(_in_cost);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_backward_convolutional_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_backward_convolutional_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_backward_convolutional_layer_t* ms = SGX_CAST(ms_ecall_backward_convolutional_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_output = ms->ms_output;
	size_t _tmp_output_len = ms->ms_output_len;
	size_t _len_output = _tmp_output_len * sizeof(float);
	float* _in_output = NULL;
	float* _tmp_input = ms->ms_input;
	size_t _tmp_input_len = ms->ms_input_len;
	size_t _len_input = _tmp_input_len * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_delta = ms->ms_delta;
	size_t _len_delta = _tmp_output_len * sizeof(float);
	float* _in_delta = NULL;
	float* _tmp_ndelta = ms->ms_ndelta;
	size_t _len_ndelta = _tmp_input_len * sizeof(float);
	float* _in_ndelta = NULL;
	float* _tmp_weight = ms->ms_weight;
	size_t _tmp_weight_len = ms->ms_weight_len;
	size_t _len_weight = _tmp_weight_len * sizeof(float);
	float* _in_weight = NULL;
	float* _tmp_bias_updates = ms->ms_bias_updates;
	size_t _tmp_bias_len = ms->ms_bias_len;
	size_t _len_bias_updates = _tmp_bias_len * sizeof(float);
	float* _in_bias_updates = NULL;
	float* _tmp_weight_updates = ms->ms_weight_updates;
	size_t _len_weight_updates = _tmp_weight_len * sizeof(float);
	float* _in_weight_updates = NULL;

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_output_len > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_input_len > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_delta) != 0 &&
		(size_t)_tmp_output_len > (SIZE_MAX / sizeof(*_tmp_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_ndelta) != 0 &&
		(size_t)_tmp_input_len > (SIZE_MAX / sizeof(*_tmp_ndelta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_weight) != 0 &&
		(size_t)_tmp_weight_len > (SIZE_MAX / sizeof(*_tmp_weight))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_bias_updates) != 0 &&
		(size_t)_tmp_bias_len > (SIZE_MAX / sizeof(*_tmp_bias_updates))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_weight_updates) != 0 &&
		(size_t)_tmp_weight_len > (SIZE_MAX / sizeof(*_tmp_weight_updates))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_delta, _len_delta);
	CHECK_UNIQUE_POINTER(_tmp_ndelta, _len_ndelta);
	CHECK_UNIQUE_POINTER(_tmp_weight, _len_weight);
	CHECK_UNIQUE_POINTER(_tmp_bias_updates, _len_bias_updates);
	CHECK_UNIQUE_POINTER(_tmp_weight_updates, _len_weight_updates);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_delta != NULL && _len_delta != 0) {
		if ( _len_delta % sizeof(*_tmp_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_delta = (float*)malloc(_len_delta);
		if (_in_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_delta, _len_delta, _tmp_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_ndelta != NULL && _len_ndelta != 0) {
		if ( _len_ndelta % sizeof(*_tmp_ndelta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_ndelta = (float*)malloc(_len_ndelta);
		if (_in_ndelta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_ndelta, _len_ndelta, _tmp_ndelta, _len_ndelta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_weight != NULL && _len_weight != 0) {
		if ( _len_weight % sizeof(*_tmp_weight) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_weight = (float*)malloc(_len_weight);
		if (_in_weight == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_weight, _len_weight, _tmp_weight, _len_weight)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_bias_updates != NULL && _len_bias_updates != 0) {
		if ( _len_bias_updates % sizeof(*_tmp_bias_updates) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_bias_updates = (float*)malloc(_len_bias_updates);
		if (_in_bias_updates == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_bias_updates, _len_bias_updates, _tmp_bias_updates, _len_bias_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_weight_updates != NULL && _len_weight_updates != 0) {
		if ( _len_weight_updates % sizeof(*_tmp_weight_updates) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_weight_updates = (float*)malloc(_len_weight_updates);
		if (_in_weight_updates == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_weight_updates, _len_weight_updates, _tmp_weight_updates, _len_weight_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_backward_convolutional_layer(ms->ms_batch, ms->ms_m, ms->ms_size, ms->ms_ic, ms->ms_out_h, ms->ms_out_w, ms->ms_h, ms->ms_w, ms->ms_stride, ms->ms_pad, _tmp_bias_len, _tmp_output_len, _tmp_input_len, _tmp_weight_len, ms->ms_activation, _in_output, _in_input, _in_delta, _in_ndelta, _in_weight, _in_bias_updates, _in_weight_updates);
	if (_in_ndelta) {
		if (memcpy_s(_tmp_ndelta, _len_ndelta, _in_ndelta, _len_ndelta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_bias_updates) {
		if (memcpy_s(_tmp_bias_updates, _len_bias_updates, _in_bias_updates, _len_bias_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_weight_updates) {
		if (memcpy_s(_tmp_weight_updates, _len_weight_updates, _in_weight_updates, _len_weight_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_output) free(_in_output);
	if (_in_input) free(_in_input);
	if (_in_delta) free(_in_delta);
	if (_in_ndelta) free(_in_ndelta);
	if (_in_weight) free(_in_weight);
	if (_in_bias_updates) free(_in_bias_updates);
	if (_in_weight_updates) free(_in_weight_updates);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_backward_cost_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_backward_cost_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_backward_cost_layer_t* ms = SGX_CAST(ms_ecall_backward_cost_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_delta = ms->ms_delta;
	size_t _tmp_input_size = ms->ms_input_size;
	size_t _len_delta = _tmp_input_size * sizeof(float);
	float* _in_delta = NULL;
	float* _tmp_n_delta = ms->ms_n_delta;
	size_t _len_n_delta = _tmp_input_size * sizeof(float);
	float* _in_n_delta = NULL;

	if (sizeof(*_tmp_delta) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_n_delta) != 0 &&
		(size_t)_tmp_input_size > (SIZE_MAX / sizeof(*_tmp_n_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_delta, _len_delta);
	CHECK_UNIQUE_POINTER(_tmp_n_delta, _len_n_delta);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_delta != NULL && _len_delta != 0) {
		if ( _len_delta % sizeof(*_tmp_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_delta = (float*)malloc(_len_delta);
		if (_in_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_delta, _len_delta, _tmp_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_n_delta != NULL && _len_n_delta != 0) {
		if ( _len_n_delta % sizeof(*_tmp_n_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_n_delta = (float*)malloc(_len_n_delta);
		if (_in_n_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_n_delta, _len_n_delta, _tmp_n_delta, _len_n_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_backward_cost_layer(_tmp_input_size, ms->ms_scale, _in_delta, _in_n_delta);
	if (_in_n_delta) {
		if (memcpy_s(_tmp_n_delta, _len_n_delta, _in_n_delta, _len_n_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_delta) free(_in_delta);
	if (_in_n_delta) free(_in_n_delta);
	return status;
}

static sgx_status_t SGX_CDECL sgx_ecall_backward_connected_layer(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_ecall_backward_connected_layer_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_ecall_backward_connected_layer_t* ms = SGX_CAST(ms_ecall_backward_connected_layer_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	float* _tmp_output = ms->ms_output;
	size_t _tmp_a_len = ms->ms_a_len;
	size_t _len_output = _tmp_a_len * sizeof(float);
	float* _in_output = NULL;
	float* _tmp_input = ms->ms_input;
	size_t _tmp_b_len = ms->ms_b_len;
	size_t _len_input = _tmp_b_len * sizeof(float);
	float* _in_input = NULL;
	float* _tmp_delta = ms->ms_delta;
	size_t _len_delta = _tmp_a_len * sizeof(float);
	float* _in_delta = NULL;
	float* _tmp_n_delta = ms->ms_n_delta;
	size_t _tmp_nd_len = ms->ms_nd_len;
	size_t _len_n_delta = _tmp_nd_len * sizeof(float);
	float* _in_n_delta = NULL;
	float* _tmp_weights = ms->ms_weights;
	size_t _tmp_c_len = ms->ms_c_len;
	size_t _len_weights = _tmp_c_len * sizeof(float);
	float* _in_weights = NULL;
	float* _tmp_bias_updates = ms->ms_bias_updates;
	int _tmp_outputs = ms->ms_outputs;
	size_t _len_bias_updates = _tmp_outputs * sizeof(float);
	float* _in_bias_updates = NULL;
	float* _tmp_weight_updates = ms->ms_weight_updates;
	size_t _len_weight_updates = _tmp_c_len * sizeof(float);
	float* _in_weight_updates = NULL;

	if (sizeof(*_tmp_output) != 0 &&
		(size_t)_tmp_a_len > (SIZE_MAX / sizeof(*_tmp_output))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_input) != 0 &&
		(size_t)_tmp_b_len > (SIZE_MAX / sizeof(*_tmp_input))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_delta) != 0 &&
		(size_t)_tmp_a_len > (SIZE_MAX / sizeof(*_tmp_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_n_delta) != 0 &&
		(size_t)_tmp_nd_len > (SIZE_MAX / sizeof(*_tmp_n_delta))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_weights) != 0 &&
		(size_t)_tmp_c_len > (SIZE_MAX / sizeof(*_tmp_weights))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_bias_updates) != 0 &&
		(size_t)_tmp_outputs > (SIZE_MAX / sizeof(*_tmp_bias_updates))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	if (sizeof(*_tmp_weight_updates) != 0 &&
		(size_t)_tmp_c_len > (SIZE_MAX / sizeof(*_tmp_weight_updates))) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	CHECK_UNIQUE_POINTER(_tmp_output, _len_output);
	CHECK_UNIQUE_POINTER(_tmp_input, _len_input);
	CHECK_UNIQUE_POINTER(_tmp_delta, _len_delta);
	CHECK_UNIQUE_POINTER(_tmp_n_delta, _len_n_delta);
	CHECK_UNIQUE_POINTER(_tmp_weights, _len_weights);
	CHECK_UNIQUE_POINTER(_tmp_bias_updates, _len_bias_updates);
	CHECK_UNIQUE_POINTER(_tmp_weight_updates, _len_weight_updates);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_output != NULL && _len_output != 0) {
		if ( _len_output % sizeof(*_tmp_output) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_output = (float*)malloc(_len_output);
		if (_in_output == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_output, _len_output, _tmp_output, _len_output)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_input != NULL && _len_input != 0) {
		if ( _len_input % sizeof(*_tmp_input) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_input = (float*)malloc(_len_input);
		if (_in_input == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_input, _len_input, _tmp_input, _len_input)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_delta != NULL && _len_delta != 0) {
		if ( _len_delta % sizeof(*_tmp_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_delta = (float*)malloc(_len_delta);
		if (_in_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_delta, _len_delta, _tmp_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_n_delta != NULL && _len_n_delta != 0) {
		if ( _len_n_delta % sizeof(*_tmp_n_delta) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_n_delta = (float*)malloc(_len_n_delta);
		if (_in_n_delta == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_n_delta, _len_n_delta, _tmp_n_delta, _len_n_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_weights != NULL && _len_weights != 0) {
		if ( _len_weights % sizeof(*_tmp_weights) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_weights = (float*)malloc(_len_weights);
		if (_in_weights == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_weights, _len_weights, _tmp_weights, _len_weights)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_bias_updates != NULL && _len_bias_updates != 0) {
		if ( _len_bias_updates % sizeof(*_tmp_bias_updates) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_bias_updates = (float*)malloc(_len_bias_updates);
		if (_in_bias_updates == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_bias_updates, _len_bias_updates, _tmp_bias_updates, _len_bias_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}
	if (_tmp_weight_updates != NULL && _len_weight_updates != 0) {
		if ( _len_weight_updates % sizeof(*_tmp_weight_updates) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_weight_updates = (float*)malloc(_len_weight_updates);
		if (_in_weight_updates == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_weight_updates, _len_weight_updates, _tmp_weight_updates, _len_weight_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	ecall_backward_connected_layer(ms->ms_batch, _tmp_outputs, ms->ms_inputs, ms->ms_a, _tmp_a_len, _tmp_b_len, _tmp_c_len, _tmp_nd_len, _in_output, _in_input, _in_delta, _in_n_delta, _in_weights, _in_bias_updates, _in_weight_updates);
	if (_in_delta) {
		if (memcpy_s(_tmp_delta, _len_delta, _in_delta, _len_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_n_delta) {
		if (memcpy_s(_tmp_n_delta, _len_n_delta, _in_n_delta, _len_n_delta)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_weights) {
		if (memcpy_s(_tmp_weights, _len_weights, _in_weights, _len_weights)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_bias_updates) {
		if (memcpy_s(_tmp_bias_updates, _len_bias_updates, _in_bias_updates, _len_bias_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_weight_updates) {
		if (memcpy_s(_tmp_weight_updates, _len_weight_updates, _in_weight_updates, _len_weight_updates)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_output) free(_in_output);
	if (_in_input) free(_in_input);
	if (_in_delta) free(_in_delta);
	if (_in_n_delta) free(_in_n_delta);
	if (_in_weights) free(_in_weights);
	if (_in_bias_updates) free(_in_bias_updates);
	if (_in_weight_updates) free(_in_weight_updates);
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[13];
} g_ecall_table = {
	13,
	{
		{(void*)(uintptr_t)sgx_hello, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_normalize_array, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_gemm, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_activate_array, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_avgpool_forward, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_forward_connected_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_forward_maxpool_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_forward_convolutional_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_rc4_crypt, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_forward_cost_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_backward_convolutional_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_backward_cost_layer, 0, 0},
		{(void*)(uintptr_t)sgx_ecall_backward_connected_layer, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[1][13];
} g_dyn_entry_table = {
	1,
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print_string(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_string_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_string_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_string_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_string_t));
	ocalloc_size -= sizeof(ms_ocall_print_string_t);

	if (str != NULL) {
		ms->ms_str = (const char*)__tmp;
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}
	
	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

