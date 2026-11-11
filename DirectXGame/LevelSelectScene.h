#pragma once

#include "BackgroundModel.h"
#include "CameraController.h"
#include "Fade.h"
#include "GameAssets.h"
#include "InputManager.h"
#include "KamataEngine.h"
#include "Skydome.h"
#include "UIMenu.h"

#include <cstdint>
#include <memory>
#include <vector>

class StageManager;

/// <summary>
/// レベルセレクト(ステージ選択)シーン。
/// StageManagerに読み込まれている全ステージを、1ステージにつき1ボタンで一覧表示する。
/// ボタンをクリックすると StageManager::SetCurrentStageIndex を呼んだ上でゲームプレイへ遷移する。
/// </summary>
class LevelSelectScene {
public:
	void Initialize(StageManager* stageManager, bool withScreenFadeIn = true); // LevelSelectScene
	void Update();
	void Draw();

	~LevelSelectScene();

	enum class Phase {
		kFadeIn,       // 画面フェードイン
		kButtonFadeIn, // ボタンのみフェードイン
		kMain,
		kFadeOut,       // 画面フェードアウト(ステージ選択→ゲームプレイ用)
		kButtonFadeOut, // ボタンのみフェードアウト(戻る→タイトル用)
	};

	bool isFinished() const { return finished_; }

	// trueなら「戻る」でメインメニューへ、falseなら選択したステージでゲームプレイへ
	bool isReturningToMenu() const { return returningToMenu_; }

private:
	bool finished_ = false;
	bool returningToMenu_ = false;

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;

	StageManager* stageManager_ = nullptr;

	UIMenu menuInput_;

	uint32_t buttonTex_[3] = {0, 0, 0};
	uint32_t buttonBackTex_ = 0;
	uint32_t cursorTex_ = 0;

	std::vector<UIMenu::ButtonID> stageButtons_;
	UIMenu::ButtonID backButton_ = UIMenu::kInvalidButton;

	// ボタンフェードの速さ(画面フェードより素早く)
	static inline float kButtonFadeDuration = 0.25f;

	KamataEngine::Camera camera_{};

	// メニュー用アセット
	GameAssets assets_;

	// メニュー共通の背景とスカイドーム
	std::unique_ptr<Skydome> skyDome_;
	std::unique_ptr<BackgroundModel> background_;
	std::unique_ptr<CameraController> cameraController_;
};