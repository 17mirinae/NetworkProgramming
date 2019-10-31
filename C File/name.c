#include <stdio.h>
#include <stdlib.h>

int main() {
	int i;
	char major[10000];
	int year;
	char schoolid[1000];
	char name[10000];

	printf("학과는? ");
	scanf("%s", major);

	printf("학년은? ");
	scanf("%d", &year);

	printf("학번은? ");
	scanf("%s", schoolid);

	printf("이름은? ");
	scanf("%s", name);

	for(i = 0;i < 5;i++) {
		printf("%s, %d, %s, %s\n", major, year, schoolid, name);
	}

	return 0;
}
