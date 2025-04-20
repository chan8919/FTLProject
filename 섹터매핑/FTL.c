#include"device.h"
#include<stdio.h>
// 매핑테이블은 테이블 파일의 4번지부터 시작 : 0~1 섹터수 2~3 유효섹터수

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
	// LSN이 영역 외의 구역을 침범할때 종료 ################
	if (LSN >= info->Number_of_Sector || LSN < 0) {
		printf("> 해당 섹터는 존재하지 않습니다.");
		return 1;
	}
	//######################################################
	// 유효 공간이 없을 때 실행#################################################################################
	if (info->Number_of_avail == info->Number_of_Sector) {
		victim = maptable[LSN];
		// 희생블록을 spare로 이동
		for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
			for (count = 0; count < info->Number_of_Sector; count++) {
				if (maptable[count] == existPSN) { // 해당 섹터가 유효한것을 확인
					tmpdata = Flash_read(existPSN);
					Flash_write(info->Number_of_Sector + existPSN - (unsigned short)(victim / BtoS)*BtoS, tmpdata);//existPSN-(unsigned short)(victim / BtoS)*BtoS : offset
					// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
					counter->Wcounter++;
				}
			}
		}
		// 희생블록 초기화
		Flash_erase((unsigned short)(victim / BtoS)*BtoS);
		// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Ecounter++;
		counter->PBN = (victim / BtoS)*BtoS;
		// spare에서 희생된 블록으로 다시 이동
		for (existPSN = info->Number_of_Sector; existPSN < info->Number_of_Sector + BtoS; existPSN++) {
			tmpdata = Flash_read(existPSN);
			if (tmpdata != ' ') {// 데이터가 존재하면
				Flash_write((unsigned short)(victim / BtoS)*BtoS + existPSN - info->Number_of_Sector, tmpdata);
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
			}
		}
		Flash_erase(info->Number_of_Sector / 32);
		// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Ecounter++;
		// 데이터 입력
		for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
			if (Flash_write(existPSN, data) == 0) {//정상적으로 입력됬으면
				
				if (maptable[LSN] == info->Number_of_Sector)
					info->Number_of_avail++;
				maptable[LSN] = existPSN;		//매핑테이블 변경
				update_table(maptable, info);	//매핑테이블 업데이트
				// test @@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
				return 0;
			}
		}
	}
	// ####################################################################################################
	for (existPSN = 0; existPSN < info->Number_of_Sector; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) {   //psn이 매핑테이블에 존재하면 다음psn으로 넘어감
				break;
			}
		}
		if (count == info->Number_of_Sector) {	//existPSN에 해당하는 PSN이 검색되지 않았을 때 실행
			count_avail++;
			if (Flash_write(existPSN, data) == 0) {	//빈 공간이 발견되어 정상적으로 저장되었으면
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
				if (maptable[LSN] == info->Number_of_Sector)
					info->Number_of_avail++;
				maptable[LSN] = existPSN;		//매핑테이블 변경
				update_table(maptable, info);	//매핑테이블 업데이트
				return 0;
			}
		}
		if (count_avail + info->Number_of_avail == info->Number_of_Sector) {	// 매핑테이블은 비어있으나 
			break;																// 쓸수 있는 공간이 없을때
		}
	}
	// 여기까지 넘어오면 공간 확보가 필요하다고 판단.
	// 희생블록 탐색
	for (existPSN = 0; existPSN < info->Number_of_Sector; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) {   //psn이 매핑테이블에 존재하면 다음 psn으로 패스
				break;
			}
		}
		if (count == info->Number_of_Sector) {	//비 유효 PSN을 찾으면
			victim = existPSN;
			break;
		}
	}
	// 희생블록을 spare로 이동
	for (existPSN = (unsigned short)(victim/BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS +BtoS; existPSN++) {
		for (count = 0; count < info->Number_of_Sector; count++) {
			if (maptable[count] == existPSN) { // 해당 섹터가 유효한것을 확인
				tmpdata = Flash_read(existPSN);
				Flash_write(info->Number_of_Sector+ existPSN-(unsigned short)(victim / BtoS)*BtoS, tmpdata);//existPSN-(unsigned short)(victim / BtoS)*BtoS : offset
				// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				counter->Wcounter++;
			} 
		}
	}
	// 희생블록 초기화
	Flash_erase((unsigned short)(victim / BtoS));
	// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	counter->Ecounter++;
	counter->PBN = (victim / BtoS)*BtoS;
	// spare에서 희생된 블록으로 다시 이동
	for (existPSN = info->Number_of_Sector; existPSN < info->Number_of_Sector+BtoS; existPSN++) {
		tmpdata = Flash_read(existPSN);
		if (tmpdata != ' ') {// 데이터가 존재하면
			Flash_write((unsigned short)(victim / BtoS)*BtoS + existPSN - info->Number_of_Sector, tmpdata);
			// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			counter->Wcounter++;
		}
	}
	Flash_erase(info->Number_of_Sector / 32);
	// test@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	counter->Ecounter++;
	// 데이터 입력
	for (existPSN = (unsigned short)(victim / BtoS)*BtoS; existPSN < (unsigned short)(victim / BtoS)*BtoS + BtoS; existPSN++) {
		if (Flash_write(existPSN, data) == 0) {//정상적으로 입력됬으면
		if(maptable[LSN]==info->Number_of_Sector)
			info->Number_of_avail++;
		maptable[LSN] = existPSN;		//매핑테이블 변경
		update_table(maptable, info);	//매핑테이블 업데이트
		// test @@@@@@@@@@@@@@@@@@@@@@@@@@
		counter->Wcounter++;
		return 0;
		}
		
	}
	printf("데이터가 입력되지 않았습니다\n");
	return 1;


}
char FTL_read(unsigned short *maptable, FTL_INFO *info, unsigned short LSN) {
	if (LSN < 0 || LSN >= info->Number_of_Sector||maptable[LSN]==info->Number_of_Sector) {
		printf("> 해당 영역은 유효하지 않습니다\n");
		return NULL;
	}
	printf(">%hd번 PSN에 저장되어있습니다. \n", maptable[LSN]);
	return Flash_read(maptable[LSN]);
}
void print_table(unsigned short *maptable, FTL_INFO *info) {
	int count;
	printf("----------------매핑 테이블-----------------------\n");
	printf("LSN		PSN		LSN		PSN		LSN		PSN\n");
	for (count = 0; count < info->Number_of_Sector; count++) {
		if (count % 3 == 0)
			printf("\n");
		printf("%d		%hd		",count,maptable[count]);
	}
	printf("--------------------------------------------------\n");
}