#include "DFUCli.h"

#define OK	0
#define ERRO_DR	-1
#define ERRO_DW	-2
#define ERRO_XY	-3
#define ERRO_FW	-4
#define ERRO_FV	-5
#define ERRO_FP	-6
#define ERRO_MM	-7
#define ERRO_FR	-8

#define Bit_open	0
#define Bit_air		4
#define Bit_star	6
#define Bit_end		7

DFUCli::DFUCli()
{
}

int DFUCli::Openhid()
{
	uint16_t pid = 0x8800;
	uint16_t vid = 0x248A;
	
	handle = hid_open(vid, pid, NULL);
	if (handle == NULL) {
		return -1;
	}

	return 0;
}

void DFUCli::Closehid()
{
	hid_close(handle);
	handle = nullptr;//这里将指针置空，增强安全性
}

int DFUCli::Openhid(uint16_t Pid, uint16_t Vid)
{
	uint16_t pid = Pid;
	uint16_t vid = Vid;

	handle = hid_open(vid, pid, NULL);
	if (handle == NULL) {
		return -1;
	}

	// 将hid_read()函数设置为非阻塞。
	if (hid_set_nonblocking(handle, 1) != 0)// 1启用非阻塞  0禁用非阻塞。
	{
		hid_close(handle);
		hid_exit();
		return -1;
	}

	return 0;
}

String^ DFUCli::Getmanu()
{
	wchar_t wstr[255];
	hid_get_manufacturer_string(handle, wstr, 255);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = wstr;
	std::string narrow = converter.to_bytes(wide);
	String^ stdToCli = marshal_as<String^>(narrow);
	return stdToCli;
}

String^ DFUCli::Getname()
{
	wchar_t wstr[255];
	hid_get_product_string(handle, wstr, 255);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = wstr;
	std::string narrow = converter.to_bytes(wide);
	String^ stdToCli = marshal_as<String^>(narrow);
	return stdToCli;
}

void ConvertToString(System::String^ str, std::string& text)
{
	char* p = (char*)(int)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str);
	text = p;
	System::Runtime::InteropServices::Marshal::FreeHGlobal((System::IntPtr)p);
}

int bcd_decimal_code(int bcd)
{
	int sum = 0, c = 1;  // sum返回十进制，c每次翻10倍

	for (int i = 1; bcd > 0; i++)
	{
		if (i >= 2)
		{
			c *= 10;
		}

		sum += (bcd % 16) * c;

		bcd /= 16;  // 除以16同理与十进制除10将小数点左移一次，取余16也同理
	}

	return sum;
}

string getweek(uint8_t index)
{
	string weekday;
	switch (index)
	{
	case 0:
		weekday = "SUN";
		break;
	case 1:
		weekday = "MON";
		break;
	case 2:
		weekday = "TUE";
		break;
	case 3:
		weekday = "WEN";
		break;
	case 4:
		weekday = "THU";
		break;
	case 5:
		weekday = "FRI";
		break;
	case 6:
		weekday = "SAT";
		break;
	default:
		weekday = "NULL";
		break;
	}
	return weekday;
}


String^ ChartoString(char* str)
{
	String^ stdToCli = marshal_as<String^>(str);
	return stdToCli;
}



int BIT(int n)
{
	return (1 << n);
}


String^ DFUCli::Send_TDE(String^ cmd)
{
	int retval;
	unsigned char rev[64];
	std::string Cmd = marshal_as<std::string>(cmd);
	char send[64];

	strcpy(send, Cmd.c_str());

	retval = hid_write(handle, (unsigned char*)send, Cmd.size());
	if (retval < 0) {
		return  "写入错误";
	}
	retval = hid_read(handle, rev, 64);
	if (retval < 0) {
		return  "读取错误";
	}
	else
	{
		string str;
		str = (char*)rev;
		String^ stdToCli = marshal_as<String^>(str);
		return stdToCli;
	}
}

int DFUCli::SendMsg(uint8_t id, String^ cmd)
{
	int retval;
	std::string Cmd = (char)id + marshal_as<std::string>(cmd);
	//char send[65];
	//send[0] = id;
	//strcpy(&send[1], Cmd.c_str());

	unsigned char* send = (unsigned char*)Cmd.c_str();

	retval = hid_write(handle, (unsigned char*)send, Cmd.size() + 1 );
	if (retval < 0) {
		return  -1;
	}
}

int DFUCli::SendMsgbyte(uint8_t id, String^ cmd)
{
	int retval;
	std::string str = marshal_as<std::string>(cmd);
	unsigned char* arr = new unsigned char[str.length() / 2];
	for (int i = 0; i < str.length() / 2; i++) {
		std::string byteStr = str.substr(i * 2, 2);
		arr[i] = std::stoul(byteStr, nullptr, 16);
	}
	unsigned char newCmd[65];
	newCmd[0] = id; // add 0x00 byte at the beginning
	memcpy(&newCmd[1], arr, str.length() / 2); // copy the original data after the 0x00 byte
	retval = hid_write(handle, newCmd, str.size() + 1);
	if (retval < 0) {
		return  -1;
	}
}

String^ DFUCli::RevMsg()
{
	unsigned char rev[64];
	int nRecvLen = hid_read(handle, rev, 64);
	//int nRecvLen = hid_get_input_report(handle, rev, 64);
	if (nRecvLen > 0)
	{
		string str;
		str = (char*)rev;
		String^ stdToCli = marshal_as<String^>(str);
		return stdToCli;
	}
	else
	{
		return"";
	}
}

String^ DFUCli::RevMsgbyte()
{
	unsigned char rev[64];
	int nRecvLen = hid_read(handle, rev, 64);
	if (nRecvLen > 0)
	{
		std::stringstream ss;
		for (int i = 0; i < nRecvLen; i++) {
			ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rev[i]);
		}
		String^ stdToCli = marshal_as<String^>(ss.str());
		return stdToCli;
	}
	else
	{
		return"";
	}
}

String^ DFUCli::Send_TDE_GetTime()
{
	int retval;
	unsigned char send[64] = " GetTime\r\n";
	unsigned char rev[64];
	retval = hid_write(handle, send, 10);
	if (retval < 0) {
		return "写入错误";
	}
	retval = hid_read(handle, rev, 64);
	if (retval < 0) {
		return "读取错误";
	}
	else
	{
		string str;
		char time[32];
		
		//str = (char*)rev;
		sscanf((char*)rev, " CurrentTime=%s", time);
		str = time;
		String^ stdToCli = marshal_as<String^>(str);
		return stdToCli;
	}
}

int DFUCli::Int_ConvertTo_Hex(int digt)
{
	char str[4];
	sprintf(str, "%02d", digt);
	int hex_value = strtol(str, NULL, 16);
	return hex_value;
}