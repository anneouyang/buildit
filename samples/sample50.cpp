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

dyn_var<int(int)> mac_4row_4col_IOHW_forint8w_s8_fpreq(builder::as_global("mac_4row_4col_IOHW_forint8w_s8_fpreq"));
dyn_var<int(int)> memset_f(builder::as_global("memset"));
dyn_var<int(int)> shift_f(builder::as_global("shift_m"));
dyn_var<int> DIM_KER_X(builder::as_global("DIM_KER_X"));
dyn_var<int> DIM_KER_Y(builder::as_global("DIM_KER_Y"));
dyn_var<int> size_q31_t(builder::as_global("sizeof(q31_t)"));


template<typename T>
void resize(T& x, int size) {
  block::to<block::array_type> (x.block_var->var_type)->size = size;
}

static void bar(const uint16_t unroll_div_factor, dyn_var<int> output_depth, dyn_var<q15_t*>runtime_buf, dyn_var<q7_t*>filter_0, dyn_var<q7_t*>filter_1, dyn_var<q7_t*>filter_2, dyn_var<q7_t*>filter_3, dyn_var<q31_t*> out_0, dyn_var<q31_t*> out_1, dyn_var<q31_t*> out_2, dyn_var<q31_t*> out_3) {
    // const uint16_t unroll_div_factor = 3;
    const uint16_t unroll_div_factor_2_pow = (1 << unroll_div_factor);
    dyn_var<int> unroll_factor = shift_f((output_depth * DIM_KER_X * DIM_KER_Y), unroll_div_factor);

    while ((unroll_factor = unroll_factor - 1) + 1) {
        /* Initialize partial sum (assume bias == NULL) */
        // q31_t sum[unroll_div_factor_2_pow * 4] = {};
        dyn_var<q31_t[]> sum;
        resize(sum, unroll_div_factor_2_pow * 4);
        memset_f(sum, 0, size_q31_t * unroll_div_factor_2_pow * 4);

        /* MAC computation */
        for(static_var<int> i = 0; i < unroll_div_factor_2_pow; i++) {
          mac_4row_4col_IOHW_forint8w_s8_fpreq(&sum[i * 4], runtime_buf, filter_0, filter_1, filter_2, filter_3);
          // filter_0 = filter_0 + 1; 
          filter_0++;
	  filter_1 = filter_1 + 1; 
          filter_2 = filter_2 + 1; 
          filter_3 = filter_3 + 1; 
        }
        
        /* Accumulate partial sum into output data */
        
        // assign_sum_to_pointwise_tmp_output_buffer_4row8col_int8(out_0, out_1, out_2, out_3, sum); 
        // out_0 += unroll_div_factor_2_pow; out_1 += unroll_div_factor_2_pow; out_2 += unroll_div_factor_2_pow; out_3 += unroll_div_factor_2_pow;

        // this is inlined below
        for(static_var<int> i = 0; i < unroll_div_factor_2_pow; i++) {
          out_0[0] = out_0[0] + sum[i * 4 + 0];
          out_0 = out_0 + 1;
          out_1[0] = out_1[0] + sum[i * 4 + 1];
          out_1 = out_1 + 1;
          out_2[0] = out_2[0] + sum[i * 4 + 2];
          out_2 = out_2 + 1;
          out_3[0] = out_3[0] + sum[i * 4 + 3];
          out_3 = out_3 + 1;
        }
    }
}

int main(int argc, char* argv[]) {
	block::c_code_generator::generate_code(builder::builder_context().extract_function_ast(bar, "bar", 3), std::cout, 0);
	return 0;
}



