#include <stdlib.h> // malloc() и free()
#include <stdint.h> // size_t
#include <unistd.h> // write()

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
// Представленное в ASCII число
//
struct ascii_vec {
	// Неизменный указатель на выделенную память
	void* memory;

	// Изменяемые указатели для просмотра числа
	char* lsc; // Least significant char
	char* msc; // Most significant char
};

typedef struct ascii_vec av_t;

// Создать новое av_t число с указанным размером
av_t av_new(size_t size) {
	av_t av;

	char* memory = malloc(size);

	for (size_t i = 0; i < size; i++)
		memory[i] = '0';

	av.memory = memory;
	av.msc = memory;
	av.lsc = memory + size - 1;

	return av;
}

// Перевести ASCII-строку в av_t число
av_t av_from_string(char* str) {
	av_t av;

	av.msc = str;
	av.lsc = str + strlen(str) - 1;
	
	// Крайне вероятно, что str - статичная память
	// Её нельзя высвобождать через free()
	av.memory = NULL;

	return av;
}

// Напечатать av_t число
void print_av(av_t* av) {
	size_t len = av->lsc - av->msc + 1;
	write(1, av->msc, len);
}

// Сложить два av_t числа
// Не изменяет a
// Не изменяет b
// Выделяет новую память
// Иными словами: c = a + b
av_t ascii_add(av_t* a, av_t* b) {
	size_t size_a = (a->lsc - a->msc);
	size_t size_b = (b->lsc - b->msc);

	// Сравнить размеры исходных чисел и выделить память с небольшим запасом
	size_t max = size_a;

	if (size_b > max)
		max = size_b;

	av_t result = av_new(max + 2);

	char* src_a = a->lsc;
	char* src_b = b->lsc;
	char* dst = result.lsc;


	// Первый проход: два источника
	size_t lim = 0;

	if (size_a > size_b) {
		lim = size_b;
	} else {
		lim = size_a;
	}

	char cf = 0;

	for (size_t i = 0; i <= lim; i++) {
		char va = *src_a;
		char vb = *src_b;

		unsigned char rs = 150 + cf + va + vb;

		if (rs < 150) {
			*dst = rs + '0';
			cf = 1;
		} else {
			*dst = rs - 150 - '0';
			cf = 0;
		}

		src_a--;
		src_b--;
		dst--;
	}


	// Второй проход нужен только если было различие в размерах
	if (size_a != size_b) {
		// Второй проход: один источник
		char* src;

		if (lim >= size_b) {
			lim = size_a - lim;
			src = src_a;
		} else {
			lim = size_b - lim;
			src = src_b;
		}

		for (size_t i = 0; i < lim; i++) {
			char va = *src;

			unsigned char rs = 150 + cf + va + '0';

			if (rs < 150) {
				*dst = rs + '0';
				cf = 1;
			} else {
				*dst = rs - 150 - '0';
				cf = 0;
			}

			src--;
			dst--;
		}
	}

	// Редкий особый случай: 9999 + 1
	if (cf) {
		*dst = '1';
	}

	// Скрыть ведущий ноль
	if (*result.msc == '0')
		result.msc++;

	return result;
}

// Обёртка над ascii_add(), которая автоматически высвободит память числа, указанного в переменной discard
av_t ascii_add_autofree(av_t* a, av_t* b, av_t* discard) {
	av_t result = ascii_add(a, b);

	if (discard->memory != NULL) {
		free(discard->memory);
		discard->memory = NULL;
	}

	return result;
}


//
// Тесты
//

/*
// 9999 + 1
void test_a() {
	av_t a =  av_from_string("9999");
	av_t b = av_from_string("1");

	av_t c = ascii_add(&a, &b);

	print_av(&a);
	printf("\n");
	print_av(&b);
	printf("\n");
	print_av(&c);
	printf("\n");
}

// Посчитать степени двойки до n = 128
void test_b() {
	av_t base = av_from_string("2");

	for (int i = 0; i < 128; i++) {
		printf("2^%d: ", i + 1);
		print_av(&base);
		printf("\n");

		base = ascii_add_autofree(&base, &base, &base);
	}
}
*/
