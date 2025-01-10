/*
 * File verification test for emmc or ddr
 *
 */
#define _GNU_SOURCE // for resolve O_DIRECT undefined

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>	// SIGxxxx types
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

static char *TAG = "File test";
static char *my_name = "file-test";

#define measure_init() struct timespec start, end

#define measure_start()                         \
	do {                                        \
		clock_gettime(CLOCK_MONOTONIC, &start); \
	} while (0)

#define measure_elapsed()                                                  \
	({                                                                     \
		clock_gettime(CLOCK_MONOTONIC, &end);                              \
		(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; \
	})

unsigned long file_test_write_file(char *filename, unsigned long block_nr, unsigned long block_size,
								   unsigned long dword_data)
{
	int fd, ret;
	unsigned long i;
	unsigned long wr_size = 0;

	unsigned long block_size_real = (block_size / sizeof(dword_data)) * sizeof(dword_data);
	printf("%s: write file: %s with data %lx, bs %ldK, size %lu\n", TAG, filename, dword_data,
		   block_size_real / 1024, block_nr * block_size_real);
	fd = open(filename, O_WRONLY | O_CREAT | O_SYNC | O_DIRECT, 0777);
	if (fd < 0) {
		printf("%s: open file failed: %s, err = %d\n", TAG, filename, errno);
		return fd;
	}

	unsigned long *block = (unsigned long *)valloc(block_size_real);
	for (i = 0; i < block_size_real / sizeof(dword_data); i++) {
		block[i] = dword_data;
	}

	ret = 0;
	for (i = 0; i < block_nr; i++) {
		int wr_size = write(fd, block, block_size_real);
		if (wr_size < (int)block_size_real) {
			printf("%s: write file failed: %s, data: %lu, wr_size = %d, err = %d\n", TAG, filename,
				   dword_data, wr_size, errno);
			ret = -1;
			goto out;
		}
		ret += wr_size;
	}

out:
	// fsync(fd);
	free(block);
	close(fd);
	return ret;
}

int file_test_verify_file(char *filename, unsigned long block_nr, unsigned long block_size,
						  unsigned long dword_data)
{
	int fd, ret;
	unsigned long i, j;
	unsigned long block_size_real = (block_size / sizeof(dword_data)) * sizeof(dword_data);

	printf("%s: verify file: %s with data %lx, bs %ldK, size %lu\n", TAG, filename, dword_data,
		   block_size_real / 1024, block_nr * block_size_real);

	fd = open(filename, O_RDONLY | O_DIRECT | O_SYNC);
	if (fd < 0) {
		printf("%s: open file failed: %s, err = %d\n", TAG, filename, errno);
		return fd;
	}

	unsigned long *block = (unsigned long *)valloc(block_size_real);
	ret = 0;

	for (i = 0; i < block_nr; i++) {
		memset(block, 0, block_size_real);
		int rd_size = read(fd, block, block_size_real);
		if (rd_size < 0) {
			printf("%s: read file failed: %s, data: %lu, err = %d\n", TAG, filename, dword_data,
				   errno);
			ret = -1;
			goto out;
		}
		for (j = 0; j < block_size_real / sizeof(dword_data); j++) {
			if (block[j] != dword_data) {
				printf("%s: verify file failed: %s, data: %lu != %lu\n", TAG, filename, block[j],
					   dword_data);
				ret = -1;
				goto out;
			}
		}
		ret += rd_size;
	}

out:
	free(block);
	close(fd);
	return ret;
}

int file_test_main(int argc, char **argv, void *data)
{
	char *working_dir_limited = NULL;
	char real_filepath[PATH_MAX], *filepath = "/cache/file_test.tmp";
	unsigned long filesize = 1024 * 1024 * 1, block_nr = 0;
	unsigned long filedata = 0x5a5a5a5a;
	unsigned long block_size = 1024 * 1024;	 // 1024K
	int rd_speed, wr_speed;
	int no_unlink = 0, ret;
	double elapsed;
	int opt;
	const char *me = argv[0];
	int ind = 0;
	opterr = 0;

	struct option long_options[] = {{"tag", required_argument, NULL, 't'},
									{"file", required_argument, NULL, 'f'},
									{"size", required_argument, NULL, 's'},
									{"bs", required_argument, NULL, 'b'},
									{"data", required_argument, NULL, 'D'},
									{"working-dir-limited", required_argument, NULL, 'w'},
									{"no-unlink", no_argument, &no_unlink, 1},
									{"help", no_argument, NULL, 'h'},
									{0, 0, 0, 0}};

	(void)data;
	my_name = argv[0];

	while ((opt = getopt_long(argc, argv, "ht:f:s:b:D:w:", long_options, &ind)) != -1) {
		switch (opt) {
			case 0:
				printf("option %s", long_options[ind].name);
				if (optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;
			case 't':
				TAG = optarg;
				break;
			case 'f':
				filepath = optarg;
				break;
			case 's':
				filesize = 1024 * strtoul(optarg, NULL, 10);
				break;
			case 'b':
				block_size = 1024 * strtoul(optarg, NULL, 10);
				break;
			case 'D':
				filedata = strtoul(optarg, NULL, 10);
				break;
			case 'w':
				working_dir_limited = optarg;
				break;
			case 'h':
				printf(
					"%s [--help] [--tag TAG] [--bs <a block size in KB>] [--file filepath] [--size <filesize in KB> [--data <32bits file data in decimal>]\n",
					my_name);
				printf("%s [--working-dir-limited <dir>] [--no-unlink]\n", my_name);
				return 0;
				break;
			default:
				break;
		}
	}
	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
		return 0;
	}

	memset(real_filepath, 0, sizeof(real_filepath));
	realpath(filepath, real_filepath);
	if (working_dir_limited &&
		strncmp(real_filepath, working_dir_limited, strlen(working_dir_limited))) {
		printf("%s: file path: %s(%s) is invalid, must be started with %s\n", TAG, filepath,
			   real_filepath, working_dir_limited);
		return -1;
	}

	block_nr = (filesize + block_size - 1) / block_size;

	// prepare to calc time of write
	sync();

	measure_init();
	measure_start();
	ret = file_test_write_file(filepath, block_nr, block_size, filedata);
	if (ret < 0) {
		printf("%s: Write to file: %s Failed!\n", TAG, filepath);
		if (!no_unlink)
			unlink(filepath);
		return -1;
	}
	elapsed = measure_elapsed();
	wr_speed = (int)(ret / (1024 * elapsed));
	sleep(1);

	// prepare to calc time of write
	sync();
	measure_start();
	ret = file_test_verify_file(filepath, block_nr, block_size, filedata);

	if (!no_unlink)
		unlink(filepath);

	if (ret < 0) {
		printf("%s: Verify the file: %s Failed!\n", TAG, filepath);
		return -1;
	}

	elapsed = measure_elapsed();
	rd_speed = (int)(ret / (1024 * elapsed));

	printf("%s: Result: success: %s\n", TAG, filepath);
	if (wr_speed > 1024)
		printf("%s: write speed %.2fMbytes/sec\n", TAG, wr_speed / 1024.0);
	else
		printf("%s: write speed %dKbytes/sec\n", TAG, wr_speed);

	if (rd_speed > 1024)
		printf("%s: read speed %.2fMbytes/sec\n", TAG, rd_speed / 1024.0);
	else
		printf("%s: read speed %dKbytes/sec\n", TAG, rd_speed);

	return 0;
}

int main(int argc, char **argv)
{
	return file_test_main(argc, argv, NULL);
}
