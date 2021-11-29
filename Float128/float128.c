#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ascii_lib.c"

//
// Количество цифр в разных числах:
// 2^32			10
// 2^64			20
// 2^128		39
// 2^256		78
// 2^512		155
// 2^1024		309
// 2^2048		617
// 2^4096		1234
//
// Такими будут затраты памяти на представление в ASCII
//

//
// При превышении отметки в 16777216, т.е. 2^24, Float32 испытает деградацию в точности
// Представлять нечётные числа станет невозможно из-за потери самого младшего бита
// Дальнейшая деградация произойдёт при превышении отметки в 2^25
//
// Если число меньше 2^24, то оно сдвигается влево до встречи выставленного бита
// Возникшее пространство представляет дробную часть
//
// Если число больше 2^24, то оно сдвигается вправо, теряя точность
// Дробная часть отсутствует
//

//
// Экспонента:
// 0x00		-127
// 0x7F		 0
// 0x80		 1
// 0xFE		 127
// 0xFF		 Infinity
//
// Примеры положительных экспонент:
// 0x79			Целая часть 0 бит 	+1 			дробная часть 23 бита		Числа от 1 до 2
// 0x80			Целая часть 1 бит 	+2 			дробная часть 22 бита
// 0x81			Целая часть 2 бита 	+4 			дробная часть 21 бит
// 0x82			Целая часть 3 бита 	+8 			дробная часть 20 бит
// 0x83			Целая часть 4 бита 	+16			дробная часть 19 бит
// 0x84			Целая часть 5 бит 	+32			дробная часть 18 бит
// ...
// 0x96			Целая часть 23 бита	+8388608	дробная часть 0 бит
// ...
//
// HIGHBIT = Exp - 0x80 + 1
//

//
// Максимальное целое число:
// Float32 		2^127 + мелочь		Экспонента 8 бит
// Float64		2^1024 + мелочь		Экспонента 11 бит
// Float128		2^16384 + мелочь	Экспонента 15 бит
//

float load_float32(int number) {
	int sign = 0;

	// Сохранить знак
	if (number < 0) {
		number = -number;
		sign = 1;
	}

	// 1	Знак
	// 8	Экспонента
	// 23	Мантисса
	#define SIGN_MASK		0b10000000000000000000000000000000
	#define EXPONENT_MASK	0b01111111100000000000000000000000
	#define MANTISSA_MASK	0b00000000011111111111111111111111

	// Использовать настоящую память для всех операций
	// Это поможет избежать предупреждений про type punning
	void* memory = malloc(4);

	float* float_view = memory;
	int* int_view = memory;

	// Очистить
	int_view[0] = 0;



	// Ведущие нули
	int lz = 0;

	// Начать двигать число вверх, пока что-нибудь не попадёт в область экспоненты
	// Верхний бит будет потерян (Мы представим его в другой форме)
	while (!(number & EXPONENT_MASK)) {
		number = number << 1;
		lz++;
	}
	
	// Используя количество ведущих нулей, подсчитать экспоненту
	int exp = (22 - lz) + 0x80;

	int_view[0] = number & MANTISSA_MASK;
	int_view[0] += exp << 23;
	int_view[0] += sign << 31;


	// Перенести результат в переменную и высвободить память
	float val = float_view[0];
	free(memory);

	return val;
}

void print_float32(float number) {
	// 1	Знак
	// 8	Экспонента
	// 23	Мантисса
	#define SIGN_MASK		0b10000000000000000000000000000000
	#define EXPONENT_MASK	0b01111111100000000000000000000000
	#define MANTISSA_MASK	0b00000000011111111111111111111111

	// Использовать настоящую память для всех операций
	// Это поможет избежать предупреждений про type punning
	void* memory = malloc(4);

	float* float_view = memory;
	int* int_view = memory;

	// Загрузить
	*float_view = number;


	// Значение экспоненты указывает самый высокий установленный бит
	int exp = (*int_view & EXPONENT_MASK) >> 23;

	if (exp < 128) {
		printf("UNIMPLEMENTED\n");
		exit(-1);
	}

	int high_bit = exp - 128 + 1;
	int mantissa_size = 23 - high_bit;

	printf("Have %d as the highbit\n", high_bit);
	printf("Mantissa is %d bits\n", mantissa_size);


	// Подвести вес разряда к рабочему диапазону
	av_t base = av_from_string("1");

	for (int i = 0; i < -mantissa_size; i++)
		base = ascii_add(&base, &base);

	// Выкинуть дробную часть, если есть
	if (mantissa_size > 0)
		*int_view = *int_view >> mantissa_size;

	// Начать считать
	av_t num = av_from_string("0");

	for (int i = 0; i < high_bit; i++) {
		if (*int_view & 1) {
			num = ascii_add(&num, &base);
			printf("HI +");
			print_av(&base);
			printf("\n");
		} else {
			printf("LO\n");
		}

		base = ascii_add(&base, &base);
		*int_view = *int_view >> 1;
	}

	// И финальный бит
	num = ascii_add(&num, &base);
	printf("HI +");
	print_av(&base);
	printf("\n");

	printf("Result: ");
	print_av(&num);
	printf("\n");

	free(memory);
}

int main() {
	printf("Load float: %f\n", load_float32(-6));

	print_float32(1.25e6);
}
