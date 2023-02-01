#pragma once
#include "stdint.h"
#include "iostream"
#include "string"
#include "hidapi.h"
#include <msclr\marshal_cppstd.h>

using namespace msclr::interop;
using namespace System;
using namespace std;

public ref class DFUCli
{
public:
	hid_device* handle;
	
	uint8_t* FileData;

	static uint32_t FileLen = 0;

	static int LoopNum = 0;
	static int Remain = 0;
	static char *SN;
	static uint8_t STATE;
	static char* HW;
	static char* FW;
	static uint8_t BV;
	static uint16_t LOG;
	static uint8_t* TIME;
	uint8_t checksum = 0;
	
	uint16_t mIndex;
	String^ openTime;
	String^ airTime;
	String^ inTime;
	String^ endTime;
	uint16_t mVolume;

	uint16_t mState;
	uint16_t* mState_bit;

	uint8_t mPower;
	uint8_t mAir;
	bool mAir_flag;
	uint8_t mERR;
	uint16_t* mERRlog;
	char* TDERev;

	DFUCli();
	int Openhid();
	void test();
	int Update(String^ filepath);

	int Update1(String^ filepath);
	int getLoopnum();
	int getLenth();
	int getRemain();
	int writeDate(int step);
	int writeRemain();

	int Update2();

	int enter_into_dfu(void);

	int check_bin_file(uint8_t* check, uint8_t len);

	int erase_flash_zone(void);

	int write_app_data(uint8_t* pdata, uint8_t len);
	int write_app_data(int x, uint8_t len);

	int verify_app(void);

	int exit_from_dfu(void);

	int getSn(void);

	String^ getsn();

	int getState(void);

	int getstate();

	int getHW(void);

	String^ gethw();

	int getFW(void);

	String^ getfw();

	int getBV(void);

	int getbv();

	int getLOG(void);

	int getlog();

	int getAT(void);

	String^ getat();

	int getInf(uint16_t index);

	bool get_state_bit(int bit);

	bool get_ERRlog_bit(int logindex, int bit);

	void dataInit();

	String^ Send_TDE(String^ cmd);

	String^ Send_TDE_GetTime();

	int clearLogs(void);
	
	
};

