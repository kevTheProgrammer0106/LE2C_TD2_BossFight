#include "Fade.h"
#include <algorithm>

using namespace KamataEngine;

void Fade::Initialize() {
	// 初期化処理

	textureHandle_ = TextureManager::Load("./Resources/white1x1.png");
	sprite_ = Sprite::Create(textureHandle_, Vector2(0, 0), Vector4(0, 0, 0, 1), Vector2(0.0f, 0.0f));
	sprite_->SetSize(Vector2(WinApp::kWindowWidth, WinApp::kWindowHeight));
	sprite_->SetColor(Vector4(0, 0, 0, 1)); // 黒色で初期化
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Update() {
	// 更新処理
	switch (status_) {
	case Status::None:
		break;
	case Status::FadeIn:
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;
		// フェードインの進行度を計算
		if (counter_ >= duration_) {
			counter_ = duration_;
		}
		// 1.0fから0.0fの範囲で進行度を計算
		sprite_->SetColor(Vector4(0, 0, 0, 1.0f - std::clamp(counter_ / duration_, 0.0f, 1.0f)));
		break;
	case Status::FadeOut:

		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;
		// フェードインの進行度を計算
		if (counter_ >= duration_) {
			counter_ = duration_;
		}
		// 0.0fから1.0fの範囲で進行度を計算
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));
		break;
	}
}

bool Fade::IsFinished() const 
{ 
	switch (status_) 
	{
	case Status::FadeIn:
	case Status::FadeOut:
		if (counter_ >= duration_) {
			return true; // フェードが完了した
		} else {
			return false; // フェードがまだ完了していない
		}
	}
	return true;
}


void Fade::Draw() {
	if (status_ == Status::None) {
		return; // フェードなしの場合は描画しない
	}
	// 描画処理
	Sprite::PreDraw();
	sprite_->Draw();
	Sprite::PostDraw();
}