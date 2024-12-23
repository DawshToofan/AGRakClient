#include "main.h"

void RunCommand(char* szCMD)
{
	if (szCMD[0] == 0x00)
		return;

	if (szCMD[0] == '/') sendServerCommand(szCMD);
	else sendChat(szCMD);
}
