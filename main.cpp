#include "stdafx.h"
#include "Classes.h"

void measure()
{
	calc a;
	a.fillmas();
	a.calculate();
	loging x(a.CPUcons, a.Scrcons, a.Zerocons);
	x.write();
	printf("Done!\n");
}

int main()
{
	while (1)
	{
		state a;
		loging log;
		a.Getstat();
		log.read();
		int calctm = log.GetCalcTime();
		system("cls");
		printf("Curent charge is: %d%, flag: %d\n", a.proc, a.flag);
		if (a.flag > 7)
		{
			calc x;
			int cc, fc, sc, tm;
			printf("Battery is charging\n");
			cc = x.GetBatteryCapacity();
			sc = x.GetBatteryRate();
			fc = cc / a.proc * 100;
			tm = ((fc - cc) * 3600) / sc;
			printf("Remaining time is: %dh%dm\n", (tm / 3600), ((tm - (int)(tm / 3600) * 3600) / 60));
		}
		else
		{
			printf("Remaining system time is: %dh%dm\n", (a.time / 3600), ((a.time - (int)(a.time / 3600) * 3600) / 60));
			printf("Remaining logs time is: %dh%dm\n", (calctm / 3600), ((calctm - (int)(calctm / 3600) * 3600) / 60));
			if (a.proc<100 && a.proc>69)
				printf("Level is High!\n");
			if (a.proc<70 && a.proc>30)
				printf("Level is Normal!\n");
			if (a.proc<31 && a.proc>15)
				printf("Level is Low!\n");
			if (a.proc<16 && a.proc>0)
				printf("Level is Critical!\n");
			if (a.flag > 7)
			{
				calc x;
				int cc, fc, sc, tm;
				printf("Battery is charging\n");
				cc = x.GetBatteryCapacity();
				sc = x.GetBatteryRate();
				fc = cc / a.proc * 100;
				tm = ((fc - cc) * 3600) / sc;
				printf("Remaining time is: %dh%dm\n", (tm / 3600), ((tm - (int)(tm / 3600) * 3600) / 60));
			}
			if (a.proc == 100){ printf("Battery is Full!\n"); }
		}
		Sleep(1000);
	}
}