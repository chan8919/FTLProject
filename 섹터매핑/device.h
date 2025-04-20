#pragma once
typedef struct FTL_INFO {
	unsigned short Number_of_Sector;
	unsigned short Number_of_avail;
}FTL_INFO;
typedef struct Counter {
	unsigned int Wcounter;
	unsigned int Ecounter;
	int PBN;
}Counter;
#define BtoS 32 // Block to Sector 1블록은 32섹터
int init(unsigned int Mbyte);
char Flash_read(unsigned short PSN);
int Flash_write(unsigned short PSN, char data);
int Flash_erase(unsigned short PSN);

void print_table(unsigned short *maptable, FTL_INFO *info);

int upload_table(unsigned short **maptable, FTL_INFO *info);
int update_table(unsigned short *maptable, FTL_INFO *info);
int FTL_write(unsigned short *maptable, FTL_INFO *info, unsigned short LSN, char data,Counter *counter);
char FTL_read(unsigned short *maptable, FTL_INFO *info, unsigned short LSN);