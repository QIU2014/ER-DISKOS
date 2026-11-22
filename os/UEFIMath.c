#include <math.h>

double sin(double value) {
    double result;
    __asm__ volatile ("fldl %1; fsin; fstpl %0" : "=m"(result) : "m"(value));
    return result;
}

double cos(double value) {
    double result;
    __asm__ volatile ("fldl %1; fcos; fstpl %0" : "=m"(result) : "m"(value));
    return result;
}

double tan(double value) {
    return sin(value) / cos(value);
}

double sqrt(double value) {
    double result;
    __asm__ volatile ("fldl %1; fsqrt; fstpl %0" : "=m"(result) : "m"(value));
    return result;
}

static double round_direction(double value, unsigned short rounding_mode) {
    unsigned short original_control;
    unsigned short adjusted_control;
    double result;

    __asm__ volatile ("fnstcw %0" : "=m"(original_control));
    adjusted_control = (unsigned short)((original_control & ~0x0c00) | rounding_mode);
    __asm__ volatile ("fldcw %0" : : "m"(adjusted_control));
    __asm__ volatile ("fldl %1; frndint; fstpl %0" : "=m"(result) : "m"(value));
    __asm__ volatile ("fldcw %0" : : "m"(original_control));
    return result;
}

double ceil(double value) {
    return round_direction(value, 0x0800);
}

double floor(double value) {
    return round_direction(value, 0x0400);
}

double exp(double value) {
    double reduced;
    double sum = 1.0;
    double term = 1.0;
    int exponent;
    int i;

    if (value != value) {
        return value;
    }
    if (value > 709.782712893384) {
        union { unsigned long long bits; double value; } infinity = {
            0x7ff0000000000000ULL
        };
        return infinity.value;
    }
    if (value < -745.133219101941) {
        return 0.0;
    }

    exponent = (int)(value * 1.4426950408889634);
    reduced = value - exponent * 0.6931471805599453;
    for (i = 1; i <= 20; i++) {
        term *= reduced / i;
        sum += term;
    }
    while (exponent > 0) {
        sum *= 2.0;
        exponent--;
    }
    while (exponent < 0) {
        sum *= 0.5;
        exponent++;
    }
    return sum;
}
