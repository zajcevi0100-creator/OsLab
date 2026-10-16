#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int port = 9999;

volatile sig_atomic_t was_sighup = 0;
volatile sig_atomic_t was_sigint = 0;
volatile sig_atomic_t running = 1;

void on_sighup(int signo) {
	was_sighup =1;
}

void on_sigint(int signo) {
	was_sigint = 1;
	running = 0;
}

int main() {
	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == -1) {
		perror("socket");
		return EXIT_FAILURE;
	}
	int opt =1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		perror("setsockopt");
		close(server);
		return EXIT_FAILURE;
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind");
		close(server);
		return EXIT_FAILURE;
	}

	if(listen(server,16) == -1)  {
		perror("listen");
		close(server);
		return EXIT_FAILURE;
	}

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = on_sighup;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGHUP, &act, NULL) == -1) {
		perror("sigaction SIGHUP");
		close(server);
		return EXIT_FAILURE;
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = on_sigint;
	if (sigaction(SIGINT, &act, NULL) == -1) {
		perror("sigaction SIGINT");
		close(server);
		return EXIT_FAILURE;
	}

	sigset_t block, old ;
	sigemptyset(&block);
	sigaddset(&block, SIGHUP);
	sigaddset(&block, SIGINT);
	if (sigprocmask(SIG_BLOCK, &block, &old) == -1) {
		perror("sigprocmask");
		close(server);
		return EXIT_FAILURE;
	}

	printf("Server start on port %d (PID %d)\n", port, getpid());
	int client = -1;
	while(running) {
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(server, &read_set);
		int max = server;
		if (client != -1) {
			FD_SET(client, &read_set);
			if (client > max) max = client;
		}
		int ready = pselect(max +1, &read_set, NULL, NULL, NULL, &old);
		if (was_sighup) {
			was_sighup =0;
			printf("SIGHUP получен\n");
		}
		if (was_sigint) {
			was_sigint =0;
			printf("SIGINT получен\n");
			break;
		}
		if (ready == -1) {
			if (errno == EINTR) continue;
			perror("pselect");
			break;
		}
		if (FD_ISSET(server, &read_set)) {
			int new_client = accept(server, NULL, NULL);
			if (new_client == -1) {
				perror("accept");
			} else if(client ==  -1) {
				client = new_client;
				printf("Client connected: fd=%d\n", client);
				continue;
			} else {
				printf("Дополнительный клиент закрыт: fd = %d\n", new_client);
				close(new_client);
				continue;
			}
		}
		if (client != -1 && FD_ISSET(client, &read_set)) {
			char buf[4096];
			ssize_t n =recv(client,buf,sizeof(buf),0);
			if (n>0) {
				printf("Получено %zd байтов\n",n);
			} else if (n == 0) {
				printf("Client disconnected\n");
				close(client);
				client = -1;
			} else {
				perror("recv");
				close(client);
				client = -1;
			}
		}
	}
	if (client != -1) close(client);
	close(server);
	sigprocmask(SIG_SETMASK, &old, NULL);
	printf("Server stopped\n");
	return EXIT_SUCCESS;
}
