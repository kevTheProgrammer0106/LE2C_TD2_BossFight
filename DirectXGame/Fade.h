#pragma once
#include "KamataEngine.h"

/// <summary>
///	フェード
/// </summary>
class Fade {
public:
	void Initialize();
	void Update();
	void Draw();

	enum class Status {
		None,   // フェードなし
		FadeIn, // フェードイン
		FadeOut // フェードアウト
	};
	// 現在のフェードの状態
	Status status_ = Status::None;

	//フェード開始
	void Start(Status status, float duration);
	void Stop() { status_ = Status::None; };
	bool IsFinished() const;

private:
	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* sprite_ = nullptr;

	// フェードの特続時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;
};