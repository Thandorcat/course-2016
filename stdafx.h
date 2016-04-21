#include <wtypes.h>
#include <windows.h>
#include <PhysicalMonitorEnumerationAPI.h>
#include <HighLevelMonitorConfigurationAPI.h>
#include <LowLevelMonitorConfigurationAPI.h>
#include <initguid.h>
#include <stdio.h>
#include <wchar.h>
#include <Powerbase.h>
#include <Setupapi.h>
#include <BatClass.h>
#include <iostream>
#include <locale>
#include <iomanip>
#include <cmath>
#include <objbase.h>
#include <Winternl.h>
#include <fstream>
#include <Wbemidl.h>
#include <comdef.h>
#include <Ntddvdeo.h>
#include <Winbase.h>
#include <fstream>

#define _WIN32_WINNT 0x0601
#define _WIN32_DCOM
#define N 3

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Kernel32.lib")
using namespace std;