#pragma once

#include <cstdint>

/// ゲーム全体の設定(画面揺れ / 画面フラッシュ / 難易度)。
/// AudioSettings と同じく GlobalVariables のグループ "GameSettings" に保存される。
class GameSettings {
public:
	// 数値はセーブデータ(json)に入るので、既存の Normal=0 / Hard=1 は変えない。Easy は後ろに追加
	enum class Difficulty : int32_t { kNormal = 0, kHard = 1, kEasy = 2 };

	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();
	static void SaveToFile();

	static bool IsScreenShakeEnabled() { return kScreenShake != 0; }
	static bool IsScreenFlashEnabled() { return kScreenFlash != 0; }
	static Difficulty GetDifficulty() {
		switch (kDifficulty) {
		case 1:
			return Difficulty::kHard;
		case 2:
			return Difficulty::kEasy;
		default:
			return Difficulty::kNormal;
		}
	}

	static void SetScreenShakeEnabled(bool enabled);
	static void SetScreenFlashEnabled(bool enabled);
	static void SetDifficulty(Difficulty difficulty);

	// 難易度ごとの被ダメージ倍率(Normal=1.0)
	static float GetIncomingDamageScale() { return GetDifficulty() == Difficulty::kHard ? kHardDamageScale : 1.0f; }

private:
	static void SyncToGlobalVariables();

	static inline int32_t kScreenShake = 1;
	static inline int32_t kScreenFlash = 1;
	static inline int32_t kDifficulty = 0;
	static inline float kHardDamageScale = 1.5f;
};