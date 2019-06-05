
#include "stdafx.h"
#include "water.h"
#include "../config/inihelper.h"

#define IS_WATER_FILE_RECORD 1

WaterDownDataPackage downData;
WaterUpDataPackage upData;

#if IS_WATER_FILE_RECORD
static char filename[100] = "";
#endif

static bool JudgeDownCheckByte(unsigned char* chars)
{
	unsigned char checkbyte = 0;
	for (int i = 0; i < CRC_DOWN_INDEX; ++i)
	{
		checkbyte ^= chars[i];
	}
	return checkbyte == chars[CRC_DOWN_INDEX];
}

Water::Water()
{
	DataInit();
}

Water::~Water()
{
}

void Water::DataInit()
{
	IsRecievedData = false;
	Roll = 0;
	Yaw = 0;
	Pitch = 0;
	UpPackageLength = sizeof(WaterUpDataPackage);
	DownPackageLength = sizeof(WaterDownDataPackage);
	frameNumber = 0;
	ControlCommand = WaterControlCommandInt8::WATER_CTL_CMD_NONE_INT8;
#if IS_WATER_FILE_RECORD
	time_t currtime = time(NULL);
	struct tm* p = gmtime(&currtime);
	sprintf_s(filename, "./datas/waterdata%d-%d-%d-%d-%d-%d.txt", p->tm_year + 1990, p->tm_mon + 1,
		p->tm_mday, p->tm_hour + 8, p->tm_min, p->tm_sec);
#endif
}

bool Water::Open()
{
	auto re = serialPort.InitPort(WATER_SERIAL_NUM, WATER_SERIAL_BAUD) == true;
	//auto re = OpenCOMDevice(WATER_SERIAL_NUM, WATER_SERIAL_BAUD) == 0;
	return re;
}

bool Water::Close()
{
	//CloseCOMDevice();
	return false;
}

bool Water::GatherData()
{
	// 数据帧处理相关
	static int uiRemainLength = 0;
	static unsigned long ulFrameNum = 0;
	static unsigned long ulFrameErr = 0;
	static int length = DownPackageLength;
	static UCHAR chData[WATER_READ_BUFFER + 102400] = {0};
	static UCHAR *pch = chData;
	int i;
	UCHAR chReadData[WATER_READ_BUFFER] = {0};
	//unsigned int uiReceived = (int)serialPort.GetCOMData(chReadData);
	unsigned int uiReceived = serialPort.GetBytesInCOM();
	unsigned char cRecved = 0;
	for (int i = 0; i < uiReceived; ++i)
	{
		serialPort.ReadChar(cRecved);
		chReadData[i] = cRecved;
	}
	if(uiReceived == 0)
	{
		return false;		
	}
	memcpy(pch, chReadData, uiReceived);    //将数据置于chData[]中
	i = 0;
	int j = uiRemainLength + uiReceived - length;
	while(i <= j)
	{
		UCHAR *pData = &chData[i];
		if((pData[0] == PACKAGE_HEADER1) && (pData[1] == PACKAGE_HEADER2) &&
			(JudgeDownCheckByte(pData) == true))
		{       	
			ulFrameNum++;
			memcpy(&downData, &pData[0], length);
			auto roll = downData.Roll / WATER_ANGLE_SCALE;
			auto pitch = downData.Pitch / WATER_ANGLE_SCALE;
			auto yaw = downData.Yaw / WATER_ANGLE_SCALE;
			auto yawOffset = downData.YawOffset / WATER_ANGLE_SCALE;
			if (roll >- 20 && roll < 20) Roll = roll;
			if (pitch >- 20 && pitch < 20) Pitch = pitch;
			if (yaw >- 20 && yaw < 20) Yaw = yaw;
			if (yawOffset > -361 && yawOffset < 361) YawOffset = yawOffset;
			ControlCommand = static_cast<WaterControlCommandInt8>(downData.Control);
			IsRecievedData = true;
			i += length;		
#if IS_WATER_FILE_RECORD
			config::RecordData(filename, Roll, Pitch, Yaw);
#endif
			continue;
		}
		else
		{
			i += 1;
		}
	}
	uiRemainLength += uiReceived - i;
	if(uiRemainLength != 0)
	{
		memcpy(chReadData, &chData[i], uiRemainLength);
		memcpy(chData, chReadData, uiRemainLength);
	}
	pch = &chData[uiRemainLength];
	return true;
}

void Water::SendData(double roll, double yaw, double pitch, 
					 uint8_t platformState, uint8_t platformWarning)
{
	static unsigned char chrTemp[BUFFER_MAX] = {0};
	size_t sendSize = sizeof(char) * UpPackageLength;
	memset(chrTemp, 0, sendSize);
	WaterUpDataPackage data;
	data.HeadOne = PACKAGE_HEADER1;
	data.HeadTwo = PACKAGE_HEADER2;
	data.TailOne = PACKAGE_TAIL1;
	data.TailTwo = PACKAGE_TAIL2;
	data.Kind = 0x01;
	data.State = 0x02;
	data.InitState = 0x02;
	data.FrameNumber = frameNumber++;
	data.Length = UP_DATA_LENGTH;
	data.Yaw = (uint32_t)(yaw * WATER_ANGLE_SCALE);
	data.Roll = (uint32_t)(roll * WATER_ANGLE_SCALE);
	data.Pitch = (uint32_t)(pitch * WATER_ANGLE_SCALE);
	data.PlatformState = platformState;
	data.PlatformWarning = platformWarning;
	memcpy(&chrTemp[0], &data, UpPackageLength); 
	for (int i = 0;i < CRC_UP_INDEX;++i)
	{
		chrTemp[CRC_UP_INDEX] ^= chrTemp[i];
	}
	serialPort.WriteData(chrTemp, UpPackageLength);
}
