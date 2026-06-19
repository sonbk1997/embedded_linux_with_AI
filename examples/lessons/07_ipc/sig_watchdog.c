/*
 * sig_watchdog.c - Section 4.4b: Software watchdog với SIGALRM
 *
 * Pattern watchdog đơn giản bằng alarm():
 *   - alarm(N) → kernel gửi SIGALRM sau N giây nếu không reset
 *   - Mỗi vòng lặp gọi alarm() lại để "kick" watchdog
 *   - Nếu vòng lặp bị block (hung), SIGALRM kích handler → abort
 *
 * Build:  gcc -g -O0 -Wall -o sig_watchdog sig_watchdog.c
 * Run:    ./sig_watchdog        → chạy bình thường, watchdog tự reset
 *         ./sig_watchdog hang   → giả lập hang → watchdog kích
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#define WATCHDOG_TIMEOUT 3	/* giây */

static void watchdog_handler(int sig)
{
	(void)sig;
	/*
	 * Dùng write() + _exit() — không dùng printf/exit trong handler.
	 * _exit() là async-signal-safe, exit() thì không.
	 */
	const char msg[] = "\n[WATCHDOG] Timeout! System hung — aborting.\n";

	write(STDERR_FILENO, msg, sizeof(msg) - 1);
	_exit(EXIT_FAILURE);
}

static void setup_watchdog(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = watchdog_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;		/* KHÔNG dùng SA_RESTART — muốn handler chạy ngay */

	sigaction(SIGALRM, &sa, NULL);
}

static void kick_watchdog(void)
{
	alarm(WATCHDOG_TIMEOUT);	/* reset countdown */
}

int main(int argc, char *argv[])
{
	int simulate_hang = (argc > 1 && strcmp(argv[1], "hang") == 0);
	int iteration = 0;

	setvbuf(stdout, NULL, _IOLBF, 0);	/* line-buffered để _exit() không mất output */

	setup_watchdog();
	kick_watchdog();	/* bắt đầu đếm lần đầu */

	printf("[main] Watchdog timeout = %d giây\n", WATCHDOG_TIMEOUT);
	if (simulate_hang)
		printf("[main] Chế độ: giả lập HANG ở vòng 3\n\n");
	else
		printf("[main] Chế độ: bình thường (watchdog tự reset)\n\n");

	while (1) {
		printf("[main] Vòng %d: đang xử lý...\n", ++iteration);

		if (simulate_hang && iteration == 3) {
			/* Giả lập hung: sleep vượt quá watchdog timeout */
			printf("[main] Vòng %d: GIẢ LẬP HANG (sleep %ds)...\n",
			       iteration, WATCHDOG_TIMEOUT + 2);
			sleep(WATCHDOG_TIMEOUT + 2);	/* → watchdog sẽ kích */
		} else {
			sleep(1);	/* công việc bình thường */
		}

		printf("[main] Vòng %d: xong, kick watchdog.\n", iteration);
		kick_watchdog();	/* reset watchdog mỗi vòng */
	}

	return 0;
}