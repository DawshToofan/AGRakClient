#include <string>  // For std::to_string

#include "main.h"

stRakClient client{};
FILE* flLog = NULL;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	srand((unsigned int)GetTickCount64());

	if (isSubstring(lpCmdLine, "--from-py"))
	{
		settings.isConsole = true;
		settings.haveParentProcess = true;
	}

	// load up settings
	if (!LoadSettings())
	{
		Log("Failed to load settings");
		getchar();
		return 0;
	}

	if (settings.isConsole)
	{
		if (!settings.haveParentProcess) SetUpConsole();
		else
		{
			std::cin >> settings.userName;
			std::cin >> settings.userPassword;
		}
	}
	else
	{
		SetUpWindow(hInstance);
		Sleep(500); // wait a bit for the dialog to create
	}

	auto* rakClient = client.rakClient = RakNetworkFactory::GetRakClientInterface();
	if (rakClient == NULL)
		return 0;
	rakClient->SetMTUSize(576);
	RegisterRPCs();

	char szInfo[1024];
	int whileCounter = 20;
	while (true)
	{
		if (!whileCounter)
		{
			sprintf_s(szInfo, sizeof szInfo, "AGRakClient %s - Username: %s", RAKSAMP_VERSION, settings.userName);
			if (client.logined)
			{
				strcat(szInfo, " (");
				strcat(szInfo, std::to_string(client.playerid).c_str());
				strcat(szInfo, ")");
			}
			if (settings.isConsole) SetConsoleTitle(szInfo);
			else SetWindowText(hwnd, szInfo);
			szInfo[0] = '\0';
		}

		UpdateNetwork();

		if (client.sleepTime)
		{
			if (GetTickCount64() > client.sleepTime)
			{
				client.connectionRequested = false;
				client.sleepTime = 0;
			}
		}
		else
		{
			if (!client.connectionRequested)
			{
				sampConnect();
				client.connectionRequested = true;
			}

			if (client.areWeConnected && client.gameInited)
			{
				if (!client.spawned)
				{
					if (!client.callSampRequestClass)
					{
						sampRequestClass();
						client.callSampRequestClass = 1;
					}
				}
				else
				{
					if (!client.isSpectating) onFootUpdateAtNormalPos();
					else spectatorUpdate();

					const auto tickCount = GetTickCount64();
					if (tickCount - client.lastStatsUpdate >= 1000 || client.money != client.lastMoney || client.drunkLevel != client.lastDrunkLevel)
					{
						RakNet::BitStream bsSend;
						bsSend.Write((BYTE)ID_STATS_UPDATE);
						client.drunkLevel -= (rand() % 90 + 20);
						if (client.drunkLevel < 0) client.drunkLevel = 0;
						bsSend.Write(client.money);
						bsSend.Write(client.drunkLevel);
						client.rakClient->Send(&bsSend, HIGH_PRIORITY, RELIABLE, 0);
						client.lastMoney = client.money;
						client.lastDrunkLevel = client.drunkLevel;
						client.lastStatsUpdate = tickCount;
					}
				}
			}
		}
		
		if (!whileCounter) whileCounter = 20;
		else --whileCounter;

		Sleep(30);
	}

	if (flLog != NULL)
	{
		fclose(flLog);
		flLog = NULL;
	}

	return 0;
}

void Log(const char* fmt, ...)
{
	char buffer[512];
	memset(buffer, 0, 512);

	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, 512, fmt, args);
	va_end(args);

	if (settings.haveParentProcess) std::cout << buffer << std::endl;
	else
	{
		if (flLog == NULL)
		{
			flLog = fopen("AGRakClient.log", "a");

			if (flLog == NULL)
				return;
		}

		SYSTEMTIME time;
		GetLocalTime(&time);

		fprintf(flLog, "[%02d:%02d:%02d.%03d] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

		fprintf(flLog, buffer);

		if (settings.isConsole) printf(buffer);
		else
		{
			LPTSTR tbuf = new TCHAR[512];
			wsprintf(tbuf, buffer);

			int lbCount = SendMessage(loghwnd, LB_GETCOUNT, 0, 0);
			WPARAM idx = SendMessage(loghwnd, LB_ADDSTRING, 0, (LPARAM)tbuf);

			SendMessage(loghwnd, LB_SETCURSEL, lbCount - 1, 0);
			SendMessage(loghwnd, LB_SETTOPINDEX, idx, 0);
		}
		fprintf(flLog, "\n");
		if (settings.isConsole) printf("\n");
		fflush(flLog);
	}
}

bool isSubstring(const char* input, const char* substr) {
	while (*input != '\0') {
		const char* p1 = input;
		const char* p2 = substr;
		while (*p1 != '\0' && *p2 != '\0' && *p1 == *p2) {
			++p1;
			++p2;
		}
		if (*p2 == '\0') {
			return true; // Found a match
		}
		++input;
	}
	return false; // No match found
}
