#include<stdio.h>
#include"device.h"

int init(unsigned int Mbyte) {	//�÷��� �޸� ���� ���� �Լ� 
						//������ megabytes
	FILE *fp = NULL;
	unsigned int byte;
	unsigned int count;
	unsigned short map_init;
	char initmemory = ' ';
	if (Mbyte > 31) {
		printf(">�ִ� 31MB���� �����մϴ�. �ٽ� init�� �ּ��� \n");	// 2byte�� ���̺��� �����ϱ� ������ 31+ spare������ ���� �����ϴ�.
		return 1;
	}
	/*####################### �÷��� �޸� �ʱ�ȭ###############*/			
	fp = fopen("FlashMemory.txt", "wb+");
	if (fp == NULL) {
		printf("Fail to creat Memory\n");
		return 1;
	}
	byte = Mbyte * 1024 * 1024;
	//���� ����
	for (count = 0; count < byte / 512; count++) {
		fwrite(&initmemory, sizeof(char), 1, fp);
	}
	//spare ���� ����
	for (count = 0; count < BtoS; count++) {
		fwrite(&initmemory, sizeof(char), 1, fp);
	}
	fclose(fp);
	printf("%d megabytes virtual flash memory are created\n", Mbyte);
	/*#########################################################*/

	/*####################### ���� ���̺� ���� ################
	  2byte : ���ͼ�   2byte : ��ȿ���� ����  2byte*����ũ�� */
	fp = fopen("Sectormap.txt", "wb+");
	// ���ͼ� ����
	map_init = byte / 512;
	fwrite(&map_init, sizeof(map_init), 1, fp);
	// ��ȿ ���ͼ� ����
	map_init = 0;
	fwrite(&map_init, sizeof(map_init), 1, fp);
	// ���� ���̺� ����
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
	if (avail != ' ') {	//�����Ͱ� �����ϸ�
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