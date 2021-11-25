#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_arr(int* base, int len) {
	printf("{");

	for (int i = 0; i < len; i++)
		printf("%d ", base[i]);
	
	printf("}\n");
}

int main() {
	time_t now = time(0);
	
	//printf("%llu\n", now);

	srand(now);


	int pool[] = {0,1,2,3,4,5,6,7,8,9};
	int truth[] = {0,0,0,0};

	int avail = 10;

	for (int i = 0; i < 4; i++) {
		int i_pool = rand() % avail;

		truth[i] = pool[i_pool];
		pool[i_pool] = pool[avail - 1];

		avail--;
	}
	
	printf("I generated a number. Try to guess it!\n");
	print_arr(truth, 4);

	int guess_count = 0;

	while (1) {
		int guess[] = {0, 0, 0, 0};

		printf("Take a guess (4 digit number): ");
		scanf("%1d%1d%1d%1d", guess + 0, guess + 1, guess + 2, guess + 3);

		//print_arr(guess, 4);

		int bulls = 0;
		int cows = 0;

		for (int i = 0; i < 4; i++) {

			// Бык = полное совпадение
			if (guess[i] == truth[i]) {
				bulls++;
				continue;
			}

			// Корова = совпадение по множеству
			for (int j = 0; j < 4; j++) {
				if (guess[i] == truth[j]) {
					cows++;
					break;
				}
			}

		}

		printf("%d bulls, %d cows\n", bulls, cows);
		guess_count++;

		if (bulls == 4) {
			printf("You win!\n");
			printf("Took you %d guesses\n", guess_count);
			break;
		}
	}
	
}