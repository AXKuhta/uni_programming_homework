#include <stdio.h>
#include <stdlib.h> // malloc()

#ifdef _MSC_VER
    #include <memory.h> // memset()
    #include <io.h> // read()

    #define read _read
    #define write _write
#else
    #include <unistd.h> // read()
#endif

#define get(x, y) src[y*src_x + x]
#define inc(x, y, value) dst[y*dst_x + x] = dst[y*dst_x + x] + value

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

	char* dst = (char*)malloc(dst_x*dst_y);

	memset(dst, 0, dst_x*dst_y);

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
void detect(char* src, int src_x, int src_y, int threshold) {
	int avg_x = 0;
	int avg_y = 0;
	int events = 0;

	// Искать пиксели, превышающие threshold
	for (int y = 0; y < src_y; y++) {
		for (int x = 0; x < src_x; x++) {
			int value = get(x, y);

			if (value > threshold) {
				avg_x += x;
				avg_y += y;
				events++;
			}
		}
	}

	// Если нашёлся хотя бы один (Должно найтись очень много), посчитать средний центр
	if (events > 0) {
		avg_x = avg_x / events;
		avg_y = avg_y / events;

		// Попробовать найти радиус, двигаясь по горизонтали от центра
		int radius = 0;

		for (int x = avg_x; x < src_x; x++) {
			int value = get(x, avg_y);

			if (value > threshold) {
				radius = x - avg_x;
				break;
			}
		}

		// Костыли
		int mult = threshold + 1;

		// Напечатать X, Y и R (С костылями)
		printf("%d %d %d\n", (avg_x + threshold) * mult, (avg_y + threshold) * mult, radius * mult);

	} else {
		printf("No detection events!\n");
	}
}

int main() {
	char* image = (char*)malloc(IMAGE_SIZE);
	int i = 0;

	// Читать с нулевого дескриптора -- это stdin
	// Этот цикл -- самая медленная часть во всей программе
	while (i < IMAGE_SIZE) {
		char value;

		// Прочитать байт
		read(0, &value, 1);

		// Пробелы не нужны
		if (value == ' ')
			continue;

		// \n не нужны
		if (value == '\n')
			continue;

		// \r не нужны
		if (value == '\r')
			continue;

		// Сохранить значение
		image[i] = value - '0';
		i++;
	}
	

//#define DETAILED_OUTPUT
	
#ifdef DETAILED_OUTPUT
	// Детальный вывод
	char* summary = image;
	int dimensions = 400;

	for (int i = 0; i < 10; i++) {
		draw(summary, dimensions, dimensions);
		detect(summary, dimensions, dimensions, i);

		summary = summarize(summary, dimensions, dimensions);
		dimensions = dimensions / 2;
	}
#else
	// Упрощённый вывод
	image = summarize(image, 400, 400);
	detect(image, 200, 200, 1);
#endif

}

