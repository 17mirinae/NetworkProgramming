#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BOARD_SIZE 5
#define MAXLINE 1000
#define NAME_LEN 20
#define FLAG "M/"

char *START_STRING = "start";
char *EXIT_STRING = "Exit";
char *YOUR_TURN = "Input Number...\n";
char *WRONG_NUMBER = "Wrong Number...\n";
char *NOT_YOUR_TURN = "Stay...\n";

int tcp_connect(int af, char *servip, unsigned short port);
void errquit(char *mesg) { perror(mesg); exit(1); }
void print_board(char buf[]); // 보드 데이터를 가지고 보드를 출력함
void string_to_board(char buf[]); // 서버에서 문자열로 받은 데이터를 이차원 배열로 저장
int board[BOARD_SIZE][BOARD_SIZE]; // 서버에서 받은 데이터를 저장할 이차원 배열

int main(int argc, char *argv[]) {
	char buf[MAXLINE];
	int maxfdp1, s;
	char *msg,*str_find;
	fd_set read_fds;

	if(argc != 2) {
		printf("사용법 : %s port\n", argv[0]);
		exit(0);
	}

	s = tcp_connect(AF_INET, "127.0.0.1", atoi(argv[1])); // TCP 연결 실행

	if(s == -1)
		errquit("tcp_connect fail");

	puts("서버에 접속되었습니다.\n");

	maxfdp1 = s + 1; // 최대 소켓번호 + 1
	FD_ZERO(&read_fds); // read_fds의 모든 비트를 지운다.

	while(1) {
		FD_SET(0, &read_fds);
		FD_SET(s, &read_fds);

		if(select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0)
			errquit("select fail");

		// 서버에서 보낸 메시지 받아오기
		if(FD_ISSET(s, &read_fds)) {
			int nbyte;
			if((nbyte = recv(s, buf, MAXLINE, 0)) > 0) {
				buf[nbyte] = 0;
				
				// FLAG를 찾는다면?
				if((str_find=strstr(buf,FLAG))!=NULL) {
					if(buf[0]!='M') {
						print_board(buf);
					}
					printf("%s",str_find+2);
				} else if ((str_find = strstr(buf, EXIT_STRING)) != NULL) {
					// 종료 문자 처리
					puts("Good bye");
					close(s);
					exit(0);
				} else {
					print_board(buf);
				}
			}
		}

		// 내가 서버에 메시지 보내기
		if(FD_ISSET(0, &read_fds)) {
			if(fgets(buf, MAXLINE, stdin)) {
				system("clear");
				if(buf[0]!="\n") {
					if(send(s, buf, strlen(buf), 0) < 0) {
						puts("Error: Write error on socket.");
					}
					if(strstr(buf, EXIT_STRING) != NULL) {
						puts("Good bye");
						close(s);
						exit(0);
					}
				}
			}
		}
	} // while end
}

int tcp_connect(int af, char *servip, unsigned short port) {
	struct sockaddr_in servaddr;
	int s;

	if((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;

	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = af;
	inet_pton(AF_INET, servip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	if(connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		return -1;

	return s;
}

void print_board(char buf[]) {
    string_to_board(buf);
	int i, j;

        printf("+----+----+----+----+----+\n");
        for(i = 0; i < BOARD_SIZE; i++)
        {
                for(j = 0; j < BOARD_SIZE; j++)
                {
                        if(board[i][j]==0)
                        {
                                printf("| ");
                                printf("%2c ", 88);
                        }
                        else
                                printf("| %2d ", board[i][j]);
                }
                printf("|\n");
                printf("+----+----+----+----+----+\n");
        }
}

void string_to_board(char buf[]) {
	char *ptr;
	int i=0;
	int temp = 0;

	int col = 0;
	int row = 0;
	
	ptr = strtok(buf, ",");
	while(ptr != NULL){
		board[0][i++]= atoi(ptr);
		ptr = strtok(NULL, ",");
	}
}
