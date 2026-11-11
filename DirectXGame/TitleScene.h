#pragma once

#include "BackgroundModel.h"
#include "CameraController.h"
#include "Fade.h"
#include "GameAssets.h"
#include "InputManager.h"
#include "KamataEngine.h"
#include "MusicManager.h"
#include "SettingsPanel.h"
#include "Skydome.h"
#include "UIMenu.h"

#include <memory>

/// <summary>
/// タイトルシーン(メインメニュー)
/// Start / Level Select / Settings / Credits。Settings と Credits は同じシーン内のパネルとして表示する。
/// </summary>
class TitleScene {
public:
	void Initialize(bool withScreenFadeIn = true, bool openCredits = false);
	void Update();
	void Draw();

	~TitleScene();

	enum class Phase {
		kFadeIn,        // 画面フェードイン
		kButtonFadeIn,  // ボタンのみフェードイン
		kMain,          // メイン処理
		kFadeOut,       // 画面フェードアウト(Play遷移用)
		kButtonFadeOut, // ボタンのみフェードアウト(Level Select遷移用)
		kMenuToPanel,   // メインボタンをフェードアウトしてパネルへ
		kPanel,         // Settings / Credits 表示中
		kPanelToMenu,   // パネルをフェードアウトしてメインへ戻る
	};

	enum class NextDestination { kNone, kGamePlay, kLevelSelect };

	bool isFinished() const { return finished_; }
	NextDestination GetNextDestination() const { return nextDestination_; }

private:
	enum class Panel { kNone, kSettings, kCredits };

	// Play / Level Select への遷移を一本化(BGM停止 + 行き先設定 + フェード開始)
	void BeginExit(NextDestination destination, bool screenFade);

	// Settings / Credits 共通の開閉
	void OpenPanel(Panel panel);
	void ClosePanel();

	void InitializePanels();
	void DrawPanel();

	// タイトル画像を描画する
	void DrawGameTitle();

	bool finished_ = false;

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;
	NextDestination nextDestination_ = NextDestination::kNone;

	// --- main menu ---
	UIMenu menuInput_;
	uint32_t buttonTex_[4] = {0, 0, 0, 0}; // play, level select, settings, credits
	uint32_t cursorTex_ = 0;

	UIMenu::ButtonID startButton_ = UIMenu::kInvalidButton;
	UIMenu::ButtonID levelSelectButton_ = UIMenu::kInvalidButton;
	UIMenu::ButtonID settingsButton_ = UIMenu::kInvalidButton;
	UIMenu::ButtonID creditsButton_ = UIMenu::kInvalidButton;

	// --- panels (shared Back button) ---
	UIMenu panelMenu_;
	UIMenu::ButtonID panelBackButton_ = UIMenu::kInvalidButton;
	uint32_t buttonBackTex_ = 0;
	Panel currentPanel_ = Panel::kNone;
	Panel pendingPanel_ = Panel::kNone;

	SettingsPanel settingsPanel_; // 音量 / 画面揺れ / フラッシュ / 難易度

	// --- credits ---
	uint32_t creditsTex_ = 0;
	std::unique_ptr<KamataEngine::Sprite> creditsSprite_;

	// --- game title ---
	uint32_t gameTitleTex_ = 0;
	std::unique_ptr<KamataEngine::Sprite> gameTitleSprite_;

	uint32_t whiteTex_ = 0;

	static inline float kButtonFadeDuration = 0.25f;

	KamataEngine::Camera camera_{};

	// メニュー用アセット
	GameAssets assets_;

	// メニュー共通の背景とスカイドーム
	std::unique_ptr<Skydome> skyDome_;
	std::unique_ptr<BackgroundModel> background_;
	std::unique_ptr<CameraController> cameraController_;

	MusicManager musicManager_;
	uint32_t menuMusicSoundDataHandle_ = 0;

	bool openCreditsOnStart_ = false;
};