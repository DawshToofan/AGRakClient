#include "main.h"
#include <string>

stSettings settings {};
TiXmlDocument xmlSettings;

int LoadSettings()
{
	settings.iClassID = 19;
	settings.iUpdateStats = 1;
	strcpy(settings.server.szAddr, "185.112.33.99");
	settings.server.iPort = 7777;

	if (!settings.haveParentProcess)
	{
		// load xml
		if (!xmlSettings.LoadFile("AGRakClient.xml"))
		{
			std::string error = "Failed to load the config file.\n\nError Description:\n";
			error += xmlSettings.ErrorDesc();
			MessageBox(NULL, error.c_str(), "Error", MB_ICONERROR);
			ExitProcess(0);
		}

		TiXmlElement* settingsElement = xmlSettings.FirstChildElement("settings");
		int consoleMode = 0;
		settingsElement->QueryIntAttribute("consoleMode", (int*)&consoleMode);
		if (consoleMode) settings.isConsole = true;

		TiXmlElement* userElement = xmlSettings.FirstChildElement("user");
		if (userElement)
		{
			settings.bHaveUser = true;
			strcpy(settings.userName, (char*)userElement->Attribute("username"));
			strcpy(settings.userPassword, (char*)userElement->Attribute("password"));
		}

		xmlSettings.Clear();
		if (!settings.bHaveUser)
		{
			MessageBox(NULL, "Failed to load users info", "Error", MB_ICONERROR);
			ExitProcess(0);
		}
	}
	return 1;
}
