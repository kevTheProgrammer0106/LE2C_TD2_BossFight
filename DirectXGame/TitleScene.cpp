#include "TitleScene.h"
#include "AudioSettings.h"
#include "InputManager.h"
#include "MusicManager.h"

using namespace KamataEngine;

TitleScene::~TitleScene() { delete fade_; }

void TitleScene::Initialize(bool withScreenFadeIn, bool openCredits) {
	openCreditsOnStart_ = openCredits;

	fade_ = new Fade();
	fade_->Initialize();

	// --- メニュー用アセット ---
	assets_.Load();

	// --- メニュー用カメラ ---
	camera_.farZ = 10000.0f;
	camera_.Initialize();

	// プレイヤーを追従しないメニュー用カメラ
	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize(&camera_);

	camera_.UpdateMatrix();

	// --- Skydome ---
	skyDome_ = std::make_unique<Skydome>();
	skyDome_->Initialize({10.0f, 10.0f, 10.0f}, assets_.modelSkyDome.get(), assets_.skyDomeTex, &camera_, {0.0f, 0.0f, 0.0f});
	skyDome_->RegisterGlobalVariables("Skydome");

	// --- Background ---
	background_ = std::make_unique<BackgroundModel>();
	background_->Initialize(&camera_, assets_.modelBackgroundHill.get(), assets_.modelBackgroundTown.get(), assets_.modelBackgroundTrees.get(), assets_.hillTex, assets_.townTex, assets_.treesTex);

	whiteTex_ = TextureManager::Load("./Resources/UserInterface/buttonBorder.png");

	// --- Game Title ---
	gameTitleTex_ = TextureManager::Load("./Resources/UserInterface/gameTitle.png");

	// タイトル画像を画面上部の中央に配置する
	constexpr float kGameTitleWidth = 720.0f;
	constexpr float kGameTitleHeight = 180.0f;
	constexpr float kGameTitleY = 15.0f;

	gameTitleSprite_.reset(Sprite::Create(gameTitleTex_, {(static_cast<float>(WinApp::kWindowWidth) - kGameTitleWidth) / 2.0f, kGameTitleY}));

	gameTitleSprite_->SetSize({kGameTitleWidth, kGameTitleHeight});

	// --- Main Menu Buttons ---
	buttonTex_[0] = TextureManager::Load("./Resources/UserInterface/buttonPlay.png");
	buttonTex_[1] = TextureManager::Load("./Resources/UserInterface/buttonLevelSelect.png");
	buttonTex_[2] = TextureManager::Load("./Resources/UserInterface/buttonSettings.png");
	buttonTex_[3] = TextureManager::Load("./Resources/UserInterface/buttonCredits.png");

	/*buttonTex_[2] = TextureManager::Load("./Resources/UserInterface/buttonSettings.png");
	buttonTex_[3] = TextureManager::Load("./Resources/UserInterface/buttonCredits.png");*/

	buttonBackTex_ = TextureManager::Load("./Resources/UserInterface/buttonBack.png");

	creditsTex_ = TextureManager::Load("./Resources/UserInterface/creditsPage.png");

	cursorTex_ = TextureManager::Load("./Resources/UserInterface/iconCursor.png");

	menuInput_.Initialize(buttonTex_, 4, &camera_);
	menuInput_.SetButtonGroupAlpha(0.0f); // 画面フェードイン完了後にボタンをフェードインさせる

	// 縦に4つ並べる(ゲーム内ポーズメニューと同じ配置)。全体が画面中央に来るようにずらす
	constexpr float kButtonWidth = 204.8f;
	constexpr float kButtonHeight = 51.2f;
	constexpr float kSpacing = 86.0f;

	const float buttonX = static_cast<float>(WinApp::kWindowWidth) / 2.0f - kButtonWidth / 2.0f;

	const float startY = static_cast<float>(WinApp::kWindowHeight) / 2.0f - kSpacing * 1.5f;

	startButton_ = menuInput_.AddButton(buttonTex_[0], buttonX, startY, kButtonWidth, kButtonHeight, "Start");

	levelSelectButton_ = menuInput_.AddButton(buttonTex_[1], buttonX, startY + kSpacing, kButtonWidth, kButtonHeight, "Level Select");

	settingsButton_ = menuInput_.AddButton(buttonTex_[2], buttonX, startY + kSpacing * 2.0f, kButtonWidth, kButtonHeight, "Settings");

	creditsButton_ = menuInput_.AddButton(buttonTex_[3], buttonX, startY + kSpacing * 3.0f, kButtonWidth, kButtonHeight, "Credits");

	menuInput_.SetButtonOnClick(startButton_, [this]() { BeginExit(NextDestination::kGamePlay, true); });

	menuInput_.SetButtonOnClick(levelSelectButton_, [this]() { BeginExit(NextDestination::kLevelSelect, false); });

	menuInput_.SetButtonOnClick(settingsButton_, [this]() { OpenPanel(Panel::kSettings); });

	menuInput_.SetButtonOnClick(creditsButton_, [this]() { OpenPanel(Panel::kCredits); });

	menuInput_.SetCursorTexture(cursorTex_);

	InitializePanels();

	// メニューBGM
	menuMusicSoundDataHandle_ = Audio::GetInstance()->LoadWave("./Resources/sfx_bgm/YellowMarch.wav");

	musicManager_.InitializeMenuTrack(menuMusicSoundDataHandle_);

	if (withScreenFadeIn) {
		phase_ = Phase::kFadeIn;
		fade_->Start(Fade::Status::FadeIn, 1.0f);
	} else {
		fade_->Stop();
		phase_ = Phase::kButtonFadeIn;
		menuInput_.StartButtonFade(true, kButtonFadeDuration);
	}
}

void TitleScene::InitializePanels() {
	// panel menu: one Back button, used by both Settings and Credits
	panelMenu_.Initialize(buttonBackTex_, &camera_);
	panelMenu_.SetButtonGroupAlpha(0.0f);
	panelMenu_.SetCursorTexture(cursorTex_);

	 constexpr float kBackWidth = 204.8f;
	constexpr float kBackHeight = 51.2f;

	const float centerX = static_cast<float>(WinApp::kWindowWidth) / 2.0f;

	panelBackButton_ = panelMenu_.AddButton(buttonBackTex_, centerX - kBackWidth / 2.0f, static_cast<float>(WinApp::kWindowHeight) - kBackHeight - 30.0f, kBackWidth, kBackHeight, "Back");

	panelMenu_.SetButtonOnClick(panelBackButton_, [this]() { ClosePanel(); });

	settingsPanel_.Initialize(whiteTex_, true); // true: 難易度の行を出す(タイトルのみ)

	// credits image, centered
	constexpr float kCreditsWidth = 640.0f;
	constexpr float kCreditsHeight = 360.0f;

	creditsSprite_.reset(Sprite::Create(creditsTex_, {(static_cast<float>(WinApp::kWindowWidth) - kCreditsWidth) / 2.0f, 120.0f}));

	creditsSprite_->SetSize({kCreditsWidth, kCreditsHeight});
}

// タイトル画像を描画する
void TitleScene::DrawGameTitle() {
	if (!gameTitleSprite_) {
		return;
	}

	Sprite::PreDraw();
	gameTitleSprite_->Draw();
	Sprite::PostDraw();
}

// BeginExit を置き換え
void TitleScene::BeginExit(NextDestination destination, bool screenFade) {
	// LevelSelectへ行く時はメニューBGMを止めない(LevelSelect側で鳴らし続ける)。ゲーム開始時だけフェードアウト
	if (destination == NextDestination::kGamePlay) {
		musicManager_.StopMenuMusic();
	}

	nextDestination_ = destination;

	if (screenFade) {
		phase_ = Phase::kFadeOut;
		fade_->Start(Fade::Status::FadeOut, 1.0f);
	} else {
		phase_ = Phase::kButtonFadeOut;
		menuInput_.StartButtonFade(false, kButtonFadeDuration);
	}
}

void TitleScene::OpenPanel(Panel panel) {
	pendingPanel_ = panel;
	phase_ = Phase::kMenuToPanel;
	menuInput_.StartButtonFade(false, kButtonFadeDuration);
}

void TitleScene::ClosePanel() {
	phase_ = Phase::kPanelToMenu;
	panelMenu_.StartButtonFade(false, kButtonFadeDuration);
	settingsPanel_.CancelDrag();
}

void TitleScene::Update() {
	menuInput_.UpdateCursor();
	panelMenu_.UpdateCursor();
	musicManager_.Update();

	// メニュー背景とスカイドームを更新する
	if (skyDome_) {
		skyDome_->ApplyGlobalVariables();
		skyDome_->Update();
	}

	if (background_) {
		background_->Update();
	}

	// 固定カメラの行列を更新する
	camera_.UpdateMatrix();

	if (openCreditsOnStart_ && phase_ == Phase::kMain) {
		openCreditsOnStart_ = false;
		OpenPanel(Panel::kCredits);
	}

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();

		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kButtonFadeIn;
			menuInput_.StartButtonFade(true, kButtonFadeDuration);
		}
		break;

	case Phase::kButtonFadeIn:
		menuInput_.UpdateButtonFade();
		menuInput_.Update();

		if (menuInput_.IsButtonFadeFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		menuInput_.Update();

		if (phase_ == Phase::kMain && InputManager::GetInstance()->IsJumpTriggered()) {
			BeginExit(NextDestination::kGamePlay, true);
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();

		if (fade_->IsFinished()) {
			finished_ = true; // scene manager swaps to GameScene
		}
		break;

	case Phase::kButtonFadeOut:
		menuInput_.UpdateButtonFade();

		if (menuInput_.IsButtonFadeFinished()) {
			finished_ = true; // scene manager swaps to LevelSelectScene (no screen fade)
		}
		break;

	case Phase::kMenuToPanel:
		menuInput_.UpdateButtonFade();

		if (menuInput_.IsButtonFadeFinished()) {
			currentPanel_ = pendingPanel_;
			phase_ = Phase::kPanel;
			panelMenu_.StartButtonFade(true, kButtonFadeDuration);
		}
		break;

	case Phase::kPanel:
		panelMenu_.UpdateButtonFade();
		panelMenu_.Update();

		if (phase_ == Phase::kPanel && InputManager::GetInstance()->IsPauseTriggered()) {
			ClosePanel();
		}
		break;

	case Phase::kPanelToMenu:
		panelMenu_.UpdateButtonFade();

		if (panelMenu_.IsButtonFadeFinished()) {
			currentPanel_ = Panel::kNone;
			phase_ = Phase::kButtonFadeIn;
			menuInput_.StartButtonFade(true, kButtonFadeDuration);
		}
		break;
	}

	if (currentPanel_ == Panel::kSettings && (phase_ == Phase::kPanel || phase_ == Phase::kPanelToMenu)) {
		settingsPanel_.Update(panelMenu_.GetButtonFadeAlpha(), phase_ == Phase::kPanel);
	}
}

void TitleScene::DrawPanel() {
	const float alpha = panelMenu_.GetButtonFadeAlpha();

	if (currentPanel_ == Panel::kSettings) {
		settingsPanel_.Draw();
	} else if (currentPanel_ == Panel::kCredits && creditsSprite_) {
		Sprite::PreDraw();
		creditsSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		creditsSprite_->Draw();
		Sprite::PostDraw();
	}

	panelMenu_.Draw(); // Back button + cursor
}

void TitleScene::Draw() {
	// 空、背景、UIの順番で描画する
	if (skyDome_) {
		skyDome_->Draw();
	}

	if (background_) {
		background_->Draw();
	}

	switch (phase_) {
	case Phase::kMain:
	case Phase::kButtonFadeIn:
	case Phase::kButtonFadeOut:
	case Phase::kMenuToPanel:
		// メインメニュー表示中はタイトル画像をボタンの後ろに描画する
		DrawGameTitle();
		menuInput_.Draw();
		return;

	case Phase::kPanel:
	case Phase::kPanelToMenu:
		DrawPanel();
		return;

	default:
		// 画面フェード中もタイトル画像を表示する
		DrawGameTitle();
		break;
	}

	fade_->Draw();
}