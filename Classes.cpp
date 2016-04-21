#pragma once
#include "stdafx.h"
#include "Classes.h"

int calc::GetDisplayBrightness()// текуща€ €ркость монитора
{
	HMONITOR hMonitor;
	DWORD number = 0;
	DWORD min = 1, max = 2, cur = 3;
	PHYSICAL_MONITOR PhMonitor;
	HANDLE hPhMonitor;
	LPSTR caps;
	BOOL i;
	const HWND hDesktop = GetDesktopWindow();
	hMonitor = MonitorFromWindow(hDesktop, MONITOR_DEFAULTTOPRIMARY);
	GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &number);
	GetPhysicalMonitorsFromHMONITOR(hMonitor, 1, &PhMonitor);
	hPhMonitor = PhMonitor.hPhysicalMonitor;
	i = GetMonitorBrightness(hPhMonitor, &min, &cur, &max);
	if (i)//если работает протокол ddc/ci
	{
		return cur;
		DestroyPhysicalMonitors(number, &PhMonitor);
	}
	else//если нет, лезть в wmi классы  
	{
		HRESULT hres;
		hres = CoInitializeEx(0, COINIT_MULTITHREADED);//именно такое значение надо, есть другое
		if (FAILED(hres))// но оно не работает в граф интерфейсах
		{
			return 1;
		}
		hres = CoInitializeSecurity(
			NULL,
			-1,
			NULL,
			NULL,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_NONE,
			NULL
			);


		if (FAILED(hres))
		{
			CoUninitialize();
			return 1;
		}
		IWbemLocator *pLoc = NULL;

		hres = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *)&pLoc);

		if (FAILED(hres))
		{
			CoUninitialize();
			return 1;
		}
		IWbemServices *pSvc = NULL;
		hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\WMI"),
			NULL,
			NULL,
			0,
			NULL,
			0,
			0,
			&pSvc
			);

		if (FAILED(hres))
		{
			pLoc->Release();
			CoUninitialize();
			return 1;
		}
		IEnumWbemClassObject* pEnumerator = NULL;
		hres = pSvc->ExecQuery(
			bstr_t("WQL"),
			bstr_t("SELECT * FROM WmiMonitorBrightness"),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator);

		if (FAILED(hres))
		{
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return 1;
		}
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;
		int ret;
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
				&pclsObj, &uReturn);

			if (0 == uReturn)
			{
				break;
			}

			VARIANT vtProp;

			hr = pclsObj->Get(L"CurrentBrightness", 0, &vtProp, 0, 0);
			ret = vtProp.intVal;
			VariantClear(&vtProp);
			pclsObj->Release();
		}
		pSvc->Release();
		pLoc->Release();
		pEnumerator->Release();
		CoUninitialize();
		DestroyPhysicalMonitors(number, &PhMonitor);
		return ret;
	}
}
double calc::GetCPUUsage()//получаем нагрузку на процессор
{
	typedef NTSTATUS(NTAPI* pfNtQuerySystemInformation)
		(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength,
		OUT PULONG ReturnLength OPTIONAL
		);

	static pfNtQuerySystemInformation NtQuerySystemInformation = NULL;

	if (NtQuerySystemInformation == NULL)
	{
		HMODULE ntDLL = ::GetModuleHandle(L"ntdll.dll");
		NtQuerySystemInformation = (pfNtQuerySystemInformation)GetProcAddress(ntDLL, "NtQuerySystemInformation");
	}

	static SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION previousInfo = { 0 };
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION currentInfo = { 0 };

	ULONG retsize;

	NtQuerySystemInformation(SystemProcessorPerformanceInformation, &currentInfo, sizeof(currentInfo), &retsize);

	double cpuUsage = -1;

	if (previousInfo.KernelTime.QuadPart != 0 || previousInfo.UserTime.QuadPart != 0)
	{
		cpuUsage = 100.0 - double(currentInfo.IdleTime.QuadPart - previousInfo.IdleTime.QuadPart) /
			double(currentInfo.KernelTime.QuadPart - previousInfo.KernelTime.QuadPart + currentInfo.UserTime.QuadPart - previousInfo.UserTime.QuadPart) * 100.0;
	}
	previousInfo = currentInfo;

	return cpuUsage;
}
BOOL calc::GetBatteryDeviceHandles(UINT *lpDeviceCount, HANDLE hBattDevices[])//хэндл батареи нужен дл€ последующего чтени€ информации о ней
{
	HDEVINFO                            hBattClassInfo;
	PSP_DEVICE_INTERFACE_DETAIL_DATA    lpInterfaceDetailData = NULL;
	SP_DEVICE_INTERFACE_DATA            spInterfaceData;
	DWORD                               dwInterfaceDetailDataSize;
	DWORD                               dwReqSize;
	DWORD                               dwErrCode;
	BOOL                                bStatus = TRUE;

	// get the device class handle for the battery ports in the system
	hBattClassInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVICE_BATTERY,
		NULL,
		NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
		);

	if (hBattClassInfo == INVALID_HANDLE_VALUE)
	{
		fwprintf(stderr, TEXT("SetupDiGetClassDevs failed with error code %d\n"), GetLastError());
		return FALSE;
	}
	else
	{
		// with the device class handle, we can now enumerate the paths to the battery ports

		UINT uiPortIndex = 0;
		spInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		for (;
			SetupDiEnumDeviceInterfaces(hBattClassInfo, 0, (LPGUID)&GUID_DEVICE_BATTERY, uiPortIndex, &spInterfaceData);
			uiPortIndex++)
		{
			// Now we have the interface data for the current battery port
			// To obtain its path, we have to make two calls into SetupDiGetDeviceInterfaceDetail. The first call
			// determines the buffer size required to hold the interface detail info

			bStatus = SetupDiGetDeviceInterfaceDetail(
				hBattClassInfo,        // Interface Device info handle
				&spInterfaceData,      // Interface data for the event class
				NULL,                  // Checking for buffer size
				0,                     // Checking for buffer size
				&dwReqSize,            // Buffer size required to get the detail data
				NULL                   // Checking for buffer size
				);

			// If this call returns ERROR_INSUFFICIENT_BUFFER, dwReqSize will be set to the required buffer size. Otherwise there is a problem.

			if (bStatus == FALSE)
			{
				dwErrCode = GetLastError();
				if (dwErrCode != ERROR_INSUFFICIENT_BUFFER)
				{
					fwprintf(stderr, TEXT("SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), dwErrCode);
					return FALSE;
				}
			}
			else
			{
				// if the call succeeded with a NULL buffer, there is some other problem
				fwprintf(stderr, TEXT("SetupDiGetDeviceInterfaceDetail failed on buffer size call. Return value was TRUE\n"));
				return FALSE;
			}

			// Allocate memory to get the interface detail data, which will contain the path to the battery port device
			dwInterfaceDetailDataSize = dwReqSize;
			lpInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_ZEROINIT, dwInterfaceDetailDataSize);
			if (lpInterfaceDetailData == NULL)
			{
				fwprintf(stderr, TEXT("Unable to allocate memory to get the interface detail data.\n"));
				return FALSE;
			}
			lpInterfaceDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

			bStatus = SetupDiGetDeviceInterfaceDetail(
				hBattClassInfo,                // Interface Device info handle
				&spInterfaceData,              // Interface data for the event class
				lpInterfaceDetailData,         // Interface detail data
				dwInterfaceDetailDataSize,     // Interface detail data size
				&dwReqSize,                    // Buffer size required to get the detail data
				NULL);                         // Interface device info

			if (bStatus == FALSE)
			{
				fwprintf(stderr, TEXT("Error in SetupDiGetDeviceInterfaceDetail failed with error: %d\n"), GetLastError());
				return FALSE;
			}

			if (hBattDevices != NULL)
			{
				// Now let's attempt to open the battery device
				hBattDevices[uiPortIndex] = CreateFile(
					lpInterfaceDetailData->DevicePath,       // device interface name
					GENERIC_READ | GENERIC_WRITE,            // dwDesiredAccess
					FILE_SHARE_READ | FILE_SHARE_WRITE,      // dwShareMode
					NULL,                                    // lpSecurityAttributes
					OPEN_EXISTING,                           // dwCreationDistribution
					0,                                       // dwFlagsAndAttributes
					NULL                                     // hTemplateFile
					);

				if (hBattDevices[uiPortIndex] == INVALID_HANDLE_VALUE)
				{
					fwprintf(stderr, TEXT("CreateFile failed with error: %d\n"), GetLastError());
					return FALSE;
				}
			}

			// we're done with the buffer, so free it
			LocalFree(lpInterfaceDetailData);
		} // END FOR
		dwErrCode = GetLastError();
		if (dwErrCode == ERROR_NO_MORE_ITEMS)
		{
			// the only correct way we exit the while loop is if SetupDiEnumDeviceInterfaces returns ERROR_NO_MORE_ITEMS
			*lpDeviceCount = uiPortIndex;
			return TRUE;
		}
		else
		{
			fwprintf(stderr, TEXT("SetupDiEnumDeviceInterfaces failed with error code %d\n"), dwErrCode);
			//return FALSE;
		}
	}
}
BOOL calc::GetBatteryStatus(HANDLE hDevice, BATTERY_STATUS* pbs)//получаем структуру статуса батареи
{
	if (hDevice == INVALID_HANDLE_VALUE) {
		fwprintf(stderr, TEXT("GetBatteryStatus, Bad battery driver handle, LastError: 0x%X"), GetLastError());
		return FALSE;
	}

	DWORD dwWait = 0;
	DWORD dwByteCount = 0;

	BATTERY_WAIT_STATUS bws = { 0 };

	if (DeviceIoControl(hDevice, IOCTL_BATTERY_QUERY_TAG,
		&dwWait, sizeof(dwWait),
		&bws.BatteryTag, sizeof(bws.BatteryTag),
		&dwByteCount, NULL))
	{
		dwByteCount = 0;

		return DeviceIoControl(
			hDevice,
			IOCTL_BATTERY_QUERY_STATUS,
			&bws, sizeof(BATTERY_WAIT_STATUS),
			pbs, sizeof(BATTERY_STATUS),
			&dwByteCount, NULL);
	}

	return FALSE;
}
int calc::GetBatteryCapacity()//значение текущей емкости
{
	UINT a;
	HANDLE batery;
	BATTERY_STATUS stat;
	GetBatteryDeviceHandles(&a, &batery);
	GetBatteryStatus(batery, &stat);
	return stat.Capacity;
}
int calc::GetBatteryPowerState()//флаг состо€ни€ батареи
{
	UINT a;
	HANDLE batery;
	BATTERY_STATUS stat;
	GetBatteryDeviceHandles(&a, &batery);
	GetBatteryStatus(batery, &stat);
	return stat.PowerState;
}
int calc::GetBatteryRate()//скорость разр€дки в текущий момент
{
	UINT a;
	HANDLE batery;
	BATTERY_STATUS stat;
	GetBatteryDeviceHandles(&a, &batery);
	GetBatteryStatus(batery, &stat);
	return stat.Rate;
}
void calc::fillmas()//непосредственныйй сбор значений о нагрузке и скорости разр€дки
{
	int CPUloadtemp[5];
	int Brightnesstemp[5];
	int Ratetemp[5];
	GetCPUUsage();
	Sleep(500);
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			CPUloadtemp[j] = (int)GetCPUUsage();
			Brightnesstemp[j] = GetDisplayBrightness();
			Ratetemp[j] = abs(GetBatteryRate());
			Sleep(1000);
			cout << Ratetemp[j] << endl;
			if ((abs(Ratetemp[j] - Rate[i - 1]) <(Rate[i - 1] * 0.1)) && i>0 && j == 0)
			{
				j--;
				continue;
			}
		}
		CPUload[i] = (CPUloadtemp[0] + CPUloadtemp[1] + CPUloadtemp[2] + CPUloadtemp[3] + CPUloadtemp[4]) / 5;
		Brightness[i] = (Brightnesstemp[0] + Brightnesstemp[1] + Brightnesstemp[2] + Brightnesstemp[3] + Brightnesstemp[4]) / 5;
		Rate[i] = (Ratetemp[0] + Ratetemp[1] + Ratetemp[2] + Ratetemp[3] + Ratetemp[4]) / 5;
		cout << "   calc " << i + 1;
		Sleep(10000);

	}
}
double calc::determinant(double **matrix)//решение системы уравнений
{
	double **copy;
	double result = 1.0;
	int sign = 1;
	bool key = true;

	copy = new double *[N];

	for (size_t i = 0; i < N; ++i)
	{
		copy[i] = new double[N];

		for (size_t j = 0; j < N; ++j)
			copy[i][j] = matrix[i][j];
	}

	for (size_t k = 0; k < N; ++k)
	{
		if (copy[k][k] == 0.0)
		{
			key = false;

			for (size_t i = k + 1; i < N; ++i)
			{
				if (copy[i][k] != 0.0)
				{
					key = true;

					std::swap(copy[k], copy[i]);

					sign *= -1;

					break;
				}
			}
		}

		if (!key)
			return 0.0;

		for (size_t i = k + 1; i < N; ++i)
		{
			double multi = copy[i][k] / copy[k][k];

			for (size_t j = 0; j < N; ++j)
				copy[i][j] -= multi * copy[k][j];
		}

		result *= copy[k][k];
	}

	for (size_t i = 0; i < N; ++i)
		delete[] copy[i];

	delete[] copy;

	return sign * result;
}
void calc::calculate()//еще немного системы уравнений 
	{
		double **matr;
		double x, xd[N];
		matr = new double *[N];
		for (size_t i = 0; i < N; i++)
		{
			matr[i] = new double[N];
		}
		for (size_t i = 0; i < N; i++)
		{
			if (i == 1)
			{
				for (size_t j = 0; j < N; j++)
					matr[j][i] = Brightness[j];
				continue;
			}
			if (i == 2)
			{
				for (size_t j = 0; j < N; j++)
					matr[j][i] = CPUload[j];
				continue;
			}
			for (size_t j = 0; j < N; j++)
				matr[j][i] = 1;
		}
		x = determinant(matr);
		for (int k = 0; k < N; k++)
		{
			for (size_t i = 0; i < N; i++)
			{
				if (i == k)
				{
					for (size_t j = 0; j < N; j++)
						matr[j][i] = Rate[j];
					continue;
				}
				if (i == 1)
				{
					for (size_t j = 0; j < N; j++)
						matr[j][i] = Brightness[j];
					continue;
				}
				if (i == 2)
				{
					for (size_t j = 0; j < N; j++)
						matr[j][i] = CPUload[j];
					continue;
				}
				for (size_t j = 0; j < N; j++)
					matr[j][i] = 1;
			}
			xd[k] = determinant(matr);
		}
		Zerocons = xd[0] / x;
		Scrcons = abs(xd[1] / x);
		CPUcons = xd[2] / x;
	}


void state::Getstat()
{
	SYSTEM_POWER_STATUS bat;
	GetSystemPowerStatus(&bat);//получение статуса батареи
	proc = (int)bat.BatteryLifePercent;
	time = bat.BatteryLifeTime;//осталось времени
	flag = bat.BatteryFlag;//флаг состо€ни€ батареи
	full = bat.BatteryFullLifeTime;//полное врем€
} 
int loging::GetCalcTime()//подсчет оставшегс€ времени
{
	double time;
	calc a;
	time = (a.GetBatteryCapacity()) / ((a.GetDisplayBrightness())*ScrconsOut + (a.GetCPUUsage())*CPUconsOut + ZeroconsOut)*3600;
	return (int)time;
}


loging::loging(double CPU, double scr, double zero)
{
	CPUconsOut = 0;
	ScrconsOut = 0;
	ZeroconsOut = 0;
	CPUconsIn = CPU;
	ScrconsIn = scr;
	ZeroconsIn = zero;
}
istream& operator>>(istream &send, loging &a){
	send >> a.CPUconsOut;
	send >> a.ScrconsOut;
	send >> a.ZeroconsOut;
	return send;
}
ostream& operator<<(ostream &send, loging &a)
{
	send << a.CPUconsIn << "\n" << a.ScrconsIn << "\n" << a.ZeroconsIn << "\n";

	return send;
}
void loging::write()//запись в файл
{
	ifstream fileOut("logs.txt");
	fileOut >> *this;
	fileOut.close();
	ofstream fileIn("logs.txt");
	if (CPUconsOut)
	{
		CPUconsIn = (CPUconsIn + CPUconsOut) / 2;
		ScrconsIn = (ScrconsIn + ScrconsOut) / 2;
		ZeroconsIn = (ZeroconsIn + ZeroconsOut) / 2;
		fileIn << *this;
		fileIn.close();
	}
	else
	{
		fileIn << *this;
		fileIn.close();
	}
}
BOOL loging::read()//чтение из файла
{
	ifstream fileOut("logs.txt");
	fileOut >> *this;
	fileOut.close();
	if (!CPUconsOut)
	{
		return FALSE;
	}
	return TRUE;
}