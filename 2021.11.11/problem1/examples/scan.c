#include <stdio.h>
#include <unistd.h> // read()
#include <stdlib.h> // malloc()

#define IMAGE_X 400
#define IMAGE_Y 400
#define IMAGE_SIZE IMAGE_X*IMAGE_Y

// Уменьшить в два раза
// Блоки 2x2 станут одним пикселем
// Кружочки с радиусом = 1 будут утеряны, но я не думаю что их вообще можно было назвать кружочками в этом случае
// В теории это уменьшение может улучшить устойчивость к шуму, на практике разницы в результатах нет
char* summarize(char* src, int src_x, int src_y) {
	int dst_x = src_x / 2;
	int dst_y = src_y / 2;

	char* dst = malloc(dst_x*dst_y);

	memset(dst, 0, dst_x*dst_y);

	char get(int x, int y) { return src[y*src_x + x]; }
	void inc(int x, int y, char value) { (void) value; dst[y*dst_x + x] = dst[y*dst_x + x] + value; }

	for (int y = 0; y < src_y; y++) {
		for (int x = 0; x < src_x; x++) {
			int tgt_x = x / 2;
			int tgt_y = y / 2;

			char value = get(x, y);

			inc(tgt_x, tgt_y, value);
		}
	}
	
	return dst;
}

// Нарисовать
void draw(char* src, int src_x, int src_y) {
	char* LUT[] = {" ", "1", "2", "3", "4", "5", "6", "7", "8", "9", "!"};

	char get(int x, int y) { return src[y*src_x + x]; }

	for (int y = 0; y < src_y; y++) {
		for (int x = 0; x < src_x; x++) {
			int value = get(x, y);

			// Не думал, что эта проверка понадобится, но она понадобилась
			// Без неё будут вылеты
			// Эх, надо было использовать uint32_t
			if (value < 0)
				value = 10;

			if (value > 10)
				value = 10;

			write(1, LUT[value], 1);
		}

		write(1, "\n", 1);
	}

	write(1, "\n", 1);
	write(1, "\n", 1);
	write(1, "\n", 1);
}

// Попытаться обнаружить круг
void detect(char* src, int src_x, int src_y, int treshold) {
	int avg_x = 0;
	int avg_y = 0;
	int events = 0;

	char get(int x, int y) { return src[y*src_x + x]; }

	for (int y = 0; y < src_y; y++) {
		for (int x = 0; x < src_x; x++) {
			int value = get(x, y);

			if (value > treshold) {
				avg_x += x;
				avg_y += y;
				events++;
			}
		}
	}

	if (events > 0) {
		avg_x = avg_x / events;
		avg_y = avg_y / events;

		printf("Avg X Y: %d %d\n", avg_x, avg_y);

		// Попробовать найти радиус, двигаясь по горизонтали от центра
		int radius = 0;

		for (int x = avg_x; x < src_x; x++) {
			int value = get(x, avg_y);

			if (value > treshold) {
				radius = x - avg_x;
				break;
			}
		}

		printf("R: %d\n", radius);

	} else {
		printf("No detection events!\n");
	}
}

int main() {
	char* image = malloc(IMAGE_SIZE);

	// Читать с нулевого дескриптора -- это stdin
	// Читать в image[i] по одному байту
	// Пропускать пробелы
	for (int i = 0; i < IMAGE_SIZE; i++) {
		char value;
		char space;

		// Прочитать ASCII цифру
		read(0, &value, 1);

		// Записать как простую цифру
		image[i] = value - '0';

		// Прочитать пробел
		read(0, &space, 1); 	// Предупреждение: нельзя просто передать этой функции NULL чтобы она пропустила данные
								// Она просто ничего не прочитает
								// Временная переменная обязательна
	}

	char* summary = image;
	int dimensions = 400;

	for (int i = 0; i < 10; i++) {
		draw(summary, dimensions, dimensions);
		detect(summary, dimensions, dimensions, i);

		summary = summarize(summary, dimensions, dimensions);
		dimensions = dimensions / 2;
	}

}

