#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"device.h"

int main() {
	char input[15];
	char data;
	char command[5];
	unsigned int LBN;
	char PSNc[11];
	char *token;
	char output;
	unsigned short *maptable;
	Counter counter;
	FTL_INFO info;
	upload_table(&maptable, &info);
	while (1) {

		printf(">\n");
		printf(">Number of Sector : %hd	 \n", info.Number_of_Block);
		printf(">\n");
		printf(">\t<Flash Memory Operating System>");
		printf("> This program use BlockMapping System\n");
		printf("> Sector's data type is character \n");
		printf("> Block consist of 32 sectors\n");
		printf("> If want Exit the Program, commanding \"end\"\n");
		printf("> Option type [ Write data ] : w (LBN) (data)                    ex) w 4 a\n");
		printf(">                              \n");
		printf("> Option type [ read data ] : w (PSN)                            ex) r 4\n");
		printf(">                           \n");
		printf("> Option type [ initializing Flash Memory ] : init (megabyte)    ex) init 4 \n");
		printf(">                                            \n");
		printf("> Option type [ Show table ] : t (Block number)                  ex) t\n");
		printf(">                              \n");
		printf(">> ");
		gets(input);
		//명령어 토큰 분리
		token = strtok(input, " ");
		strcpy(command, token);
		token = strtok(NULL, " ");
		if (token) {
			strcpy(PSNc, token);
		}
		token = strtok(NULL, " ");
		if (token) {
			data = *token;
		}
		LBN = atoi(PSNc);
		if (strcmp(command, "init") == 0) {
			init(LBN);

			upload_table(&maptable, &info);
		}
		else if (strcmp(command, "w") == 0) {
			if (FTL_write(maptable, &info, LBN, data, &counter) == 0) {
				upload_table(&maptable, &info);
				printf("> 데이터 쓰기에 성공했습니다\n");
				printf("쓰기 : %hd 회,  지우기 : %hd회, 지워진 블록 :%d 번 블록\n", counter.Wcounter, counter.Ecounter, counter.PBN);
			}
			else {
				printf("> 데이터 쓰기에 실패했습니다\n");
			}
		}
		else if (strcmp(command, "r") == 0) {
			output = FTL_read(maptable, &info, LBN);
			if (output == NULL) {
				printf(">    데이터 읽기에 실패했습니다.");
			}
			else {
				printf(">   %d 주소에 저장된 값 : %c\n", LBN, output);
			}
		}
		else if (strcmp(command, "t") == 0) {
			print_table(maptable, &info);
		}
		else if (strcmp(command, "end") == 0) {
			break;
		}
	}
	system("pause");
	return 0;
}
