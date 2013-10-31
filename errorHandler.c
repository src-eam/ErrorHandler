#include <stdio.h>
#include <stdlib.h>
#include <stdio_ext.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <string.h>

#define SIZE_NUMBERS 10		//первоначальный размер выделения памяти под массив чисел
#define REALLOC_SIZE_NUMBERS 10 //на сколько будет увеличиваться выделение памяти при вызове realloc
#define PATH_MAX 512
#define EMUL_ERROR 0

double my_rand() {
	return (double) rand() / (double) RAND_MAX;
}

//обертки
void *wrapper_calloc(size_t num, size_t size) {
	return (my_rand() >= EMUL_ERROR) ? calloc(num, size) : NULL;
}

void *wrapper_realloc(void *ptr, size_t size) {
	return (my_rand() >= EMUL_ERROR) ? realloc(ptr, size) : NULL;
}

DIR *wrapper_opendir(char *dirname) {
	return (my_rand() >= EMUL_ERROR) ? opendir(dirname) : NULL;
}

struct dirent *wrapper_readdir(DIR *ptr) {
	return (my_rand() >= EMUL_ERROR) ? readdir(ptr) : NULL;
}

int wrapper_closedir(DIR *ptr) {
	return (my_rand() >= EMUL_ERROR) ? closedir(ptr) : -1;
}

FILE * wrapper_fopen(const char * filename, const char * mode) {
	return (my_rand() >= EMUL_ERROR) ? fopen(filename, mode) : NULL;
}

int wrapper_fclose(FILE * stream) {
	return (my_rand() >= EMUL_ERROR) ? fclose(stream) : EOF;
}

size_t wrapper_fread(void *buf, size_t size, size_t count, FILE *stream) {
	return (my_rand() >= EMUL_ERROR) ? fread(buf, size, count, stream) : 0;
}

//чтение из файла и инициализация массива чисел
unsigned int *readFile(FILE *fp, unsigned int *numbers, size_t *size_numbers,
		size_t *index) {
	char byte[0];
	int i = 0;
	size_t j = *index;
	char line[11];
	while (!feof(fp) && fp != NULL
			&& (wrapper_fread(byte, sizeof(char), 1, fp) == 1)) {
		if (byte[0] >= '0' && byte[0] <= '9') {
			line[i] = byte[0];
			i++;
			if (i > 10) {
				i = 0;
				while (byte[0] != '\n' && !feof(fp) && fp != NULL) {
					wrapper_fread(byte, sizeof(char), 1, fp);
				}
			}
		} else if (byte[0] == '\n' && i != 0) {
			line[i] = '\0';
			numbers[j] = atoi(line);
			j++;
			(*index)++;
			if (*index == *size_numbers) {
				(*size_numbers) += REALLOC_SIZE_NUMBERS;
				numbers = (unsigned int*) wrapper_realloc(numbers,
						(*size_numbers) * sizeof(unsigned int));
				if (!numbers) {
					fprintf(stderr,
							"Память закончилась. Ошибка при распределении памяти(realloc 1).\n");
					return NULL;
				}
			}
			i = 0;
		} else if (fp != NULL) {
			i = 0;
			while (byte[0] != '\n' && !feof(fp) && fp != NULL)
				wrapper_fread(byte, sizeof(char), 1, fp);
		}
	}
	if (i != 0) {
		line[i] = '\0';
		numbers[j] = atoi(line);
		j++;
		(*index)++;
		if (*index == *size_numbers) {
			(*size_numbers) += REALLOC_SIZE_NUMBERS;
			numbers = (unsigned int*) wrapper_realloc(numbers,
					(*size_numbers) * sizeof(unsigned int));
			if (!numbers) {
				fprintf(stderr,
						"Память закончилась. Ошибка при распределении памяти(realloc 2).\n");
				return NULL;
			}
		}
	}

	return numbers;
}

//сортирвка чисел
void sort_bubble(unsigned int *numbers, size_t n) {
	unsigned int temp;
	for (size_t i = 0; i < n; i++) {
		for (size_t j = 0; j < n - i - 1; j++) {
			if (numbers[j] > numbers[j + 1]) {
				temp = numbers[j + 1];
				numbers[j + 1] = numbers[j];
				numbers[j] = temp;
			}
		}
	}

}

int main(int argc, char *argv[]) {
	srand(time(NULL));
	if (argc < 2) {
		fprintf(stderr, "Ошибка. Каталог не задан.\n");
		return 1;
	}
	FILE *fp;
	unsigned int *numbers, *prom;
	size_t size_numbers = SIZE_NUMBERS;
	size_t index = 0;
	numbers = wrapper_calloc(size_numbers, sizeof(unsigned int));
	if (!numbers) {
		fprintf(stderr, "Ошибка при распределении памяти.(calloc)\n");
		return 1;
	}
	int len_path = strlen(argv[1]);
	if (argv[1][len_path - 1] != '/') {
		argv[1][len_path] = '/';
		argv[1][len_path + 1] = '\0';
		len_path++;
	}
	DIR *dir;
	struct dirent *ent;
	dir = wrapper_opendir(argv[1]);
	if (!dir || (len_path > (PATH_MAX - 2))) {
		fprintf(stderr, "Ошибка открытия %s каталога.\n", argv[1]);
		return 1;
	}
	char * filename = wrapper_calloc(PATH_MAX, sizeof(char));
	if (!filename) {
		fprintf(stderr, "Ошибка распределения памяти. \n");
		return 1;
	}
	int closeFile = 0;
	while ((ent = wrapper_readdir(dir))) {
		if ((int) ent->d_type == 8) {
			filename = wrapper_calloc(PATH_MAX, sizeof(char));
			if (!filename) {
				fprintf(stderr, "Ошибка распределения памяти. \n");
				if (index == 0)
					return 1;
				else
					break;
			}
			strncpy(filename, argv[1], len_path);
			strcat(filename, ent->d_name);
			fp = fopen(filename, "rb");
			if (fp == NULL) {
				fprintf(stderr, "Ошибка открытия %s файла\n", filename);
			} else {
//				printf("Обработка файла %s\n",filename);
				prom = readFile(fp, numbers, &size_numbers, &index);
				if (fp != NULL) {
					closeFile = wrapper_fclose(fp);
					if (closeFile != 0)
						fprintf(stderr, "Ошибка закрытия %s файла\n", filename);
				}
				if (!prom)
					break;
				else {
					numbers = prom;
				}
			}
			free(filename);
		}
	}
	int close = wrapper_closedir(dir);
	if (close != 0)
		fprintf(stderr, "Ошибка закрытия %s каталога\n", argv[1]);

	printf("\n---------------------\nРезультат:\n");
	if (index != 0) {
		sort_bubble(numbers, index);
		for (int i = 0; i < index; i++)
			printf("%u ", numbers[i]);
		printf("\n");
	}

	return 0;
}
