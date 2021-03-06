
#include "stdafx.h"
#include "sensor.h"
#include "Com.h"

#include <math.h>

Sensor::Sensor(int port)
{
	init(&info);
	init(&offsetinfo);
	init(&readInfo);
	this->port = port;
	openPort(port, SENSOR_BAUD);
	offsetinfo.Yaw = 66;
}

Sensor::Sensor()
{
	init(&info); 
	init(&offsetinfo);
	init(&readInfo);
	this->port = SENSOR_PORT1;
	openPort(SENSOR_PORT1, SENSOR_BAUD);
	offsetinfo.Yaw = 66;
}

Sensor::~Sensor()
{

}

SensorInfo_t Sensor::ProvideSensorInfo()
{
	unsigned short usLength = 0;
	static char chrBuffer[SENSOR_BUFFER_LENGTH];
	static unsigned char ucharBuffer[SENSOR_BUFFER_LENGTH];
	if (isStart == false)
	{
		return offsetinfo;
	}
	/*
	usLength = serialPort.GetBytesInCOM();
	unsigned char cRecved;
	for (int i = 0; i < usLength; ++i)
	{
		serialPort.ReadChar(cRecved);
		chrBuffer[i] = cRecved;
	}
	*/
	//usLength = CollectUARTData(SENSOR_PORT1, chrBuffer);
	usLength = serialPort.GetCOMData(ucharBuffer);
	memcpy(chrBuffer, ucharBuffer, usLength);
	if (usLength > 0)
	{
		hardware.CopeSerialData(chrBuffer, usLength);
		info.Accel[0] = hardware.stcAcc.a[0] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ACC;
		info.Accel[1] = hardware.stcAcc.a[1] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ACC;
		info.Accel[2] = hardware.stcAcc.a[2] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ACC;
		info.Gyro[0] = hardware.stcGyro.w[0] / SENSOR_MAX_AD_VAL * MPU6050_MAX_GYRO;
		info.Gyro[1] = hardware.stcGyro.w[1] / SENSOR_MAX_AD_VAL * MPU6050_MAX_GYRO;
		info.Gyro[2] = hardware.stcGyro.w[2] / SENSOR_MAX_AD_VAL * MPU6050_MAX_GYRO;
		readInfo.Roll = hardware.stcAngle.Angle[0] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;
		readInfo.Pitch = hardware.stcAngle.Angle[1] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;
		readInfo.Yaw = hardware.stcAngle.Angle[2] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;

		info.Roll = readInfo.Roll - offsetinfo.Roll;
		info.Pitch = readInfo.Pitch - offsetinfo.Pitch;
		info.Yaw = readInfo.Yaw - offsetinfo.Yaw;

	}
	return info;
}

SensorInfo_t Sensor::GatherData()
{
	if (isStart == false)
	{
		return offsetinfo;
	}
	// 数据帧处理相关
	static int uiRemainLength = 0;
	static unsigned long ulFrameNum = 0;
	static unsigned long ulFrameErr = 0;
	static UCHAR chData[READBUFFER + 102400] = {0};
	static UCHAR *pch = chData;
	static struct SAngle angle;
	static struct SGyro gyro;
	int i;
	UCHAR chReadData[READBUFFER] = {0};
	unsigned int uiReceived = (int)serialPort.GetCOMData(chReadData);
	if(uiReceived == 0)
	{
		return info;		
	}
	memcpy(pch, chReadData, uiReceived);    //将数据置于chData[]中
	i = 0;
	int j = uiRemainLength + uiReceived - 11;
	while(i <= j)
	{
		UCHAR *pData = &chData[i];
		if((pData[0] == 0x55) && (pData[1] == 0x53))
		{       	
			ulFrameNum++;
			memcpy(&angle, &pData[2], 8);
			readInfo.Roll = angle.Angle[0] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;
			readInfo.Pitch = angle.Angle[1] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;
			readInfo.Yaw = angle.Angle[2] / SENSOR_MAX_AD_VAL * MPU6050_MAX_ANGLE;
			info.Roll = readInfo.Roll - offsetinfo.Roll;
			info.Pitch = readInfo.Pitch - offsetinfo.Pitch;
			info.Yaw = readInfo.Yaw - offsetinfo.Yaw;
			i += 11;		
			continue;
		}
		if((pData[0] == 0x55) && (pData[1] == 0x53))
		{       	
			ulFrameNum++;
			memcpy(&angle, &pData[2], 8);
			for (int i = 0;i < 3;++i)
			{
				info.Gyro[i] = gyro.w[i] / SENSOR_MAX_AD_VAL * MPU6050_MAX_GYRO;
			}
			i += 11;		
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
	return info;
}

bool Sensor::IsReady()
{
	return info.Roll != 0 && info.Gyro[0] != 0 && info.Accel[0] != 0;
}

bool Sensor::openPort(int i, int baud)
{
	//isStart = serialPort.InitPort(i, baud) == true;
	//isStart = OpenCOMDevice(SENSOR_PORT1, SENSOR_BAUD) == 0;
	isStart = serialPort.InitCOM(i, baud, 1, 0, 8);
	return isStart;
}

bool Sensor::closePort()
{
	//CloseCOMDevice();
	isStart = false;
	return isStart;
}

void Sensor::UpdateOffset()
{
	offsetinfo.Roll = readInfo.Roll;
	offsetinfo.Pitch = readInfo.Pitch;
	offsetinfo.Yaw = readInfo.Yaw;
}

void Sensor::init(SensorInfo_t* info)
{
	info->Roll = 0;
	info->Pitch = 0;
	info->Yaw = 0;
	for (auto i = 0; i < SENSOR_AXES_NUM; ++i)
	{
		info->Gyro[i] = 0;
		info->Accel[i] = 0;
	}
}

SensorInfo_t GetAverageSenrorValue(SensorInfo_t* info1, SensorInfo_t* info2, SensorInfo_t* info3)
{
	SensorInfo_t avr;
	avr.Roll = (info1->Roll + info2->Roll + info3->Roll) / 3.0;
	avr.Pitch = (info1->Pitch + info2->Pitch + info3->Pitch) / 3.0;
	avr.Yaw = (info1->Yaw + info2->Yaw + info3->Yaw) / 3.0;
	for (int i = 0; i < SENSOR_AXES_NUM; ++i)
	{
		avr.Accel[i] = (info1->Accel[i] + info2->Accel[i] + info3->Accel[i]) / 3.0;
		avr.Gyro[i] = (info1->Gyro[i] + info2->Gyro[i] + info3->Gyro[i]) / 3.0;
	}
	return avr;
}

SensorInfo_t avr = { 0 };
SensorInfo_t avrZero = { 0 };

SensorInfo_t GetAverageSenrorValue(SensorInfo_t* infos, size_t infonum, bool* isReady)
{
	float readyCount = 0;
	double sumPitch = 0;
	double sumRoll = 0;
	double sumYaw = 0;
	double sumAcc[SENSOR_AXES_NUM] = { 0, 0, 0 };
	double sumGyro[SENSOR_AXES_NUM] = { 0, 0, 0 };
	bool isready[SENSOR_COUNT] = {0};	
	for (size_t i = 0; i < infonum; ++i)
	{
		auto info = infos[i];
		isready[i] = info.Accel[ACC_GYRO_Z_INDEX] > 0;
		*isReady = (*isReady) || isready[i];
		readyCount += (isready[i] == true ? 1 : 0);
		sumPitch += (isready[i] == true ? info.Pitch : 0);
		sumRoll += (isready[i] == true ? info.Roll : 0);
		sumYaw += (isready[i] == true ? info.Yaw : 0);
		for (auto j = 0; j < SENSOR_AXES_NUM; ++j)
		{
			sumAcc[j] += (isready[i] == true ? info.Accel[j] : 0);
			sumGyro[j] += (isready[i] == true ? info.Gyro[j] : 0);
		}
	}
	if (readyCount == 0)
		return avrZero;
	avr.Pitch = sumPitch / readyCount;
	avr.Roll = sumRoll / readyCount;
	avr.Yaw = sumYaw / readyCount;
	for (auto j = 0; j < SENSOR_AXES_NUM; ++j)
	{
		avr.Accel[j] = sumAcc[j] / readyCount;
		avr.Gyro[j] = sumGyro[j] / readyCount;
	}
	return avr;
}
