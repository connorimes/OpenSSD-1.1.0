#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BLOCK_SIZE 512

char* create_data(int size, char* data) {
	while (size-- > 0) {
		*data++ = 'a';
	}
	*data = '\0';
}

/*
* Pass the block device location, offset, and data size as the command line arguments.
*/
int main(int argc, char* argv[] )
{
	if (argc < 4) {
		fprintf(stderr, "Must pass device path, offset, and data size as arguments\n");
		return -1;
	}
	// get block device to use
	char* dev = argv[1];
	printf("Device is: %s\n", dev);

	int offset = atoi(argv[2]);
	if (offset < 0) {
		printf("Offset must be > 0\n");
		return -1;
	}
	printf("Offset is: %d\n", offset);

	int size = atoi(argv[3]);
	if (size < 1) {
		printf("Size must be greater than 0\n");
		return -1;
	}
	printf("Data size is: %d\n", size);

	if (access(dev, R_OK | W_OK) == -1) {
		// device does not exist
		perror("Device not found or cannot read/write");
		return -1;
	}
	// char data[size];
	char* data;
	if(posix_memalign((void**)&data, BLOCK_SIZE, size) < 0) {
		perror("Failed to align memory");
		return -1;
	}
	create_data(size, data);
	// printf("%s\n", data);

	// open block device
	int f = open(dev, "rw");

	if (f < 0) {
		fprintf(stderr, "Error opening block device %s", dev);
	} else {
		printf("Writing to device\n");
		lseek(f, offset, SEEK_SET);
		int wrote = write(f, data, size);
		printf("Wrote %d characters\n", wrote);
		//fsync(f);
		// don't forget to close
		printf("Closing device\n");
		close(f);
	}
	free(data);
	data = NULL;
	return(0);
}
