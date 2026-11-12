#define NOMINMAX
#include "GameUI.h"
#include "Player.h"
#include <algorithm>

using namespace KamataEngine;

// ==================================================================
// 調整用の定数 (位置・大きさを変えたいときはここだけ触る)
// ==================================================================
namespace {
// ゲージ画像(360x20)の中で、中身が入っている範囲(px)
constexpr float kGaugeTexWidth = 360.0f;
constexpr float kGaugeTexHeight = 20.0f;
constexpr float kGaugeFillLeft = 3.0f;
constexpr float kGaugeFillRight = 357.0f;

// プレイヤーUI
constexpr float kPlayerGaugeScale = 1.5f;
constexpr float kPlayerGaugeBottomMargin = 40.0f; // 画面下端からスタミナゲージの下端までの余白
constexpr float kPlayerGaugeSpacing = 36.0f;      // HPとスタミナの縦の間隔
constexpr float kLabelTexWidth = 96.0f;
constexpr float kLabelTexHeight = 20.0f;
constexpr float kPlayerLabelGap = 10.0f; // ラベルとゲージの間の余白
constexpr float kHPLabelShiftX = 18.0f;  // 画像内でHPの右端がSTAMINAより手前にある分(画像のpx)
} // namespace

// ==================================================================
// 本体 (Initialize / Update / Draw は各UIの関数を呼ぶだけ)
// ==================================================================
void GameUI::Initialize() {
	gaugeFrameTexture_ = TextureManager::Load("./Resources/UserInterface/uiPlayerGaugeFrame.png");
	hpGaugeTexture_ = TextureManager::Load("./Resources/UserInterface/uiPlayerHPGauge.png");
	staminaGaugeTexture_ = TextureManager::Load("./Resources/UserInterface/uiPlayerStaminaGauge.png");
	hpLabelTexture_ = TextureManager::Load("./Resources/UserInterface/uiPlayerHPText.png");
	staminaLabelTexture_ = TextureManager::Load("./Resources/UserInterface/uiPlayerStaminaText.png");

	InitializePlayerUI();
}

void GameUI::Update(const Player* player) { UpdatePlayerUI(player); }

void GameUI::Draw() {
	Sprite::PreDraw();
	// 後に描いたものが手前に出る
	DrawPlayerUI();
	Sprite::PostDraw();
}

// ==================================================================
// 共通部品
// ==================================================================
GameUI::Gauge GameUI::CreateGauge(uint32_t frameTexture, uint32_t fillTexture) const {
	Gauge gauge;
	gauge.frame.reset(Sprite::Create(frameTexture, {0.0f, 0.0f}));
	gauge.fill.reset(Sprite::Create(fillTexture, {0.0f, 0.0f}));
	return gauge;
}

void GameUI::SetGauge(Gauge& gauge, const Vector2& position, float scale, float ratio) {
	gauge.ratio = std::clamp(ratio, 0.0f, 1.0f);

	gauge.frame->SetPosition(position);
	gauge.frame->SetSize({kGaugeTexWidth * scale, kGaugeTexHeight * scale});

	if (gauge.ratio <= 0.0f) {
		return; // 中身は描かないので計算しない
	}
	const float visibleWidth = kGaugeFillLeft + (kGaugeFillRight - kGaugeFillLeft) * gauge.ratio;
	gauge.fill->SetPosition(position);
	gauge.fill->SetTextureRect({0.0f, 0.0f}, {visibleWidth, kGaugeTexHeight});
	gauge.fill->SetSize({visibleWidth * scale, kGaugeTexHeight * scale});
}

void GameUI::DrawGauge(const Gauge& gauge) {
	gauge.frame->Draw(); // 枠(内側が黒)を先に
	if (gauge.ratio > 0.0f) {
		gauge.fill->Draw();
	}
}

// ==================================================================
// プレイヤーUI (HP / スタミナのゲージ)
// ==================================================================
void GameUI::InitializePlayerUI() {
	playerHP_ = CreateGauge(gaugeFrameTexture_, hpGaugeTexture_);
	playerStamina_ = CreateGauge(gaugeFrameTexture_, staminaGaugeTexture_);
	playerHPLabel_.reset(Sprite::Create(hpLabelTexture_, {0.0f, 0.0f}));
	playerStaminaLabel_.reset(Sprite::Create(staminaLabelTexture_, {0.0f, 0.0f}));
	showPlayerUI_ = false;
}

void GameUI::UpdatePlayerUI(const Player* player) {
	showPlayerUI_ = (player != nullptr);
	if (!player) {
		return;
	}
	const float gaugeW = kGaugeTexWidth * kPlayerGaugeScale;
	const float gaugeH = kGaugeTexHeight * kPlayerGaugeScale;
	const float x = (static_cast<float>(WinApp::kWindowWidth) - gaugeW) / 2.0f;
	const float staminaY = static_cast<float>(WinApp::kWindowHeight) - kPlayerGaugeBottomMargin - gaugeH;
	const float hpY = staminaY - kPlayerGaugeSpacing;
	SetGauge(playerHP_, {x, hpY}, kPlayerGaugeScale, player->GetHPRatio());
	SetGauge(playerStamina_, {x, staminaY}, kPlayerGaugeScale, player->GetStaminaRatio());
	const float labelW = kLabelTexWidth * kPlayerGaugeScale;
	const float labelH = kLabelTexHeight * kPlayerGaugeScale;
	const float labelX = x - kPlayerLabelGap - labelW;
	playerHPLabel_->SetPosition({labelX + kHPLabelShiftX * kPlayerGaugeScale, hpY});
	playerHPLabel_->SetSize({labelW, labelH});
	playerStaminaLabel_->SetPosition({labelX, staminaY});
	playerStaminaLabel_->SetSize({labelW, labelH});
}

void GameUI::DrawPlayerUI() {
	if (!showPlayerUI_) {
		return;
	}
	DrawGauge(playerHP_);
	DrawGauge(playerStamina_);
	playerHPLabel_->Draw();
	playerStaminaLabel_->Draw();
}