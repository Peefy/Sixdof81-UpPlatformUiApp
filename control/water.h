
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
#define CRC_DOWN_INDEX    22

#define WATER_ANGLE_SCALE 1000.0

#define UP_DATA_LENGTH    16
#define DOWN_DATA_LENGTH  17

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

enum class PlatformWarningType
{
	NORMAL = 1,
	MECHANICAL_ERR = 2,
	DATA_ERR = 3,
	RUN_ERR = 4,
	LOGICAL_ERR = 5,
	DATA_OUT_OF_RANGE = 6
};

#pragma pack (1)
typedef struct
{
	uint8_t HeadOne;          //0
	uint8_t HeadTwo;          //1
	uint8_t Length;           //2       
	uint8_t FrameNumber;      //3  
	uint8_t Kind;             //4
	uint8_t State;            //5
	uint8_t InitState;        //6
	uint8_t PlatformState;    //7
	uint8_t PlatformWarning;  //8
	int32_t Yaw;              //9-12
	int32_t Pitch;            //13-16
	int32_t Roll;             //17-20
	int8_t  Crc;              //21
	uint8_t TailOne;          //22
	uint8_t TailTwo;          //23
} WaterUpDataPackage;
#pragma pack () 

#pragma pack (1)
typedef struct
{
	uint8_t HeadOne;      //0
	uint8_t HeadTwo;      //1
	uint8_t Length;       //2  
	uint8_t FrameNumber;  //3 
	uint8_t Kind;         //4
	uint8_t Control;      //5
	int32_t Yaw;          //6-9
	int32_t Pitch;        //10-13
	int32_t Roll;         //14-17
	int32_t YawOffset;    //18-21
	int8_t  Crc;          //22
	uint8_t TailOne;      //23
	uint8_t TailTwo;      //24
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
	double YawOffset;
	WaterControlCommandInt8 ControlCommand;
	PlatformWarningType PlatformWarning;
	bool Open();
	bool Close();
	bool GatherData();
	void SendData(double roll, double yaw, double pitch, 
		uint8_t platformState, uint8_t platformWarning);
private:
	uint8_t frameNumber;
	CSerialPort serialPort;
protected:
	int UpPackageLength;
	int DownPackageLength;
	void DataInit();
};



#endif 
