// Include the headers
#include "blocks/c_code_generator.h"
#include "builder/builder.h"
#include "builder/builder_context.h"
#include "builder/static_var.h"
#include "builder/dyn_var.h"
#include <iostream>

// Include the BuildIt types
using builder::dyn_var;
using builder::static_var;

// Define the TinyEngine types
const char q31_t_s[] = "q31_t";
using q31_t = builder::name<q31_t_s>;
const char q15_t_s[] = "q15_t";
using q15_t = builder::name<q15_t_s>;
const char q7_t_s[] = "q7_t";
using q7_t = builder::name<q7_t_s>;

// Define other types not supported by BuildIt but supported in C
// const char uint16_t_s[] = "uint16_t";
// using uint16_t = builder::name<uint16_t_s>;

static inline void transpose_depthwise_mac_kernelx_yrow_uniweight(static_var<int> ks, static_var<int> n, dyn_var<q31_t*> sum, 
                       dyn_var<q7_t*> two_column_buffer, dyn_var<q7_t*> ksrc_transposed, dyn_var<uint16_t> input_width, 
                       dyn_var<uint16_t> STRIDE, dyn_var<uint16_t> IN_PAD, dyn_var<uint16_t> OUT_PAD) {
    for(static_var<int> i = 0; i < ks; i++) {
      for(static_var<int> j = 0; j < ks; j++) {
        for(static_var<int> k = 0; k < n; k++) {
          sum[k] += two_column_buffer[k + j] * ksrc_transposed[i * ks + j];
        }
      }
      if (i < ks - 1) {
        two_column_buffer += (input_width - 1) * STRIDE + 1 + IN_PAD * 2 + OUT_PAD;
      }
    }
}

int main(int argc, char* argv[]) {
	// block::c_code_generator::generate_code(builder::builder_context().extract_function_ast(transpose_depthwise_mac_kernelx_yrow_uniweight, "transpose_depthwise_mac_kernelx_yrow_uniweight", 3, 2), std::cout, 0);
    block::c_code_generator::generate_code(builder::builder_context().extract_function_ast(transpose_depthwise_mac_kernelx_yrow_uniweight, "transpose_depthwise_mac_kernelx_yrow_uniweight", 7, 2), std::cout, 0);
	return 0;
}