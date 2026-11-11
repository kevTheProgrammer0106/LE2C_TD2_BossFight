#include "MusicManager.h"
#include "AudioSettings.h"
#include "GlobalVariables.h"

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "MusicManager";
}

void MusicManager::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "IngameBaseVolume", kIngameBaseVolume);
	gv->AddItem(kGroupName, "ArenaBaseVolume", kArenaBaseVolume);
	gv->AddItem(kGroupName, "MenuBaseVolume", kMenuBaseVolume);
	gv->AddItem(kGroupName, "FadeSpeed", kFadeSpeed);
}

void MusicManager::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kIngameBaseVolume = gv->GetFloatValue(kGroupName, "IngameBaseVolume");
	kArenaBaseVolume = gv->GetFloatValue(kGroupName, "ArenaBaseVolume");
	kMenuBaseVolume = gv->GetFloatValue(kGroupName, "MenuBaseVolume");
	kFadeSpeed = gv->GetFloatValue(kGroupName, "FadeSpeed");
}

void MusicManager::Initialize(uint32_t ingameSoundDataHandle, uint32_t arenaSoundDataHandle) {
	ingameSoundDataHandle_ = ingameSoundDataHandle;
	arenaSoundDataHandle_ = arenaSoundDataHandle;

	ingameVolumeCurrent_ = 0.0f;
	arenaVolumeCurrent_ = 0.0f;
	ingameVolumeTarget_ = 1.0f; // インゲームBGMはデフォルトで鳴らしておく
	arenaVolumeTarget_ = 0.0f;
	arenaActive_ = false;

	// 既にこのトラック用のボイスが生きていれば(Retry/Restartなど)使い回し、
	// 無ければ新規に再生する。両トラックともループ再生しておき、以降は音量だけを操作する。
	if (ingameVoiceHandle_ == 0 && ingameSoundDataHandle_ != 0) {
		ingameVoiceHandle_ = Audio::GetInstance()->PlayWave(ingameSoundDataHandle_, true, 0.0f);
	}
	if (arenaVoiceHandle_ == 0 && arenaSoundDataHandle_ != 0) {
		arenaVoiceHandle_ = Audio::GetInstance()->PlayWave(arenaSoundDataHandle_, true, 0.0f);
	}

	initialized_ = true;
	ApplyVolumes();
}

void MusicManager::InitializeMenuTrack(uint32_t menuSoundDataHandle) {
	menuSoundDataHandle_ = menuSoundDataHandle;

	// 既に鳴っている場合は音量を0へ戻さず、今の音量から目標へフェードさせる(途切れ防止)
	if (menuVoiceHandle_ == 0 && menuSoundDataHandle_ != 0) {
		menuVolumeCurrent_ = 0.0f;
		menuVoiceHandle_ = Audio::GetInstance()->PlayWave(menuSoundDataHandle_, true, 0.0f);
	}
	menuVolumeTarget_ = 1.0f;

	// ゲーム側のBGMが残っていればメニューへ向けてフェードアウトさせる
	ingameVolumeTarget_ = 0.0f;
	arenaVolumeTarget_ = 0.0f;
	arenaActive_ = false;

	initialized_ = true;
	ApplyVolumes();
}

void MusicManager::StopMenuMusic() { menuVolumeTarget_ = 0.0f; }

void MusicManager::StartArenaMusic() {
	if (arenaActive_)
		return;
	arenaActive_ = true;
	ingameVolumeTarget_ = 0.0f;
	arenaVolumeTarget_ = 1.0f;
}

void MusicManager::StopArenaMusic() {
	if (!arenaActive_)
		return;
	arenaActive_ = false;
	ingameVolumeTarget_ = 1.0f;
	arenaVolumeTarget_ = 0.0f;
}

void MusicManager::Update() {
	if (!initialized_)
		return;

	float step = kFadeSpeed * (1.0f / 60.0f);

	auto stepToward = [step](float current, float target) {
		if (current < target)
			return std::min(current + step, target);
		if (current > target)
			return std::max(current - step, target);
		return current;
	};

	ingameVolumeCurrent_ = stepToward(ingameVolumeCurrent_, ingameVolumeTarget_);
	arenaVolumeCurrent_ = stepToward(arenaVolumeCurrent_, arenaVolumeTarget_);
	menuVolumeCurrent_ = stepToward(menuVolumeCurrent_, menuVolumeTarget_);

	ApplyVolumes();
}

void MusicManager::ApplyVolumes() {
	if (ingameVoiceHandle_ != 0) {
		Audio::GetInstance()->SetVolume(ingameVoiceHandle_, ingameVolumeCurrent_ * kIngameBaseVolume * AudioSettings::GetMusicVolume());
	}
	if (arenaVoiceHandle_ != 0) {
		Audio::GetInstance()->SetVolume(arenaVoiceHandle_, arenaVolumeCurrent_ * kArenaBaseVolume * AudioSettings::GetMusicVolume());
	}
	if (menuVoiceHandle_ != 0) {
		Audio::GetInstance()->SetVolume(menuVoiceHandle_, menuVolumeCurrent_ * kMenuBaseVolume * AudioSettings::GetMusicVolume());
	}
}

void MusicManager::Reset() {
	// メニューへ戻った時にのみ呼ぶ想定。まず現在鳴っているボイスを即座に無音化してから
	// ハンドルを手放す(Stop APIが無い前提なので、孤立したボイスが音量0のまま鳴り続けるだけにする)。
	if (ingameVoiceHandle_ != 0)
		Audio::GetInstance()->SetVolume(ingameVoiceHandle_, 0.0f);
	if (arenaVoiceHandle_ != 0)
		Audio::GetInstance()->SetVolume(arenaVoiceHandle_, 0.0f);
	if (menuVoiceHandle_ != 0)
		Audio::GetInstance()->SetVolume(menuVoiceHandle_, 0.0f);

	ingameVoiceHandle_ = 0;
	arenaVoiceHandle_ = 0;
	menuVoiceHandle_ = 0;
	ingameSoundDataHandle_ = 0;
	arenaSoundDataHandle_ = 0;
	menuSoundDataHandle_ = 0;

	ingameVolumeCurrent_ = 0.0f;
	arenaVolumeCurrent_ = 0.0f;
	menuVolumeCurrent_ = 0.0f;
	ingameVolumeTarget_ = 1.0f;
	arenaVolumeTarget_ = 0.0f;
	menuVolumeTarget_ = 0.0f;

	arenaActive_ = false;
	initialized_ = false;
}