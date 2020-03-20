// SampleMod.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "SampleMod.h"
#include "logger.h"
#include "il2cpp_functions.hpp"
#include "il2cpp_utils.hpp"
extern "C" { // name mangle 
#include <funchook.h>
}

static uint64_t il2cpp_string_new_orig_offset;
void* (*il2cpp_string_new_orig)(const char* text);



static void (*gunfire_orig)(void* self);
typedef void (*gunfire)(void* self);
static void gunfire_hook(void* self)
{
	LOG("Gun::Fire() hook called");
	return gunfire_orig(self);
}

static uint64_t gunreload_tramp = NULL;
typedef void (*gunreload)(void* self, bool ignoreBulletWaste);
//void h_gunreload(void* self, bool ignoreBulletWaste)
//{
//	LOG("Gun::Reload() hook called");
//	return PLH::FnCast(gunreload_tramp, gunreload)(self, ignoreBulletWaste);
//}

funchook_t* funchook;

SAMPLEMOD_API int load(HANDLE logHandle, HMODULE gameAssembly) {
	init_logger(logHandle);
	LOG("Beginning load!\n");
	il2cpp_functions::Init(gameAssembly);
	// Install hooks onto gameAssembly here
	auto attemptProc = GetProcAddress(gameAssembly, "il2cpp_string_new");
	auto base = (uint64_t)gameAssembly;
	LOG("GameAssembly.dll base: %lx\n", base);
	auto gunfire_ptr = il2cpp_utils::GetMethod("", "Gun", "Fire", 0)->methodPointer;
	LOG("Gun::Fire() proc address: %p\n", gunfire_ptr);
	LOG("Attempting to patch gunfire...\n");

	
	funchook = funchook_create();
	gunfire_orig = (gunfire)il2cpp_utils::GetMethod("", "Gun", "Fire", 0)->methodPointer;
	funchook_prepare(funchook, (void**)&gunfire_orig, gunfire_hook);
	
	int rv = funchook_install(funchook, 0);
	if (rv != 0) {
		LOG("ERROR: failed to install open and fopen hooks. (%s)\n", funchook_error_message(funchook));
		return -1;
	}	

	LOG("Installed hooks!!\n");
	return 0;
}
