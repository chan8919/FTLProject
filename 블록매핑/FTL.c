#include"device.h"
#include<stdio.h>
// 매핑테이블은 테이블 파일의 2번지부터 시작 : 0~1 블록수 

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
	// 매핑테이블에 해당 LBN이 존재하는지 탐색
	// 매핑테이블에서는 사용하는 마지막 블록+1의 주소를 경계값으로 사용한다.
	if (maptable[LSN / BtoS] != info->Number_of_Block) {	// 해당 블록에 대한 테이블이 존재하면
		if (Flash_write(maptable[LSN / BtoS]*BtoS + LSN % BtoS, data) == 0)	// 데이터가 정상적으로 입력되었으면
		{
			counter->Wcounter++;
			printf("> 데이터가 정상적으로 입력되었습니다.");
			return 0;
		}
		else // 해당 자리에 이미 데이터가 들어있으면
		{
			//spare영역에 새로운 데이터 입력
			Flash_write(info->Number_of_Block*BtoS + LSN % BtoS,data);
			counter->Wcounter++;
			//spare영역에 기존 데이터 입력
			for (count = 0; count < BtoS; count++) {
				tmpdata = Flash_read(maptable[LSN / BtoS] + count);
				if (tmpdata != ' ') {
					Flash_write(info->Number_of_Block*BtoS + count, tmpdata);
					counter->Wcounter++;
				}
			}
			//기존 block 초기화
			Flash_erase(maptable[LSN / BtoS]);
			counter->PBN = maptable[LSN / BtoS];
			counter->Ecounter++;
			//spare에 있던 데이터를 다시 PBN으로 이동
			for (count = 0; count < BtoS; count++) {
				tmpdata = Flash_read(info->Number_of_Block*BtoS + count);
				if (tmpdata != ' ') {
					Flash_write(maptable[LSN / BtoS]*BtoS + count, tmpdata);
					counter->Wcounter++;
				}
			}
			Flash_erase(info->Number_of_Block);
			counter->Ecounter++;
			printf(">  성공적으로 데이터를 입력했습니다.\n");
			return 0;
		}

	}
	else {			//해당 블록에 대한 테이블이 존재하지 않으면
		// 매핑테이블에 할당되지 않은 PBN 검색
		for (count_PBN = 0; count_PBN < info->Number_of_Block; count_PBN++) {
			for (count = 0; count < info->Number_of_Block; count++) {
				if (maptable[count] == count_PBN) {
					break;		//해당 PBN은 매핑테이블에 존재하므로 다음으로 이동
				}
			}
			if (count == info->Number_of_Block) {	// count_PBN에 해당하는 PBN이 매핑테이블에 존재하지 않으면
				if (Flash_write(count_PBN*BtoS+LSN%BtoS, data) == 0) {	// 해당 위치에 데이터 입력. overwrite에러가 나지 않으면 종료
					counter->Wcounter++;
					maptable[LSN/BtoS] = count_PBN;
					update_table(maptable, info);
					return 0;
				}
			}	//입력에 실패하면 다음 빈 블록 검색
		}
		// 빈 블록을 찾지 못했으면 victim블록을 선정
		for (count_PBN = 0; count_PBN < info->Number_of_Block; count_PBN++) {
			for (count = 0; count < info->Number_of_Block; count++) {
				if (maptable[count] == count_PBN) {
					break;		//해당 PBN은 매핑테이블에 존재하므로 다음으로 이동
				}
			}
			if(count == info->Number_of_Block) {	// count_PBN에 해당하는 PBN이 매핑테이블에 존재하지 않으면
				//기존 block 초기화
				Flash_erase(count_PBN);
				counter->PBN = count_PBN;
				counter->Ecounter++;
				Flash_write(count_PBN*BtoS+LSN%BtoS, data);
				counter->Wcounter++;
				maptable[LSN / BtoS] = count_PBN;
				update_table(maptable, info);
				printf(">  성공적으로 데이터를 입력했습니다.\n");
				return 0;
			}	//입력
		}
	}
	printf(">쓰기가 정상적으로 작동되지 못했습니다\n");
	return 1;

}
char FTL_read(unsigned short *maptable, FTL_INFO *info, unsigned short LSN) {
	if (LSN < 0 || LSN > info->Number_of_Block*BtoS) {
		printf("> 해당 영역은 유효하지 않습니다\n");
		return NULL;
	}
	printf(">%hd번 PSN에 저장되어있습니다. \n", (maptable[LSN / BtoS]*BtoS + LSN % BtoS));
	return Flash_read(maptable[LSN / BtoS]*BtoS+LSN%BtoS);
}
void print_table(unsigned short *maptable, FTL_INFO *info) {
	int count;
	printf("----------------매핑 테이블-----------------------\n");
	printf("LBN		PBN		LBN		PBN		LBN		PBN\n");
	for (count = 0; count < info->Number_of_Block; count++) {
		if (count % 3 == 0)
			printf("\n");
		printf("%d		%hd		", count, maptable[count]);
	}
	printf("--------------------------------------------------\n");
}