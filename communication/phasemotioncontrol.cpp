
#include "stdafx.h"

#include <math.h>

#include "phasemotioncontrol.h"
#include "../Sixdofdll2010.h"

#include "../config/inihelper.h"
#include "../control/pid.h"

#define RANGE_V(x, min, max)    (((x)<(min) ? (min) : ( (x)>(max) ? (max):(x) )))
#define ASSERT_INDEX(index) 	if (index < 0 && index >= AXES_COUNT) return;

#define IS_NAVI_PID_READ_FILE 1

#if IS_BIG_MOTION
#define MOTION_P 0.0001
#define MOTION_I 0.0000001
#define MOTION_D 0.0
#define MAX_VEL  4.0

#define RISE_MOTION_P 0.00004
#define RISE_MOTION_I 0.0000002
#define RISE_MOTION_D 0.0 
#define RISE_MAX_VEL  0.3 

#define NAVI_MOTION_P 0.0008
#define NAVI_MOTION_I 0.00012
#define NAVI_MOTION_D 0.00004
#define NAVI_MAX_VEL  4.0
#else
#define MOTION_P 0.00055
#define MOTION_I 0.000002
#define MOTION_D 0.0
#define MAX_VEL  6.0

#define RISE_MOTION_P 0.00004
#define RISE_MOTION_I 0.0000002
#define RISE_MOTION_D 0.0
#define RISE_MAX_VEL  0.75

#define NAVI_MOTION_P 0.001
#define NAVI_MOTION_I 0.00016
#define NAVI_MOTION_D 0.00006
#define NAVI_MAX_VEL  6.0
#endif 

PID_Type MotionLocationPidControler[AXES_COUNT] = 
{
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL },
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL },
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL },
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL },
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL },
	{ MOTION_P, MOTION_I, MOTION_D, -MAX_VEL, MAX_VEL }
};

PID_Type MotionRisePidControler[AXES_COUNT] = 
{
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL },
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL },
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL },
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL },
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL },
	{ RISE_MOTION_P, RISE_MOTION_I, RISE_MOTION_D, -RISE_MAX_VEL, RISE_MAX_VEL }
};

PID_Type MotionNavigationPidControler[AXES_COUNT] = 
{
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL },
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL },
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL },
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL },
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL },
	{ NAVI_MOTION_P, NAVI_MOTION_I, NAVI_MOTION_D, -NAVI_MAX_VEL, NAVI_MAX_VEL }
};

static double last_pulse[AXES_COUNT] = { 0, 0, 0, 0, 0, 0 };
static double now_vel[AXES_COUNT] = { 0, 0, 0, 0, 0, 0 };
static double last_str_vel[AXES_COUNT] = { 0, 0, 0, 0, 0, 0 };
static double speed_scale = 100;
static ULONG last_pulses[AXES_COUNT] = {0};

PhaseMotionControl::PhaseMotionControl()
{
	disposed = false; 
	for (int i = 0; i < AXES_COUNT; ++i)
	{
		NowPluse[i] = 0;
		pos[i] = 0;
	}
	AvrPulse = 0;
	naviP = NAVI_MOTION_P;
	naviI = NAVI_MOTION_I;
	naviD = NAVI_MOTION_D;
#if IS_NAVI_PID_READ_FILE
	naviP = config::ParseDoubleJsonFromFile(JSON_PARA_FILE_NAME, JSON_NAVI_P_KEY);
	naviI = config::ParseDoubleJsonFromFile(JSON_PARA_FILE_NAME, JSON_NAVI_I_KEY);
	naviD = config::ParseDoubleJsonFromFile(JSON_PARA_FILE_NAME, JSON_NAVI_D_KEY);
	for (int i = 0;i < AXES_COUNT;++i)
	{
		MotionNavigationPidControler->Real_P = naviP;
		MotionNavigationPidControler->Real_I = naviI;
		MotionNavigationPidControler->Real_D = naviD;
	}
#endif
	InitCard();
}

PhaseMotionControl::~PhaseMotionControl()
{
	if (this->disposed == false)
	{
		
	}
}

bool PhaseMotionControl::InitCard()
{
	return sixdofDioAndCount.Init();
}

void PhaseMotionControl::Close(SixDofPlatformStatus laststatus)
{
	RenewNowPulse();
	if (lockobj.try_lock())
	{
		config::RecordStatusAndPulse(nullptr, laststatus, NowPluse);
		lockobj.unlock();
	}
	this->disposed = true;
}

void PhaseMotionControl::SetMotionVeloctySingle(int index, double velocity)
{
	ASSERT_INDEX(index);
	velocity = RANGE_V(velocity, -MAX_VEL, MAX_VEL);
	sixdofDioAndCount.SetMotionVel(index, velocity);
}

void PhaseMotionControl::SetMotionVelocty(double* velocity, int axexnum)
{
	sixdofDioAndCount.SetMotionVel(velocity);
}

bool PhaseMotionControl::ServoAllOnOff(bool isOn)
{
#if IS_BIG_MOTION
	sixdofDioAndCount.EnableAllMotor(isOn);
#else
	sixdofDioAndCount.EnableAllMotor(isOn);
#endif
	return true;
}

void PhaseMotionControl::SingleUp(int index)
{
	UnlockServo(index);
	SetMotionVeloctySingle(index, RISE_VEL);
}

void PhaseMotionControl::SingleDown(int index)
{
	UnlockServo(index);
	SetMotionVeloctySingle(index, -DOWN_VEL);
}

void PhaseMotionControl::AllTestUp()
{
	double vel = RISE_VEL;
	double vels[AXES_COUNT] = {vel, vel, vel, vel, vel, vel};
	UnlockServo();
	SetMotionVelocty(vels, AXES_COUNT);
}

void PhaseMotionControl::AllTestDown()
{
	double vel = -DOWN_VEL;
	double vels[AXES_COUNT] = {vel, vel, vel, vel, vel, vel};
	UnlockServo();
	SetMotionVelocty(vels, AXES_COUNT);
}

bool PhaseMotionControl::ResetStatus()
{
	if (lockobj.try_lock())
	{
		sixdofDioAndCount.ClearPulseCount();
		for (int i = 0;i < AXES_COUNT;++i)
		{
			NowPluse[i] = 0;
		}
		AvrPulse = 0;
		lockobj.unlock();
	}
	return true;
}

void PhaseMotionControl::EnableServo()
{
	bool bits[AXES_COUNT] = {MOTION_ENABLE_LEVEL};
	for (int i = 0;i < AXES_COUNT;++i)
	{
		bits[i] = MOTION_ENABLE_LEVEL;
	}
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionEnableBit(bits);
#else
	sixdofDioAndCount.SetMotionEnableBit(bits);
#endif
	
}

void PhaseMotionControl::LockServo()
{
	bool bits[AXES_COUNT] = {MOTION_LOCK_LEVEL};
	for (int i = 0;i < AXES_COUNT;++i)
	{
		bits[i] = MOTION_LOCK_LEVEL;
	}
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionLockBit(bits);
#else
	sixdofDioAndCount.SetMotionEnableBit(bits);
	sixdofDioAndCount.SetMotionLockBit(bits);
#endif
	isrising = false;
	isfalling = false;
	enableMove = false;
}

void PhaseMotionControl::UnlockServo()
{
	bool bits[AXES_COUNT] = {!MOTION_LOCK_LEVEL};
	for (int i = 0;i < AXES_COUNT;++i)
	{
		bits[i] = !MOTION_LOCK_LEVEL;
	}
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionLockBit(bits);
#else
	sixdofDioAndCount.SetMotionEnableBit(bits);
	sixdofDioAndCount.SetMotionLockBit(bits);
#endif
}

void PhaseMotionControl::EnableServo(int index)
{
	ASSERT_INDEX(index);
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionLockBit(index, MOTION_ENABLE_LEVEL);
#else
	sixdofDioAndCount.SetMotionEnableBit(index, MOTION_ENABLE_LEVEL);
	sixdofDioAndCount.SetMotionLockBit(index, MOTION_ENABLE_LEVEL);
#endif
}

void PhaseMotionControl::LockServo(int index)
{
	ASSERT_INDEX(index);
	enableMove = false;
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionLockBit(index, MOTION_LOCK_LEVEL);
#else
	sixdofDioAndCount.SetMotionEnableBit(index, MOTION_LOCK_LEVEL);
	sixdofDioAndCount.SetMotionLockBit(index, MOTION_LOCK_LEVEL);
#endif
}

void PhaseMotionControl::UnlockServo(int index)
{
	ASSERT_INDEX(index);
#if IS_BIG_MOTION
	sixdofDioAndCount.SetMotionLockBit(index, !MOTION_LOCK_LEVEL);
#else
	sixdofDioAndCount.SetMotionEnableBit(index, !MOTION_LOCK_LEVEL);
	sixdofDioAndCount.SetMotionLockBit(index, !MOTION_LOCK_LEVEL);
#endif
}

void PhaseMotionControl::Rise()
{
	LockServo();
	for (int i = 0;i < AXES_COUNT;++i)
	{
		MyPidParaInit(MotionRisePidControler);
	}
	Sleep(10);
	isrising = true;
	Sleep(10);
	isfalling = false;
	enableMove = false;
	UnlockServo();
}

void PhaseMotionControl::Down()
{
#if IS_PID_DOWN
	LockServo();
	Sleep(10);
	isfalling = true;
	isrising = false;
	enableMove = false;
	UnlockServo();
#else
	AllTestDown();
#endif	
}

void PhaseMotionControl::PidCsp(double * pulse)
{
	if (lockobj.try_lock())
	{	
		for (auto i = 0; i < AXES_COUNT; ++i)
		{
			pulse[i] = pulse[i] + MIDDLE_POS;
			pulse[i] = RANGE_V(pulse[i], HALF_RPM_POS, MAX_POS - HALF_RPM_POS);
			now_vel[i] = MyDeltaPID_Real(&MotionLocationPidControler[i], \
				NowPluse[i], pulse[i]);
		}
		lockobj.unlock();
	}
	SetMotionVelocty(now_vel, AXES_COUNT);
}

void PhaseMotionControl::NaviPidCsp(double * pulse)
{
	if (lockobj.try_lock())
	{	
		for (auto i = 0; i < AXES_COUNT; ++i)
		{
			pulse[i] = pulse[i] + MIDDLE_POS;
			pulse[i] = RANGE_V(pulse[i], HALF_RPM_POS, MAX_POS - HALF_RPM_POS);
			now_vel[i] = MyDeltaPID_Real(&MotionNavigationPidControler[i], \
				NowPluse[i], pulse[i]);
		}
		lockobj.unlock();
	}
	SetMotionVelocty(now_vel, AXES_COUNT);
}

void PhaseMotionControl::SlowPidCsp(double * pulse)
{
	if (lockobj.try_lock())
	{	
		for (auto i = 0; i < AXES_COUNT; ++i)
		{
			pulse[i] = pulse[i] + MIDDLE_POS;
			pulse[i] = RANGE_V(pulse[i], 0, MAX_POS);
			now_vel[i] = MyDeltaPID_Real(&MotionRisePidControler[i], \
				NowPluse[i], pulse[i]);
		}
		lockobj.unlock();
	}

	SetMotionVelocty(now_vel, AXES_COUNT);
}

double* PhaseMotionControl::GetMotionNowEncoderVelocity()
{
	auto pulses = sixdofDioAndCount.ReadPulseCount();
	if (lockobj.try_lock())
	{	
		for(auto i = 0; i < AXES_COUNT; ++i)
		{
			NowPluse[i] = pulses[i];
		}
		lockobj.unlock();
	}
	return NowPluse;
}

// x y z roll pitch yaw
double* PhaseMotionControl::GetNowPoseFromLength()
{
	if (lockobj.try_lock())
	{
		memcpy(polelenthmm, NowPluse, sizeof(double) * AXES_COUNT);
		lockobj.unlock();
	}
	for (int i = 0;i < AXES_COUNT;++i)
	{
		polelenthmm[i] = (polelenthmm[i] - MIDDLE_POS) * PULSE_COUNT_TO_MM_SCALE; 
	}
	auto pose = FromLengthToPose(polelenthmm);
	memcpy(posefromlength, pose, sizeof(double) * AXES_COUNT);
	return posefromlength;
}

double* PhaseMotionControl::GetNowPoleLength()
{
	if (lockobj.try_lock())
	{
		memcpy(polelenthmm, NowPluse, sizeof(double) * AXES_COUNT);
		lockobj.unlock();
	}
	for (int i = 0;i < AXES_COUNT;++i)
	{
		polelenthmm[i] = (polelenthmm[i] - MIDDLE_POS) * PULSE_COUNT_TO_MM_SCALE; 
	}
	return polelenthmm;
}

double PhaseMotionControl::GetMotionAveragePulse()
{
	double pulse_num = 0;
	if (lockobj.try_lock())
	{
		for(auto i = 0; i < AXES_COUNT; ++i)
		{
			auto actual_pos = NowPluse[i];
			pulse_num += actual_pos;
		}
		lockobj.unlock();
	}

	double avr_pulse = pulse_num / (double)AXES_COUNT;
	AvrPulse = avr_pulse;
	return avr_pulse;
}

void PhaseMotionControl::RenewNowPulse()
{
	if(lockobj.try_lock())
	{
		auto pulses = sixdofDioAndCount.ReadPulseCount();
#if IS_PULSE_CARD_REVERSE
		NowPluse[0] = pulses[3];
		NowPluse[1] = pulses[4];
		NowPluse[2] = pulses[5];
		NowPluse[3] = pulses[0];
		NowPluse[4] = pulses[1];
		NowPluse[5] = pulses[2];
#else
		for (int i = 0; i < AXES_COUNT ;++i)
		{
			NowPluse[i] = pulses[i];
		}
#endif
		lockobj.unlock();
	}
}

void PhaseMotionControl::SetDDAPositions(double* positions)
{
	//this->enableMove = true;
	pulses.push_back(positions);
}

int PhaseMotionControl::GetDDAPositionsCount()
{
	return pulses.size();
}

void PhaseMotionControl::DDAControlThread()
{
	while (true)
	{
		double eps = AXES_COUNT * AXES_COUNT;
		if (isrising == true)
		{
			RenewNowPulse();
			double pulses[AXES_COUNT] = {0};
			SlowPidCsp(pulses);
			for (int i = 0; i < AXES_COUNT; ++i)
			{
				if (abs(NowPluse[i] - MIDDLE_POS) <= eps)
				{
					ServoSingleStop(i);
					LockServo(i);
				}
			}
		}
		if(isfalling == true)
		{
			RenewNowPulse();
			double pulses[AXES_COUNT] = {-MIDDLE_POS,-MIDDLE_POS,-MIDDLE_POS,-MIDDLE_POS,-MIDDLE_POS,-MIDDLE_POS};
			SlowPidCsp(pulses);
			for (int i = 0; i < AXES_COUNT; ++i)
			{
				if (abs(NowPluse[i] - 0) <= 1)
				{
					//ServoSingleStop(i);
					//LockServo(i);
				}
			}
		}
		if(enableMove == true)
		{
			RenewNowPulse();
			double pulses[AXES_COUNT] = {0};
			double sum = 0;
			memcpy(pulses, pos, sizeof(double) * AXES_COUNT);
			SlowPidCsp(pulses);
			for (int i = 0; i < AXES_COUNT; ++i)
			{
				sum += abs(pulses[i] - NowPluse[i]); 
			}
			if (sum <= eps * AXES_COUNT)
			{
				ServoStop();
			}	
		}
		ReadAllSwitchStatus();
		Sleep(DDA_CONTROL_THREAD_DELAY);
	}
}

void PhaseMotionControl::MoveToZeroPulseNumber()
{
	for (int i = 0;i < AXES_COUNT;++i)
	{
		MyPidParaInit(&MotionRisePidControler[i]);
	}
	Sleep(10);
	isrising = true;
	UnlockServo();
}

void PhaseMotionControl::PidControllerInit()
{
	for (int i = 0;i < AXES_COUNT;++i)
	{
		MyPidParaInit(&MotionLocationPidControler[i]);
		MyPidParaInit(&MotionNavigationPidControler[i]);
	}
}

bool PhaseMotionControl::ServoStop()
{
	StopRiseDownMove();
	LockServo();
	return true;
}

bool PhaseMotionControl::ServoSingleStop(int index)
{
	enableMove = false;
	double vel = 0;
	SetMotionVeloctySingle(index, vel);
	return true;
}

void PhaseMotionControl::StopRiseDownMove()
{
	enableMove = false;
	isrising = false;
	isfalling = false;
	pulses.clear();
	double vel[AXES_COUNT];
	memset(vel, 0, sizeof(double) * AXES_COUNT);
	SetMotionVelocty(vel, AXES_COUNT);
}

bool PhaseMotionControl::IsAllAtBottom()
{
	for (auto i = 0; i < AXES_COUNT; ++i)
	{
		auto isAtBottom = IsAtBottoms[i];
		if (isAtBottom == false)
		{
			return false;
		}
	}
	return true;
}

void PhaseMotionControl::ReadAllSwitchStatus()
{

#if IS_BIG_MOTION

#endif
	for (int i = 0;i < AXES_COUNT;++i)
	{
		
#if IS_BIG_MOTION
		sixdofDioAndCount.ReadKBit(i, &IsAtBottoms[i]);
#else
		sixdofDioAndCount.ReadKBit(i, &IsAtBottoms[i]);
#endif
		if (IsAtBottoms[i] == true && isrising == false)
		{
			ServoSingleStop(i);
			LockServo(i);
		}
	}
}

bool PhaseMotionControl::CheckStatus(SixDofPlatformStatus& status)
{
	char* str = SixDofStatusText[status];
	double pulse = 0;
	ReadAllSwitchStatus();
	switch (status)
	{
	case SIXDOF_STATUS_BOTTOM:
		break;
	case SIXDOF_STATUS_READY:
		break;
	case SIXDOF_STATUS_MIDDLE:
		break;
	case SIXDOF_STATUS_RUN:
		break;
	case SIXDOF_STATUS_ISRISING:
		pulse = GetMotionAveragePulse();
		if (pulse >= (RISE_R - 1) * PULSE_COUNT_RPM)
		{
			status = SIXDOF_STATUS_READY;
		}			
		break;
	case SIXDOF_STATUS_ISFALLING:			
		if (IsAllAtBottom() == true)
		{
			status = SIXDOF_STATUS_BOTTOM;
		}
		break;
	default:
		break;
	}
	str = SixDofStatusText[status];
	Status = status;
	return true;
}

bool PhaseMotionControl::PowerOnSelfTest(SixDofPlatformStatus laststatus, double * lastpulse)
{
	if(isSelfTest == true)
		return false;
	switch (laststatus)
	{
	case SIXDOF_STATUS_BOTTOM:
		//AllTestDown();
		break;
	case SIXDOF_STATUS_READY:
		//下降
		AllTestDown();
		break;
	case SIXDOF_STATUS_MIDDLE:
		AllTestDown();
		break;
	case SIXDOF_STATUS_RUN:
		//回中
		//MoveToLocation(lastpulse, AXES_COUNT, false);
		//下降
		AllTestDown();
		break;
	case SIXDOF_STATUS_ISRISING:
		//下降
		AllTestDown();
		break;
	case SIXDOF_STATUS_ISFALLING:
		//下降
		AllTestDown();
		break;
	default:
		break;
	}
	isSelfTest = true;
	return true;
}

void PhaseMotionControl::Test()
{
#if IS_BIG_MOTION
	sixdofDioAndCount.Test();
#else
	sixdofDioAndCount.Test();
#endif
}

void PhaseMotionControl::PowerStart(bool isStart)
{
#if IS_BIG_MOTION
	sixdofDioAndCount.Start(isStart);
#else
	//sixdofDioAndCount.Start(isStart);
#endif

}

void PhaseMotionControl::PowerCheckStart(bool isStart)
{
#if IS_BIG_MOTION
	sixdofDioAndCount.CheckStart(isStart);
#else
	//sixdofDioAndCount.CheckStart(isStart);
#endif
}
