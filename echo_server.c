#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/types.h>

#define MAXDATA 1024

void fatal(char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int open_accepting_socket(int port) {
  struct sockaddr_in addr;
  int sock, sockopt;

  memset(&addr, 0, sizeof(addr));
  // アドレスファミリーはINET（インターネット）
  addr.sin_family = AF_INET;
  // クライアント側のIPアドレスは指定しない
  addr.sin_addr.s_addr = INADDR_ANY;
  // ポート番号をネットワーク順序で指定
  addr.sin_port = htons(port);

  // ソケットを作成
  // プロトコルファミリはインターネット
  sock = socket(PF_INET, SOCK_STREAM, 0);
  // 失敗
  if(sock < 0) {
    fatal("socket");
  }
  sockopt = 1;
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) == -1) {
    perror("sockopt");
  }

  if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fatal("bind");
  }

  if(listen(sock, SOMAXCONN) < 0) {
    fatal("listen");
  }

  return sock;
}

void echo_server(int client) {
  char buf[MAXDATA];
  int rv;

  while(1) {
    rv = read(client, buf, MAXDATA);
    // EOFの場合
    if(rv == 0) {
      break;
    } else if(rv < 0) { // エラーの場合
      perror("read");
      break;
    } else {
      write(client, buf, rv);
    }
  }

  printf("Connection closed.\n");
}

void accepting_loop(int sock) {
  struct sockaddr_in client;
  socklen_t clientlen = sizeof(client);
  int client_sock;

  while(1) {
    client_sock = accept(sock, (struct sockaddr *)&client, &clientlen);
    // クライアントと接続できていない
    if(client_sock < 0) {
      // システムコールが中断された
      if(errno != EINTR) {
	perror("accept");
	close(sock);
      } else {
	continue;
      }
    } else {  // 正常動作
      pid_t pid = fork(); // 子プロセスを作る
      // 子プロセス生成ミス
      if(pid < 0) {
	perror("can not fork");
	close(sock);
      }
      else if (pid == 0) {
	puts("child");
	echo_server(client_sock);
	close(client_sock);
	exit(0);
      } else {
	puts("parents");
      }
    }
  }
}

int main() {
  int sock;

  sock = open_accepting_socket(7777);
  accepting_loop(sock);
  puts("end.");
  close(sock);

  return 0;
}
