/*
 * mmu.c - Minh họa hoạt động của MMU: TLB, page fault, memory protection
 *
 * Bài 05: Virtual Memory
 * Mục 4: MMU và cách CPU truy cập bộ nhớ qua MMU
 *
 * Build: gcc -g -O0 -Wall -Wextra -o mmu mmu.c
 * Run:   ./mmu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#define PAGE_SIZE	4096
#define MB		(1024 * 1024)

/* ------------------------------------------------------------------ */
/* Helper: đo thời gian nanosecond                                     */
/* ------------------------------------------------------------------ */
static long ns_now(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/* ------------------------------------------------------------------ */
/* Demo 1: Virtual address layout của process                          */
/* Quan sát: mỗi segment nằm ở vùng địa chỉ khác nhau                 */
/* ------------------------------------------------------------------ */
int global_init   = 42;	/* Data segment */
int global_uninit;		/* BSS segment  */

static void demo_address_layout(void);	/* forward decl để lấy địa chỉ */

static void demo_address_layout(void)
{
	int   stack_var = 100;
	int  *heap_ptr  = malloc(sizeof(int));

	printf("============================================================\n");
	printf("DEMO 1: Virtual Address Layout\n");
	printf("============================================================\n");
	printf("Text  (demo_address_layout): %p\n", (void *)demo_address_layout);
	printf("Data  (global_init):   %p\n", (void *)&global_init);
	printf("BSS   (global_uninit): %p\n", (void *)&global_uninit);
	printf("Heap  (malloc):        %p\n", (void *)heap_ptr);
	printf("Stack (local_var):     %p\n", (void *)&stack_var);
	printf("Lib   (printf):        %p\n", (void *)printf);
	printf("\n");

	printf("Khoảng cách Heap → Stack: %ld GB (virtual gap, không tốn RAM)\n",
	       ((long)&stack_var - (long)heap_ptr) / (1024 * 1024 * 1024));
	printf("\n");
	printf("→ Chạy lệnh sau để xem full map:\n");
	printf("  cat /proc/%d/maps\n", getpid());
	printf("\n");

	free(heap_ptr);
}

/* ------------------------------------------------------------------ */
/* Demo 2: TLB hit vs TLB miss                                         */
/* Sequential access (TLB hit) vs random access (TLB miss)            */
/* TLB cache địa chỉ đã dịch — access cùng page liên tiếp = fast     */
/* ------------------------------------------------------------------ */
static void demo_tlb(void)
{
	/*
	 * So sánh hai access pattern:
	 *   Stride-64:   nhiều access trên cùng 1 page → TLB entry tái dụng nhiều lần
	 *   Stride-4096: mỗi access nhảy sang page mới → TLB phải nạp entry mới mỗi lần
	 *
	 * Dùng buffer 128MB (vượt L3 cache ~32MB) để loại bỏ cache effect.
	 */
	const size_t buf_size   = 128 * MB;
	const size_t n_accesses = 4 * 1024 * 1024;	/* 4M lần */
	const size_t stride_hit = 64;			/* trong 1 page   */
	const size_t stride_miss = PAGE_SIZE;		/* nhảy qua page  */
	volatile char *buf;
	size_t        i;
	long          t_start, t_hit, t_miss;

	buf = malloc(buf_size);
	if (!buf) {
		perror("malloc");
		return;
	}
	memset((void *)buf, 0, buf_size);	/* force tất cả page vào RAM */

	printf("============================================================\n");
	printf("DEMO 2: TLB Hit vs TLB Miss\n");
	printf("============================================================\n");
	printf("Buffer: 128 MB (vượt L3 cache để loại cache effect)\n");
	printf("Accesses: %zu lần\n\n", n_accesses);
	printf("Stride-%-4zu: nhiều access trong cùng 1 page → TLB entry tái dụng\n",
	       stride_hit);
	printf("Stride-%-4zu: mỗi access sang page mới   → TLB nạp entry mới\n\n",
	       stride_miss);

	/* Stride-64: 4096/64 = 64 access / page → TLB entry dùng 64 lần */
	t_start = ns_now();
	for (i = 0; i < n_accesses; i++)
		buf[(i * stride_hit) % buf_size]++;
	t_hit = ns_now() - t_start;

	/* Stride-4096: 1 access / page → TLB entry nạp rồi bỏ ngay */
	t_start = ns_now();
	for (i = 0; i < n_accesses; i++)
		buf[(i * stride_miss) % buf_size]++;
	t_miss = ns_now() - t_start;

	printf("Stride-%-4zu (TLB-friendly):   %6ld ms\n",
	       stride_hit, t_hit / 1000000);
	printf("Stride-%-4zu (TLB-unfriendly): %6ld ms\n",
	       stride_miss, t_miss / 1000000);
	if (t_hit > 0)
		printf("TLB miss overhead: %.1fx chậm hơn\n",
		       (double)t_miss / t_hit);
	printf("\n");
	printf("→ Stride-64:   TLB entry dùng cho 64 access → chi phí chia đều\n");
	printf("→ Stride-4096: TLB entry chỉ dùng 1 lần → overhead tối đa\n");
	printf("\n");
	printf("Tip: đo TLB miss thực tế bằng perf:\n");
	printf("  perf stat -e dTLB-load-misses,dTLB-store-misses ./mmu\n");
	printf("\n");

	free((void *)buf);
}

/* ------------------------------------------------------------------ */
/* Demo 3: Page fault — lazy allocation                                */
/* malloc() chỉ reserve virtual space, physical page chỉ cấp          */
/* khi ghi lần đầu vào page đó (page fault)                            */
/* Quan sát VmRSS tăng dần theo từng lần ghi                          */
/* ------------------------------------------------------------------ */
static long read_vmrss_kb(void)
{
	FILE *f;
	char  line[128];
	long  rss = -1;

	f = fopen("/proc/self/status", "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "VmRSS: %ld kB", &rss) == 1)
			break;
	}
	fclose(f);
	return rss;
}

static void demo_page_fault(void)
{
	const size_t alloc_size = 8 * MB;	/* 8MB */
	char        *buf;
	size_t       i;

	printf("============================================================\n");
	printf("DEMO 3: Page Fault — Lazy Allocation\n");
	printf("============================================================\n");
	printf("Sẽ malloc(%zu MB) rồi ghi từng page và quan sát VmRSS\n\n",
	       alloc_size / MB);

	printf("Trước malloc:          VmRSS = %ld kB\n", read_vmrss_kb());

	buf = malloc(alloc_size);
	if (!buf) {
		perror("malloc");
		return;
	}

	printf("Sau  malloc (chưa ghi):VmRSS = %ld kB  ← chưa tăng!\n",
	       read_vmrss_kb());

	/* Ghi từng page — mỗi lần ghi lần đầu vào page → page fault */
	for (i = 0; i < alloc_size; i += PAGE_SIZE) {
		buf[i] = 'X';
		if (i == 0 || i == 2 * MB || i == 4 * MB || i == 6 * MB) {
			printf("Sau khi ghi %2zu MB:    VmRSS = %ld kB\n",
			       i / MB, read_vmrss_kb());
		}
	}
	/* In lần cuối cho đủ */
	printf("Sau khi ghi toàn bộ:   VmRSS = %ld kB  ← tăng ~8MB\n",
	       read_vmrss_kb());
	printf("\n");
	printf("→ malloc() chỉ tạo VMA (virtual), không cấp physical page\n");
	printf("→ Mỗi lần ghi lần đầu vào page mới → CPU báo page fault\n");
	printf("→ Kernel xử lý: cấp physical page → cập nhật page table\n");
	printf("→ VmRSS tăng dần theo số page đã được touch\n");
	printf("\n");

	free(buf);
}

/* ------------------------------------------------------------------ */
/* Demo 4: Memory protection — MMU enforce permission                  */
/* Ghi vào vùng read-only → SIGSEGV                                   */
/* Dùng sigsetjmp/siglongjmp để bắt lỗi mà không crash chương trình  */
/* ------------------------------------------------------------------ */
static sigjmp_buf segv_jmp;

static void segv_handler(int sig)
{
	(void)sig;
	siglongjmp(segv_jmp, 1);
}

static void demo_memory_protection(void)
{
	void              *ro_page;
	struct sigaction   sa, old_sa;
	volatile char     *ptr;

	printf("============================================================\n");
	printf("DEMO 4: Memory Protection (MMU Permission Bits)\n");
	printf("============================================================\n");

	/* Cấp 1 page, đặt permission PROT_READ only */
	ro_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ro_page == MAP_FAILED) {
		perror("mmap");
		return;
	}

	/* Ghi data khi còn PROT_WRITE */
	*(char *)ro_page = 'A';
	printf("Ghi vào page khi có PROT_WRITE: OK (giá trị = '%c')\n",
	       *(char *)ro_page);

	/* Đổi thành read-only */
	mprotect(ro_page, PAGE_SIZE, PROT_READ);
	printf("Đã đổi permission thành PROT_READ only\n");

	/* Cài signal handler tạm để bắt SIGSEGV */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = segv_handler;
	sigaction(SIGSEGV, &sa, &old_sa);

	ptr = (volatile char *)ro_page;

	printf("Thử đọc từ read-only page: '%c'  ← OK\n", *ptr);
	printf("Thử GHI vào read-only page...\n");

	if (sigsetjmp(segv_jmp, 1) == 0) {
		*ptr = 'B';		/* → MMU phát hiện vi phạm → SIGSEGV */
		printf("KHÔNG xảy ra lỗi (không đúng!)\n");
	} else {
		printf("→ SIGSEGV! MMU chặn ghi vào vùng read-only\n");
	}

	/* Restore signal handler */
	sigaction(SIGSEGV, &old_sa, NULL);

	munmap(ro_page, PAGE_SIZE);
	printf("\n");
	printf("→ MMU kiểm tra permission bit trong page table entry\n");
	printf("→ Vi phạm → CPU raise exception → kernel gửi SIGSEGV\n");
	printf("→ Đây là cơ chế bảo vệ memory giữa các vùng nhớ\n");
	printf("\n");
}

/* ------------------------------------------------------------------ */
/* Demo 5: /proc/self/maps — đọc toàn bộ VMA của process hiện tại    */
/* ------------------------------------------------------------------ */
static void demo_proc_maps(void)
{
	FILE *f;
	char  line[256];
	int   count = 0;

	printf("============================================================\n");
	printf("DEMO 5: /proc/self/maps — Virtual Memory Areas (VMAs)\n");
	printf("============================================================\n");
	printf("Format: [start-end] [perm] [offset] [dev] [inode] [file]\n");
	printf("perm: r=read w=write x=execute p=private s=shared\n\n");

	f = fopen("/proc/self/maps", "r");
	if (!f) {
		perror("fopen /proc/self/maps");
		return;
	}
	while (fgets(line, sizeof(line), f)) {
		/* Bỏ newline */
		line[strcspn(line, "\n")] = '\0';
		printf("  %s\n", line);
		count++;
	}
	fclose(f);

	printf("\nTổng cộng %d VMAs — chỉ những vùng này mới có trong page table\n",
	       count);
	printf("Phần còn lại của 256TB virtual space: không có entry → unmapped\n");
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
	printf("\n");
	printf("╔══════════════════════════════════════════════════════════╗\n");
	printf("║           MMU Demo — Bài 05: Virtual Memory             ║\n");
	printf("╚══════════════════════════════════════════════════════════╝\n");
	printf("\n");

	demo_address_layout();
	demo_tlb();
	demo_page_fault();
	demo_memory_protection();
	demo_proc_maps();

	return 0;
}