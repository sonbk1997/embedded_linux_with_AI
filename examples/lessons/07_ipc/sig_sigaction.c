/*
 * sig_sigaction.c - Section 4.2: signal() vs sigaction()
 *
 * So sánh hai cách đăng ký signal handler.
 * Demo SA_RESTART: khi signal đến trong lúc read() đang chờ,
 * syscall tự động restart thay vì trả EINTR.
 *
 * Build:  gcc -g -O0 -Wall -o sig_sigaction sig_sigaction.c
 * Run:    ./sig_sigaction
 *         (terminal khác) kill -USR1 <pid>   → thấy EINTR nếu bỏ SA_RESTART
 *                         kill -TERM <pid>   → graceful stop
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static volatile sig_atomic_t g_stop = 0;

static void handler(int sig)
{
	const char *msg_usr1 = "[handler] SIGUSR1\n";
	const char *msg_term = "[handler] SIGTERM — g_stop = 1\n";
	const char *msg_int  = "[handler] SIGINT (Ctrl+C) — dùng kill -TERM để thoát\n";
	const char *msg_tstp = "[handler] SIGTSTP (Ctrl+Z) — bị chặn, không suspend\n";

	if (sig == SIGUSR1) {
		write(STDOUT_FILENO, msg_usr1, 18);
	} else if (sig == SIGTERM) {
		write(STDOUT_FILENO, msg_term, 31);
		g_stop = 1;
	} else if (sig == SIGINT) {
		write(STDOUT_FILENO, msg_int, 53);
	} else if (sig == SIGTSTP) {
		write(STDOUT_FILENO, msg_tstp, 53);
	}
}

static void setup_sigaction(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;	/* syscall tự restart khi bị interrupt */

	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);	/* Ctrl+C — không terminate ngay */
	sigaction(SIGTSTP, &sa, NULL);	/* Ctrl+Z — không suspend ngay */
}

int main(void)
{
	char buf[64];
	ssize_t n;

	printf("PID = %d\n", getpid());
	printf("SA_RESTART demo: gõ gì đó rồi Enter, hoặc kill -USR1 %d\n", getpid());
	printf("kill -TERM %d để thoát.\n\n", getpid());

	setup_sigaction();

	while (!g_stop) {
		printf("Nhập chuỗi (hoặc đợi signal): ");
		fflush(stdout);

		n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
		if (n < 0) {
			/*
			 * Với SA_RESTART, dòng này sẽ không bao giờ chạy khi
			 * SIGUSR1 đến. Không có SA_RESTART thì n == -1, errno == EINTR.
			 */
			if (errno == EINTR)
				printf("[main] read() bị EINTR (không có SA_RESTART)\n");
			else
				perror("read");
			continue;
		}
		if (n == 0)
			break;

		buf[n - 1] = '\0';	/* bỏ newline */
		printf("[main] Nhận: \"%s\"\n", buf);
	}

	printf("Thoát sạch.\n");
	return 0;
}