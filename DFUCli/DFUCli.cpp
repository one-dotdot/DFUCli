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

void ConvertToString(System::String^ str, std::string& text)
{
	char* p = (char*)(int)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str);
	text = p;
	System::Runtime::InteropServices::Marshal::FreeHGlobal((System::IntPtr)p);
}
void DFUCli::test() {}
int DFUCli::Update(String^ filepath)
{
	FILE* fp;
	uint8_t* FileData;
	uint32_t FileLen = 0;
	errno_t err;
	int retval = 0;
	int LoopNum = 0;
	int Remain = 0;
	int i = 0;
	
	std::string FilePath = marshal_as<std::string>(filepath);

	err = fopen_s(&fp, FilePath.c_str(), "rb");
	if (err) {
		//FilePath.ReleaseBuffer();
		//MessageBox(__T("打开升级文件失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		return ERRO_FP;
	}
	//FilePath.ReleaseBuffer();

	// Step 1: Get File Length
	fseek(fp, 0, SEEK_END);
	FileLen = ftell(fp);

	// Step 2: Read total file data
	fseek(fp, 0, SEEK_SET);
	FileData = (uint8_t*)malloc(FileLen);
	if (!FileData) {
		fclose(fp);
		//MessageBox(__T("内存申请失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		return ERRO_MM;
	}

	retval = fread(FileData, 1, FileLen, fp);
	if (retval != FileLen) {
		fclose(fp);
		free(FileData);
		//MessageBox(__T("读取文件失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		return -3;
	}
	fclose(fp);

	hid_set_nonblocking(handle, 0);   // Blocking Mode

	// Step 3: Enter DFU
	retval = enter_into_dfu();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 4: Check Bin File		
	retval = check_bin_file(&FileData[FileLen - 256], 8);
	if (retval) {
		goto DFU_OUT;
	}

	// Step 5: Erase Firmware Flash Zone
	retval = erase_flash_zone();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 6: Write APP Data	
	LoopNum = FileLen / 56;
	Remain = FileLen % 56;
	//mDownloadProgress.SetPos(0);
	for (i = 0; i < LoopNum; i++) {
		retval = write_app_data(FileData + i * 56, 56);
		if (retval) {
			goto DFU_OUT;
		}

		//mDownloadProgress.SetPos(((i + 1) * 56 * 100) / FileLen);
	}

	if (Remain) {
		retval = write_app_data(FileData + LoopNum * 56, Remain);
		if (retval) {
			goto DFU_OUT;
		}
		//mDownloadProgress.SetPos(100);
	}

	// Step 7: Verify APP
	retval = verify_app();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 8: Exit DFU
	retval = exit_from_dfu();

DFU_OUT:
	free(FileData);
	switch (retval) {
	case 0:
		return OK;
		//MessageBox(__T("升级成功 !!"), __T("DFU升级工具"), MB_OK);
		break;
	case -1:
	case -2:
		return ERRO_DR;
		//MessageBox(__T("设备读写错误 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -3:
		return ERRO_XY;
		//MessageBox(__T("协议解析错误 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -4:
		return ERRO_FW;
		//MessageBox(__T("版本信息验证失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -5:
		return ERRO_FV;
		//MessageBox(__T("文件校验失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	default:
		break;
	}
	return 0;
}
int DFUCli::Update1(String^ filepath)
{
	FILE* fp;
	errno_t err;
	int retval = 0;
	int i = 0;

	std::string FilePath = marshal_as<std::string>(filepath);

	err = fopen_s(&fp, FilePath.c_str(), "rb");
	if (err) {
		return ERRO_FP;
	}

	// Step 1: Get File Length
	fseek(fp, 0, SEEK_END);
	FileLen = ftell(fp);

	// Step 2: Read total file data
	fseek(fp, 0, SEEK_SET);
	FileData = (uint8_t*)malloc(FileLen);
	if (!FileData) {
		fclose(fp);
		return ERRO_MM;
	}

	retval = fread(FileData, 1, FileLen, fp);
	if (retval != FileLen) {
		fclose(fp);
		free(FileData);
		return ERRO_FR;
	}
	fclose(fp);

	hid_set_nonblocking(handle, 0);   // Blocking Mode

	// Step 3: Enter DFU
	retval = enter_into_dfu();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 4: Check Bin File		
	retval = check_bin_file(&FileData[FileLen - 256], 8);
	if (retval) {
		goto DFU_OUT;
	}

	// Step 5: Erase Firmware Flash Zone
	retval = erase_flash_zone();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 6: Write APP Data	
	LoopNum = FileLen / 56;
	Remain = FileLen % 56;

DFU_OUT:
	switch (retval) {
	case 0:
		return OK;
		break;
	case -1:
	case -2:
		return ERRO_DR;
		break;
	case -3:
		return ERRO_XY;
		break;
	case -4:
		return ERRO_FW;
		break;
	case -5:
		return ERRO_FV;
		break;
	default:
		break;
	}
	return 0;

}

int DFUCli::getLoopnum()
{
	int ret = LoopNum;
	return ret;
}

int DFUCli::getLenth()
{
	int ret = FileLen;
	return ret;
}

int DFUCli::getRemain()
{
	int ret = Remain;
	return ret;
}

int DFUCli::writeDate(int step)
{
	return write_app_data(FileData + step * 56, 56);
}

int DFUCli::writeRemain()
{
	return write_app_data(FileData + LoopNum * 56, Remain);
}

int DFUCli::Update2()
{
	int retval;
	// Step 7: Verify APP
	retval = verify_app();
	if (retval) {
		goto DFU_OUT;
	}

	// Step 8: Exit DFU
	retval = exit_from_dfu();

DFU_OUT:
	free(FileData);
	switch (retval) {
	case 0:
		return OK;
		//MessageBox(__T("升级成功 !!"), __T("DFU升级工具"), MB_OK);
		break;
	case -1:
	case -2:
		return ERRO_DR;
		//MessageBox(__T("设备读写错误 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -3:
		return ERRO_XY;
		//MessageBox(__T("协议解析错误 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -4:
		return ERRO_FW;
		//MessageBox(__T("版本信息验证失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	case -5:
		return ERRO_FV;
		//MessageBox(__T("文件校验失败 !!"), __T("DFU升级工具"), MB_OK | MB_ICONERROR);
		break;
	default:
		break;
	}
	return 0;

}

int DFUCli::enter_into_dfu(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x01;   // Len
	SendBuf[3] = 0x01;   // Command Code 0x01 - Enter DFU
	SendBuf[4] = SendBuf[2] + SendBuf[3]; // Check Sum;
	SendBuf[5] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x01) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::check_bin_file(uint8_t* check, uint8_t len)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	int i = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x09;   // Len
	SendBuf[3] = 0x02;   // Command Code 0x02 - Check Bin File
	memcpy(SendBuf + 4, check, len);
	for (i = 0; i < 2 + len; i++) {
		SendBuf[4 + len] += SendBuf[2 + i];
	}
	SendBuf[5 + len] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x02) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -4;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::erase_flash_zone(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x01;   // Len
	SendBuf[3] = 0x03;   // Command Code 0x03 - Erase Firmware Flash Zone
	SendBuf[4] = SendBuf[2] + SendBuf[3]; // Check Sum;
	SendBuf[5] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x03) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::write_app_data(uint8_t* pdata, uint8_t len)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	int i = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 1 + len;  // Len
	SendBuf[3] = 0x04;   // Command Code 0x04 - Write APP Data
	memcpy(SendBuf + 4, pdata, len);
	for (i = 0; i < 2 + len; i++) {
		SendBuf[4 + len] += SendBuf[2 + i];
	}
	SendBuf[5 + len] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x04) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::write_app_data(int x, uint8_t len)
{
	uint8_t* pdata = FileData + x * 56;
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	int i = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 1 + len;  // Len
	SendBuf[3] = 0x04;   // Command Code 0x04 - Write APP Data
	memcpy(SendBuf + 4, pdata, len);
	for (i = 0; i < 2 + len; i++) {
		SendBuf[4 + len] += SendBuf[2 + i];
	}
	SendBuf[5 + len] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x04) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::verify_app(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x01;   // Len
	SendBuf[3] = 0x05;   // Command Code 0x05 - Verify APP
	SendBuf[4] = SendBuf[2] + SendBuf[3]; // Check Sum;
	SendBuf[5] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x05) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -5;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::exit_from_dfu(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x01;   // Len
	SendBuf[3] = 0x06;   // Command Code 0x06 - Exit DFU
	SendBuf[4] = SendBuf[2] + SendBuf[3]; // Check Sum;
	SendBuf[5] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x02) {
		return -3;
	}

	if (RecvBuf[2] != 0x06) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}

	if (RecvBuf[4] != (RecvBuf[1] + RecvBuf[2] + RecvBuf[3])) {
		return -3;
	}

	if (RecvBuf[5] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::getSn(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	SN = (char*)malloc(16);
	
	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x01;   // 0x0701 - Get Machine SN
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x10) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x01) {
		return -3;
	}
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3];
	for (int i = 4; i < 18; i++)
	{
		SN[i-4] = RecvBuf[i];
		checksum += RecvBuf[i];
		//SN = strcat(SN, (char *)RecvBuf + i);
	}
	SN[14] = '\0';
	if (RecvBuf[18] != checksum) {
		return -3;
	}

	if (RecvBuf[19] != 0x55) {
		return -3;
	}

	return 0;
}

String^ DFUCli::getsn()
{
	string sn = SN;
	String^ stdToCli = marshal_as<String^>(sn);
	return stdToCli;
}
int DFUCli::getState(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x06;   // 0x0706 - Get Machine State
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x03) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x06) {
		return -3;
	}
	STATE = RecvBuf[4];
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3] + RecvBuf[4];
	
	if (RecvBuf[5] != checksum) {
		return -3;
	}

	if (RecvBuf[6] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::getstate()
{
	int st = STATE;
	return st;
}

int DFUCli::getHW(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	HW = (char*)malloc(8);

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x03;   // 0x0703 - Get Machine HW Version
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x07) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x03) {
		return -3;
	}
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3];
	for (int i = 4; i < 9; i++)
	{
		HW[i - 4] = RecvBuf[i];
		checksum += RecvBuf[i];
	}
	HW[5] = '\0';
	if (RecvBuf[9] != checksum) {
		return -3;
	}

	if (RecvBuf[10] != 0x55) {
		return -3;
	}

	return 0;
}

String^ DFUCli::gethw()
{
	string hw = HW;
	String^ stdToCli = marshal_as<String^>(hw);
	return stdToCli;
}

int DFUCli::getFW(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	FW = (char*)malloc(8);

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x04;   // 0x0703 - Get Machine FW Version
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x08) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x04) {
		return -3;
	}
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3];
	for (int i = 4; i < 10; i++)
	{
		FW[i - 4] = RecvBuf[i];
		checksum += RecvBuf[i];
	}
	FW[6] = '\0';
	if (RecvBuf[10] != checksum) {
		return -3;
	}

	if (RecvBuf[11] != 0x55) {
		return -3;
	}

	return 0;
}

String^ DFUCli::getfw()
{
	string fw = FW;
	String^ stdToCli = marshal_as<String^>(fw);
	return stdToCli;
}

int DFUCli::getBV(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x05;   // 0x0705 - Get Machine Current Battery Voltage
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x03) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x05) {
		return -3;
	}
	BV = RecvBuf[4];
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3] + RecvBuf[4];

	if (RecvBuf[5] != checksum) {
		return -3;
	}

	if (RecvBuf[6] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::getbv()
{
	int bv = BV;
	return bv;
}

int DFUCli::getLOG(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x07;   // 0x0707 - Get Recorded Injection Logs Number
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x04) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x07) {
		return -3;
	}

	LOG = RecvBuf[4] <<8;
	LOG += RecvBuf[5];

	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3] + RecvBuf[4] + RecvBuf[5];

	if (RecvBuf[6] != checksum) {
		return -3;
	}

	if (RecvBuf[7] != 0x55) {
		return -3;
	}

	return 0;
}

int DFUCli::getlog()
{
	int log;
	log = LOG;
	return log;
	//String^ stdToCli = marshal_as<String^>(str);
	//return stdToCli;
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

int DFUCli::getAT(void)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf[64] = { 0 };
	int retval = 0;
	TIME = (uint8_t*)malloc(8);

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf, 0x00, sizeof(RecvBuf));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x02;   // Len
	SendBuf[3] = 0x07;   // Command Code 
	SendBuf[4] = 0x02;   // 0x0702 - Get Machine Activation Time
	SendBuf[5] = SendBuf[2] + SendBuf[3] + SendBuf[4]; // Check Sum;
	SendBuf[6] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	retval = hid_read(handle, RecvBuf, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf[1] != 0x09) {
		return -3;
	}

	if (RecvBuf[2] != 0x07) {
		return -3;
	}

	if (RecvBuf[3] != 0x02) {
		return -3;
	}
	checksum = RecvBuf[1] + RecvBuf[2] + RecvBuf[3];
	for (int i = 4; i < 11; i++)
	{
		TIME[i - 4] = RecvBuf[i];
		checksum += RecvBuf[i];
	}
	if (RecvBuf[11] != checksum) {
		return -3;
	}

	if (RecvBuf[12] != 0x55) {
		return -3;
	}

	return 0;
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

String^ DFUCli::getat()
{
	char time[32];
	int year, mon, day, h, min, sec;
	string week;
	year = bcd_decimal_code(TIME[6]) + 1970;
	mon = bcd_decimal_code(TIME[5]);
	week = getweek(TIME[4]);
	day = bcd_decimal_code(TIME[3]);
	h = bcd_decimal_code(TIME[2]);
	min = bcd_decimal_code(TIME[1]);
	sec = bcd_decimal_code(TIME[0]);
	sprintf(time, "%4d/%02d/%02d %s %02d:%02d:%02d", year, mon, day, week, h, min, sec);
	String^ stdToCli = marshal_as<String^>(time);
	return stdToCli;
}

String^ TimetoString(uint8_t* TIME)
{
	char time[32];
	int year, mon, day, h, min, sec;
	year = bcd_decimal_code(TIME[6]) + 1970;
	mon = bcd_decimal_code(TIME[5]);
	day = bcd_decimal_code(TIME[3]);
	h = bcd_decimal_code(TIME[2]);
	min = bcd_decimal_code(TIME[1]);
	sec = bcd_decimal_code(TIME[0]);
	sprintf(time, "%4d/%02d/%02d %02d:%02d:%02d", year, mon, day, h, min, sec);
	String^ stdToCli = marshal_as<String^>(time);
	return stdToCli;
}

int DFUCli::getInf(uint16_t index)
{
	uint8_t SendBuf[64] = { 0 };
	uint8_t RecvBuf1[64] = { 0 };
	uint8_t RecvBuf2[64] = { 0 };
	uint8_t RecvBuf3[64] = { 0 };
	int retval = 0;
	//初始化
	dataInit();

	memset(SendBuf, 0x00, sizeof(SendBuf));
	memset(RecvBuf1, 0x00, sizeof(RecvBuf1));
	memset(RecvBuf2, 0x00, sizeof(RecvBuf2));
	memset(RecvBuf3, 0x00, sizeof(RecvBuf3));

	SendBuf[0] = 0x00;   // Report ID;
	SendBuf[1] = 0xAA;   // Head
	SendBuf[2] = 0x03;   // Len
	SendBuf[3] = 0x08;   // Command Code 
	SendBuf[4] = index >> 8;
	SendBuf[5] = index;		// index
	SendBuf[6] = SendBuf[2] + SendBuf[3] + SendBuf[4] + SendBuf[5]; // Check Sum;
	SendBuf[7] = 0x55;   // Tail	

	retval = hid_write(handle, SendBuf, 64);   // Report Size is 64 bytes
	if (retval < 0) {
		return -1;
	}

	//校验
	retval = hid_read(handle, RecvBuf1, 64);
	if (retval < 0) {
		return -2;
	}
	retval = hid_read(handle, RecvBuf2, 64);
	if (retval < 0) {
		return -2;
	}
	retval = hid_read(handle, RecvBuf3, 64);
	if (retval < 0) {
		return -2;
	}

	if (RecvBuf1[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf1[1] != 0x3c) {
		return -3;
	}

	if (RecvBuf1[2] != 0x08) {
		return -3;
	}
	for (int i = 1; i < 62; i++)
	{
		checksum += RecvBuf1[i];
	}
	if (RecvBuf1[62] != checksum) {
		return -3;
	}
	if (RecvBuf1[63] != 0x55) {
		return -3;
	}

	if (RecvBuf2[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf2[1] != 0x3c) {
		return -3;
	}

	if (RecvBuf2[2] != 0x08) {
		return -3;
	}
	checksum = 0;
	for (int i = 1; i < 62; i++)
	{
		checksum += RecvBuf2[i];
	}
	if (RecvBuf2[62] != checksum) {
		return -3;
	}
	if (RecvBuf2[63] != 0x55) {
		return -3;
	}

	if (RecvBuf3[0] != 0xAA) {
		return -3;
	}

	if (RecvBuf3[1] != 0x3c) {
		return -3;
	}

	if (RecvBuf3[2] != 0x08) {
		return -3;
	}
	checksum = 0;
	for (int i = 1; i < 62; i++)
	{
		checksum += RecvBuf3[i];
	}
	if (RecvBuf3[62] != checksum) {
		return -3;
	}
	if (RecvBuf3[63] != 0x55) {
		return -3;
	}
	
	//数据处理
	mIndex = RecvBuf1[3] << 8;
	mIndex += RecvBuf1[4];

	//时间处理
	uint8_t opentime[8], airtime[8], intime[8], endtime[8];
	for (int i = 0; i < 7; i++)
	{
		opentime[i] = RecvBuf2[i + 24] & 0x7F;
		airtime[i] = RecvBuf2[i + 31] & 0x7F;
		intime[i] = RecvBuf2[i + 38] & 0x7F;
		endtime[i] = RecvBuf2[i + 45] & 0x7F;
	}
	openTime = TimetoString(opentime);
	airTime = TimetoString(airtime);
	inTime = TimetoString(intime);
	endTime = TimetoString(endtime);

	//信息处理
	mVolume = RecvBuf2[52] << 8;
	mVolume += RecvBuf2[53];

	mState = RecvBuf2[56];

	mPower = RecvBuf2[59];

	if (RecvBuf2[60] && 0x80)
		mAir = RecvBuf2[60] & 0x7F;
	else
		mAir = 0;

	mERR = RecvBuf2[61];
	if (mERR)
	{
		mERRlog = (uint16_t*)malloc(mERR);
		for (uint8_t i = 0; i < mERR; i++)
		{
			mERRlog[i] = RecvBuf3[i+3] << 8;
			mERRlog[i] += RecvBuf3[i+4];
		}
	}

	return 0;
}

void DFUCli::dataInit()
{
	checksum = 0;
	mIndex = 0;
	openTime = "";
	airTime = "";
	inTime = "";
	endTime = "";
	mVolume = 0;
	mState = 0;
	mPower = 0;
	mAir = 0;
	mERR = 0;
	free(mERRlog);
}

