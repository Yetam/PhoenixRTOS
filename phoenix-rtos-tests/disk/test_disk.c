/*
 * Phoenix-RTOS
 *
 * Disk benchmark - based on lmbench (https://github.com/intel/lmbench/blob/master/src/disk.c)
 *
 * Copyright 2020 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errno.h"
#include "fcntl.h"
#include "inttypes.h"
#include "stdint.h"
#include "stdio.h"
#include "unistd.h"

#include "sys/mman.h"
#include "sys/time.h"


/* Seek test definitions */
#define SEEK_POINTS     2000    /* Number of seeks to perform */
#define SEEK_MIN_STRIDE 512     /* Min seek stride */
#define SEEK_MIN_TIME   1000    /* Min valid seek time in usec */
#define SEEK_MAX_TIME   1000000 /* Max valid seek time in usec */

/* Zone test definitions */
#define ZONE_POINTS     150     /* Number of zones to test */
#define ZONE_MIN_STRIDE 512     /* Min zone stride */


typedef struct timeval timeval_t;


/* Calculates time delta in usec */
static uint64_t test_disk_gettime(timeval_t *start, timeval_t *end)
{
	timeval_t diff;

	diff.tv_sec = end->tv_sec - start->tv_sec;
	diff.tv_usec = end->tv_usec - start->tv_usec;

	if ((diff.tv_usec < 0) && (diff.tv_sec > 0)) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}

	if ((diff.tv_usec < 0) || (end->tv_sec < start->tv_sec)) {
		diff.tv_sec = 0;
		diff.tv_usec = 0;
	}

	return diff.tv_usec + 1000000 * diff.tv_sec;
}


/* Performs lseek to 64-bit offset */
static int test_disk_lseek(int fd, uint64_t offs)
{
	uint64_t i, step = 1 << 30;

	lseek(fd, 0, 0);

	/* Perform safe seek in 1GB steps */
	for (i = 0; offs - i > step; i += step) {
		if (lseek(fd, (off_t)step, SEEK_CUR) < 0)
			break;
	}

	return lseek(fd, (off_t)(offs - i), SEEK_CUR);
}


/* Estimates disk size with 1MB accuracy */
static uint64_t test_disk_getsize(int fd)
{
	uint64_t offs = 0, step = 1 << 30;
	char buff[512];

	/* Move forward in 1GB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;
	step = 1 << 25;

	/* Move forward in 32MB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;
	step = 1 << 20;

	/* Move forward in 1MB steps */
	for (offs += step; !((test_disk_lseek(fd, offs) < 0) || (read(fd, buff, sizeof(buff)) != sizeof(buff))); offs += step);
	offs -= step;

	return offs;
}


/* Measures seek + 1 sector read */
static ssize_t test_disk_seektime(int fd, uint64_t offs)
{
	timeval_t start, end;
	char buff[512];

	gettimeofday(&start, NULL);

	test_disk_lseek(fd, offs);
	if (read(fd, buff, sizeof(buff)) != sizeof(buff)) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		return -EIO;
	}

	gettimeofday(&end, NULL);

	return test_disk_gettime(&start, &end);
}


/* Measures len bytes read */
static ssize_t test_disk_zonetime(int fd, uint64_t offs, char *buff, uint64_t len)
{
	timeval_t start, end;
	ssize_t ret;

	test_disk_lseek(fd, offs);
	if (read(fd, buff, 1024) != 1024) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		return -EIO;
	}

	gettimeofday(&start, NULL);

	while (len > 0) {
		if ((ret = read(fd, buff, len)) < 0) {
			fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs + 1024);
			return -EIO;
		}
		len -= ret;
	}

	gettimeofday(&end, NULL);

	return test_disk_gettime(&start, &end);
}


/* Runs seek test */
static int test_disk_seek(int fd, uint64_t disksz)
{
	uint64_t i, j, t, time = 0, stride = (disksz / SEEK_POINTS / SEEK_MIN_STRIDE + 1) * SEEK_MIN_STRIDE;
	unsigned int nseeks = 0;

	for (i = 0, j = (disksz > stride) ? disksz - stride : 0; i < j; i += stride, j -= stride) {
		if ((t = test_disk_seektime(fd, i)) < 0)
			return t;

		/* Seek with time outside this range is either cached or a weirdo */
		if ((t > SEEK_MIN_TIME) && (t < SEEK_MAX_TIME)) {
			time += t / 1000;
			nseeks++;
		}

		if ((t = test_disk_seektime(fd, j)) < 0)
			return t;

		/* Seek with time outside this range is either cached or a weirdo */
		if ((t > SEEK_MIN_TIME) && (t < SEEK_MAX_TIME)) {
			time += t / 1000;
			nseeks++;
		}
	}

	if (nseeks)
		printf("test_disk: average seek time: %" PRIu64 "ms\n", time / nseeks);
	else
		fprintf(stderr, "test_disk: no seeks measured\n");

	return EOK;
}


/* Runs zone test */
static int test_disk_zone(int fd, uint64_t disksz, uint64_t blocksz)
{
	uint64_t stride = (disksz / ZONE_POINTS / ZONE_MIN_STRIDE + 1) * ZONE_MIN_STRIDE;
	uint64_t offs, t, time = 0, len = blocksz / _PAGE_SIZE * _PAGE_SIZE;
	unsigned int nzones = 0; 
	char *buff;

	if ((buff = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, NULL, 0)) == MAP_FAILED) {
		fprintf(stderr, "test_disk: failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Move to disk start */
	offs = 0;
	test_disk_lseek(fd, offs);

	/* Catch permissions */
	if (read(fd, buff, 512) != 512) {
		fprintf(stderr, "test_disk: IO error at offs=%" PRIu64 "\n", offs);
		return -EIO;
	}

	for (offs = 0; offs < disksz - len - 1024; offs += stride) {
		if ((t = test_disk_zonetime(fd, offs, buff, len)) < 0)
			return t;
		time += t / 1000;
		nzones++;
	}
	munmap(buff, len);

	if (nzones)
		printf("test_disk: average zone read time: %" PRIu64 "ms\n", time / nzones);
	else
		fprintf(stderr, "test_disk: no zone reads measured\n");

	return EOK;
}


int main(int argc, char *argv[])
{
	uint64_t size;
	int fd;

	if (argc != 2) {
		printf("Usage: %s <disk device>\n", argv[0]);
		return EOK;
	}

	printf("test_disk: starting, main is at %p\n", main);

	if ((fd = open(argv[1], 0)) < 0) {
		fprintf(stderr, "test_disk: failed to open disk %s\n", argv[1]);
		return -EINVAL;
	}

	size = test_disk_getsize(fd);
	printf("test_disk: disk %s has %" PRIu64 "MB of storage capacity\n", argv[1], size / (1 << 20));

	printf("test_disk: starting seek test...\n");
	test_disk_seek(fd, size);

	printf("test_disk: starting zone test...\n");
	test_disk_zone(fd, size, 1 << 20);

	return EOK;
}
