/*
 * sig_graceful_shutdown.c - Section 4.4a: Graceful shutdown
 *
 * Giả lập embedded daemon nhận SIGTERM từ systemd:
 *   - Daemon đang ghi log định kỳ
 *   - Khi nhận SIGTERM → hoàn thành công việc hiện tại → cleanup → exit
 *
 * Build:  gcc -g -O0 -Wall -o sig_graceful sig_graceful_shutdown.c
 * Run:    ./sig_graceful &
 *         kill -TERM <pid>     → graceful stop (như systemd stop)
 *         kill -KILL <pid>     → force kill (không thể bắt)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

static volatile sig_atomic_t g_running = 1;

static void handler(int sig)
{
	(void)sig;
	g_running = 0;
	/* Chú ý: không write gì ở đây — để main loop xử lý */
}

static void do_work(int iteration)
{
	/* Giả lập xử lý mất thời gian (đọc sensor, ghi log...) */
	printf("[daemon] Vòng %d: đang xử lý...\n", iteration);
	sleep(1);
	printf("[daemon] Vòng %d: hoàn thành.\n", iteration);
}

static void cleanup(void)
{
	printf("[daemon] Đóng UART...\n");
	printf("[daemon] Flush log buffer...\n");
	printf("[daemon] Release shared memory...\n");
	printf("[daemon] Cleanup done — exit(0)\n");
}

int main(void)
{
	struct sigaction sa;
	int iteration = 0;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	printf("[daemon] Khởi động, PID = %d\n", getpid());
	printf("[daemon] Gửi: kill -TERM %d để dừng graceful\n\n", getpid());

	while (g_running) {
		/*
		 * Kiểm tra flag TRƯỚC khi bắt đầu mỗi vòng.
		 * Nếu signal đến giữa chừng, vẫn để vòng hiện tại
		 * hoàn thành — không cắt giữa chừng.
		 */
		do_work(++iteration);
	}

	printf("\n[daemon] Nhận SIGTERM — bắt đầu shutdown...\n");
	cleanup();

	return 0;
}