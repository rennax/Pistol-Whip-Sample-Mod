// SampleMod.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "SampleMod.h"
#include "logger.h"
#include "il2cpp_functions.hpp"
#include "il2cpp_utils.hpp"
#include "utils-functions.hpp"



#include <string>
extern "C" { // name mangle 
#include <funchook.h>
}

//Fast port of https://github.com/Kylemc1413/QuestMods

static Il2CppObject* GameData;
static float lastAcc = -1;   // To ensure it updates immediately
static int lastBullets = -1; // To ensure it updates immediately
static int onBeatHits = 0;
static int totalHits = 0;
static char bufferAcc[512];
static char bufferBeat[512];


static void(*playerActionManagerGameStart_orig)(void* self, void* e);
typedef void(*playerActionManagerGameStart)(void* self, void* e);
static void playerActionManagerGameStart_hook(void* self, void* e)
{
    LOG("Called playerActionManager::GameStart() hook!\n");
    playerActionManagerGameStart_orig(self, e);
    GameData = nullptr;
    GameData = il2cpp_utils::GetFieldValue(reinterpret_cast<Il2CppObject*>(self), "playerData");
    LOG("GameData: %p\n", GameData);
    onBeatHits = 0;
    totalHits = 0;
    lastAcc = -1;
    lastBullets = -1;
    LOG("Finished PlayerActionManager GameStart Hook!\n");
    return;
}



static void(*gunAmmoDisplayUpdate_orig)(void* self);
typedef void(*gunAmmoDisplayUpdate)(void* self);
static void gunAmmoDisplayUpdate_hook(void* self)
{
    gunAmmoDisplayUpdate_orig(self);

    static auto tmp_text_class = il2cpp_utils::GetClassFromName("TMPro", "TMP_Text");
    static auto get_text_method = il2cpp_utils::GetMethod(tmp_text_class, "get_text", 0);
    static auto set_text_method = il2cpp_utils::GetMethod(tmp_text_class, "set_text", 1);
    static auto set_richText_method = il2cpp_utils::GetMethod(tmp_text_class, "set_richText", 1);
    if (GameData == nullptr)
    {
        return;
    }
    int bulletCount;
    il2cpp_utils::GetFieldValue(&bulletCount, reinterpret_cast<Il2CppObject*>(self), "currentBulletCount");
    // int previousBulletCount;
    // il2cpp_utils::GetFieldValue(&previousBulletCount, reinterpret_cast<Il2CppObject*>(self), "previousBulletCount");
    // if (bulletCount == previousBulletCount && bulletCount == lastBullets) return; // No shot fired
    if (bulletCount == lastBullets)
        return; // No shot fired
    float accuracy;
    //   float beatAccuracy = 0;
    //   if(totalHits > 0)
    il2cpp_utils::GetFieldValue(&accuracy, GameData, "accuracy");
    if (lastAcc == accuracy)
        return; // No accuracy change
    LOG("OnBeat Hits: %i | Total Hits: %i\n", onBeatHits, totalHits);

    float beatAccuracy = 0;
    if (totalHits != 0)
        beatAccuracy = (float)onBeatHits / (float)totalHits;
    Il2CppObject* displayTextObj = il2cpp_utils::GetFieldValue((Il2CppObject*)self, "displayText");

    LOG("displayTextObj: %p\n", displayTextObj);
    Il2CppString* displayText;
    il2cpp_utils::RunMethod(&displayText, displayTextObj, get_text_method);
    LOG("displayText: %p\n", displayText);

    std::string text = to_utf8(csstrtostr(displayText));
    LOG("displayText text: %s\n", text.data());
    LOG("Accuracy: %.2f\n", accuracy);

    sprintf_s(bufferAcc, "(%.2f%%)", accuracy * 100.0f);

    sprintf_s(bufferBeat, "(%.2f%%)", beatAccuracy * 100.0f);
    text = std::to_string(bulletCount) + "\n<size=50%>" + std::string(bufferAcc) + "<size=40%>Acc\n<size=50%>" + std::string(bufferBeat) + "<size=40%>Beat";
    LOG("Updated text: %s\n", text.data());
    bool richTextValue = true;
    il2cpp_utils::RunMethod(displayTextObj, set_richText_method, &richTextValue);
    il2cpp_utils::RunMethod(displayTextObj, set_text_method, il2cpp_functions::string_new(text.data()));

    lastAcc = accuracy;
    lastBullets = bulletCount;
    return;
}

static void (*gunfire_orig)(void* self);
typedef void (*gunfire)(void* self);
static void gunfire_hook(void* self)
{
	LOG("Gun::Fire() hook called\n");
    lastAcc = -1;
	return gunfire_orig(self);
}

static void (*gunreload_orig)(void* self, bool ignoreBulletWaste);
typedef void (*gunreload)(void* self, bool ignoreBulletWaste);
void gunreload_hook(void* self, bool ignoreBulletWaste)
{
	LOG("Gun::Reload() hook called\n");
    lastAcc = -1;
	return gunreload_orig(self, ignoreBulletWaste);
}

static void (*gameDataAddScore_orig)(void* self, void* scoreItem);
typedef void (*gameDataAddScore)(void* self, void* scoreItem);
static void gameDataAddScore_hook(void* self, void* scoreItem)
{
    LOG("GameData::AddScore() hook called\n");
    gameDataAddScore_orig(self, scoreItem);

    int onBeatValue;
    auto tmp = reinterpret_cast<int*>(scoreItem);
    onBeatValue = tmp[5];
    if (onBeatValue == 100)
        onBeatHits++;
    totalHits++;
    LOG("onBeatValue of score being added: %i", onBeatValue);
}

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

    gunreload_orig = (gunreload)il2cpp_utils::GetMethod("", "Gun", "Reload", 1)->methodPointer;
    funchook_prepare(funchook, (void**)&gunreload_orig, gunreload_hook);

    gameDataAddScore_orig = (gameDataAddScore)il2cpp_utils::GetMethod("", "GameData", "AddScore", 1)->methodPointer;
    funchook_prepare(funchook, (void**)&gameDataAddScore_orig, gameDataAddScore_hook);

    playerActionManagerGameStart_orig = (playerActionManagerGameStart)il2cpp_utils::GetMethod("", "PlayerActionManager", "OnGameStart", 1)->methodPointer;
    funchook_prepare(funchook, (void**)&playerActionManagerGameStart_orig, playerActionManagerGameStart_hook);

    gunAmmoDisplayUpdate_orig = (gunAmmoDisplayUpdate)il2cpp_utils::GetMethod("", "GunAmmoDisplay", "Update", 0)->methodPointer;
    funchook_prepare(funchook, (void**)&gunAmmoDisplayUpdate_orig, gunAmmoDisplayUpdate_hook);
	
	int rv = funchook_install(funchook, 0);
	if (rv != 0) {
		LOG("ERROR: failed to install open and fopen hooks. (%s)\n", funchook_error_message(funchook));
		return -1;
	}	

	LOG("Installed hooks!!\n");
	return 0;
}
