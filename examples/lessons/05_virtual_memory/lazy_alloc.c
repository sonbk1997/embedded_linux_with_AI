#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096
#define MB (1024 * 1024)

/* Đọc trực tiếp từ /proc/self/status — không dùng system() */
static long read_vmrss_kb(void)
{
	FILE *f;
	char line[128];
	long rss = -1;

	f = fopen("/proc/self/status", "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "VmRSS: %ld", &rss) == 1)
			break;
	}
	fclose(f);
	return rss;
}

static long read_vmsize_kb(void)
{
	FILE *f;
	char line[128];
	long vsz = -1;

	f = fopen("/proc/self/status", "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "VmSize: %ld", &vsz) == 1)
			break;
	}
	fclose(f);
	return vsz;
}

static void print_memory_usage(const char *stage)
{
	printf("\n=== %s ===\n", stage);
	printf("Physical memory (RSS): %ld kB\n", read_vmrss_kb());
	printf("Virtual memory:        %ld kB\n", read_vmsize_kb());
}

int main(void)
{
	printf("Lazy Allocation Demo\n");
	print_memory_usage("INITIAL STATE");

	/* Step 1: Allocate 10MB virtual memory */
	printf("\n1. Allocating 10MB with malloc()...\n");
	char *big_buffer = malloc(10 * MB);

	printf("malloc() returned: %p\n", big_buffer);
	print_memory_usage("AFTER MALLOC (no physical memory yet)");

	/*
	 * Step 2: Dùng mmap() + mincore() để đếm chính xác page resident.
	 * VmRSS từ /proc/status bị ảnh hưởng bởi THP (Transparent Huge Pages),
	 * nhảy theo chunk lớn thay vì 4KB.
	 * mincore() trả về đúng bit per-page: 1 = page đang ở RAM.
	 */
	printf("\n2. Writing page by page — dùng mincore() đếm pages resident...\n");
	printf("   %-6s  %-10s  %-14s  %s\n",
	       "Page", "Địa chỉ", "Pages in RAM", "Status");
	printf("   %-6s  %-10s  %-14s  %s\n",
	       "------", "----------", "--------------", "------");

	int total_pages = 10;
	char *mmap_buf = mmap(NULL, (size_t)total_pages * PAGE_SIZE,
			      PROT_READ | PROT_WRITE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	unsigned char vec[total_pages];

	/* Trước khi ghi: tất cả page chưa resident */
	memset(vec, 0, sizeof(vec));
	mincore(mmap_buf, (size_t)total_pages * PAGE_SIZE, vec);
	int resident = 0;
	for (int j = 0; j < total_pages; j++)
		resident += (vec[j] & 1);
	printf("   Trước khi ghi: %d/%d pages in RAM\n\n", resident, total_pages);

	for (int i = 0; i < total_pages; i++) {
		mmap_buf[i * PAGE_SIZE] = 'A' + i;	/* page fault */

		/* Đếm lại pages resident sau mỗi lần ghi */
		mincore(mmap_buf, (size_t)total_pages * PAGE_SIZE, vec);
		resident = 0;
		for (int j = 0; j < total_pages; j++)
			resident += (vec[j] & 1);

		printf("   Page%-2d  %p  %2d/%d in RAM   ← +1 page fault\n",
		       i, (void *)&mmap_buf[i * PAGE_SIZE],
		       resident, total_pages);
	}
	munmap(mmap_buf, (size_t)total_pages * PAGE_SIZE);

	/* Step 3: Touch toàn bộ buffer */
	printf("\n3. Writing entire buffer (force all page faults)...\n");
	memset(big_buffer, 'C', 10 * MB);
	print_memory_usage("AFTER MEMSET (all pages allocated)");

	free(big_buffer);
	print_memory_usage("AFTER FREE");

	return 0;
}
