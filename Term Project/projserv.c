#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define BOARD_SIZE 5
#define MAXLINE 511
#define MAX_SOCK 1024
#define PEOPLE 2

//종료 빙고수
#define WIN 3

char *EXIT_STRING = "Exit";
char *START_STRING = "M/Connected to chat server\n";
char *NOT_YOUR_TURN = "M/Stay...\n";
char *YOUR_TURN = "M/Input Number...\n";
char *WRONG_NUMBER = "M/Wrong Number...\n";
char *WINNER = "M/Win. Bye\n";
char *LOSER = "M/Lose. Bye\n";
char *DRAW = "M/Draw. Bye\n";
char *OPPOSITE_ENTER="M/Your opponent entered ";

int user_turn = 0; // 사용자 턴제
int maxfdp1; // 최대 소켓번호 + 1
int num_chat = 0; // 연결된 소켓 개수
int clisock_list[MAX_SOCK]; // 소켓 리스트
int listen_sock; // 연결을 대기하는 소켓
int board[2][BOARD_SIZE][BOARD_SIZE]; // 보드 판떼기[사용자 구분][가로][세로]

void addClient(int s, struct sockaddr_in *newCliaddr); // 플레이어 추가
int getmax(); // 소켓의 최대번호를 구함
void removeClient(int s); // 플레이어 제거
int tcp_listen(int host, int port, int backlog);
int set_nonblock(int sockfd); // 소켓을 넌블록으로 설정
int is_nonblock(int sockfd); // 소켓이 넌블록 모드인지 확인
void errquit(char *mesg) { perror(mesg); exit(1); } // 에러 뜨면 메시지 띄워주고 종료
void make_board(int cli_id); // 판떼기 만들어줌
int board_to_string(int cli_id); // 보드 만든 거 문자열로 바꿔서 보내주는 것까지 수행
void game_set(); // 보드를 만들어서 플레이어들에게 보내주는 것까지 수행

int main(int argc, char *argv[]) {
        struct sockaddr_in cliaddr;
        char buf[MAXLINE+1];

        // 상대가 문자열 입력햇을때 전송하는 메시지 내용을 담는 변수
        char *opponent_input_msg;
        int i, j, nbyte, accp_sock, addrlen = sizeof(struct sockaddr_in), count, clilen;
        int bingo_result = 0; // 빙고 개수
        fd_set read_fds;
        int gs=0;
        // Game State - 0 --> 시작 전
        // Game State - 1 --> 게임 시작 전 세팅
        // Game State - 2 --> 차례 구분
        // Game State - 3 --> 게임 중

        if(argc != 2) {
                printf("사용법: %s, port\n", argv[0]);
                exit(0);
        }

        listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5); // argv[1]으로 들어오는 요청 받음

        if(listen_sock == -1)
                errquit("tcp_listen fail");
        if(set_nonblock(listen_sock) == -1)
                errquit("set_nonblock fail");

        for(count = 0; ;count++) {
                clilen = sizeof(cliaddr);
                accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clilen); // 연결 받은 소켓?

                if(accp_sock == -1 && errno != EWOULDBLOCK)
                        errquit("accept fail");
                else if(accp_sock > 0) {
                        clisock_list[num_chat] = accp_sock;
                        if(is_nonblock(accp_sock) != 0 && set_nonblock(accp_sock) < 0)
                                errquit("set_nonblock fail");

                        // accp_sock(소켓 번호)를 clisock_list에 넣어주고, cliaddr 안에 있는 sin_addr을 출력해봄
                        addClient(accp_sock, &cliaddr);

                        printf("%d번째 사용자 추가.\n", num_chat);
                        if(num_chat==2){
                                gs=1;
                        }
                }

                // 클라이언트가 보낸 메시지를 모든 클라이언트에게 방송
                for(i = 0;i < num_chat;i++) {
                        errno = 0;
                        if(gs == 1) {
                                game_set(); // 보드 안에 들어갈 숫자를 만들어서 문자열로 변환해 보내줌
                                gs++; // game state == 2로 게임 시작
                        } // 접속한 두 명의 플레이어들에게 보드를 만들어 보내주었음
                        // 이건 게임 시작시 한번만

                        if(gs == 2) { // 여기서 턴 메세지 보내기!!
                                send(clisock_list[(user_turn)%2], YOUR_TURN, strlen(YOUR_TURN), 0);             // 현재 턴인 사람
                                send(clisock_list[(user_turn+1)%2], NOT_YOUR_TURN, strlen(NOT_YOUR_TURN), 0);   // 현재 턴 아닌 사람
                                // board to string
                                gs++; // game state == 3
                        }

                        if(gs == 3) { // 게임 시작
                                nbyte = recv(clisock_list[i], buf, MAXLINE, 0); // 클라이언트에게서 메세지를 받아옴
                                if(nbyte == 0) {
                                        removeClient(i);
                                        continue;
                                }
                                else if(nbyte == -1 && errno == EWOULDBLOCK)
                                        continue;

                                // 종료문자 처리 (Exit 입력시)
                                if(strstr(buf, EXIT_STRING) != NULL) {
                                        removeClient(i);
                                        continue;
                                }

                                // 모든 채팅 참가자에게 메시지 방송
                                buf[nbyte] = 0;
                                if (i == user_turn % 2) {
                                        int inx; // 인덱스 역할
                                        int res; // 결과 넣는 변수
                                        
                                        if (nbyte == 0) {
                                                printf("null\n");
                                        } else {
                                                printf("nbyte:: %d\n",nbyte);
                                                printf("not null\n");
                                        }

                                        printf("buf : %s\n", buf);
                                        res = remove_from_board(user_turn % 2, buf);
                                        printf("res : %d\n", res);

                                        if(res){
                                                strcpy(opponent_input_msg,OPPOSITE_ENTER); // OPPOSITE_ENTER 메세지를 버퍼에 저장 (상대방이 입력한 숫자를 출력하기 위함)
                                                strcat(opponent_input_msg,buf); // 버퍼에 위에 저장했던 버퍼를 이어서 저장

                                                // 상대편에 위에 저장했던 버퍼를 보내줌. (상대방이 입력한 숫자 전송)
                                                send(clisock_list[(user_turn + 1) % 2], opponent_input_msg, strlen(opponent_input_msg), 0);
                                                remove_from_board((user_turn + 1) % 2, buf); // 전송이 끝났으니 이제 숫자를 비교해서 0으로 변환시켜줄 차례
                                                user_turn++;
                                                gs = 2;

                                                board_to_string(0); // 문자열로 변환 뒤 0번째 플레이어에게 보내줌
                                                board_to_string(1); // 문자열로 변환 뒤 1번째 플레이어에게 보내줌
                                                bingo_result = check_bingo(); // check_bingo에서 나온 결과를 bingo_result 변수에 저장

                                                switch(bingo_result) {
                                                        case 1:
                                                                send(clisock_list[0], WINNER, strlen(WINNER), 0);
                                                                send(clisock_list[1], LOSER, strlen(LOSER), 0);

                                                                gs = 0;
                                                                break;
                                                        case 2:
                                                                send(clisock_list[0], LOSER, strlen(LOSER), 0);
                                                                send(clisock_list[1], WINNER, strlen(WINNER), 0);

                                                                gs = 0;
                                                                break;
                                                        case 3:
                                                                send(clisock_list[0], DRAW, strlen(DRAW), 0);
                                                                send(clisock_list[1], DRAW, strlen(DRAW), 0);

                                                                gs = 0;
                                                                break;
                                                }
                                                        if(bingo_result!=0) {
                                                                removeClient(0);
                                                                removeClient(0);
                                                        }
                                                } else {
                                                        // 다른 번호를 입력했을 때에 등장할 예정
                                                        send(clisock_list[user_turn % 2], WRONG_NUMBER, strlen(WRONG_NUMBER), 0);
                                                        board_to_string(user_turn % 2);
                                        }
                                } else {
                                        send(clisock_list[(user_turn + 1) % 2], NOT_YOUR_TURN, strlen(NOT_YOUR_TURN), 0);   // 자기 턴 아닌사람한테
                                }
                        }
                }
        }
}
int check_bingo() {
        int i, j, win[PEOPLE]={0,0};
        int count;

        for(j = 0; j < PEOPLE; j++) {
                count=0;
                for(i = 0; i < BOARD_SIZE; i++) // 가로, 세로
                {
                        if(board[j][i][0]==0 && board[j][i][1]==0 && board[j][i][2]==0 && board[j][i][3]==0 && board[j][i][4]==0) //가로 검사
                                count++;
                        if(board[j][0][i]==0 && board[j][1][i]==0 && board[j][2][i]==0 && board[j][3][i]==0 && board[j][4][i]==0) //세로 감사
                                count++;
                }

                // 대각선 검사
                if(board[j][0][0]==0 && board[j][1][1]==0 && board[j][2][2]==0 && board[j][3][3]==0 && board[j][4][4]==0)
                        count++;
                if(board[j][0][4]==0 && board[j][1][3]==0 && board[j][2][2]==0 && board[j][3][1]==0 && board[j][4][0]==0)
                        count++;
                if(count==WIN) {
                        win[j]=1;
                }
        }

        if(win[0]&&win[1]) { // 비김
                return 3;
        } else if(win[0]) { // 이겼음
                return 1;
        } else if(win[1]) { // 졌음
                return 2;
        } else {
                return 0;
        }
}

int remove_from_board(int turn, char num[]) { // 입력받은 숫자랑 비교해 유효하면 0으로 바꿔 넣는 기능
    int number = atoi(num);
    int inx;
    for(inx = 0;inx < BOARD_SIZE*BOARD_SIZE;inx++){  // compare
        if(board[turn][0][inx] == number){
            board[turn][0][inx] = 0;
            break;
        }
    }
    return inx==25||number<1? 0:1;
}

void game_set(){ // 보드에 데이터를 넣고 해당 보드의 데이터를 문자열로 변환해서 보내주는 기능
    int a;
    for(a = 0; a < num_chat ; a++){
            make_board(a);
            board_to_string(a);
            puts("");
    }
    user_turn = 0;
}

void addClient(int s, struct sockaddr_in *newcliaddr) { // 연결 소켓 하나 추가하는 기능
        char buf[20];
        inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
        printf("New Client : %s\n", buf);

        clisock_list[num_chat] = s;
        num_chat++;
}

void removeClient(int s) { // 연결 중인 소켓들 중 건네받은 소켓번호 소켓 제거
        send(clisock_list[s],EXIT_STRING,strlen(EXIT_STRING),0);
        close(clisock_list[s]);
        if(s != num_chat - 1)
                clisock_list[s] = clisock_list[num_chat - 1];
        num_chat--;
        printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 : %d\n", num_chat);
}

int is_nonblock(int sockfd) {
        int val;
        // 기존의 플래그 값을 얻어온다.
        val = fcntl(sockfd, F_GETFL, 0);
        // 넌블록 모드인지 확인
        if(val & O_NONBLOCK)
                return 0;
        return -1;
}

int set_nonblock(int sockfd) {
        int val;
        val = fcntl(sockfd, F_GETFL, 0);
        if(fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1)
                return -1;
        return 0;
}

int getmax() {
        int max = listen_sock;
        int i;

        for(i = 0;i < num_chat;i++)
                if(clisock_list[i] > max)
                        max = clisock_list[i];

        return max;
}

int tcp_listen(int host, int port, int backlog) {
        int sd;
        struct sockaddr_in servaddr;

        sd = socket(AF_INET, SOCK_STREAM, 0);
        if(sd == -1) {
                perror("socket fail");
                exit(1);
        }

        bzero((char *)&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(host);
        servaddr.sin_port = htons(port);

        if(bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("bind fail");
                exit(1);
        }

        listen(sd, backlog);
        return sd;
}

void make_board(int user_id) { // 보드에 데이터를 집어넣는 기능
        int i, j, temp;
        int rd;
        srand(time(NULL));
        for(i =0; i< BOARD_SIZE*BOARD_SIZE ; i++){
                board[user_id][0][i] = i+1;
        }

        for(i=0; i<1000; i++){
                rd = (rand()+(user_id+73))%25;
                temp =board[user_id][0][0];
                board[user_id][0][0] = board[user_id][0][rd];
                board[user_id][0][rd] = temp;
        }
}

int board_to_string(int cli_id) { // 보드의 데이터를 문자열로 변환해서 보내는 기능
        int i = 0;
        char temp[10];
        char msg[MAXLINE] = "";
        for(;i < BOARD_SIZE*BOARD_SIZE;i++) {
                sprintf(temp, "%d", board[cli_id][0][i]);
                strcat(msg, temp);
                if(i != (BOARD_SIZE * BOARD_SIZE)-1) {
                        strcat(msg, ",");
                }
        }

        printf("%s", msg);
        send(clisock_list[cli_id], msg, strlen(msg), 0);
	return 1;
}
