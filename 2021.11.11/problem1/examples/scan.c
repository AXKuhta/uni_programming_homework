#include <stdio.h>
#include <unistd.h> // read()
#include <stdlib.h> // malloc()

#define IMAGE_X 400
#define IMAGE_Y 400
#define IMAGE_SIZE IMAGE_X*IMAGE_Y

char image[IMAGE_SIZE];

// Уменьшить в два раза
char* summarize(char* src, int src_x, int src_y) {
	int dst_x = src_x / 2;
	int dst_y = src_y / 2;

	char* dst = malloc(dst_x*dst_y);

	// Смешной трюк
	// Можно брать и инкрементить любые ячейки вплоть до 10 раз
	// Внутри останется правильный код ASCII цифры
	memset(dst, '0', dst_x*dst_y);

	char get(int x, int y) { return src[y*src_x + x]; }
	void inc(int x, int y, char value) { (void) value; dst[y*dst_x + x] = dst[y*dst_x + x] + value; }

	for (int y = 0; y < src_y; y++) {
		for (int x = 0; x < src_x; x++) {
			int tgt_x = x / 2;
			int tgt_y = y / 2;

			char value = get(x, y);

			inc(tgt_x, tgt_y, value - '0');
		}
	}

	char dget(int x, int y) { return dst[y*dst_x + x]; }

	// Нарисовать
	for (int y = 0; y < dst_x; y++) {
		for (int x = 0; x < dst_y; x++) {
			int value = dget(x, y);
			write(1, &value, 1);
		}

		write(1, "\n", 1);
	}

	write(1, "\n", 1);
	write(1, "\n", 1);
	write(1, "\n", 1);

	return dst;
}


// write(1, "a", 1);
// write(1, "\n", 1);


int main() {

	// Читать с нулевого дескриптора -- это stdin
	// Читать в image[i] по одному байту
	// Пропускать пробелы
	for (int i = 0; i < IMAGE_SIZE; i++) {
		char temp;

		// Прочитать цифру
		read(0, image + i, 1);

		// Прочитать пробел
		read(0, &temp, 1); 	// Предупреждение: нельзя просто передать этой функции NULL чтобы она пропустила данные
							// Она просто ничего не прочитает
							// Временная переменная обязательна
	}

	char* summary = image;
	int dimensions = 400;

	for (int i = 0; i < 10; i++) {
		summary = summarize(summary, dimensions, dimensions);
		dimensions = dimensions / 2;
	}

}

