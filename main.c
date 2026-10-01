#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "BFL.h"

enum SudokuBoardStatus
{
	VALID,
	INVALID,
};

typedef struct Sudoku
{
	unsigned int rows;
	unsigned int cols;
	unsigned int *board;
	enum SudokuBoardStatus status;
} Sudoku;

char **extract_lines_from_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
	{
		fprintf(stderr, "ERROR: Couldn't open %s\n", path);
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		fclose(f);
		return NULL;
	}
	long file_size = ftell(f);
	if (file_size == -1)
	{
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		fclose(f);
		return NULL;
	}
	rewind(f);
	char *line = calloc(file_size + 1, sizeof(*line));
	if (!line)
	{
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		fclose(f);
		return NULL;
	}

	if (fread(line, sizeof(*line), file_size, f) != (size_t)file_size)
	{
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		fclose(f);
		free(line);
		return NULL;
	}
	fclose(f);
	char **lines = bfl_split(line, '\n');
	free(line);
	if (!lines)
	{
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		return NULL;
	}

	return lines;
}

void	sudoku_print_board(const Sudoku *sudoku)
{
	printf("%ux%u Sudoku\n\n", sudoku->cols, sudoku->rows);

	if (sudoku->board)
	{
		for (unsigned int i = 0; i < sudoku->rows; ++i)
		{
			for (unsigned int j = 0; j < sudoku->cols; ++j)
			{
				printf("%u ", sudoku->board[i * sudoku->cols + j]);
			}
			printf("\n");
		}
	}
}

Sudoku sudoku_parse(char **lines)
{
	Sudoku sudoku;
	memset(&sudoku, 0, sizeof(sudoku));

	sudoku.rows = bfl_splitlen(lines);
	for (unsigned int i = 0; i < sudoku.rows; ++i)
	{
		char **numbers = bfl_split(lines[i], ' ');
		if (!numbers)
		{
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			sudoku.status = INVALID;
			return sudoku;
		}
		unsigned int cols = bfl_splitlen(numbers);
		bfl_free(&numbers, 2);
		if (i > 0)
		{
			if (cols != sudoku.cols)
			{
				sudoku.status = INVALID;
				fprintf(stderr, "ERROR: Sudoku must have the same amount of columns.\n\
Expected: %u\n\
Got: %u\n", sudoku.cols, cols);
				return sudoku;
			}
		}
		sudoku.cols = cols;
	}

	unsigned int squared_cols = (unsigned int)sqrt(sudoku.cols);
	if (sudoku.cols != sudoku.rows
		|| sudoku.cols == 0
		|| sudoku.cols != squared_cols * squared_cols)
	{
		fprintf(stderr, "ERROR: Sudoku must be a perfect square\n");
		sudoku.status = INVALID;
		return sudoku;
	}

	sudoku.board = calloc(sudoku.cols * sudoku.rows, sizeof(*sudoku.board));
	if (!sudoku.board)
	{
		fprintf(stderr, "ERROR: Couldn't allocate Sudoku board\n");
		sudoku.status = INVALID;
		return sudoku;
	}

	unsigned int board_index = 0;
	for (unsigned int i = 0; i < sudoku.rows && board_index < sudoku.cols * sudoku.rows; ++i)
	{
		char **numbers = bfl_split(lines[i], ' ');
		if (!numbers)
		{
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			sudoku.status = INVALID;
			free(sudoku.board);
			sudoku.board = NULL;
			return sudoku;
		}
		for (unsigned int j = 0; j < sudoku.cols; ++j)
		{
			size_t number_len = strlen(numbers[j]);
			for (size_t k = 0; k < number_len; ++k)
			{
				if (!isdigit(numbers[j][k]))
				{
					fprintf(stderr, "ERROR: Everything in the board must be a number.\n\
Expected: A number\n\
Got: %c\n", numbers[j][k]);
					sudoku.status = INVALID;
					free(sudoku.board);
					sudoku.board = NULL;
					bfl_free(&numbers, 2);
					return sudoku;
				}
			}

			sudoku.board[board_index] = bfl_atoi(numbers[j]);
			if (sudoku.board[board_index] > sudoku.cols)
			{
				fprintf(stderr, "ERROR: The board contains an invalid number.\n\
Expected: Number between 0 and %u\n\
Got: %u\n", sudoku.cols, sudoku.board[board_index]);
				sudoku.status = INVALID;
				free(sudoku.board);
				sudoku.board = NULL;
				bfl_free(&numbers, 2);
				return sudoku;
			}
			++board_index;
		}
		bfl_free(&numbers, 2);
	}
	
	return sudoku;
}

static bool sudoku_is_valid_subgrid(const Sudoku *sudoku, const unsigned int subgrid_size, const unsigned int subgrid_index)
{
	unsigned int current_numbers[sudoku->rows];
	memset(current_numbers, 0, sizeof(current_numbers));
	unsigned int cn_index = 0;
	for (unsigned int i = 0; i < subgrid_size; ++i)
	{
		unsigned int x = subgrid_index / subgrid_size * subgrid_size;
		unsigned int y = (subgrid_index - x) * subgrid_size;
		for (unsigned int j = 0; j < subgrid_size; ++j)
		{
			unsigned int n = sudoku->board[(i + x) * sudoku->cols + (j + y)];
			for (unsigned int k = 0; k < cn_index; ++k)
			{
				if (current_numbers[k] == n && n != 0)
				{
					return false;
				}
			}
			current_numbers[cn_index] = n;
			++cn_index;
		}
	}
	return true;
}

static bool sudoku_is_valid_row(const Sudoku *sudoku, const unsigned int x0)
{
	unsigned int current_numbers[sudoku->cols];
	memset(current_numbers, 0, sizeof(current_numbers));
	unsigned int cn_index = 0;
	for (unsigned int i = 0; i < sudoku->cols; ++i)
	{
		unsigned int n = sudoku->board[x0 * sudoku->cols + i];
		for (unsigned int k = 0; k < cn_index; ++k)
		{
			if (current_numbers[k] == n && n != 0)
			{
				return false;
			}
		}
		current_numbers[cn_index] = n;
		++cn_index;
	}
	return true;
}

static bool sudoku_is_valid_col(const Sudoku *sudoku, const unsigned int y0)
{
	unsigned int current_numbers[sudoku->rows];
	memset(current_numbers, 0, sizeof(current_numbers));
	unsigned int cn_index = 0;
	for (unsigned int i = 0; i < sudoku->rows; ++i)
	{
		unsigned int n = sudoku->board[i * sudoku->cols + y0];
		for (unsigned int k = 0; k < cn_index; ++k)
		{
			if (current_numbers[k] == n && n != 0)
			{
				return false;
			}
		}
		current_numbers[cn_index] = n;
		++cn_index;
	}
	return true;
}

bool	sudoku_is_valid_board(Sudoku *sudoku)
{
	unsigned int subgrid_size = (unsigned int)sqrt(sudoku->rows);
	for (unsigned int i = 0; i < sudoku->rows; ++i)
	{
		for (unsigned int j = 0; j < sudoku->cols; ++j)
		{
			unsigned int subgrid_index = i / subgrid_size * subgrid_size + j / subgrid_size;
			if (!sudoku_is_valid_subgrid(sudoku, subgrid_size, subgrid_index))
			{
				fprintf(stderr, "ERROR: Sudoku can't have repeated numbers on subgrid\n");
				sudoku->status = INVALID;
				return false;
			}
			if (!sudoku_is_valid_row(sudoku, i))

			{
				fprintf(stderr, "ERROR: Sudoku can't have repeated numbers on row\n");
				sudoku->status = INVALID;
				return false;
			}
			if (!sudoku_is_valid_col(sudoku, j))
			{
				fprintf(stderr, "ERROR: Sudoku can't have repeated numbers on column\n");
				sudoku->status = INVALID;
				return false;
			}
		}
	}
	return true;
}

int	main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "ERROR: No file provided\n");
		return 1;
	}
	char **lines = extract_lines_from_file(argv[1]);
	if (!lines)
	{
		return 1;
	}
	Sudoku sudoku = sudoku_parse(lines);
	bfl_free(&lines, 2);
	if (sudoku.status == INVALID)
	{
		return 1;
	}
	sudoku_print_board(&sudoku);
	if (!sudoku_is_valid_board(&sudoku))
	{
		free(sudoku.board);
		return 1;
	}
	free(sudoku.board);
	return 0;
}
