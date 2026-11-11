#define NOMINMAX
#include "GameSettings.h"
#include "GlobalVariables.h"

#include <algorithm>

namespace {
constexpr const char* kGroupName = "GameSettings";
}

void GameSettings::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "ScreenShake", kScreenShake);
	gv->AddItem(kGroupName, "ScreenFlash", kScreenFlash);
	gv->AddItem(kGroupName, "Difficulty", kDifficulty);
	gv->AddItem(kGroupName, "HardDamageScale", kHardDamageScale);
}

void GameSettings::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kScreenShake = gv->GetIntValue(kGroupName, "ScreenShake") != 0 ? 1 : 0;
	kScreenFlash = gv->GetIntValue(kGroupName, "ScreenFlash") != 0 ? 1 : 0;
	kDifficulty = std::clamp(gv->GetIntValue(kGroupName, "Difficulty"), 0, 2);
	kHardDamageScale = std::max(gv->GetFloatValue(kGroupName, "HardDamageScale"), 0.1f);
}

void GameSettings::SyncToGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->SetValue(kGroupName, "ScreenShake", kScreenShake);
	gv->SetValue(kGroupName, "ScreenFlash", kScreenFlash);
	gv->SetValue(kGroupName, "Difficulty", kDifficulty);
}

void GameSettings::SetScreenShakeEnabled(bool enabled) {
	kScreenShake = enabled ? 1 : 0;
	SyncToGlobalVariables();
}
void GameSettings::SetScreenFlashEnabled(bool enabled) {
	kScreenFlash = enabled ? 1 : 0;
	SyncToGlobalVariables();
}
void GameSettings::SetDifficulty(Difficulty difficulty) {
	kDifficulty = static_cast<int32_t>(difficulty);
	SyncToGlobalVariables();
}

void GameSettings::SaveToFile() { GlobalVariables::GetInstance()->SaveFile(kGroupName); }