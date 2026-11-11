#define NOMINMAX

#include "UIMenu.h"

using namespace KamataEngine;

void UIMenu::Initialize(uint32_t tex, KamataEngine::Camera* camera) {
	camera_ = camera;

	buttonTextures_.clear();
	buttonTextures_.push_back(tex);

	barTexture_ = TextureManager::Load("./Resources/UserInterface/buttonBorder.png");

	worldTransform_.Initialize();

	Vector2 targetPos = {worldTransform_.translation_.x, worldTransform_.translation_.y};

	cursorSprite_.reset(Sprite::Create(tex, targetPos));
	cursorSprite_->SetAnchorPoint({0.0f, 0.0f});
	cursorSprite_->SetSize({64.0f, 65.0f});

	if (clickSoundDataHandle_ == 0) {
		clickSoundDataHandle_ = Audio::GetInstance()->LoadWave("./Resources/sfx_bgm/defaultSound.wav");
	}
}

void UIMenu::Initialize(const uint32_t* textures, size_t textureCount, KamataEngine::Camera* camera) {
	camera_ = camera;

	buttonTextures_.clear();
	if (textures != nullptr && textureCount > 0) {
		buttonTextures_.assign(textures, textures + textureCount);
	}

	barTexture_ = TextureManager::Load("./Resources/UserInterface/buttonBorder.png");

	worldTransform_.Initialize();

	uint32_t cursorTexture = 0;
	if (!buttonTextures_.empty()) {
		cursorTexture = buttonTextures_[0];
	}

	Vector2 targetPos = {worldTransform_.translation_.x, worldTransform_.translation_.y};

	cursorSprite_.reset(Sprite::Create(cursorTexture, targetPos));
	cursorSprite_->SetAnchorPoint({0.5f, 0.5f});
	cursorSprite_->SetSize({32.0f, 32.0f});

	if (clickSoundDataHandle_ == 0) {
		clickSoundDataHandle_ = Audio::GetInstance()->LoadWave("./Resources/sfx_bgm/defaultSound.wav");
	}
}

/// <summary>
///  Used in kPlay, hides default cursor for custom one
/// </summary>
void UIMenu::UpdateCursor() {
	ShowCursor(false);
	Vector2 targetPos = {worldTransform_.translation_.x, worldTransform_.translation_.y};
	cursorSprite_->SetPosition(targetPos);
}

void UIMenu::Update() {
	// マウス座標/クリック状態はInputManagerから取得する。ここではKamataEngine::Inputへは直接触らない。
	InputManager* input = InputManager::GetInstance();

	float mouseX = static_cast<float>(input->GetMouseX());
	float mouseY = static_cast<float>(input->GetMouseY());

	worldTransform_.translation_ = {mouseX, mouseY, 0.0f};
	WorldTransformUpdate(worldTransform_);

	bool mouseDown = input->IsPrimaryHeld();
	bool mouseTriggeredDown = input->IsPrimaryTriggered();
	UpdateButtons(mouseX, mouseY, mouseDown, mouseTriggeredDown);

#ifdef _DEBUG
	ImGui::Begin("Cursor");
	ImGui::DragFloat3("translate", &worldTransform_.translation_.x);
	ImGui::End();
#endif
}

void UIMenu::Draw() {
	Sprite::PreDraw();
	// 全てのバー(奥) -> 全てのボタン本体(手前)の順。隣のボタンのバーが本体を隠さないようにする
	for (Button& button : buttons_) {
		if (button.visible && button.barSprite) {
			button.barSprite->Draw();
		}
	}
	for (Button& button : buttons_) {
		if (button.visible && button.sprite) {
			// 登場/退場はスライドで表現するので、アルファは常に固定
			button.sprite->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
			button.sprite->Draw();
		}
	}
	if (cursorVisible_) {
		cursorSprite_->Draw();
	}
	Sprite::PostDraw();
}

void UIMenu::StartButtonFade(bool fadeIn, float duration) {
	buttonFadeStatus_ = fadeIn ? ButtonFadeStatus::kFadeIn : ButtonFadeStatus::kFadeOut;
	// 登場はゆっくりイージング、退場は素早く。呼び出し側の値は下限/上限で補正する
	const float clamped = fadeIn ? std::max(duration, kSlideInDuration) : std::min(duration, kSlideOutDuration);
	buttonFadeDuration_ = std::max(clamped, 0.0001f);
	buttonFadeTimer_ = 0.0f;
	buttonFadeAlpha_ = fadeIn ? 0.0f : 1.0f;
	buttonSlide_ = fadeIn ? 0.0f : 1.0f;
}

void UIMenu::UpdateButtonFade() {
	if (buttonFadeStatus_ == ButtonFadeStatus::kNone) {
		return;
	}

	buttonFadeTimer_ += 1.0f / 60.0f;
	if (buttonFadeTimer_ >= buttonFadeDuration_) {
		buttonFadeTimer_ = buttonFadeDuration_;
	}

	float t = std::clamp(buttonFadeTimer_ / buttonFadeDuration_, 0.0f, 1.0f);
	const bool fadeIn = (buttonFadeStatus_ == ButtonFadeStatus::kFadeIn);
	buttonFadeAlpha_ = fadeIn ? t : (1.0f - t);
	// 登場: 減速しながら本来の位置へ / 退場: 加速しながら端へ
	buttonSlide_ = fadeIn ? EaseOut(0.0f, 1.0f, t) : (1.0f - EaseIn(0.0f, 1.0f, t));
}

#pragma region UIボタン(AABB)

bool UIMenu::IsInsideAABB(const ButtonAABB& aabb, float x, float y) { return x >= aabb.left && x <= aabb.right && y >= aabb.top && y <= aabb.bottom; }

UIMenu::SlideSide UIMenu::ResolveSide(const Button& button) const {
	if (button.side != SlideSide::kAuto) {
		return button.side;
	}
	// 画面中央ちょうどのボタンも左扱い(浮動小数の誤差ぶん1pxの余裕を持たせる)
	const float centerX = (button.aabb.left + button.aabb.right) * 0.5f;
	return (centerX <= static_cast<float>(WinApp::kWindowWidth) * 0.5f + 1.0f) ? SlideSide::kLeft : SlideSide::kRight;
}

// 完全に画面外へ出るぶんのずれ(hidden)に、(1 - スライド進行度)を掛ける。
// 左: ホバーで最大まで伸びたバーの右端が画面の左端(x=0)に来る位置
// 右: ホバーで最大まで伸びたバーの左端が画面の右端に来る位置
float UIMenu::ComputeSlideOffset(const Button& button) const {
	const float screenW = static_cast<float>(WinApp::kWindowWidth);
	const float maxExtend = ComputeMaxHoverExtend(button);
	const float hidden = (ResolveSide(button) == SlideSide::kLeft) ? -(button.aabb.right + maxExtend) : (screenW - (button.aabb.left - maxExtend));
	return hidden * (1.0f - buttonSlide_);
}

// バーの基本の長さ: 並び順が1つ後ろになるごとに kLengthStep だけ短くなる。ボタン本体の端より内側には入らない
float UIMenu::ComputeBaseBarEdge(const Button& button) const {
	const float shorten = static_cast<float>(button.index) * kLengthStep;
	if (ResolveSide(button) == SlideSide::kLeft) {
		return std::clamp(button.aabb.right - shorten, button.aabb.left, button.aabb.right);
	}
	return std::clamp(button.aabb.left + shorten, button.aabb.left, button.aabb.right);
}

float UIMenu::ComputeMaxHoverExtend(const Button& button) const { return std::max(kHoverExtend - static_cast<float>(button.index) * kHoverExtendStep, 0.0f); }

// 当たり判定: ボタン本来の高さのまま、画面端からバー(またはボタン本体の遠い端の長い方)まで。スライド位置に追従する
UIMenu::ButtonAABB UIMenu::ComputeHitRect(const Button& button) const {
	const float screenW = static_cast<float>(WinApp::kWindowWidth);
	ButtonAABB rect = button.aabb;
	const float edge = ComputeBaseBarEdge(button);
	if (ResolveSide(button) == SlideSide::kLeft) {
		rect.left = 0.0f;
		rect.right = std::max(edge, button.aabb.right);
	} else {
		rect.left = std::min(edge, button.aabb.left);
		rect.right = screenW;
	}
	const float offset = ComputeSlideOffset(button);
	rect.left += offset;
	rect.right += offset;
	return rect;
}

// バーの表示矩形: 画面端から基本の長さ+ホバーの伸びまで。高さはボタンの kBarHeightRatio で縦中央
UIMenu::ButtonAABB UIMenu::ComputeBarRect(const Button& button) const {
	const float screenW = static_cast<float>(WinApp::kWindowWidth);
	const float height = button.aabb.bottom - button.aabb.top;
	const float barHeight = height * kBarHeightRatio;
	const float centerY = (button.aabb.top + button.aabb.bottom) * 0.5f;

	ButtonAABB rect;
	rect.top = centerY - barHeight * 0.5f;
	rect.bottom = centerY + barHeight * 0.5f;

	const float edge = ComputeBaseBarEdge(button);
	if (ResolveSide(button) == SlideSide::kLeft) {
		rect.left = 0.0f;
		rect.right = edge + button.hoverExtend;
	} else {
		rect.left = edge - button.hoverExtend;
		rect.right = screenW;
	}
	const float offset = ComputeSlideOffset(button);
	rect.left += offset;
	rect.right += offset;
	return rect;
}

// ボタン本体 = 本来のボタン矩形(元のサイズ)をスライドぶんずらしたもの
UIMenu::ButtonAABB UIMenu::ComputeFaceRect(const Button& button) const {
	ButtonAABB rect = button.aabb;
	const float offset = ComputeSlideOffset(button);
	rect.left += offset;
	rect.right += offset;
	return rect;
}

UIMenu::ButtonID UIMenu::AddButton(uint32_t textureHandle, float x, float y, float width, float height, const std::string& label) {
	Button button;
	button.id = nextButtonID_++;
	button.index = buttons_.size();
	button.aabb = {x, y, x + width, y + height};
	button.label = label;

	// バーは本体より先に描くので別スプライト。本体は元のサイズのまま
	button.barSprite.reset(Sprite::Create(barTexture_, {x, y}));
	button.barSprite->SetSize({width, height});
	button.sprite.reset(Sprite::Create(textureHandle, {x, y}));
	button.sprite->SetSize({width, height});

	buttons_.push_back(std::move(button));
	ApplyButtonVisual(buttons_.back());
	return buttons_.back().id;
}
UIMenu::ButtonID UIMenu::AddButton(size_t textureIndex, float x, float y, float width, float height, const std::string& label) {
	if (textureIndex >= buttonTextures_.size()) {
		return kInvalidButton;
	}
	return AddButton(buttonTextures_[textureIndex], x, y, width, height, label);
}

void UIMenu::ClearButtons() { buttons_.clear(); }

UIMenu::Button* UIMenu::FindButton(ButtonID id) {
	for (Button& button : buttons_) {
		if (button.id == id) {
			return &button;
		}
	}
	return nullptr;
}
const UIMenu::Button* UIMenu::FindButton(ButtonID id) const {
	for (const Button& button : buttons_) {
		if (button.id == id) {
			return &button;
		}
	}
	return nullptr;
}

void UIMenu::SetButtonPosition(ButtonID id, float x, float y) {
	Button* button = FindButton(id);
	if (!button) {
		return;
	}
	float width = button->aabb.right - button->aabb.left;
	float height = button->aabb.bottom - button->aabb.top;
	button->aabb = {x, y, x + width, y + height};
	ApplyButtonVisual(*button);
}
void UIMenu::SetButtonSize(ButtonID id, float width, float height) {
	Button* button = FindButton(id);
	if (!button) {
		return;
	}
	button->aabb.right = button->aabb.left + width;
	button->aabb.bottom = button->aabb.top + height;
	ApplyButtonVisual(*button);
}
void UIMenu::SetButtonSlideSide(ButtonID id, SlideSide side) {
	if (Button* button = FindButton(id)) {
		button->side = side;
		ApplyButtonVisual(*button);
	}
}
void UIMenu::SetButtonVisible(ButtonID id, bool visible) {
	if (Button* button = FindButton(id)) {
		button->visible = visible;
	}
}
void UIMenu::SetButtonEnabled(ButtonID id, bool enabled) {
	if (Button* button = FindButton(id)) {
		button->enabled = enabled;
		if (!enabled) {
			button->isHeld = false;
			button->isHovered = false;
			button->holdTimer = 0.0f;
			button->pressStartedInside = false;
		}
	}
}
void UIMenu::SetButtonTexture(ButtonID id, uint32_t textureHandle) {
	if (Button* button = FindButton(id)) {
		if (button->sprite) {
			button->sprite->SetTextureHandle(textureHandle);
		}
	}
}
void UIMenu::SetButtonOnClick(ButtonID id, std::function<void()> onClick) {
	if (Button* button = FindButton(id)) {
		button->onClick = std::move(onClick);
	}
}

bool UIMenu::IsButtonClicked(ButtonID id) const {
	const Button* button = FindButton(id);
	return button && button->wasClickedThisFrame;
}
bool UIMenu::IsButtonHeld(ButtonID id) const {
	const Button* button = FindButton(id);
	return button && button->isHeld;
}
float UIMenu::GetButtonHoldTime(ButtonID id) const {
	const Button* button = FindButton(id);
	return button ? button->holdTimer : 0.0f;
}
bool UIMenu::IsButtonHovered(ButtonID id) const {
	const Button* button = FindButton(id);
	return button && button->isHovered;
}

void UIMenu::UpdateButtons(float mouseX, float mouseY, bool mouseDown, bool mouseTriggeredDown) {
	// ---- 1) ホバーの素の判定。判定矩形が縦に重なるので、一番手前(並び順が最後)の1つだけをホバー扱いにする ----
	// 押し込み中のボタンがあれば、そのボタンが優先(他のボタンに奪われない)
	std::vector<char> rawHover(buttons_.size(), 0);
	int owner = -1;
	for (size_t i = 0; i < buttons_.size(); ++i) {
		const Button& button = buttons_[i];
		if (!button.visible || !button.enabled) {
			continue;
		}
		rawHover[i] = IsInsideAABB(ComputeHitRect(button), mouseX, mouseY) ? 1 : 0;
		if (button.isHeld) {
			owner = static_cast<int>(i);
		}
	}
	if (owner < 0) {
		for (size_t i = 0; i < buttons_.size(); ++i) {
			if (rawHover[i]) {
				owner = static_cast<int>(i);
			}
		}
	}

	// ---- 2) 状態の更新 ----
	for (size_t i = 0; i < buttons_.size(); ++i) {
		Button& button = buttons_[i];
		button.wasClickedThisFrame = false;

		if (!button.visible || !button.enabled) {
			button.isHovered = false;
			button.isHeld = false;
			button.holdTimer = 0.0f;
			button.pressStartedInside = false;
			button.scale = 1.0f;
			button.hoverExtend = 0.0f;
			ApplyButtonVisual(button);
			continue;
		}

		// 判定は現在のスライド位置にあるバー全体(画面外にいる間はマウスが届かないので自然に無効)
		button.isHovered = (static_cast<int>(i) == owner) && rawHover[i];

		if (button.isHeld) {
			if (mouseDown) {
				button.holdTimer += 1.0f / 60.0f;
			} else {
				if (button.pressStartedInside && button.isHovered) {
					button.wasClickedThisFrame = true;

					if (clickSoundDataHandle_ != 0) {
						AudioSettings::PlaySfx(clickSoundDataHandle_, false, clickSoundVolume_);
					}
					if (button.onClick) {
						button.onClick();
					}
				}
				button.isHeld = false;
				button.holdTimer = 0.0f;
				button.pressStartedInside = false;
			}
		} else if (button.isHovered && mouseTriggeredDown) {
			button.isHeld = true;
			button.pressStartedInside = true;
			button.holdTimer = 0.0f;
		}
		float targetScale = button.isHeld ? kPressScale : (button.isHovered ? kHoverScale : 1.0f);
		button.scale += (targetScale - button.scale) * kScaleLerp;

		// ホバー中はバーが伸び、外れると縮む(並び順が後ろほど伸びが小さい)
		const float targetExtend = button.isHovered ? ComputeMaxHoverExtend(button) : 0.0f;
		button.hoverExtend += (targetExtend - button.hoverExtend) * kHoverExtendLerp;

		ApplyButtonVisual(button);
	}
}

// バー(画面端から伸びる帯)とボタン本体をスプライトへ反映する。
// scaleはボタン本体だけに、中心を保って掛ける。判定矩形(ComputeHitRect)はscaleの影響を受けない
void UIMenu::ApplyButtonVisual(Button& button) const {
	if (button.barSprite) {
		const ButtonAABB bar = ComputeBarRect(button);
		button.barSprite->SetSize({bar.right - bar.left, bar.bottom - bar.top});
		button.barSprite->SetPosition({bar.left, bar.top});
		button.barSprite->SetColor(button.isHovered ? kBarHoverColor : kBarColor);
	}
	if (button.sprite) {
		const ButtonAABB face = ComputeFaceRect(button);
		const float w = face.right - face.left;
		const float h = face.bottom - face.top;
		const float sw = w * button.scale;
		const float sh = h * button.scale;
		button.sprite->SetSize({sw, sh});
		button.sprite->SetPosition({face.left - (sw - w) * 0.5f, face.top - (sh - h) * 0.5f});
	}
}
#pragma endregion