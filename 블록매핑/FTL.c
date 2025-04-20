#include"device.h"
#include<stdio.h>
// �������̺��� ���̺� ������ 2�������� ���� : 0~1 ��ϼ� 

int upload_table(unsigned short **maptable, FTL_INFO *info) {
	FILE *fp;
	unsigned short count;
	unsigned short *newtable;
	fp = fopen("Blockmap.txt", "rb");
	if (fp == NULL) {
		printf("> can't open Blockmap please do INIT .txt");
		return 1;
	}
	fread(&info->Number_of_Block, sizeof(info->Number_of_Block), 1, fp);
	newtable = (unsigned short*)malloc(sizeof(unsigned short)*(info->Number_of_Block));
	for (count = 0; count < info->Number_of_Block; count++) {
		fread(&newtable[count], sizeof(newtable[count]), 1, fp);
	}
	*maptable = newtable;
	fclose(fp);
	printf("> successfully upload table\n");
	return 0;
}
int update_table(unsigned short *maptable, FTL_INFO *info) {
	FILE *fp;
	unsigned short count;
	fp = fopen("Blockmap.txt", "rb+");
	if (fp == NULL) {
		printf("> can't open Blockmap.txt");
		return 1;
	}
	fwrite(&info->Number_of_Block, sizeof(info->Number_of_Block), 1, fp);
	for (count = 0; count < info->Number_of_Block; count++) {
		fwrite(&maptable[count], sizeof(maptable[count]), 1, fp);
	}
	fclose(fp);
	printf("> successfully upDate table\n");
	return 0;

}
int FTL_write(unsigned short *maptable, FTL_INFO *info, unsigned short LSN, char data, Counter *counter) {
	unsigned short count;
	unsigned short victim;
	unsigned short count_PBN;
	unsigned short testcounter = 0;
	char tmpdata;
	counter->Ecounter = 0;
	counter->Wcounter = 0;
	counter->PBN = -1;
	// �������̺� �ش� LBN�� �����ϴ��� Ž��
	// �������̺����� ����ϴ� ������ ���+1�� �ּҸ� ��谪���� ����Ѵ�.
	if (maptable[LSN / BtoS] != info->Number_of_Block) {	// �ش� ��Ͽ� ���� ���̺��� �����ϸ�
		if (Flash_write(maptable[LSN / BtoS]*BtoS + LSN % BtoS, data) == 0)	// �����Ͱ� ���������� �ԷµǾ�����
		{
			counter->Wcounter++;
			printf("> �����Ͱ� ���������� �ԷµǾ����ϴ�.");
			return 0;
		}
		else // �ش� �ڸ��� �̹� �����Ͱ� ���������
		{
			//spare������ ���ο� ������ �Է�
			Flash_write(info->Number_of_Block*BtoS + LSN % BtoS,data);
			counter->Wcounter++;
			//spare������ ���� ������ �Է�
			for (count = 0; count < BtoS; count++) {
				tmpdata = Flash_read(maptable[LSN / BtoS] + count);
				if (tmpdata != ' ') {
					Flash_write(info->Number_of_Block*BtoS + count, tmpdata);
					counter->Wcounter++;
				}
			}
			//���� block �ʱ�ȭ
			Flash_erase(maptable[LSN / BtoS]);
			counter->PBN = maptable[LSN / BtoS];
			counter->Ecounter++;
			//spare�� �ִ� �����͸� �ٽ� PBN���� �̵�
			for (count = 0; count < BtoS; count++) {
				tmpdata = Flash_read(info->Number_of_Block*BtoS + count);
				if (tmpdata != ' ') {
					Flash_write(maptable[LSN / BtoS]*BtoS + count, tmpdata);
					counter->Wcounter++;
				}
			}
			Flash_erase(info->Number_of_Block);
			counter->Ecounter++;
			printf(">  ���������� �����͸� �Է��߽��ϴ�.\n");
			return 0;
		}

	}
	else {			//�ش� ��Ͽ� ���� ���̺��� �������� ������
		// �������̺� �Ҵ���� ���� PBN �˻�
		for (count_PBN = 0; count_PBN < info->Number_of_Block; count_PBN++) {
			for (count = 0; count < info->Number_of_Block; count++) {
				if (maptable[count] == count_PBN) {
					break;		//�ش� PBN�� �������̺� �����ϹǷ� �������� �̵�
				}
			}
			if (count == info->Number_of_Block) {	// count_PBN�� �ش��ϴ� PBN�� �������̺� �������� ������
				if (Flash_write(count_PBN*BtoS+LSN%BtoS, data) == 0) {	// �ش� ��ġ�� ������ �Է�. overwrite������ ���� ������ ����
					counter->Wcounter++;
					maptable[LSN/BtoS] = count_PBN;
					update_table(maptable, info);
					return 0;
				}
			}	//�Է¿� �����ϸ� ���� �� ��� �˻�
		}
		// �� ����� ã�� �������� victim����� ����
		for (count_PBN = 0; count_PBN < info->Number_of_Block; count_PBN++) {
			for (count = 0; count < info->Number_of_Block; count++) {
				if (maptable[count] == count_PBN) {
					break;		//�ش� PBN�� �������̺� �����ϹǷ� �������� �̵�
				}
			}
			if(count == info->Number_of_Block) {	// count_PBN�� �ش��ϴ� PBN�� �������̺� �������� ������
				//���� block �ʱ�ȭ
				Flash_erase(count_PBN);
				counter->PBN = count_PBN;
				counter->Ecounter++;
				Flash_write(count_PBN*BtoS+LSN%BtoS, data);
				counter->Wcounter++;
				maptable[LSN / BtoS] = count_PBN;
				update_table(maptable, info);
				printf(">  ���������� �����͸� �Է��߽��ϴ�.\n");
				return 0;
			}	//�Է�
		}
	}
	printf(">���Ⱑ ���������� �۵����� ���߽��ϴ�\n");
	return 1;

}
char FTL_read(unsigned short *maptable, FTL_INFO *info, unsigned short LSN) {
	if (LSN < 0 || LSN > info->Number_of_Block*BtoS) {
		printf("> �ش� ������ ��ȿ���� �ʽ��ϴ�\n");
		return NULL;
	}
	printf(">%hd�� PSN�� ����Ǿ��ֽ��ϴ�. \n", (maptable[LSN / BtoS]*BtoS + LSN % BtoS));
	return Flash_read(maptable[LSN / BtoS]*BtoS+LSN%BtoS);
}
void print_table(unsigned short *maptable, FTL_INFO *info) {
	int count;
	printf("----------------���� ���̺�-----------------------\n");
	printf("LBN		PBN		LBN		PBN		LBN		PBN\n");
	for (count = 0; count < info->Number_of_Block; count++) {
		if (count % 3 == 0)
			printf("\n");
		printf("%d		%hd		", count, maptable[count]);
	}
	printf("--------------------------------------------------\n");
}