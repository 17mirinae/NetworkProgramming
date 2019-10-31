// 파일명: fork_test.c
// fork() 시스템 콜 사용 예시
// 사용법: fork_test

#include <sys/types.h>
#include <unistd.h>
int global_var = 0;

int main(void) {
	pid_t pid;
	int local_var = 0;
	if((pid = fork()) < 0) {
		printf("fork error\n");
		exit(0);
	// 자식 프로세스
	} else if(pid == 0) {
		global_var++;
		local_var++;
		printf("CHILD - my pid is %d and parent's pid is %d\n", getpid(), getppid());
	} else { // 부모 프로세스
		sleep(2);
		global_var += 5;
		local_var += 5;
		printf("PARENT - my pid is %d, child's pid is %d\n", getpid(), pid);
	}

	printf("\tglobal var : %d\n", global_var);
	printf("\tlocal var : %d\n", local_var);
}		
