#include "main.h"

void Packet_AUTH_KEY(Packet* p)
{
	RakNet::BitStream bsAuth((unsigned char*)p->data, p->length, false);

	BYTE byteAuthLen;
	char szAuth[260];

	bsAuth.IgnoreBits(8); // ID_AUTH_KEY
	bsAuth.Read(byteAuthLen);
	bsAuth.Read(szAuth, byteAuthLen);
	szAuth[byteAuthLen] = '\0';

	char szAuthKey[260];

	gen_auth_key(szAuthKey, szAuth);
	RakNet::BitStream bsKey;
	BYTE byteAuthKeyLen = (BYTE)strlen(szAuthKey);

	bsKey.Write((BYTE)ID_AUTH_KEY);
	bsKey.Write((BYTE)byteAuthKeyLen);
	bsKey.Write(szAuthKey, byteAuthKeyLen);

	client.rakClient->Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, NULL);
}

void Packet_ConnectionSucceeded(Packet* p)
{
	RakNet::BitStream bsSuccAuth((unsigned char*)p->data, p->length, false);

	unsigned int uiChallenge;

	bsSuccAuth.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
	bsSuccAuth.IgnoreBits(32); // binaryAddress
	bsSuccAuth.IgnoreBits(16); // port

	bsSuccAuth.Read(client.playerid);
	bsSuccAuth.Read(uiChallenge);

	client.isConnected = true;
	Log("[%s] Connected. Joining the game...", client.nickName);

	int iVersion = NETGAME_VERSION;
	unsigned int uiClientChallengeResponse = uiChallenge ^ iVersion;
	BYTE byteMod = 1;

	char auth_bs[4 * 16] = { 0 };
	gen_gpci(auth_bs, 0x3e9);

	BYTE byteAuthBSLen;
	byteAuthBSLen = (BYTE)strlen(auth_bs);
	BYTE byteNameLen = (BYTE)strlen(client.nickName);
	BYTE iClientVerLen = (BYTE)strlen(settings.szClientVersion);

	RakNet::BitStream bsSend;

	bsSend.Write(iVersion);
	bsSend.Write(byteMod);
	bsSend.Write(byteNameLen);
	bsSend.Write(client.nickName, byteNameLen);
	bsSend.Write(uiClientChallengeResponse);

	bsSend.Write(byteAuthBSLen);
	bsSend.Write(auth_bs, byteAuthBSLen);
	bsSend.Write(iClientVerLen);
	bsSend.Write(settings.szClientVersion, iClientVerLen);

	client.rakClient->RPC(RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);

	client.areWeConnected = true;
}

void resetPools()
{
	
	client.sleepTime = GetTickCount64() + (10 * 1000);
	client.areWeConnected = false;
	client.callSampRequestClass = false;
	client.spawned = false;
	client.playerHealth = 100.0f;
	client.playerArmour = 0.0f;
	client.logined = false;
	client.money = 0;
	client.drunkLevel = 0;
	sampDisconnect();
	Log("[%s] Reconnecting in %d seconds.", client.nickName, 10);
}

void UpdatePlayerScoresAndPings(int iWait, int iMS)
{
	static auto dwLastUpdateTick = 0uLL;

	if (iWait)
	{
		if ((GetTickCount64() - dwLastUpdateTick) > (decltype(dwLastUpdateTick))iMS)
		{
			dwLastUpdateTick = GetTickCount64();
			RakNet::BitStream bsParams;
			client.rakClient->RPC(RPC_UpdateScoresPingsIPs, &bsParams, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
		}
	}
	else
	{
		RakNet::BitStream bsParams;
		client.rakClient->RPC(RPC_UpdateScoresPingsIPs, &bsParams, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
	}
}

void UpdateNetwork()
{
	unsigned char packetIdentifier;
	Packet* pkt;

	while (pkt = client.rakClient->Receive())
	{
		if ((unsigned char)pkt->data[0] == ID_TIMESTAMP)
		{
			if (pkt->length > sizeof(unsigned char) + sizeof(unsigned int))
				packetIdentifier = (unsigned char)pkt->data[sizeof(unsigned char) + sizeof(unsigned int)];
			else
				return;
		}
		else
			packetIdentifier = (unsigned char)pkt->data[0];

		switch (packetIdentifier)
		{
		case ID_DISCONNECTION_NOTIFICATION:
			Log("{SYSTEM} [%s] Connection was closed by the server.", client.nickName);
			resetPools();
			break;
		case ID_CONNECTION_BANNED:
			Log("{SYSTEM} [%s] You are banned.", client.nickName);
			resetPools();
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			Log("{SYSTEM} [%s] Connection attempt failed.", client.nickName);
			resetPools();
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			Log("{SYSTEM} [%s] The server is full.", client.nickName);
			resetPools();
			break;
		case ID_INVALID_PASSWORD:
			Log("{SYSTEM} [%s] Invalid server password.", client.nickName);
			resetPools();
			break;
		case ID_CONNECTION_LOST:
			Log("{SYSTEM} [%s] The connection was lost.", client.nickName);
			resetPools();
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			Packet_ConnectionSucceeded(pkt);
			break;
		case ID_AUTH_KEY:
			Packet_AUTH_KEY(pkt);
			break;
		case ID_PLAYER_SYNC:
		case ID_VEHICLE_SYNC:
		case ID_PASSENGER_SYNC:
		case ID_AIM_SYNC:
		case ID_TRAILER_SYNC:
		case ID_UNOCCUPIED_SYNC:
		case ID_MARKERS_SYNC:
		case ID_BULLET_SYNC:
			break;
		}

		client.rakClient->DeallocatePacket(pkt);
	}

	UpdatePlayerScoresAndPings(1, 3000);
}
