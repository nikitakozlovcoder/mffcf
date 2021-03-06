// ConsoleApplication1.cpp: определяет точку входа для консольного приложения.
//
#include <Windows.h>
#include "stdafx.h"
#include <Windows.h>
#include <iostream>
#include "MemMan.h"
#include "csgo.hpp"
#include <thread>

MemMan MemClass;

using namespace hazedumper::netvars;
using namespace hazedumper::signatures;
using namespace std;

struct variables
{
	int myTeam;
	uintptr_t localPlayer;
	uintptr_t gameModule;
	uintptr_t engineModule;
	uintptr_t glowObject;
	uintptr_t clientState;
	float closestAngle;
	uintptr_t closest;
	vector<float> angles;
} val;

struct ClrRender
{
	BYTE red, green, blue;
};

struct VectorStr
{
	float x, y, z;
};
ClrRender clrEnemy;
ClrRender clrTeam;

struct GlowStruct
{
	BYTE base[4];
	float red;
	float green;
	float blue;
	float alpha;
	BYTE buffer[16];
	bool renderWhenOccluded;
	bool renderWhenUnOccluded;
	bool fullBloom;
	BYTE buffer1[5];
	int glowStyle;
};




vector<float> calcAngle(vector<float> src, float dst[3])
{

	vector<float> angles = { 0 ,0 ,0, 0 };
	double delta[3] = { (src[0] - dst[0]), (src[1] - dst[1]), (src[2] - dst[2]) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles[0] = (float)(asinf(delta[2] / hyp) * 57.295779513082f);
	angles[1] = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles[2] = 0.0f;
	angles[3] = hyp;

	if (delta[0] >= 0.0) { angles[1] += 180.0f; }
	//cout << angles[0] << endl;
	//cout << angles[1] << endl;
	if (angles[0] > 180.0f)
	{
		angles[0] -= 360.0f;
	}
	else if (angles[0] < -180.0f)
	{
		angles[0] += 360.0f;
	}
	if (angles[1] > 180.0f)
	{
		angles[1] -= 360.0f;
	}
	else if (angles[1] < -180.0f)
	{
		angles[1] += 360.0f;
	}
	//cout << "src" << endl;
	//cout << src << endl;
	//cout << src.y << endl;
	//cout << src.z << endl;
	//cout << "anglesStart" << endl;
	//cout << angles[0] << endl;
	//cout << angles[1] << endl;
	//cout << "anglesEnd" << endl;
	return angles;
}
vector<float> getHeadPosition(uintptr_t entity)
{
	uintptr_t bones = MemClass.readMem<uintptr_t>(entity + m_dwBoneMatrix);
	float enemyHeadX = MemClass.readMem<float>(bones + 0x30 * 8 + 0x0C);
	float enemyHeadY = MemClass.readMem<float>(bones + 0x30 * 8 + 0x1C);
	float enemyHeadZ = MemClass.readMem<float>(bones + 0x30 * 8 + 0x2C);

	vector<float> enemyHeadPos = { enemyHeadX, enemyHeadY, enemyHeadZ };
	return enemyHeadPos;
	//cout << enemyHeadX << endl;
	//cout << enemyHeadY << endl;
	//cout << enemyHeadZ << endl;
}
void aimAt(uintptr_t entity)
{
	int health = MemClass.readMem<int>(entity + m_iHealth);
	if (entity != 0 && health > 0)
	{
		//VectorStr vecOrigin = MemClass.readMem<VectorStr>(val.localPlayer + m_vecOrigin);
		//VectorStr vecOffset = MemClass.readMem<VectorStr>(val.localPlayer + m_vecViewOffset);
		//vector<float> vec{ vecOrigin.x + vecOffset.x, vecOrigin.y + vecOffset.y, vecOrigin.z + vecOffset.z };

		//vector<float> enemyPos = getHeadPosition(entity);
		//float enemyPosArr[3] = { enemyPos[0], enemyPos[1], enemyPos[2] };
		//vector<float> angles = calcAngle(vec, enemyPosArr);
		VectorStr anglesStr;
		anglesStr.x = val.angles[0];
		anglesStr.y = val.angles[1];
		anglesStr.z = 0.0f;
		MemClass.writeMem<VectorStr>(val.clientState + dwClientState_ViewAngles, anglesStr);
	}
	else
	{
		val.closest = 0;
		val.closestAngle = 10000.0f;
	}

}
void findClosest()
{
	for (short int i = 0; i < 64; i++)
	{
		uintptr_t entity = MemClass.readMem<uintptr_t>(val.gameModule + dwEntityList + i * 0x10);
		bool dormant = MemClass.readMem<bool>(entity + m_bDormant);
		if (!dormant)
		{

			int entityTeam = MemClass.readMem<int>(entity + m_iTeamNum);
			if (val.myTeam != entityTeam)
			{

				VectorStr vecOrigin = MemClass.readMem<VectorStr>(val.localPlayer + m_vecOrigin);
				VectorStr vecOffset = MemClass.readMem<VectorStr>(val.localPlayer + m_vecViewOffset);
				vector<float> vec{ vecOrigin.x + vecOffset.x, vecOrigin.y + vecOffset.y, vecOrigin.z + vecOffset.z };

				vector<float> enemyPos = getHeadPosition(entity);
				float enemyPosArr[3] = { enemyPos[0], enemyPos[1], enemyPos[2] };
				vector<float> angles = calcAngle(vec, enemyPosArr);
				VectorStr playerAngles = MemClass.readMem<VectorStr>(val.clientState + dwClientState_ViewAngles);
				float yawDif = abs(angles[1] - playerAngles.y);
				float realDist = sin(yawDif * 3.14159265359 / 180) * angles[3];
				cout << "enemy" << endl;
				cout << yawDif << endl;
				cout << "enemyEnd" << endl;
				
				
				if (realDist < val.closestAngle)
				{
					//cout << "true" << endl;
					val.closest = entity;
					val.angles = angles;
					val.closestAngle = realDist;
				}
			}

		}
	}
}
void handleAim()
{

	while (true)
	{
		
		
		
		while (GetAsyncKeyState(VK_RBUTTON))
		{
			val.closest = 0;
			val.closestAngle = 10000.0f;
			findClosest();
			aimAt(val.closest);
			Sleep(1);
		}

		Sleep(1);
	}
}





GlowStruct SetGlowColor(GlowStruct Glow, uintptr_t entity)
{
	bool defusing = MemClass.readMem<bool>(entity + m_bIsDefusing);
	if (defusing)
	{
		Glow.red = 1.0f;
		Glow.green = 1.0f;
		Glow.blue = 1.0f;
	}
	else
	{
		int health = MemClass.readMem<int>(entity + m_iHealth);
		Glow.red = health * -0.01 + 1;
		Glow.green = health * 0.01;
	}
	Glow.alpha = 1.0f;
	Glow.renderWhenOccluded = true;
	Glow.renderWhenUnOccluded = false;
	return Glow;
}
GlowStruct SetGlowColorTeam(GlowStruct Glow, uintptr_t entity)
{
	Glow.blue = 1.0f;
	Glow.alpha = 1.0f;
	Glow.renderWhenOccluded = true;
	Glow.renderWhenUnOccluded = false;
	return Glow;
}

void SetTeamGlow(uintptr_t entity, int glowIndex)
{
	GlowStruct TGlow;
	GlowStruct TGlowNew;
	TGlow = MemClass.readMem<GlowStruct>(val.glowObject + (glowIndex * 0x38));
	TGlowNew = SetGlowColorTeam(TGlow, entity);


	if (TGlowNew.green != TGlow.green || TGlowNew.blue != TGlow.blue || TGlowNew.alpha != TGlow.alpha || TGlowNew.renderWhenOccluded != TGlow.renderWhenOccluded || TGlowNew.renderWhenUnOccluded != TGlow.renderWhenUnOccluded)
	{
		MemClass.writeMem<GlowStruct>(val.glowObject + (glowIndex * 0x38), TGlowNew);
	}
}

void SetEnemyGlow(uintptr_t entity, int glowIndex)
{
	GlowStruct EGlow;
	GlowStruct EGlowNew;
	EGlow = MemClass.readMem<GlowStruct>(val.glowObject + (glowIndex * 0x38));
	EGlowNew = SetGlowColor(EGlow, entity);
	if (EGlowNew.red != EGlow.red || EGlowNew.blue != EGlow.blue || EGlowNew.alpha != EGlow.alpha || EGlowNew.renderWhenOccluded != EGlow.renderWhenOccluded || EGlowNew.renderWhenUnOccluded != EGlow.renderWhenUnOccluded)
	{
		MemClass.writeMem<GlowStruct>(val.glowObject + (glowIndex * 0x38), EGlowNew);
	}

}

void HandleGlow()
{
	val.glowObject = MemClass.readMem<uintptr_t>(val.gameModule + dwGlowObjectManager);


	for (short int i = 0; i < 64; i++)
	{
		uintptr_t entity = MemClass.readMem<uintptr_t>(val.gameModule + dwEntityList + i * 0x10);
		bool dormant = MemClass.readMem<bool>(entity + m_bDormant);
		if (!dormant)
		{

			int glowIndx = MemClass.readMem<int>(entity + m_iGlowIndex);
			int entityTeam = MemClass.readMem<int>(entity + m_iTeamNum);
			//int myTeam = MemClass.readMem<int>(val.localPlayer + m_iTeamNum);
			//cout << val.myTeam << endl;
			//cout << "they "+ entityTeam << endl;
			if (val.myTeam == entityTeam)
			{

				MemClass.writeMem<ClrRender>(entity + m_clrRender, clrTeam);
				SetTeamGlow(entity, glowIndx);
			}
			else
			{

			//	MemClass.writeMem<float>(val.glowObject + ((entity * 0x38) + 0x4), 2);
				//MemClass.writeMem<float>(val.glowObject + ((entity * 0x38) + 0x8), 0);
				//MemClass.writeMem<float>(val.glowObject + ((entity * 0x38) + 0xc), 0);
			//	MemClass.writeMem<float>(val.glowObject + ((entity * 0x38) + 0x10), 1.7);
			//	MemClass.writeMem<bool>(val.glowObject + ((entity * 0x38) + 0x24), true);
			//	MemClass.writeMem<bool>(val.glowObject + ((entity * 0x38) + 0x25), false);
				MemClass.writeMem<ClrRender>(entity + m_clrRender, clrEnemy);
				SetEnemyGlow(entity, glowIndx);
			}
		}
	}
}

void SetBrightness()
{
	clrTeam.red = 0;
	clrTeam.blue = 255;
	clrTeam.green = 0;

	clrEnemy.red = 255;
	clrEnemy.blue = 0;
	clrEnemy.green = 0;

	float brightness = 7.0f;

	int ptr = MemClass.readMem<int>(val.engineModule + model_ambient_min);
	int xorptr = *(int*)&brightness ^ ptr;
	MemClass.writeMem<int>(val.engineModule + model_ambient_min, xorptr);
}

int main()
{
	int procID = MemClass.getProcess(L"csgo.exe");
	val.gameModule = MemClass.getModule(procID, L"client_panorama.dll");
	val.engineModule = MemClass.getModule(procID, L"engine.dll");


	if (val.localPlayer == NULL)
		while (val.localPlayer == NULL)
			val.localPlayer = MemClass.readMem<uintptr_t>(val.gameModule + dwLocalPlayer);
	val.myTeam = MemClass.readMem<int>(val.localPlayer + m_iTeamNum);
	SetBrightness();
	//byte check = MemClass.readMem<BYTE>(val.gameModule + force_update_spectator_glow);
	//cout << check << endl;
	MemClass.writeMem<BYTE>(val.gameModule + force_update_spectator_glow, 0xEB); //switch on 0x74
	
	val.clientState = MemClass.readMem<uintptr_t>(val.engineModule + dwClientState);
	thread aimThread(handleAim);

	while (true)
	{
		//int localHealth = MemClass.readMem<int>(val.localPlayer + m_iHealth);
		if (GetAsyncKeyState(VK_NUMPAD0))
		{
			val.localPlayer = MemClass.readMem<uintptr_t>(val.gameModule + dwLocalPlayer);
			val.myTeam = MemClass.readMem<int>(val.localPlayer + m_iTeamNum);
			val.clientState = MemClass.readMem<uintptr_t>(val.engineModule + dwClientState);
			MemClass.writeMem<BYTE>(val.gameModule + force_update_spectator_glow, 0x74); //switch on 0x74
			Sleep(3);
			MemClass.writeMem<BYTE>(val.gameModule + force_update_spectator_glow, 0xEB);

		}
		HandleGlow();

		Sleep(1);
	}
	return 0;
}
