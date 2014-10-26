#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

static inline int is_hex(char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'A' && c <= 'F') ||
		(c >= 'a' && c <= 'f');
}

/* memmem(): A strstr() work-alike for non-text buffers */
static inline void *memmem(const void *s1, const void *s2, size_t len1, size_t len2)
{
	char *bf = (char *)s1, *pt = (char *)s2;
	size_t i, j;

	if (len2 > len1)
		return NULL;

	for (i = 0; i <= (len1 - len2); ++i) {
		for (j = 0; j < len2; ++j)
			if (pt[j] != bf[i + j]) break;
		if (j == len2) return (bf + i);
	}
	return NULL;
}

static void print_help(int argc, char *argv[])
{
	fprintf(stderr, "Get hex string offset in file.\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, " %s <hex_bytes> <file> [options]\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -s <start_offset>           begin search from offset\n");
}

int main(int argc, char *argv[])
{
	char *patt_hex = NULL, *file_path = NULL;
	size_t start_offset = 0;
	size_t patt_hex_len, patt_len, file_len;
	unsigned i;
	int opt;
	char pattern[128], *file_data, *patt_pos;
	FILE *fp;

	while ((opt = getopt(argc, argv, "s:h")) != -1) {
		switch (opt) {
		case 's':
			start_offset = (size_t)strtoul(optarg, NULL, 10);
			break;
		case 'h':
			print_help(argc, argv);
			exit(0);
			break;
		case '?':
			return 1;
		}
	}
	if (argc - optind < 2) {
		fprintf(stderr, "*** Insufficient parameters.\n");
		print_help(argc, argv);
		return 1;
	}
	patt_hex = argv[optind];
	file_path = argv[optind + 1];

	/* Convert the input hex pattern into binary. */
	patt_hex_len = strlen(patt_hex);
	patt_len = 0;
	for (i = 0; i < patt_hex_len; i += 2) {
		if (patt_len >= sizeof(pattern)) {
			fprintf(stderr, "*** Input pattern is too long.\n");
			return 1;
		}
		if (!is_hex(patt_hex[i]) || !is_hex(patt_hex[i + 1])) {
			fprintf(stderr, "*** Incomplete hexadecimal bytes.\n");
			return 1;
		}
		pattern[patt_len] = 0;
		sscanf(patt_hex + i, "%2hhx", &pattern[patt_len++]);
	}

	/* Read all file content. */
	if (!(fp = fopen(file_path, "rb"))) {
		fprintf(stderr, "*** Failed to open file: %s.\n",
			strerror(errno));
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	file_data = malloc(file_len);
	if (fread(file_data, 1, file_len, fp) != file_len) {
		fprintf(stderr, "*** File was not read completely.\n");
		return 1;
	}
	fclose(fp);

	/* Check 'start_offset' with 'file_len'. */
	if (start_offset >= file_len) {
		fprintf(stderr, "*** Starting offset exceeds file size.\n");
		return 1;
	}

	/* Get offset of the pattern in file. */
	patt_pos = memmem(file_data + start_offset, pattern, file_len - start_offset, patt_len);
	free(file_data);
	if (patt_pos) {
		printf("%lu\n", (unsigned long)(patt_pos - file_data));
		return 0;
	} else {
		return 2;
	}

	return 0;
}
