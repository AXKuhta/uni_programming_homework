#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
// Экспонента
//
// Примеры положительных экспонент:
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

//
// Числа от 0 до 1: альтернативный режим, мантисса 24 бита
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


	// Вставить мантиссу и выполнить нормализацию
	int_view[0] = number;

	// Ведущие нули
	int lz = 0;

	while (!(int_view[0] & EXPONENT_MASK)) {
		int_view[0] = int_view[0] << 1;
		lz++;
	}

	// Если число равно 2 или 1, то lo_exp станет отрицательным
	int lo_exp = 21 - lz;

	// Экспонента:
	// 0x00		-127
	// 0xFE		 127
	// 0xFF		 Infinity
	//
	// Загрузить положительную экспоненту
	int_view[0] += (0x80 | lo_exp) << 23;

	float val = float_view[0];

	free(memory);

	return val;
}

int main() {
	printf("Load float: %f\n", load_float32(16777216+3));
}
