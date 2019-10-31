// 파일명 : udp_mydaytime.c
// 기능 : daytime 서비스를 요청하는 UDP(비연결형) 클라이언트

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXLINE 511

int main(int argc, char *argv[]) {
	struct sockaddr_in servaddr;
	int s, nbyte, addrlen = sizeof(servaddr);
	struct hostent *hp;
	struct in_addr in;
	char buf[MAXLINE+1];
	char buf_again[20];

	if(argc != 2) {
		printf("Usage: %s hostname\n", argv[0]);
		exit(0);
	}

	// 소켓 생성
	if((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket fail");
		exit(1);
	}

	hp = gethostbyname(argv[1]);

	// 서버의 소켓주소 구조체 servaddr을 '\0'으로 초기화
	bzero((char *)&servaddr, addrlen);

	memcpy(&in.s_addr, hp->h_addr_list[0], sizeof(in.s_addr));
	inet_ntop(AF_INET, &in, buf_again, sizeof(buf_again));

	//servaddr의 주소 지정
	servaddr.sin_family = AF_INET;
	//inet_pton(AF_INET, hp->h_aliases[0], &servaddr.sin_addr);
	servaddr.sin_addr.s_addr = inet_addr(buf_again);
	servaddr.sin_port = htons(13); // daytime 서비스 포트

	if(sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr, addrlen) < 0) {
		perror("sendto fail");
		exit(1);
	}
	//서버가 보내주는 daytime 데이터의 수신 및 화면 출력
	nbyte = recvfrom(s, buf, MAXLINE, 0, (struct sockaddr *)&servaddr, &addrlen);

	if(nbyte < 0) {
		perror("recvfrom fail");
		exit(1);
	}

	buf[nbyte] = 0;
	printf("%s\n", buf);
	close(s);


	return 0;
}
