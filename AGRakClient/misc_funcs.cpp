#include "main.h"

void onFootUpdateAtNormalPos()
{
	ONFOOT_SYNC_DATA ofSync;
	memset(&ofSync, 0, sizeof(ONFOOT_SYNC_DATA));

	ofSync.byteHealth = (BYTE)client.playerHealth;
	ofSync.byteArmour = (BYTE)client.playerArmour;
	ofSync.fQuaternion[3] = client.normalModeRot;
	ofSync.vecPos[0] = client.normalModePos[0];
	ofSync.vecPos[1] = client.normalModePos[1];
	ofSync.vecPos[2] = client.normalModePos[2];
	SendOnFootFullSyncData(&ofSync, 0);
}

void spectatorUpdate()
{
	SPECTATOR_SYNC_DATA spSync;
	memset(&spSync, 0, sizeof(SPECTATOR_SYNC_DATA));

	spSync.vecPos[0] = client.normalModePos[0];
	spSync.vecPos[1] = client.normalModePos[1];
	spSync.vecPos[2] = client.normalModePos[2];

	SendSpectatorData(&spSync);
}

int sampConnect()
{
	if (client.rakClient == NULL) return 0;
	Log("[%s] Connecting to %s:%d..", settings.userName, settings.server.szAddr, settings.server.iPort);
	strcpy(client.nickName, settings.userName);

	client.rakClient->SetPassword("");
	return (int)client.rakClient->Connect(settings.server.szAddr, settings.server.iPort, 0, 0, 5);
}

void sampDisconnect()
{
	if (client.rakClient == NULL) return;
	client.rakClient->Disconnect(500);
}

void sampRequestClass()
{
	if (client.rakClient == NULL) return;

	RakNet::BitStream bsSpawnRequest;
	bsSpawnRequest.Write(settings.iClassID);
	client.rakClient->RPC(RPC_RequestClass, &bsSpawnRequest, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void sampSpawn()
{
	if (client.rakClient == NULL) return;
	if (client.spawned == false)
	{
		client.normalModePos[0] = client.SpawnInfo.vecPos[0];
		client.normalModePos[1] = client.SpawnInfo.vecPos[1];
		client.normalModePos[2] = client.SpawnInfo.vecPos[2];
		client.normalModeRot = client.SpawnInfo.fRotation;
	}


	RakNet::BitStream bsSendRequestSpawn;
	client.rakClient->RPC(RPC_RequestSpawn, &bsSendRequestSpawn, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);

	RakNet::BitStream bsSendSpawn;
	client.rakClient->RPC(RPC_Spawn, &bsSendSpawn, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);

	client.isSpectating = false;

	Log("[%s] You have been spawned!", client.nickName);
}

void sendServerCommand(const char* szCommand)
{
	RakNet::BitStream bsParams;
	int iStrlen = strlen(szCommand);
	bsParams.Write(iStrlen);
	bsParams.Write(szCommand, iStrlen);
	client.rakClient->RPC(RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void sendChat(char* szMessage)
{
	RakNet::BitStream bsSend;
	BYTE byteTextLen = static_cast<BYTE>(strlen(szMessage));
	bsSend.Write(byteTextLen);
	bsSend.Write(szMessage, byteTextLen);
	client.rakClient->RPC(RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

void sendDialogResponse(WORD wDialogID, BYTE bButtonID, WORD wListBoxItem, const char* szInputResp)
{
	BYTE respLen = (BYTE)strlen(szInputResp);
	RakNet::BitStream bsSend;
	bsSend.Write(wDialogID);
	bsSend.Write(bButtonID);
	bsSend.Write(wListBoxItem);
	bsSend.Write(respLen);
	bsSend.Write(szInputResp, respLen);
	client.rakClient->RPC(RPC_DialogResponse, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, FALSE, UNASSIGNED_NETWORK_ID, NULL);
}

int gen_gpci(char buf[64], unsigned long factor) /* by bartekdvd */
{
	unsigned char out[6 * 4] = { 0 };

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for (int i = 0; i < 6 * 4; ++i)
		out[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	out[(6 * 4) - 1] = 0;

	BIG_NUM_MUL((unsigned long*)out, (unsigned long*)out, factor);

	unsigned int notzero = 0;
	buf[0] = '0'; buf[1] = '\0';

	if (factor == 0) return 1;

	int pos = 0;
	for (int i = 0; i < 24; i++)
	{
		unsigned char tmp = out[i] >> 4;
		unsigned char tmp2 = out[i] & 0x0F;

		if (notzero || tmp)
		{
			buf[pos++] = (char)((tmp > 9) ? (tmp + 55) : (tmp + 48));
			if (!notzero) notzero = 1;
		}

		if (notzero || tmp2)
		{
			buf[pos++] = (char)((tmp2 > 9) ? (tmp2 + 55) : (tmp2 + 48));
			if (!notzero) notzero = 1;
		}
	}
	buf[pos] = 0;

	return pos;
}

#define endian_swap8(x) (x)
#define endian_swap16(x) ((x>>8) | (x<<8))
#define endian_swap32(x) ((x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24))
#define endian_swap64(x) ((x>>56) | ((x<<40) & 0x00FF000000000000) | \
		((x<<24) & 0x0000FF0000000000) | ((x<<8)  & 0x000000FF00000000) | \
		((x>>8)  & 0x00000000FF000000) | ((x>>24) & 0x0000000000FF0000) | \
        ((x>>40) & 0x000000000000FF00) | (x<<56))

#define ROTL(value, shift) ((value << shift) | (value >> (sizeof(value)*8 - shift)))
#define ROTR(value, shift) ((value >> shift) | (value << (sizeof(value)*8 - shift)))
#define swap(x,y,T) {T tmp = x; x = y; y = tmp;}

void SHA1(char* message, unsigned long* out)
{
	unsigned long h0 = 0x67452301;
	unsigned long h1 = 0xEFCDAB89;
	unsigned long h2 = 0x98BADCFE;
	unsigned long h3 = 0x10325476;
	unsigned long h4 = 0xC3D2E1F0;

	unsigned long len = 0;
	unsigned long long bitlen = 0;

	while (message[len])
	{
		len++;
		bitlen += 8;
	}

	unsigned long complement = (55 - (len % 56)) + 8 * (((len + 8) / 64));
	unsigned long newlen = len + complement + 8 + 1;
	char* pMessage = new char[newlen];
	if (!pMessage)
		return;

	memcpy(pMessage, message, len);
	pMessage[len] = -128;
	memset(pMessage + len + 1, 0, complement);

	*(unsigned long long*)& pMessage[len + 1 + complement] = endian_swap64(bitlen);

	unsigned long chunks = newlen / 64;
	unsigned long w[80];

	for (unsigned long x = 0; x < chunks; x++)
	{
		for (unsigned long i = 0; i < 16; i++)
			w[i] = endian_swap32(*(unsigned long*)(&pMessage[x * 64 + i * 4]));

		memset(&w[16], 0, 64 * 4);

		for (unsigned long i = 16; i <= 79; i++)
			w[i] = ROTL((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);

		unsigned long a = h0;
		unsigned long b = h1;
		unsigned long c = h2;
		unsigned long d = h3;
		unsigned long e = h4;

		for (unsigned long i = 0; i <= 79; i++)
		{
			unsigned long f;
			unsigned long k;

			if (0 <= i && i <= 19)
			{
				f = (b & c) | ((~b) & d);
				k = 0x5A827999;
			}
			else if (20 <= i && i <= 39)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}
			else if (40 <= i && i <= 59)
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}
			else if (60 <= i && i <= 79)
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			unsigned long temp = (ROTL(a, 5) + f + e + k + w[i]) & 0xFFFFFFFF;
			e = d;
			d = c;
			c = ROTL(b, 30);
			b = a;
			a = temp;
		}

		h0 = (h0 + a) & 0xFFFFFFFF;
		h1 = (h1 + b) & 0xFFFFFFFF;
		h2 = (h2 + c) & 0xFFFFFFFF;
		h3 = (h3 + d) & 0xFFFFFFFF;
		h4 = (h4 + e) & 0xFFFFFFFF;
	}

	delete[] pMessage;

	out[0] = h0;
	out[1] = h1;
	out[2] = h2;
	out[3] = h3;
	out[4] = h4;
}

void SHA1(char* message, char buf[64])
{
	if (!buf) return;
	unsigned long out[5];
	SHA1(message, out);
	sprintf_s(buf, 64, "%.8X%.8X%.8X%.8X%.8X", out[0], out[1], out[2], out[3], out[4]);
}

const static BYTE auth_hash_transform_table[100] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x08, 0x06, 0x00, 0x00, 0x00, 0xE4, 0xB5, 0xB7, 0x0A, 0x00, 0x00, 0x00,
	0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00, 0x0B,
	0x13, 0x01, 0x00, 0x9A, 0x9C, 0x18, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41,
	0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8E, 0x7C, 0xFB, 0x51, 0x93, 0x00, 0x00,
	0x00, 0x20, 0x63, 0x48, 0x52, 0x4D, 0x00, 0x00, 0x7A, 0x25, 0x00, 0x00,
	0x80, 0x83, 0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x80, 0xE9, 0x00, 0x00,
	0x75, 0x30, 0x00, 0x00
};

BYTE transform_auth_sha1(BYTE value, BYTE xorr)
{
	BYTE result = value;

	for (BYTE i = 0; i < 100; i++)
	{
		result = result ^ auth_hash_transform_table[i] ^ xorr;
	}

	return result;
}

const static BYTE code_from_CAnimManager_AddAnimation[20] = {
	0xFF, 0x25, 0x34, 0x39, // gta_sa.exe + 0x4D3AA0
	0x4D, 0x00, 0x90, 0x90, // gta_sa.exe + 0x4D3AA4
	0x90, 0x90, 0x56, 0x57, // gta_sa.exe + 0x4D3AAC
	0x50, 0x8B, 0x44, 0x24, // gta_sa.exe + 0x4D3AA8
	0x14, 0x8D, 0x0C, 0x80  // gta_sa.exe + 0x4D3AB0
};

char samp_sub_100517E0(BYTE a1)
{
	char result = a1 + '0';

	if (a1 + '0' > '9')
	{
		result = a1 + '7';
	}

	return result;

}
void auth_stringify(char* out, PBYTE hash)
{
	BYTE i = 0;
	PBYTE j = hash;

	do
	{
		out[i] = samp_sub_100517E0(*j >> 4); i++;
		out[i] = samp_sub_100517E0(*j & 0xF); i++;

		j++;
	} while (i < 40);

	out[i] = '\0';
}

void gen_auth_key(char buf[260], char* auth_in)
{
	char message[260];
	if (!auth_in) return;
	sprintf_s(message, 260, "%s", auth_in);

	unsigned long out[5];
	BYTE* pb_out = (PBYTE)&out;

	SHA1(message, out);

	for (BYTE i = 0; i < 5; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x2F); }
	for (BYTE i = 5; i < 10; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x45); }
	for (BYTE i = 10; i < 15; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0x6F); }
	for (BYTE i = 15; i < 20; i++) { pb_out[i] = transform_auth_sha1(pb_out[i], 0xDB); }
	for (BYTE i = 0; i < 20; i++) { pb_out[i] ^= code_from_CAnimManager_AddAnimation[i]; }

	auth_stringify(buf, pb_out);
}
