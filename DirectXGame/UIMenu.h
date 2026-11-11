#pragma once

#include "Global.h"
#include "InputManager.h"
#include <functional>
#include <string>
#include <vector>

/// <summary>
/// UIボタン群とマウスカーソルの表示・当たり判定・スライド演出だけを扱うクラス。
/// 各ボタンは「バー(画面端から伸びる無地の帯)」と「ボタン本体(文字入りテクスチャ、元のサイズのまま)」を別スプライトで持つ。
/// バーは画面の端(x=0)からボタンの元の位置の右端まで伸び、ボタンの並び順が後ろになるほど短くなる。
/// ホバーするとバーが伸びる(これも並び順が後ろほど小さい)。
/// 両者は一緒に画面の端からスライドして登場し(イージング)、退場時は素早く端へ戻る。
/// ゲームプレイ入力(移動/ジャンプ/攻撃等)は一切扱わない — それはInputManagerの役目。
/// マウス座標・クリック状態はInputManager経由で取得する(KamataEngine::Inputへは直接触らない)。
/// </summary>
class UIMenu {
public:
	// 既存の単一テクスチャ用。今までのUIはこのまま使える。
	void Initialize(uint32_t tex, KamataEngine::Camera* camera);

	// 複数テクスチャ用。ボタンごとに使用するテクスチャを選択できる。
	void Initialize(const uint32_t* textures, size_t textureCount, KamataEngine::Camera* camera);

	void Update();
	void UpdateCursor();

	void Draw();

	struct ButtonAABB {
		float left = 0.0f;
		float top = 0.0f;
		float right = 0.0f;
		float bottom = 0.0f;
	};

	using ButtonID = int32_t;
	static constexpr ButtonID kInvalidButton = -1;

	// どちらの端からスライドするか。kAutoはボタンの中心が画面の中央より左(中央ちょうど含む)なら左、右なら右
	enum class SlideSide { kAuto, kLeft, kRight };

	// 既存の単一テクスチャ用
	ButtonID AddButton(uint32_t textureHandle, float x, float y, float width, float height, const std::string& label = "");

	// 複数テクスチャ用。textureIndexで使用するテクスチャを指定する。
	ButtonID AddButton(size_t textureIndex, float x, float y, float width, float height, const std::string& label = "");

	void ClearButtons();

	void SetButtonPosition(ButtonID id, float x, float y);
	void SetButtonSize(ButtonID id, float width, float height);
	void SetButtonVisible(ButtonID id, bool visible);
	void SetButtonEnabled(ButtonID id, bool enabled);
	void SetButtonTexture(ButtonID id, uint32_t textureHandle);
	void SetButtonOnClick(ButtonID id, std::function<void()> onClick);
	void SetButtonSlideSide(ButtonID id, SlideSide side);

	bool IsButtonClicked(ButtonID id) const;
	bool IsButtonHeld(ButtonID id) const;
	float GetButtonHoldTime(ButtonID id) const;
	bool IsButtonHovered(ButtonID id) const;

	void SetClickSoundVolume(float volume) { clickSoundVolume_ = volume; }
	void SetClickSoundHandle(uint32_t soundDataHandle) { clickSoundDataHandle_ = soundDataHandle; }

	// fadeIn=true: 端からイージングで登場 / false: 素早く端へ退場。durationは下限/上限で補正される
	void StartButtonFade(bool fadeIn, float duration);
	void UpdateButtonFade();
	bool IsButtonFadeFinished() const { return buttonFadeStatus_ == ButtonFadeStatus::kNone || buttonFadeTimer_ >= buttonFadeDuration_; }
	float GetButtonFadeAlpha() const { return buttonFadeAlpha_; }
	// 0=完全に画面外 / 1=本来の位置。アルファ(暗転など他の用途)とスライドの両方に反映される
	void SetButtonGroupAlpha(float alpha) {
		buttonFadeAlpha_ = alpha;
		buttonSlide_ = alpha;
	}
	float GetButtonSlideProgress() const { return buttonSlide_; }

	void SetCursorTexture(uint32_t textureHandle) {
		if (cursorSprite_) {
			cursorSprite_->SetTextureHandle(textureHandle);
		}
	}
	// GameSceneが自前のカーソルを描く時にfalseにする
	void SetCursorVisible(bool visible) { cursorVisible_ = visible; }

private:
	struct Button {
		ButtonID id = kInvalidButton;
		size_t index = 0;                                // 追加順(バーの長さ/ホバーの伸びを決める)
		ButtonAABB aabb{};                               // ボタン本体の本来の位置(スライドが完了した時の位置)
		std::unique_ptr<KamataEngine::Sprite> sprite;    // ボタン本体(文字入りテクスチャ。サイズは元のまま)
		std::unique_ptr<KamataEngine::Sprite> barSprite; // 画面端から伸びる無地のバー
		std::string label;
		bool visible = true;
		bool enabled = true;
		SlideSide side = SlideSide::kAuto;

		bool isHovered = false;
		bool isHeld = false;
		bool wasClickedThisFrame = false;
		float holdTimer = 0.0f;
		bool pressStartedInside = false;
		float scale = 1.0f;
		float hoverExtend = 0.0f; // ホバーでバーが伸びている現在の長さ(px)

		std::function<void()> onClick;
	};

	Button* FindButton(ButtonID id);
	const Button* FindButton(ButtonID id) const;
	static bool IsInsideAABB(const ButtonAABB& aabb, float x, float y);
	void UpdateButtons(float mouseX, float mouseY, bool mouseDown, bool mouseTriggeredDown);

	// 実際にスライドする端(kAutoを解決した結果)
	SlideSide ResolveSide(const Button& button) const;
	// 現在のスライドによる横方向のずれ(0=本来の位置)
	float ComputeSlideOffset(const Button& button) const;
	// 並び順に応じたバーの基本の長さ(ホバーの伸びを含まない)の端の位置。左なら右端のx、右なら左端のx(スライド前)
	float ComputeBaseBarEdge(const Button& button) const;
	// 並び順に応じたホバー時の最大の伸び(px)
	float ComputeMaxHoverExtend(const Button& button) const;
	// 当たり判定の矩形(スライド位置込み)。ホバーの伸びは含まない
	ButtonAABB ComputeHitRect(const Button& button) const;
	// バーの表示矩形(スライド位置込み)。ホバーの伸びを含む
	ButtonAABB ComputeBarRect(const Button& button) const;
	// ボタン本体の矩形(スライド位置込み、元のサイズ)
	ButtonAABB ComputeFaceRect(const Button& button) const;

	std::vector<Button> buttons_;
	ButtonID nextButtonID_ = 0;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Camera* camera_ = nullptr;

	std::unique_ptr<KamataEngine::Sprite> cursorSprite_ = nullptr;
	bool cursorVisible_ = true;

	// 複数ボタン用のテクスチャ一覧
	std::vector<uint32_t> buttonTextures_;
	uint32_t barTexture_ = 0; // バー用の白テクスチャ(色はスプライトの色で付ける)

	enum class ButtonFadeStatus { kNone, kFadeIn, kFadeOut };
	ButtonFadeStatus buttonFadeStatus_ = ButtonFadeStatus::kNone;
	float buttonFadeDuration_ = 0.0f;
	float buttonFadeTimer_ = 0.0f;
	float buttonFadeAlpha_ = 1.0f;
	float buttonSlide_ = 1.0f; // 0=画面外 / 1=本来の位置

	// スライド演出の調整値
	static inline float kSlideInDuration = 0.5f;  // 登場にかける最短時間(秒)
	static inline float kSlideOutDuration = 0.2f; // 退場にかける最長時間(秒。素早く戻る)

	// バーの調整値
	static inline float kLengthStep = 40.0f;      // ボタンが1つ後ろになるごとにバーが短くなる量(px)
	static inline float kHoverExtend = 120.0f;    // ホバー時にバーが伸びる最大長(px。1つ目のボタン)
	static inline float kHoverExtendStep = 20.0f; // ボタンが1つ後ろになるごとにホバーの伸びが減る量(px)
	static inline float kHoverExtendLerp = 0.25f; // 毎フレームの伸び縮みの補間率
	static inline float kBarHeightRatio = 0.55f;  // バーの高さ(ボタンの高さに対する割合。縦中央)
	static inline KamataEngine::Vector4 kBarColor = {0.12f, 0.14f, 0.22f, 0.90f};
	static inline KamataEngine::Vector4 kBarHoverColor = {0.30f, 0.36f, 0.55f, 0.95f};

	// バーとボタン本体のスプライトを更新する。ホバー/押下のscaleはボタン本体の中心を保って適用
	void ApplyButtonVisual(Button& button) const;

	static inline float kLabelCursorGap = 22.0f; // space between the cursor and the bottom of the label box
	static inline float kLabelFontSize = 24.0f;
	static inline float kLabelPadX = 12.0f;
	static inline float kLabelPadY = 6.0f;

	static inline float kHoverScale = 1.12f; // enlarged while hovered
	static inline float kPressScale = 0.95f; // slightly pressed in while held
	static inline float kScaleLerp = 0.25f;  // per-frame ease toward the target

	uint32_t clickSoundDataHandle_ = 0;
	float clickSoundVolume_ = 1.0f;
};