#pragma once

#define NOMINMAX

#include <KamataEngine.h>
#include <cstdint>

/// <summary>
/// Master / Music / SFX volume settings.
/// Every one-shot SFX in the game goes through PlaySfx(), every BGM volume goes through GetMusicVolume(),
/// so changing a value here is heard immediately by everything.
///
/// For now the values are edited from the ImGui "Audio Settings" window (DrawImGui, called from main.cpp).
/// When the real settings menu exists, call SetMasterVolume / SetMusicVolume / SetSfxVolume / SaveToFile
/// from it and delete the DrawImGui() call - nothing else has to change.
///
/// Values live in the GlobalVariables group "AudioSettings" so they are also saved/loaded as
/// Resources/GlobalVariables/AudioSettings.json like every other tunable.
/// </summary>
class AudioSettings {
public:
	static void RegisterGlobalVariables();
	// Pulls the values back out of GlobalVariables (call once per frame, main.cpp does).
	static void ApplyGlobalVariables();

	// ImGui window (debug builds). Saves to disk when a slider is released.
	static void DrawImGui();

	// 0.0 - 1.0. Setters clamp and write through to GlobalVariables.
	static void SetMasterVolume(float volume);
	static void SetMusicVolume(float volume);
	static void SetSfxVolume(float volume);
	static void SaveToFile();

	static float GetMasterVolume() { return kMasterVolume; }
	static float GetMusicSetting() { return kMusicVolume; }
	static float GetSfxSetting() { return kSfxVolume; }

	// Effective (master * category) multipliers.
	static float GetMusicVolume() { return kMasterVolume * kMusicVolume; }
	static float GetSfxVolume() { return kMasterVolume * kSfxVolume; }

	// Drop-in replacement for Audio::GetInstance()->PlayWave(handle, loop, volume) for sound effects.
	static uint32_t PlaySfx(uint32_t soundDataHandle, bool loop, float volume = 1.0f);

private:
	static void SyncToGlobalVariables();

	static inline float kMasterVolume = 1.0f;
	static inline float kMusicVolume = 1.0f;
	static inline float kSfxVolume = 1.0f;
};