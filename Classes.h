#pragma once
class calc
{
public:
	int CPUload[N];
	int Brightness[N];
	int Rate[N];
	int Capacity;
	double CPUcons;
	double Scrcons;
	double Zerocons;
	int GetDisplayBrightness();
	double GetCPUUsage();
	BOOL GetBatteryDeviceHandles(UINT *lpDeviceCount, HANDLE hBattDevices[]);
	BOOL GetBatteryStatus(HANDLE hDevice, BATTERY_STATUS* pbs);
	int GetBatteryCapacity();
	int GetBatteryPowerState();
	int GetBatteryRate();
	void fillmas();
	double determinant(double **matrix);
	void calculate();
	
};
class state
{
public:
	int flag;
	int proc;
	int time;
	int full;
	void Getstat();
};
class loging
{
	int i;
	double CPUconsIn;
	double ScrconsIn;
	double ZeroconsIn;
	double CPUconsOut;
	double ScrconsOut;
	double ZeroconsOut;
public:
	loging(){};
	loging(double CPU, double scr, double zero);
	int GetCalcTime();
	void write();
	BOOL read();
	friend istream& operator>>(istream &send, loging &a);
	friend ostream& operator<<(ostream &send, loging &a);
};
