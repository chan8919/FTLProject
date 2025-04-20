#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"device.h"

int main() {
	char input[15];
	char data;
	char command[5];
	unsigned int LSN;
	char PSNc[11];
	char *token;
	char output;
	unsigned short *maptable;
	Counter counter;
	FTL_INFO info;
	upload_table(&maptable, &info);
	while (1) {
		
		printf(">\n");
		printf(">Number of Sector : %hd		Number of ableSector : %hd \n", info.Number_of_Sector, info.Number_of_avail);
		printf(">\n");
		printf(">\t<Flash Memory Operating System>");
		printf("> This program use SectorMapping System\n");
		printf(">\n");
		printf("> Sector's data type is character \n");
		printf("> Block consist of 32 sectors\n");
		printf("> If want Exit the Program, commanding \"end\"\n");
		printf("> Option type [ Write data ] : w (LSN) (data)                    ex) w 4 a\n");
		printf(">                              \n");
		printf("> Option type [ read data ] : w (PSN)                            ex) r 4\n");
		printf(">                           \n");
		printf("> Option type [ initializing Flash Memory ] : init (megabyte)    ex) init 4 \n");
		printf(">                                            \n");
		printf("> Option type [ Show table ] : t (Block number)                  ex) t\n");
		printf(">                              \n");
		printf(">> ");
		gets(input);
		//��ɾ� ��ū �и�
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
		LSN = atoi(PSNc);
		if (strcmp(command, "init") == 0) {
			init(LSN);

			upload_table(&maptable, &info);
		}
		else if (strcmp(command, "w") == 0) {
			if (FTL_write(maptable, &info, LSN, data, &counter) == 0) {
				upload_table(&maptable, &info);
				printf("> ������ ���⿡ �����߽��ϴ�\n");
				printf("���� : %hd ȸ,  ����� : %hdȸ, ������ ��� :%d �� ���\n", counter.Wcounter, counter.Ecounter, counter.PBN);
			}
			else {
				printf("> ������ ���⿡ �����߽��ϴ�\n");
			}
		}
		else if (strcmp(command, "r") == 0) {
			output = FTL_read(maptable,&info,LSN);
			if (output == NULL) {
				printf(">    ������ �б⿡ �����߽��ϴ�.");
			}
			else {
				printf(">   %d �ּҿ� ����� �� : %c\n", LSN, output);
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
