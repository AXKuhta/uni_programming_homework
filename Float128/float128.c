#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ascii_lib.c"

//
// Float128:
// Знак			1 бит
// Экспонента	15 бит
// Мантисса 	112 бит
//

struct float128_s {
	uint64_t hi;
	uint64_t lo;
};

typedef	struct float128_s float128_t;


// Получить знак
// Есть минус		1
// Нет минуса		0
int float_128_get_sign(float128_t* src) {
	if (src->hi & 0x8000000000000000) {
		return 1;
	} else {
		return 0;
	}
}

// Получить экспоненту
uint16_t float128_get_exp(float128_t* src) {
	return ((src->hi >> 48) & 0x7FFF);
}


void print_float128(float128_t* src) {
	printf("Result: ");

	if (float_128_get_sign(src))
		printf("-");

	uint64_t exp = float128_get_exp(src);

	int high_bit = exp - 16384 + 1;
	int mantissa_size = 112 - high_bit;

	if (mantissa_size < 0)
		mantissa_size = 0;

	//
	// =================
	// Вывод целой части
	// =================
	//
	if (high_bit >= 0) {

		// Подвести вес разряда к рабочему диапазону
		av_t base = av_from_string("1");

		for (int i = 0; i < high_bit - 112; i++)
			base = ascii_add(&base, &base);

		// Начать считать
		av_t num = av_from_string("0");

		// Но ограничить high_bit
		if (high_bit > 112)
			high_bit = 112;


		uint64_t slice = 0;

		if (high_bit > 64) {
			slice = src->lo;
		} else {
			slice = src->hi >> (48 - high_bit);
		}

		for (int i = 0; i < high_bit; i++) {
			// Если собираемся просмотреть 65-й бит, то пора переключить кусок
			if (i == 64) slice = src->hi;

			if (slice & 1)
				num = ascii_add(&num, &base);

			base = ascii_add(&base, &base);
			slice = slice >> 1;
		}

		// И финальный бит
		// Затем напечатать
		num = ascii_add(&num, &base);

		print_av(&num);
	}
}

int main() {
	// float128_t a = {0x7FFF000000000000, 0x0000000000000000}; // 2^16384						OK
	// float128_t a = {0x7FFF000000000000, 0x0000000000000001}; // 2^16384 + 2^16272			OK
	// float128_t a = {0x7FFF100000000000, 0x0000000000000001}; // 2^16384 + 2^16380 + 2^16272	OK

	//float128_t a = {0x4FFF000000000000, 0x0000000000000000}; // 2^4096						OK
	//float128_t a = {0x400F000000000000, 0x0000000000000000}; // 2^16							OK
	float128_t a = {0x400F000300000000, 0x0000000000000000}; // 2^16 + 3						OK

	print_float128(&a);
}