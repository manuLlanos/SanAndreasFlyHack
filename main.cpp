#include "vector3.h"
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>

// TODO:
// turn it into a gmod style noclip, press a key to keep the speeds at 0
// arrow keys increase or decrease relative speeds by adding, not setting


const DWORD CAMERA_PITCH_ADDRESS = 0x00B6F248;
const DWORD CAMERA_ROLL_ADDRESS = 0x00B6F258;
const DWORD HEALTH_OFFSETS[2] = {0x0076F980, 0x540};
const DWORD SPEED_OFFSET = 0x0076F3B8;

const float JUMP_SPEED = 1.0;
const float PI = 3.14159265359;


float clamp(float, float, float);
void WriteSpeed(HANDLE process, DWORD addresses[], Vector3 speed);

DWORD ObtenerPID(const char* name) {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(snap, &entry)) {
			do {
				if (_strcmpi(name, entry.szExeFile) == 0) {
					CloseHandle(snap);
					return entry.th32ProcessID;
				}
			} while (Process32Next(snap, &entry));
		}
	}
	return 0;
}

DWORD ObtenerModulo(const char* name, DWORD pid) {
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, pid);
	if (snap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 entry;
		entry.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(snap, &entry)) {
			do {
				if (_strcmpi(name, entry.szModule) == 0) {
					CloseHandle(snap);
					return (DWORD)entry.modBaseAddr;
				}
			} while (Module32Next(snap, &entry));
		}
	}
	return 0;
}


int main(int argc, char** argv)
{
	DWORD pid = ObtenerPID("gta_sa.exe");
	if (!pid) {
		printf_s("Couldn't find process ID\n");
		return 0;
	}
	DWORD base_addr = ObtenerModulo("gta_sa.exe", pid);
	if (!base_addr) {
		printf_s("Couldn't get base address\n");
		return 0;
	}
	printf_s("PID: %d\nBase Addr: %x\n", pid, base_addr);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	printf_s("SAN ANDREAS FLYHACK\nB - toggle fly mode\nshift/lctrl - increase/decrease height\nMove using WASD keys and mouse\nup/down - Change Speed\nF4 - quit process\n");
	//camera roll goes between pi and -pi ("side" motion)
	// it goes anticlockwise from 0 looking at west, to pi looking at east
	//camera pitch goes between -pi/2 and pi/2 ("vertical" motion)
	float camera_pitch = 0;
	float camera_roll = 0;

	//x, y, z
	Vector3 speed;
	float modifier = 1.0;
	DWORD speed_address1;
	DWORD SPEED_ADDRESSES[] = { 0, 0, 0 };
	//read speed addresses
	ReadProcessMemory(hProcess, (LPCVOID)(base_addr + SPEED_OFFSET), &speed_address1, sizeof(DWORD), NULL);
	SPEED_ADDRESSES[0] = speed_address1 + 0x44;
	SPEED_ADDRESSES[1] = speed_address1 + 0x48;
	SPEED_ADDRESSES[2] = speed_address1 + 0x4C;
	
	bool fly_enabled = false;

	// for god mode
	const float DESIRED_HEALTH = 200;
	DWORD health_address1;
	
	while (true) {
		ReadProcessMemory(hProcess, (LPCVOID)(CAMERA_ROLL_ADDRESS), &camera_roll, sizeof(float), NULL);
		ReadProcessMemory(hProcess, (LPCVOID)(CAMERA_PITCH_ADDRESS), &camera_pitch, sizeof(float), NULL);

		//write health
		ReadProcessMemory(hProcess, (LPCVOID)(base_addr + HEALTH_OFFSETS[0]), &health_address1, sizeof(DWORD), NULL);
		WriteProcessMemory(hProcess, (LPVOID)(health_address1 + HEALTH_OFFSETS[1]), &DESIRED_HEALTH, sizeof(float), NULL);


		if (GetKeyState('B') & 0x8000) {
			fly_enabled = !fly_enabled;
		}

		//quit
		if (GetKeyState(VK_F4) && 0x8000) {
			printf_s("Closing process...\n");
			break;
		}


		if (!fly_enabled) {
			continue;
		}

		if (GetKeyState(VK_UP) & 0x8000) {
			modifier += 0.1;
		}

		if (GetKeyState(VK_DOWN) & 0x8000) {
			modifier -= 0.1;
		}

		modifier = clamp(modifier, 0.1, 5.0);

		speed = Vector3();
		

		//higher order bit is 1 when pressed
		if (GetAsyncKeyState(VK_SHIFT) && 0x8000) {
			speed.z += 2.0 * JUMP_SPEED;
		}
		if (GetAsyncKeyState(VK_LCONTROL) && 0x8000) {
			speed.z -= JUMP_SPEED;
		}
		//go towards camera target
		if (GetAsyncKeyState('W') && 0x8000) {
			speed = speed + Vector3(- cos(camera_roll), -sin(camera_roll), 0.1 + sin(camera_pitch));
		}
		//back
		if (GetAsyncKeyState('S') && 0x8000) {
			speed = speed + Vector3(cos(camera_roll), sin(camera_roll), -0.1 - sin(camera_pitch));
		}
		//left
		if (GetAsyncKeyState('A') && 0x8000) {
			speed = speed + Vector3(cos(camera_roll - PI / 2), sin(camera_roll - PI / 2), 0);
		}
		//right
		if (GetAsyncKeyState('D') && 0x8000) {
			speed = speed + Vector3(cos(camera_roll + PI / 2), sin(camera_roll + PI / 2), 0);
		}
		
		WriteSpeed(hProcess, SPEED_ADDRESSES, speed * modifier);
	}
	
	return 0;
}


void WriteSpeed(HANDLE hProcess, DWORD addresses[], Vector3 speed)
{
	WriteProcessMemory(hProcess, (LPVOID)(addresses[0]), &speed.x, sizeof(float), NULL);
	WriteProcessMemory(hProcess, (LPVOID)(addresses[1]), &speed.y, sizeof(float), NULL);
	WriteProcessMemory(hProcess, (LPVOID)(addresses[2]), &speed.z, sizeof(float), NULL);
}


float clamp(float v, float min, float max)
{
	const float t = v < min ? min : v;
	return t > max ? max : t;
}