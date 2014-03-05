#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define BLOCK_SIZE 512
#define WRITE 'w'
#define READ 'r'

char* create_data(int size, char* data) {
	while (size-- > 0) {
		*data++ = 'a';
	}
	*data = '\0';
}

int write_dev(const int f, const int size, const int offset) {
	if (lseek(f, offset, SEEK_SET) < 0) {
		perror("Failed to seek");
		return -1;
	}

	char* data;
	// must align the memory for DMA
	if(posix_memalign((void**)&data, BLOCK_SIZE, size) < 0) {
		perror("Failed to align memory");
		return -1;
	}

	create_data(size, data);
	// printf("%s\n", data);

	struct timespec start, end;
	if (clock_gettime(CLOCK_REALTIME, &start) < 0) {
		perror("Failed to get clock time start.");
		return -1;
	}

	printf("Writing to device\n");
	const int c_wrote = write(f, data, size);
	// Must force write to device
	fsync(f);

	if (clock_gettime(CLOCK_REALTIME, &end) < 0) {
		perror("Failed to get clock time end.");
		return -1;
	}

	const long total = (1000000000 * (end.tv_sec - start.tv_sec)) + (end.tv_nsec - start.tv_nsec);
	printf("Wrote %d characters in %lu ns\n", c_wrote, total);

	// cleanup from malloc in posix_memalign
	free(data);
	return c_wrote;
}

int read_dev(const int f, const int size, const int offset) {
	if (lseek(f, offset, SEEK_SET) < 0) {
		perror("Failed to seek");
		return -1;
	}

	printf("Reading from device\n");
	int remaining = size;
	int c_read = 0;
	char buf[BLOCK_SIZE];

	struct timespec start, end;
	if (clock_gettime(CLOCK_REALTIME, &start) < 0) {
		perror("Failed to get clock time start.");
		return -1;
	}

	while (remaining > 0) {
		remaining -= BLOCK_SIZE;
		// make sure we don't read too much
		const int r_size = remaining >= 0 ? BLOCK_SIZE : BLOCK_SIZE + remaining;
		const int curr_read = read(f, buf, r_size);
		c_read += curr_read;
		if (curr_read != r_size) {
			fprintf(stderr, "Failed to read block\n");
		}
		// printf("%s\n", buf);
	}

	if (clock_gettime(CLOCK_REALTIME, &end) < 0) {
		perror("Failed to get clock time end.");
		return -1;
	}

	const long total = (1000000000 * (end.tv_sec - start.tv_sec)) + (end.tv_nsec - start.tv_nsec);
	printf("Read %d characters in %lu ns\n", c_read, total);

	return c_read;
}

/*
* Pass the block device location, offset, and data size as the command line arguments.
*/
int main(const int argc, const char* argv[] )
{
	if (argc < 5) {
		fprintf(stderr, "Must pass device path, offset, data size, and o'r' or 'w' as arguments\n");
		return -1;
	}
	// get block device to use
	const char* dev = argv[1];
	printf("Device is: %s\n", dev);

	const int offset = atoi(argv[2]);
	if (offset < 0) {
		printf("Offset must be > 0\n");
		return -1;
	}
	printf("Offset is: %d\n", offset);

	const int size = atoi(argv[3]);
	if (size < 1) {
		printf("Size must be greater than 0\n");
		return -1;
	}
	printf("Data size is: %d\n", size);

	const char action = argv[4][0];

	if (access(dev, R_OK | W_OK) == -1) {
		// device does not exist
		perror("Device not found or cannot read/write");
		return -1;
	}

	// open block device
	const int f = open(dev, O_RDWR | O_NONBLOCK);
	if (f < 0) {
		fprintf(stderr, "Error opening block device %s\n", dev);
		return -1;
	}

	if (action == WRITE) {
		// write
		const int c_wrote = write_dev(f, size, offset);
		if (c_wrote != size) {
			fprintf(stderr, "Write size did not match requested size\n");
		}
	} else {
		// read
		const int c_read = read_dev(f, size, offset);
		if (c_read != size) {
			fprintf(stderr, "Read size did not match requested size\n");
		}
	}

	// The close causes extra reads that will be captured by UART debug
	// Let the user complete capture before this happens
	printf("Complete the debug capture, then press any key to close the device file.\nWarning: This will cause extra reads!\n");
	getchar();

	printf("Closing device\n");
	if(close(f) < 0) {
		perror("Failed to close device file");
	}

	return 0;
}
