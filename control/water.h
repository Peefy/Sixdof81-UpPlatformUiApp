
#ifndef _WATER_H_
#define _WATER_H_

#include "stdint.h"

#include "../communication/SerialPort.h"

#define WATER_SERIAL_NUM   8
#define WATER_SERIAL_BAUD  115200
#define BUFFER_MAX 4096

#define PACKAGE_HEADER1    0XAA
#define PACKAGE_HEADER2    0X55
#define PACKAGE_TAIL1      0XFE
#define PACKAGE_TAIL2      0X5B

#define CRC_UP_INDEX      21
#define CRC_DOWN_INDEX    18

#define WATER_ANGLE_SCALE 1000.0

#define UP_DATA_LENGTH    16
#define DOWN_DATA_LENGTH  13

#define WATER_READ_BUFFER 10240

enum class WaterControlCommandInt8
{
	WATER_CTL_POWER_ON_INT8       =  0,    //串口API控制指令：电源开
	WATER_CTL_POWER_OFF_INT8      =  1,    //串口API控制指令：电源关
	WATER_CTL_CMD_RISE_INT8       =  2,    //串口API控制指令：上升
	WATER_CTL_CMD_DOWN_INT8       =  3,    //串口API控制指令：下降
	WATER_CTL_CMD_CONNECT_INT8    =  6,    //串口API控制指令：连接
	WATER_CTL_CMD_DISCONNECT_INT8 =  7,    //串口API控制指令：断开
	WATER_CTL_CMD_PAUSE_INT8      =  8,    //串口API控制指令：暂停
	WATER_CTL_CMD_RECOVER_INT8    =  9,    //串口API控制指令：暂停恢复
	WATER_CTL_CMD_CHECK_ON_INT8   =  10,   //串口API控制指令：检修开
	WATER_CTL_CMD_CHECK_OFF_INT8  =  11,   //串口API控制指令：检修关
	WATER_CTL_CMD_NONE_INT8       =  100,  //串口API控制指令：空
};

#pragma pack (1)
typedef struct
{
	uint8_t HeadOne;
	uint8_t HeadTwo;
	uint8_t Length;        
	uint8_t FrameNumber;  
	uint8_t Kind;
	uint8_t State;
	uint8_t InitState;
	uint8_t PlatformState;
	uint8_t PlatformWarning;
	int32_t Yaw;
	int32_t Pitch;
	int32_t Roll;
	int8_t  Crc;
	uint8_t TailOne;
	uint8_t TailTwo;
} WaterUpDataPackage;
#pragma pack () 

#pragma pack (1)
typedef struct
{
	uint8_t HeadOne;
	uint8_t HeadTwo;
	uint8_t Length;        
	uint8_t FrameNumber;  
	uint8_t Kind;
	uint8_t Control;
	int32_t Yaw;
	int32_t Pitch;
	int32_t Roll;
	int8_t  Crc;
	uint8_t TailOne;
	uint8_t TailTwo;
} WaterDownDataPackage;
#pragma pack () 

class Water
{
public:
	Water();
	~Water();
	bool IsRecievedData;
	double Roll;
	double Yaw;
	double Pitch;
	WaterControlCommandInt8 ControlCommand;
	bool Open();
	bool Close();
	bool GatherData();
	void TestSendData();
	void SendData(double roll, double yaw, double pitch, 
		uint8_t platformState, uint8_t platformWarning);
private:
	uint8_t frameNumber;
	CSerialPort serialPort;
protected:
	int UpPackageLength;
	int DownPackageLength;
	void DataInit();
	void RenewData();
};



#endif 
