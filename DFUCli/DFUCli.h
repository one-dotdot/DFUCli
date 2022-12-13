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
	
	
};

