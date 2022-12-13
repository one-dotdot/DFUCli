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
