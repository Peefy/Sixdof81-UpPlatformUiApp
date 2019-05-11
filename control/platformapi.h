
#ifndef _PLATFORM_API_H_
#define _PLATFORM_API_H_

#include <stdint.h>
#include "../communication/SerialPort.h"

#define API_PORT_NUMBER          6
#define API_PORT_BAUDRATE        115200
#define API_DATA_HEAD_ONE        0xAA
#define API_DATA_HEAD_TWO        0xBB
#define API_DATA_TAIL_ONE        0xCC
#define API_DATA_TAIL_TWO        0xDD
#define API_DATA_PACKAGE_LEGNTH  83

#define API_ANGLE_SCALE          0.001

#define API_CTL_CMD_RISE_INT32        111
#define API_CTL_CMD_DOWN_INT32        222
#define API_CTL_CMD_CONNECT_INT32     333
#define API_CTL_CMD_DISCONNECT_INT32  444
#define API_CTL_CMD_PAUSE_INT32       555
#define API_CTL_CMD_RECOVER_INT32     666

#pragma pack (1)
typedef struct
{
	uint8_t HeadOne;
	uint8_t HeadTwo;
	int32_t X;                 //0.001
	int32_t Y;                 //0.001
	int32_t Z;                 //0.001
	int32_t Roll;              //0.001
	int32_t Pitch;             //0.001
	int32_t Yaw;               //0.001
	uint8_t ControlCommand;
	uint8_t StateByte;
	uint8_t CheckByte;
	uint8_t TailOne;
	uint8_t TailTwo;
} ApiDataPackage;
#pragma pack () 

class ApiControl
{
public:
	ApiControl();
	~ApiControl();
	bool Open();
	bool Close();
	void GatherData();
	bool IsStart;
	bool IsRecievedData;
	ApiDataPackage Data;
private:
	CSerialPort serialPort;
protected:
};

#endif
