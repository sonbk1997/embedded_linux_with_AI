/*
 * sig_safe_flag.c - Section 4.3: async-signal-safe
 *
 * Minh họa pattern an toàn: dùng volatile sig_atomic_t thay vì
 * gọi printf()/malloc() trực tiếp trong handler.
 *
 * Build:  gcc -g -O0 -Wall -o sig_safe_flag sig_safe_flag.c
 * Run:    ./sig_safe_flag
 *         (Ctrl+C hoặc kill -TERM <pid> để thoát)
 *         (kill -USR1 <pid> để trigger custom event)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/*
 * volatile: báo compiler đừng cache biến trong register.
 * sig_atomic_t: đảm bảo đọc/ghi atomic — an toàn giữa handler và main.
 */
static volatile sig_atomic_t g_shutdown  = 0;
static volatile sig_atomic_t g_usr1_flag = 0;

static void handler(int sig)
{
	/*
	 * CHỈ làm việc tối thiểu, async-signal-safe:
	 *   - Ghi flag atomic
	 *   - Dùng write() nếu cần output (KHÔNG dùng printf)
	 */
	if (sig == SIGTERM || sig == SIGINT)
		g_shutdown = 1;
	else if (sig == SIGUSR1)
		g_usr1_flag = 1;
}

int main(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	printf("PID = %d\n", getpid());
	printf("kill -USR1 %d  → trigger event\n", getpid());
	printf("Ctrl+C hoặc kill -TERM %d  → shutdown\n\n", getpid());

	while (!g_shutdown) {
		/*
		 * Kiểm tra flag trong main loop — nơi an toàn để
		 * gọi printf(), malloc(), hay bất kỳ hàm nào.
		 */
		if (g_usr1_flag) {
			g_usr1_flag = 0;	/* clear flag */
			printf("[main] Xử lý SIGUSR1 event (an toàn)\n");
		}

		printf("[main] Working...\n");
		sleep(1);
	}

	/* Cleanup thực hiện tại đây — không phải trong handler */
	printf("\n[main] Nhận shutdown signal — cleanup...\n");
	printf("[main] Đóng file, flush buffer, release resource...\n");
	printf("[main] Thoát sạch.\n");

	return 0;
}