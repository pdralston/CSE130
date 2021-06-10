#ifndef MATH_FUNCTIONS_HEADER
#define MATH_FUNCTIONS_HEADER

#include <inttypes.h>

int8_t add(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a + b;
    if (!((a ^ b) < 0) && (a ^ result) < 0) {
        return -1;
    }
    return 0;

}

int8_t subtract(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a - b;
    if ((a ^ b) < 0 && (a ^ result) < 0){
        return -1;
    }
    return 0;
}

int8_t multiply(const int64_t& a, const int64_t& b, int64_t& result) {
    result = a * b;
    if (a != 0 && b != 0 && a != result / b) {
        return -1;
    }
    return 0;
}

int8_t div(const int64_t& a, const int64_t& b, int64_t& result) {
    if (b == 0) { return -1; }
    result = a / b;
    return 0;
}

int8_t mod(const int64_t& a, const int64_t& b, int64_t& result) {
    if (b == 0) { return -1; }
    result = a % b;
    return 0;
}

#endif