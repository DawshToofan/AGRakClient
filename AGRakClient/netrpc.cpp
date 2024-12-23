#include <RakClient.h>
#include "main.h"
#include <string>

struct stSAMPDialog sampDialog;

int pinTextID[11];

void InitGame(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsInitGame((unsigned char*)Data, (iBitLength / 8) + 1, false);

	PLAYERID MyPlayerID;
	bool bLanMode, bStuntBonus;
	BYTE byteVehicleModels[212];

	bool m_bZoneNames, m_bUseCJWalk, m_bAllowWeapons, m_bLimitGlobalChatRadius;
	float m_fGlobalChatRadius, m_fNameTagDrawDistance;
	bool m_bDisableEnterExits, m_bNameTagLOS, m_bManualVehicleEngineAndLight;
	bool m_bShowPlayerTags;
	int m_iShowPlayerMarkers;
	BYTE m_byteWorldTime, m_byteWeather;
	float m_fGravity;
	int m_iDeathDropMoney;
	bool m_bInstagib;
	int iSpawnsAvailable;
	int iNetModeNormalOnfootSendRate, iNetModeNormalIncarSendRate, iNetModeFiringSendRate, iNetModeSendMultiplier;
	BYTE m_bLagCompensation;

	bsInitGame.ReadCompressed(m_bZoneNames);
	bsInitGame.ReadCompressed(m_bUseCJWalk);
	bsInitGame.ReadCompressed(m_bAllowWeapons);
	bsInitGame.ReadCompressed(m_bLimitGlobalChatRadius);
	bsInitGame.Read(m_fGlobalChatRadius);
	bsInitGame.ReadCompressed(bStuntBonus);
	bsInitGame.Read(m_fNameTagDrawDistance);
	bsInitGame.ReadCompressed(m_bDisableEnterExits);
	bsInitGame.ReadCompressed(m_bNameTagLOS);
	bsInitGame.ReadCompressed(m_bManualVehicleEngineAndLight); // 
	bsInitGame.Read(iSpawnsAvailable);
	bsInitGame.Read(MyPlayerID);
	bsInitGame.ReadCompressed(m_bShowPlayerTags);
	bsInitGame.Read(m_iShowPlayerMarkers);
	bsInitGame.Read(m_byteWorldTime);
	bsInitGame.Read(m_byteWeather);
	bsInitGame.Read(m_fGravity);
	bsInitGame.ReadCompressed(bLanMode);
	bsInitGame.Read(m_iDeathDropMoney);
	bsInitGame.ReadCompressed(m_bInstagib);

	// Server's send rate restrictions
	bsInitGame.Read(iNetModeNormalOnfootSendRate);
	bsInitGame.Read(iNetModeNormalIncarSendRate);
	bsInitGame.Read(iNetModeFiringSendRate);
	bsInitGame.Read(iNetModeSendMultiplier);

	bsInitGame.Read(m_bLagCompensation);

	BYTE unk;
	bsInitGame.Read(unk);
	bsInitGame.Read(unk);
	bsInitGame.Read(unk);

	BYTE byteStrLen;
	bsInitGame.Read(byteStrLen);
	char g_szHostName[256];
	if (byteStrLen)
	{
		memset(g_szHostName, 0, sizeof(g_szHostName));
		bsInitGame.Read(g_szHostName, byteStrLen);
	}
	g_szHostName[byteStrLen] = '\0';

	bsInitGame.Read((char*)&byteVehicleModels[0], 212);

	Log("[%s] Connected to %.64s", client.nickName, g_szHostName);

	bsInitGame.Read((char*)&byteVehicleModels[0], 212);

	client.playerid = MyPlayerID;
	client.gameInited = true;
}

void ConnectionRejected(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	BYTE byteRejectReason;

	bsData.Read(byteRejectReason);

	if (byteRejectReason == REJECT_REASON_BAD_VERSION)
	{
		Log("{SYSTEM} [%s] Bad SA-MP version.", client.nickName);
	}
	else if (byteRejectReason == REJECT_REASON_BAD_NICKNAME)
	{
		Log("{SYSTEM} [%s] Bad nickname.", client.nickName);
	}
	else if (byteRejectReason == REJECT_REASON_BAD_MOD)
	{
		Log("{SYSTEM} [%s] Bad mod version.", client.nickName);
	}
	else if (byteRejectReason == REJECT_REASON_BAD_PLAYERID)
	{
		Log("{SYSTEM} [%s] Bad player ID.", client.nickName);
	}
	else
		Log("{SYSTEM} [%s] ConnectionRejected: unknown", client.nickName);
}

void ClientMessage(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	DWORD dwStrLen, dwColor;
	char szMsg[257];
	memset(szMsg, 0, 257);

	bsData.Read(dwColor);
	bsData.Read(dwStrLen);
	if (dwStrLen > 256) return;

	bsData.Read(szMsg, dwStrLen);
	szMsg[dwStrLen] = 0;

	char szNonColorEmbeddedMsg[257];
	int iNonColorEmbeddedMsgLen = 0;

	for (size_t pos = 0; pos < strlen(szMsg) && szMsg[pos] != '\0'; pos++)
	{
		if (!((*(unsigned char*)(&szMsg[pos]) - 32) >= 0 && (*(unsigned char*)(&szMsg[pos]) - 32) < 224))
			continue;

		if (pos + 7 < strlen(szMsg))
		{
			if (szMsg[pos] == '{' && szMsg[pos + 7] == '}')
			{
				pos += 7;
				continue;
			}
		}

		szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = szMsg[pos];
		iNonColorEmbeddedMsgLen++;
	}

	szNonColorEmbeddedMsg[iNonColorEmbeddedMsgLen] = 0;

	Log("[CMSG] [%s] %s", client.nickName, szNonColorEmbeddedMsg);

	if (!client.logined && (!strcmp(szNonColorEmbeddedMsg, "It appears that you're using RakSAMP.") || !strcmp(szNonColorEmbeddedMsg, "You will be automatically put to sleep and you cannot get out from asleep mode.") || isSubstring(szNonColorEmbeddedMsg, "Welcome ")))
	{
		Log("[%s] Login successfully.", client.nickName);
		Log("[%s] Set Player ID: %i", client.nickName, client.playerid);
		sampSpawn();
		client.logined = true;
	}
}

void RequestClass(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	BYTE byteRequestOutcome = 0;

	bsData.Read(byteRequestOutcome);

	if (byteRequestOutcome)
	{
		bsData.Read((PCHAR)&client.SpawnInfo, sizeof(PLAYER_SPAWN_INFO));
	}
}

void ScrDialogBox(RPCParameters* rpcParams)
{
	if (!client.gameInited) return;

	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;
	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read(client.dialogid);
	bsData.Read(sampDialog.bDialogStyle);

	bsData.Read(sampDialog.bTitleLength);
	bsData.Read(sampDialog.szTitle, sampDialog.bTitleLength);
	sampDialog.szTitle[sampDialog.bTitleLength] = 0;

	bsData.Read(sampDialog.bButton1Len);
	bsData.Read(sampDialog.szButton1, sampDialog.bButton1Len);
	sampDialog.szButton1[sampDialog.bButton1Len] = 0;

	bsData.Read(sampDialog.bButton2Len);
	bsData.Read(sampDialog.szButton2, sampDialog.bButton2Len);
	sampDialog.szButton2[sampDialog.bButton2Len] = 0;

	stringCompressor->DecodeString(sampDialog.szInfo, 256, &bsData);
	if (client.dialogid == 8586)
	{
		sampSpawn();
		sendDialogResponse(client.dialogid, 1, 0, "");
	}
	else if (client.dialogid == 8426)
	{
		sendDialogResponse(client.dialogid, 1, 0, "");
	}
	else if (client.dialogid == 12346)
	{
		sendDialogResponse(client.dialogid, 1, 0, settings.userPassword);
		Log("[%s] Login request has been sent.", client.nickName);
	}
	else if (client.dialogid == 12347)
	{
		Log("[%s] Your password doesn't correct.", client.nickName);
		sampDisconnect();
	}
	
	client.dialogid = 0;
}

void ScrSetPlayerPos(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read(client.normalModePos[0]);
	bsData.Read(client.normalModePos[1]);
	bsData.Read(client.normalModePos[2]);
}

void ScrSetPlayerFacingAngle(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);
	bsData.Read(client.normalModeRot);
}

void ScrSetSpawnInfo(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read((PCHAR)&client.SpawnInfo, sizeof(PLAYER_SPAWN_INFO));

	client.normalModePos[0] = client.SpawnInfo.vecPos[0];
	client.normalModePos[1] = client.SpawnInfo.vecPos[1];
	client.normalModePos[2] = client.SpawnInfo.vecPos[2];
}

void ScrSetPlayerHealth(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read(client.playerHealth);
}

void ScrSetPlayerArmour(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read(client.playerArmour);
}

void ScrTogglePlayerSpectating(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	BOOL bToggle;
	bsData.Read(bToggle);

	if (client.isSpectating && !bToggle && !client.spawned)
	{
		sampSpawn();
		client.spawned = true;
	}

	client.isSpectating = bToggle;
}

void ScrSetDrunkLevel(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	bsData.Read(client.drunkLevel);
}

void ScrHaveSomeMoney(RPCParameters* rpcParams)
{
	PCHAR Data = reinterpret_cast<PCHAR>(rpcParams->input);
	int iBitLength = rpcParams->numberOfBitsOfData;

	RakNet::BitStream bsData((unsigned char*)Data, (iBitLength / 8) + 1, false);

	int iGivenMoney;
	bsData.Read(iGivenMoney);

	client.money += iGivenMoney;
}

void ScrResetMoney(RPCParameters* rpcParams)
{
	client.money = 0;
}

void RegisterRPCs()
{
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_InitGame, InitGame);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ConnectionRejected, ConnectionRejected);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ClientMessage, ClientMessage);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_RequestClass, RequestClass);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrDialogBox, ScrDialogBox);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetPlayerPos, ScrSetPlayerPos);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetPlayerFacingAngle, ScrSetPlayerFacingAngle);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetSpawnInfo, ScrSetSpawnInfo);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetPlayerHealth, ScrSetPlayerHealth);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetPlayerArmour, ScrSetPlayerArmour);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrTogglePlayerSpectating, ScrTogglePlayerSpectating);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrSetPlayerDrunkLevel, ScrSetDrunkLevel);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrHaveSomeMoney, ScrHaveSomeMoney);
	client.rakClient->RegisterAsRemoteProcedureCall(RPC_ScrResetMoney, ScrResetMoney);
}
