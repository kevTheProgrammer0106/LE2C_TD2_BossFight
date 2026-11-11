#define NOMINMAX

#include "AudioSettings.h"
#include "GlobalVariables.h"

#include <algorithm>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "AudioSettings";
float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }
} // namespace

void AudioSettings::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "MasterVolume", kMasterVolume);
	gv->AddItem(kGroupName, "MusicVolume", kMusicVolume);
	gv->AddItem(kGroupName, "SfxVolume", kSfxVolume);
}

void AudioSettings::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kMasterVolume = Clamp01(gv->GetFloatValue(kGroupName, "MasterVolume"));
	kMusicVolume = Clamp01(gv->GetFloatValue(kGroupName, "MusicVolume"));
	kSfxVolume = Clamp01(gv->GetFloatValue(kGroupName, "SfxVolume"));
}

void AudioSettings::SyncToGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->SetValue(kGroupName, "MasterVolume", kMasterVolume);
	gv->SetValue(kGroupName, "MusicVolume", kMusicVolume);
	gv->SetValue(kGroupName, "SfxVolume", kSfxVolume);
}

void AudioSettings::SetMasterVolume(float volume) {
	kMasterVolume = Clamp01(volume);
	SyncToGlobalVariables();
}
void AudioSettings::SetMusicVolume(float volume) {
	kMusicVolume = Clamp01(volume);
	SyncToGlobalVariables();
}
void AudioSettings::SetSfxVolume(float volume) {
	kSfxVolume = Clamp01(volume);
	SyncToGlobalVariables();
}

void AudioSettings::SaveToFile() { GlobalVariables::GetInstance()->SaveFile(kGroupName); }

uint32_t AudioSettings::PlaySfx(uint32_t soundDataHandle, bool loop, float volume) { return Audio::GetInstance()->PlayWave(soundDataHandle, loop, volume * GetSfxVolume()); }

void AudioSettings::DrawImGui() {
#ifdef _DEBUG
	ImGui::Begin("Audio Settings");

	bool changed = false;
	bool released = false;
	auto slider = [&](const char* label, float& value) {
		if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f, "%.2f"))
			changed = true;
		if (ImGui::IsItemDeactivatedAfterEdit())
			released = true;
	};

	slider("Master", kMasterVolume);
	slider("Music", kMusicVolume);
	slider("SFX", kSfxVolume);

	if (ImGui::Button("Reset to 1.0")) {
		kMasterVolume = kMusicVolume = kSfxVolume = 1.0f;
		changed = true;
		released = true;
	}
	ImGui::TextDisabled("Music applies live. SFX applies to the next sound played.");

	if (changed)
		SyncToGlobalVariables();
	if (released)
		SaveToFile();

	ImGui::End();
#endif
}