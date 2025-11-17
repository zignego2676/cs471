#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
	FILE *fp = fopen("test5.txt", "w");
	if (!fp) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	srand(time(NULL));

	for (int i = 0 ; i < 50 ; i++) {
		int count = rand() % 20 + 1;
		for (int j = 0; j < count; j++) {
			int num = rand() % 1000 + 1;
			fprintf(fp, "%d", num);
			if (j < count - 1){
				fputc(' ', fp);
			}
		}
		fputc('\n', fp);
	}

	fclose(fp);
	return EXIT_SUCCESS;
}

