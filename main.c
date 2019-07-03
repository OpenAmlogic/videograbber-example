#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define VIDEOGRABBER_IOC_MAGIC			'D'
#define VIDEOGRABBER_IOC_SETUP			_IOW(VIDEOGRABBER_IOC_MAGIC, 0x00, struct videograbber_setup_t)
#define VIDEOGRABBER_IOC_GET_FRAME		_IOR(VIDEOGRABBER_IOC_MAGIC, 0x01, struct videograbber_vframe_t)

enum videograbber_pixelformat
{
	VIDEOGRABBER_FORMAT_RGB888,
	VIDEOGRABBER_FORMAT_BGR888,
	VIDEOGRABBER_FORMAT_ABGR8888
};

struct videograbber_setup_t
{
	int out_width;
	int out_height;
	int out_stride;
	int out_format;
};

struct videograbber_vframe_t {
	unsigned long canvas_phys_addr[3];
	int width[3];
	int stride[3];
	int height[3];
};

int readIntFromFile(const char *path, int base, int *out)
{
	FILE *file = fopen(path, "r");
	if (!file)
		return -1;

	if (base == 10)
		fscanf(file, "%d", out);
	else if (base == 16)
		fscanf(file, "%x", out);
	else
		return -1;

	fclose(file);

	return 0;
}

/*
This example shows how to get a single raw videoframe using mmap.
It is the fastest method to grab a raw frame and have a look at it's data, or write it to a file.
This is probably best for things like ambient light video-frame analysis and things alike.

Call VIDEOGRABBER_IOC_GET_FRAME again to get an updated frame.
Subsequent calls to VIDEOGRABBER_IOC_GET_FRAME overwrite the preceding frame!

The memory will be freed once /dev/videograber is closed

In this example the content is written to a file.
 */
int mapSingleFrame(const char *path, int width, int height)
{
	int ret = -1;
	int fd = -1;

	fd = open("/dev/videograbber", O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "%s: failed to open device (%m)\n", __FUNCTION__);
		return -1;
	}

	struct videograbber_setup_t setup = { 0 };
	setup.out_width = width;
	setup.out_height = height;
	setup.out_stride = width * 4; // bytes per line (you can use your target's stride here if needed)
	setup.out_format = VIDEOGRABBER_FORMAT_ABGR8888; // -1 would be VIDEOGRABBER_FORMAT_BGR888
	if (ioctl(fd, VIDEOGRABBER_IOC_SETUP, &setup) < 0)
	{
		fprintf(stderr, "%s: can't setup videograbber (%m)\n", __FUNCTION__);
		goto err;
	}

	struct videograbber_vframe_t vf;
	if (ioctl(fd, VIDEOGRABBER_IOC_GET_FRAME, &vf) != 0)
	{
		fprintf(stderr, "%s: can't get current frame (%m)\n", __FUNCTION__);
		goto err;
	}

	size_t mapLength = vf.stride[0] * vf.height[0];
	void *srcAddr = mmap(NULL, mapLength, PROT_READ, MAP_SHARED, fd, vf.canvas_phys_addr[0]);
	if (srcAddr == MAP_FAILED)
	{
		fprintf(stderr, "%s: error while mapping src buffer (%m)\n", __FUNCTION__);
		goto err;
	}

	FILE *dump = fopen(path, "wb");
	if (!dump)
	{
		fprintf(stderr, "%s: can't open output file %s (%m)\n", path, __FUNCTION__);
		goto err;
	}
	fwrite(srcAddr, mapLength, 1, dump);
	fclose(dump);

	munmap(srcAddr, mapLength);

	close(fd);
	ret = 0;

err:
	if (fd >= 0)
		close(fd);

	return ret;
}

/*
This example shows how to read a single raw videoframe into an arbitary buffer (here: void* framebuffer).
This is required if you want to reuse the catpured frame once read.

Dreambox OS uses this way to store a video frame into a pixmap in enigma2.

In this example the content of the arbitary buffer is written to a file.
 */
int readSingleFrame(const char *path, int width, int height)
{
	int ret = -1;
	int fd = -1;
	void *framebuffer = NULL;

	fd = open("/dev/videograbber", O_RDWR);
	if (fd < 0)
	{
		fprintf(stderr, "%s: failed to open device (%m)\n", __FUNCTION__);
		return -1;
	}

	struct videograbber_setup_t setup = { 0 };
	setup.out_width = width;
	setup.out_height = height;
	setup.out_stride = -1; // bytes per line (you can use your target's stride here if needed)
	setup.out_format = VIDEOGRABBER_FORMAT_ABGR8888; // -1 would be VIDEOGRABBER_FORMAT_BGR888
	if (ioctl(fd, VIDEOGRABBER_IOC_SETUP, &setup) < 0)
	{
		fprintf(stderr, "%s: can't setup videograbber (%m)\n", __FUNCTION__);
		goto err;
	}

	int dump_size = 4 * width * height; // 4 bytes per pixel

	framebuffer = malloc(dump_size);
	if (framebuffer == NULL)
	{
		fprintf(stderr, "%s: failed to malloc framebuffer (%m)\n", __FUNCTION__);
		goto err;
	}

	if (read(fd, framebuffer, dump_size) <= 0)
	{
		fprintf(stderr, "%s: error while read (%m)\n", __FUNCTION__);
		goto err;
	}

	FILE *dump = fopen(path, "wb");
	if (!dump)
	{
		fprintf(stderr, "%s: can't open output file %s (%m)\n", path, __FUNCTION__);
		goto err;
	}
	fwrite(framebuffer, dump_size, 1, dump);
	fclose(dump);

	ret = 0;

err:
	if (fd >= 0)
		close(fd);

	if (framebuffer)
		free(framebuffer);
	return ret;
}

int zoomWidth(int width, int height, int aspect)
{
	int calculatedAspect = 256 * height / width;

	if (aspect == calculatedAspect)
		return width;

	return 256 * height / aspect;
}

int main(void)
{
	const char *path1 = "/tmp/dump1.abgr8888";
	const char *path2 = "/tmp/dump2.abgr8888";
	int width = 1280;
	int height = 720;
	// set a fixed aspect ratio of 16:9
	// calucation: 256 * height / width
	int aspect = 0x90;

	readIntFromFile("/sys/class/video/frame_width", 10, &width);
	readIntFromFile("/sys/class/video/frame_height", 10, &height);
	readIntFromFile("/sys/class/video/frame_aspect_ratio", 16, &aspect);

	width = zoomWidth(width, height, aspect); //adjust aspect of source -> force 16:9

	if (mapSingleFrame(path1, width, height) == 0)
	{
		printf("Mapping video frame into %s\n", path1);
		printf("Verify: Upload file to www.rawpixels.net, set width to %d, height to %d and predefined format to RGB32\n\n", width, height);
	}
	else
		printf("Mapping failed\n");

	if (readSingleFrame(path2, width, height) == 0)
	{
		printf("Read video frame into %s\n", path2);
		printf("Verify: Upload file to www.rawpixels.net, set width to %d, height to %d and predefined format to RGB32\n\n", width, height);
	}
	else
		printf("Reading failed\n");

	return 0;
}

