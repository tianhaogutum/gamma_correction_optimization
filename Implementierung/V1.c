#include "V1.h"
#define MAX_gamma 67075968//if base is not 1 and gamma exceeds this limit, then the result of power multiplied by 255 is always smaller then one, and after conversion to uint_8 it will be zero
#include <stdio.h>//for testing and debugging, should be removed

static float pow_with_taylor_expansion(float, float);
static float int_power(float, uint32_t);

void gamma_correct_V1(const uint8_t *img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t *result)
{
    float sum_coeffs = a + b + c;
    size_t num_pixel = width * height;
    float Q_x_y = 0;
    size_t i = 0;
    while (i < num_pixel)
    {
        Q_x_y = (a * img[3 * i] + b * img[3 * i + 1] + c * img[3 * i + 2]) / sum_coeffs;
        result[i] = pow_with_taylor_expansion(Q_x_y / 255, gamma) * 255;
        printf("%u\n", result[i]);//for testing
        i++;
    }
}

static float pow_with_taylor_expansion(float base, float gamma) // comput base ** gamma
{                                                              // we choose a = 1
    if(base == 1.0){
        return 1;
    }
    if(gamma > MAX_gamma){
        return 0;
    }
    uint32_t index = gamma;
    gamma -= index;
    
    
    
    
    float ret = 0;
    ret += 1;
    float base_minus_a = base - 1;
    uint32_t counter = 1; // counter of terms
    float current_pow = gamma;
    float term = current_pow * base_minus_a;
    while (term > 0.00000000001 || term < -0.00000000001)
    {
        ret += term;
        counter += 1;
        current_pow = current_pow - 1;
        term = ((term * current_pow) / counter) * base_minus_a;
    }
    return ret;
}

static float int_power(float base, uint32_t int_pow){//int_pow has 32 bits, i-th bit set means to add base ** (2 ** i) to the result, so use a loop to traverse int_pow
    float current_digit = base;
    float result = 0;
    uint32_t index_bit = 1;
    for(size_t i = 0; i < 32; ++ i){
        if(index_bit & int_pow){
            result *= current_digit;
        }
        index_bit <<= 1;
        current_digit *= current_digit;
    }
    return result;
}