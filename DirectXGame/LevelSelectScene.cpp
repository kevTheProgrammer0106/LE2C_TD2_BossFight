#include "LevelSelectScene.h"
#include "InputManager.h"
#include "StageManager.h"

using namespace KamataEngine;

LevelSelectScene::~LevelSelectScene() { delete fade_; }

void LevelSelectScene::Initialize(StageManager* stageManager, bool withScreenFadeIn) {
	stageManager_ = stageManager;

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

	// レベルごとに別のボタンテクスチャを使用する。
	buttonTex_[0] = TextureManager::Load("./Resources/UserInterface/buttonLevelOne.png");
	buttonTex_[1] = TextureManager::Load("./Resources/UserInterface/buttonLevelTwo.png");
	buttonTex_[2] = TextureManager::Load("./Resources/UserInterface/buttonLevelThree.png");

	buttonBackTex_ = TextureManager::Load("./Resources/UserInterface/buttonBack.png");

	menuInput_.Initialize(buttonTex_, 3, &camera_);
	menuInput_.SetButtonGroupAlpha(0.0f);

	constexpr float kButtonWidth = 204.8f;
	constexpr float kButtonHeight = 51.2f;
	constexpr float kButtonSpacing = 86.0f;

	float centerX = static_cast<float>(WinApp::kWindowWidth) / 2.0f - kButtonWidth / 2.0f;

	float startY = 160.0f;

	int32_t stageCount = stageManager_->GetStageCount();

	for (int32_t stageIndex = 0; stageIndex < stageCount; ++stageIndex) {
		const StageData& data = stageManager_->GetStageData(stageIndex);

		float y = startY + static_cast<float>(stageIndex) * kButtonSpacing;

		// ステージ番号に対応するテクスチャを使用する。
		UIMenu::ButtonID id = menuInput_.AddButton(static_cast<size_t>(stageIndex), centerX, y, kButtonWidth, kButtonHeight, data.name);

		int32_t capturedIndex = stageIndex;

		menuInput_.SetButtonOnClick(id, [this, capturedIndex]() {
			stageManager_->SetCurrentStageIndex(capturedIndex);
			returningToMenu_ = false;
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		});

		stageButtons_.push_back(id);
	}

	float backY = startY + static_cast<float>(stageButtons_.size()) * kButtonSpacing + 40.0f;

	// BackはLevel1のテクスチャを流用。
	backButton_ = menuInput_.AddButton(buttonBackTex_, centerX, backY, kButtonWidth, kButtonHeight, "Back");

	menuInput_.SetButtonOnClick(backButton_, [this]() {
		returningToMenu_ = true;
		phase_ = Phase::kButtonFadeOut;
		menuInput_.StartButtonFade(false, kButtonFadeDuration);
	});

	cursorTex_ = TextureManager::Load("./Resources/UserInterface/iconCursor.png");
	menuInput_.SetCursorTexture(cursorTex_);

	if (withScreenFadeIn) {
		phase_ = Phase::kFadeIn;
		fade_->Start(Fade::Status::FadeIn, 1.0f);
	} else {
		fade_->Stop();
		phase_ = Phase::kButtonFadeIn;
		menuInput_.StartButtonFade(true, kButtonFadeDuration);
	}
}

void LevelSelectScene::Update() {
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

	menuInput_.UpdateCursor();

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
		if (InputManager::GetInstance()->IsPauseTriggered()) {
			returningToMenu_ = true;
			phase_ = Phase::kButtonFadeOut;
			menuInput_.StartButtonFade(false, kButtonFadeDuration);
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;

	case Phase::kButtonFadeOut:
		menuInput_.UpdateButtonFade();
		if (menuInput_.IsButtonFadeFinished()) {
			finished_ = true; // scene manager will swap to TitleScene (no screen fade)
		}
		break;
	}
}

void LevelSelectScene::Draw() {
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
		menuInput_.Draw();
		return;

	default:
		break;
	}

	fade_->Draw();
}