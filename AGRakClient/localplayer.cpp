#include "main.h"

unsigned long long dwLastOnFootDataSentTick = GetTickCount64();

void SendOnFootFullSyncData(ONFOOT_SYNC_DATA* pofSync, int sendDeathNoti)
{
	if (pofSync == NULL)
		return;

	RakNet::BitStream bsPlayerSync;

	if (dwLastOnFootDataSentTick && dwLastOnFootDataSentTick < (GetTickCount64() - 40uLL))
	{
		client.currentPosition[0] = pofSync->vecPos[0];
		client.currentPosition[1] = pofSync->vecPos[1];
		client.currentPosition[2] = pofSync->vecPos[2];

		bsPlayerSync.Write((BYTE)ID_PLAYER_SYNC);
		bsPlayerSync.Write((PCHAR)pofSync, sizeof(ONFOOT_SYNC_DATA));

		client.rakClient->Send(&bsPlayerSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);

		if (sendDeathNoti && pofSync->byteHealth == 0)
		{
			RakNet::BitStream bsPlayerDeath;

			bsPlayerDeath.Write(0);
			bsPlayerDeath.Write(-1);
			client.rakClient->RPC(RPC_Death, &bsPlayerDeath, HIGH_PRIORITY, RELIABLE_ORDERED, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
		}

		dwLastOnFootDataSentTick = GetTickCount64();
	}
}

void SendSpectatorData(SPECTATOR_SYNC_DATA* pSpecData)
{
	RakNet::BitStream bsSpecSync;

	bsSpecSync.Write((BYTE)ID_SPECTATOR_SYNC);
	bsSpecSync.Write((PCHAR)pSpecData, sizeof(SPECTATOR_SYNC_DATA));

	client.rakClient->Send(&bsSpecSync, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
}
