#include<stdio.h>
#include"device.h"

int init(unsigned int Mbyte) {	//플레시 메모리 공간 생성 함수 
						//단위는 megabytes
	FILE *fp = NULL;
	unsigned int byte;
	unsigned int count;
	unsigned short map_init;
	char initmemory = ' ';
	if (Mbyte > 31) {
		printf(">최대 31MB까지 가능합니다. 다시 init해 주세요 \n");	// 2byte로 테이블을 관리하기 때문에 31+ spare영역을 관리 가능하다.
		return 1;
	}
	/*####################### 플래시 메모리 초기화###############*/			
	fp = fopen("FlashMemory.txt", "wb+");
	if (fp == NULL) {
		printf("Fail to creat Memory\n");
		return 1;
	}
	byte = Mbyte * 1024 * 1024;
	//섹터 생성
	for (count = 0; count < byte / 512; count++) {
		fwrite(&initmemory, sizeof(char), 1, fp);
	}
	//spare 섹터 생성
	for (count = 0; count < BtoS; count++) {
		fwrite(&initmemory, sizeof(char), 1, fp);
	}
	fclose(fp);
	printf("%d megabytes virtual flash memory are created\n", Mbyte);
	/*#########################################################*/

	/*####################### 매핑 테이블 생성 ################
	  2byte : 섹터수   2byte : 유효섹터 갯수  2byte*섹터크기 */
	fp = fopen("Sectormap.txt", "wb+");
	// 섹터수 설정
	map_init = byte / 512;
	fwrite(&map_init, sizeof(map_init), 1, fp);
	// 유효 섹터수 설정
	map_init = 0;
	fwrite(&map_init, sizeof(map_init), 1, fp);
	// 매핑 테이블 설정
	map_init = byte / 512;
	for (count = 0; count < byte / 512; count++) {
		fwrite(&map_init, sizeof(map_init), 1, fp);
	}
	fclose(fp);
	return 0;
}
char Flash_read(unsigned short PSN) {
	FILE *fp = NULL;
	char data;
	fp = fopen("FlashMemory.txt", "rb");
	if (fp == NULL) {
		printf(">Fail to open Memory for read\n");
		return NULL;
	}
	fseek(fp, sizeof(data)*PSN, SEEK_SET);
	fread(&data, sizeof(data), 1, fp);
	fclose(fp);
	return data;

}
int Flash_write(unsigned short PSN, char data) {
	FILE *fp = NULL;
	char avail;
	fp = fopen("FlashMemory.txt", "rb+");
	if (fp == NULL) {
		printf(">Fail to open Memory for write\n");
		return 1;
	}
	fseek(fp, sizeof(data)*(PSN), SEEK_SET);
	fread(&avail, sizeof(data), 1, fp);
	if (avail != ' ') {	//데이터가 존재하면
		fclose(fp);
		return 1;	//overwrite
	}
	fseek(fp, -sizeof(data), SEEK_CUR);
	fwrite(&data, sizeof(data), 1, fp);
	fclose(fp);
	return 0;

}
int Flash_erase(unsigned short PSN) {
	FILE *fp = NULL;
	fp = fopen("FlashMemory.txt", "rb+");
	char eraser = ' ';
	int count;
	if (fp == NULL) {
		printf(">Fail to open Memory for erase\n");
		return 1;
	}
	fseek(fp, sizeof(eraser)*BtoS*PSN, SEEK_SET);
	for (count = 0; count < BtoS; count++) {
		fwrite(&eraser, sizeof(eraser), 1, fp);
	}
	printf(">successfully erase\n");
	fclose(fp);
	return 0;
}