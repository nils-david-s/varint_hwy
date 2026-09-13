#include "vectorlength_discrepancy.h"
#include <stdint.h>
#include "hwy/highway.h"
#if HWY_ONCE
int main(void) {
    uint32_t *input_data = (uint32_t*) malloc(2*sizeof(uint32_t));
    uint32_t *output_data_with_d = (uint32_t*) malloc(4*sizeof(uint32_t));
    uint32_t *output_data_without_d = (uint32_t*) malloc(4*sizeof(uint32_t));
    
    input_data[0] = 1;
    input_data[1] = 2;

    for (size_t i = 0; i < 4; ++i) {
        output_data_with_d[i] = 0;
        output_data_without_d[i] = 0;
    }

    call_test_concat_with_d(input_data, output_data_with_d);
    call_test_concat_without_d(input_data, output_data_without_d);
    printf("Input data:\n");
    for (size_t i = 0; i < 2; ++i) {
        printf("Input[%zu] = %u \n", i, input_data[i]);
    }

    printf("Expected Output:\nExpected_output_data[0]=1\nExpected_output_data[1]=1\n");

    printf("Output data with d:\n");
    for (size_t i = 0; i < 4; ++i) {
        printf("Ouput_data_with_d[%zu] = %u \n", i, output_data_with_d[i]);
    }

    printf("Output data without d:\n");
    for (size_t i = 0; i < 4; ++i) {
        printf("Ouput_data_without_d[%zu] = %u \n", i, output_data_without_d[i]);
    }
}
#endif // HWY_ONCE

