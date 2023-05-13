#pragma once
#include "stdint.h"
#include "iostream"
#include "string"
#include "hidapi.h"
#include <msclr\marshal_cppstd.h>
#include <iomanip>
#include <sstream>
#include <locale>
#include <codecvt>

using namespace msclr::interop;
using namespace System;
using namespace std;

public ref class DFUCli
{
public:
	hid_device* handle;

	DFUCli();
	int Openhid();
	void Closehid();
	int Openhid(uint16_t Pid, uint16_t Vid);

	String^ Getmanu();

	String^ Getname();

	String^ Send_TDE(String^ cmd);

	int SendMsg(uint8_t id, String^ cmd);

	int SendMsgbyte(uint8_t id, String^ cmd);

	String^ RevMsg();

	String^ RevMsgbyte();

	String^ Send_TDE_GetTime();

	int Int_ConvertTo_Hex(int digt);
	
};

