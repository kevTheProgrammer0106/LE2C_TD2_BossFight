#pragma once
#include "KamataEngine.h"
#include <memory>

class Player;

/// <summary>
/// プレイ中の画面UIをすべて担当するクラス。GameScene からは Initialize / Update / Draw の3つだけ呼ぶ。
///
/// UIを足すときの決まり:
///  1. UIごとに InitializeXxx / UpdateXxx / DrawXxx の3つ組を作る
///  2. Initialize / Update / Draw からその3つ組を呼ぶ (Draw は呼ぶ順番 = 重なり順。後に呼ぶほど手前)
///  3. ゲームのロジックは書かない。値は Player や敵から「読むだけ」にする
///  4. 位置や大きさの定数は GameUI.cpp の先頭にまとめる
/// </summary>
class GameUI {
public:
	void Initialize();
	// player が nullptr ならプレイヤーのUIは出さない。
	void Update(const Player* player);
	// 3Dモデルの描画後、フェードの前に呼ぶ
	void Draw();

private:
	// ================= 共通部品 =================
	// ゲージ1本 = 枠 + 中身。枠は内側が黒く塗られているので、枠 -> 中身の順に描く。
	struct Gauge {
		std::unique_ptr<KamataEngine::Sprite> frame;
		std::unique_ptr<KamataEngine::Sprite> fill;
		float ratio = 1.0f;
	};
	Gauge CreateGauge(uint32_t frameTexture, uint32_t fillTexture) const;
	// 位置・拡大率・割合(0.0-1.0)をゲージに反映する。中身は右側を切り取って割合の長さにする。
	static void SetGauge(Gauge& gauge, const KamataEngine::Vector2& position, float scale, float ratio);
	static void DrawGauge(const Gauge& gauge);

	// ================= プレイヤーUI =================
	void InitializePlayerUI();
	void UpdatePlayerUI(const Player* player);
	void DrawPlayerUI();

	Gauge playerHP_;
	Gauge playerStamina_;
	bool showPlayerUI_ = false;

	std::unique_ptr<KamataEngine::Sprite> playerHPLabel_;
	std::unique_ptr<KamataEngine::Sprite> playerStaminaLabel_;

	// ================= テクスチャ =================
	uint32_t gaugeFrameTexture_ = 0;
	uint32_t hpGaugeTexture_ = 0;
	uint32_t staminaGaugeTexture_ = 0;
	uint32_t hpLabelTexture_ = 0;
	uint32_t staminaLabelTexture_ = 0;
};