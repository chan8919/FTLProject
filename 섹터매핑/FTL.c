#include"device.h"
#include<stdio.h>
// �������̺��� ���̺� ������ 4�������� ���� : 0~1 ���ͼ� 2~3 ��ȿ���ͼ�

int upload_table(unsigned short **maptable,FTL_INFO *info) {
	FILE *fp;
	unsigned short count;
	unsigned short *newtable;
	fp = fopen("Sectormap.txt", "rb");
	if (fp == NULL) {
		printf("> can't open Sectormap please do INIT .txt");
		return 1;
	}
	fread(&info->Number_of_Sector, sizeof(info->Number_of_Sector), 1, fp);
	fread(&info->Number_of_avail, sizeof(info->Number_of_avail), 1, fp);
	newtable = (unsigned short*)malloc(sizeof(unsigned short)*(info->Number_of_Sector));
	for (count = 0; count < info->Number_of_Sector; count++) {
		fread(&newtable[count],sizeof(newtable[count]),1,fp);
	}
	*maptable = newtable;
	fclose(fp);
	printf("> successfully upload table\n");
	return 0;
}
int update_table(unsigned short *maptable,FTL_INFO *info) {
	FILE *fp;
	unsigned short count;
	fp = fopen("Sectormap.txt", "rb+");
	if (fp == NULL) {
		printf("> can't open Sectormap.txt");
		return 1;
	}
	fwrite(&info->Number_of_Sector, sizeof(info->Number_of_Sector), 1, fp);
	fwrite(&info->Number_of_avail, sizeof(info->Number_of_avail), 1, fp);
	for (count = 0; count < info->Number_of_Sector; count++) {
		fwrite(&maptable[count], sizeof(maptable[count]), 1, fp);
	}
	fclose(fp);
	printf("> successfully upDate table\n");
	return 0;

}
int FTL_write(unsigned short *maptable,FTL_INFO *info,unsigned short LSN,char data,Counter *counter) {
	unsigned short count;
	unsigned short existPSN;
	unsigned short count_avail=0;
	unsigned short victim;
	unsigned short testcounter=0;
	char tmpdata;
	counter->Ecounter = 0;
	counter->Wcounter = 0;
	counter->PBN = -1;
	// LSN�� ���� ���� ������ ħ���Ҷ� ���� ################
	if (LSN >= info->Number_of_Sector || LSN < 0) {
		printf("> �ش� ���ʹ� �������� �ʽ��ϴ�.");
		return 1;
	}
	//######################################################
	// ��ȿ ������ ���� �� ����#################################################################################
	if (info->Number_of_avail == info->Number_of_Sector) {
		victim = maptable[LSN];
		// �������� spare�� �̵�
		for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
			for (count = 0; count < info->Number_of_Sector; count++) {
				if (maptable[count] == existPSN) { // �ش� ���Ͱ� ��ȿ�Ѱ��� Ȯ��
					tmpdata = Flash_read(existPSN);
					Flash_write(info->Number_of_Sector + existPSN - (unsigned short)(victim / BtoS)*BtoS, tmpdata);//existPSN-(unsigned short)(victim / BtoS)*BtoS : offset
					// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
					counter->Wcounter++;
				}
			}
		}
		// ������ �ʱ�ȭ
		Flash_erase((unsigned short)(victim / BtoS)*BtoS);
		// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Ecounter++;
		counter->PBN = (victim / BtoS)*BtoS;
		// spare���� ����� ������� �ٽ� �̵�
		for (existPSN = info->Number_of_Sector; existPSN < info->Number_of_Sector + BtoS; existPSN++) {
			tmpdata = Flash_read(existPSN);
			if (tmpdata != ' ') {// �����Ͱ� �����ϸ�
				Flash_write((unsigned short)(victim / BtoS)*BtoS + existPSN - info->Number_of_Sector, tmpdata);
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
			}
		}
		Flash_erase(info->Number_of_Sector / 32);
		// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Ecounter++;
		// ������ �Է�
		for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
			if (Flash_write(existPSN, data) == 0) {//���������� �Է�����
				
				if (maptable[LSN] == info->Number_of_Sector)
					info->Number_of_avail++;
				maptable[LSN] = existPSN;		//�������̺� ����
				update_table(maptable, info);	//�������̺� ������Ʈ
				// test @@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
				return 0;
			}
		}
	}
	// ####################################################################################################
	for (existPSN = 0; existPSN < info->Number_of_Sector; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) {   //psn�� �������̺� �����ϸ� ����psn���� �Ѿ
				break;
			}
		}
		if (count == info->Number_of_Sector) {	//existPSN�� �ش��ϴ� PSN�� �˻����� �ʾ��� �� ����
			count_avail++;
			if (Flash_write(existPSN, data) == 0) {	//�� ������ �߰ߵǾ� ���������� ����Ǿ�����
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
				if (maptable[LSN] == info->Number_of_Sector)
					info->Number_of_avail++;
				maptable[LSN] = existPSN;		//�������̺� ����
				update_table(maptable, info);	//�������̺� ������Ʈ
				return 0;
			}
		}
		if (count_avail + info->Number_of_avail == info->Number_of_Sector) {	// �������̺��� ��������� 
			break;																// ���� �ִ� ������ ������
		}
	}
	// ������� �Ѿ���� ���� Ȯ���� �ʿ��ϴٰ� �Ǵ�.
	// ������ Ž��
	for (existPSN = 0; existPSN < info->Number_of_Sector; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) {   //psn�� �������̺� �����ϸ� ���� psn���� �н�
				break;
			}
		}
		if (count == info->Number_of_Sector) {	//�� ��ȿ PSN�� ã����
			victim = existPSN;
			break;
		}
	}
	// �������� spare�� �̵�
	for (existPSN = (unsigned short)(victim/BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS +BtoS; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) { // �ش� ���Ͱ� ��ȿ�Ѱ��� Ȯ��
				tmpdata = Flash_read(existPSN);
				Flash_write(info->Number_of_Sector+ existPSN-(unsigned short)(victim / BtoS)*BtoS, tmpdata);//existPSN-(unsigned short)(victim / BtoS)*BtoS : offset
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
			} 
		}
	}
	// ������ �ʱ�ȭ
	Flash_erase((unsigned short)(victim / BtoS));
	// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	counter->Ecounter++;
	counter->PBN = (victim / BtoS)*BtoS;
	// spare���� ����� ������� �ٽ� �̵�
	for (existPSN = info->Number_of_Sector; existPSN < info->Number_of_Sector+BtoS; existPSN++) {
		tmpdata = Flash_read(existPSN);
		if (tmpdata != ' ') {// �����Ͱ� �����ϸ�
			Flash_write((unsigned short)(victim / BtoS)*BtoS + existPSN - info->Number_of_Sector, tmpdata);
			// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			counter->Wcounter++;
		}
	}
	Flash_erase(info->Number_of_Sector / 32);
	// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	counter->Ecounter++;
	// ������ �Է�
	for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
		if (Flash_write(existPSN, data) == 0) {//���������� �Է�����
		if(maptable[LSN]==info->Number_of_Sector)
			info->Number_of_avail++;
		maptable[LSN] = existPSN;		//�������̺� ����
		update_table(maptable, info);	//�������̺� ������Ʈ
		// test @@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Wcounter++;
		return 0;
		}
		
	}
	printf("�����Ͱ� �Էµ��� �ʾҽ��ϴ�\n");
	return 1;


}
char FTL_read(unsigned short *maptable, FTL_INFO *info, unsigned short LSN) {
	if (LSN < 0 || LSN >= info->Number_of_Sector||maptable[LSN]==info->Number_of_Sector) {
		printf("> �ش� ������ ��ȿ���� �ʽ��ϴ�\n");
		return NULL;
	}
	printf(">%hd�� PSN�� ����Ǿ��ֽ��ϴ�. \n", maptable[LSN]);
	return Flash_read(maptable[LSN]);
}
void print_table(unsigned short *maptable, FTL_INFO *info) {
	int count;
	printf("----------------���� ���̺�-----------------------\n");
	printf("LSN		PSN		LSN		PSN		LSN		PSN\n");
	for (count = 0; count < info->Number_of_Sector; count++) {
		if (count % 3 == 0)
			printf("\n");
		printf("%d		%hd		",count,maptable[count]);
	}
	printf("--------------------------------------------------\n");
}