/*
 * sig_basic.c - Section 4.1: Signal là gì?
 *
 * Minh họa signal như "interrupt trên MCU":
 *   - SIGINT  (Ctrl+C): dừng chương trình
 *   - SIGUSR1 (kill -USR1 <pid>): user-defined event
 *
 * Build:  gcc -g -O0 -Wall -o sig_basic sig_basic.c
 * Run:    ./sig_basic
 *         (terminal khác) kill -USR1 <pid>
 *         (hoặc Ctrl+C để thoát)
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void handler(int sig)
{
	/* write() là async-signal-safe, printf() thì không */
	const char *msg_int  = "Nhận SIGINT (Ctrl+C) — thoát...\n";
	const char *msg_usr1 = "Nhận SIGUSR1 — custom event!\n";

	if (sig == SIGINT)
		write(STDOUT_FILENO, msg_int, 33);
	else if (sig == SIGUSR1)
		write(STDOUT_FILENO, msg_usr1, 30);
}

int main(void)
{
	printf("PID = %d\n", getpid());
	printf("Gửi signal: kill -USR1 %d\n", getpid());
	printf("Hoặc nhấn Ctrl+C để thoát.\n\n");

	/* Dùng signal() — đủ để demo khái niệm, không dùng trong production */
	signal(SIGINT,  handler);
	signal(SIGUSR1, handler);

	/* Main loop — tương tự vòng lặp chính trên MCU */
	int count = 0;

	while (1) {
		printf("Đang chạy... vòng %d\n", ++count);
		sleep(2);
	}

	return 0;
}