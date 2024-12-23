struct stServer
{
	char szAddr[256];
	int iPort;
};

struct stSettings
{
	struct stServer server;
	int iClassID;
	int iUpdateStats;
	char szClientVersion[20];
	bool isConsole;
	bool haveParentProcess;
	bool bHaveUser;
	char userName[32];
	char userPassword[128];
};
extern struct stSettings settings;

int LoadSettings();
