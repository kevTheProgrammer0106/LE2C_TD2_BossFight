#pragma once

#include "GameSettings.h"
#include <KamataEngine.h>
#include <array>
#include <memory>

/// 設定パネルの中身(音量スライダー / 画面揺れ・画面フラッシュのチェックボックス / 難易度ボタン)。
/// TitleScene と GameScene(ポーズ中)で共有する。Backボタンは各シーン側で持つ。
/// 各項目にアイコンがあり、ホバーするとカーソルの下に説明画像(tip)が出る。
class SettingsPanel {
public:
	// showDifficulty: 難易度の行を出すか(タイトルのみ true)
	void Initialize(uint32_t whiteTexture, bool showDifficulty);
	// alpha: パネルのフェード値 / interactive: 操作を受け付けるか
	void Update(float alpha, bool interactive);
	void Draw();
	// 閉じる時に呼ぶ。ドラッグ中なら保存して終わる
	void CancelDrag();

private:
	struct Rect {
		float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
	};

	// 画像が無い時は white1x1 を placeholder 色で描く(レイアウト確認用)
	struct Element {
		std::unique_ptr<KamataEngine::Sprite> sprite;
		Rect rect;
		bool hasTexture = false;
		KamataEngine::Vector4 placeholder{1.0f, 1.0f, 1.0f, 1.0f};
	};

	enum Tip : int { kTipMaster, kTipMusic, kTipSfx, kTipShake, kTipFlash, kTipDifficulty, kTipCount, kTipNone = -1 };

	struct Slider {
		float (*get)() = nullptr;
		void (*set)(float) = nullptr;
		Rect bar;
		Rect hit;
		bool dragging = false;
		std::unique_ptr<KamataEngine::Sprite> trackSprite, fillSprite, knobSprite;
		Element icon;
	};
	struct Checkbox {
		bool (*get)() = nullptr;
		void (*set)(bool) = nullptr;
		Rect hit;
		int tip = kTipNone;
		bool checked = false;
		Element icon, boxOff, boxOn;
	};

	static bool Inside(const Rect& r, float x, float y) { return x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h; }
	static bool LoadIfExists(const char* fileName, uint32_t& outTex);
	void MakeElement(Element& e, const char* fileName, const Rect& rect, const KamataEngine::Vector4& placeholder);
	void LoadTip(int tip, const char* fileName);
	static void ApplyElement(Element& e, bool hover, float alpha, float brightness);

	uint32_t whiteTex_ = 0;
	bool showDifficulty_ = false;

	std::array<Slider, 3> sliders_;
	std::array<Checkbox, 2> checkboxes_;

	std::array<std::unique_ptr<KamataEngine::Sprite>, kTipCount> tipSprites_; // 画像が無い項目は nullptr
	int hoveredTip_ = kTipNone;
	bool tipVisible_ = false;

	// ---- レイアウト(1280x720想定。ここを触れば配置が変わる) ----
	static constexpr float kIconSize = 48.0f;
	static constexpr float kIconGap = 16.0f;
	static constexpr float kBoxSize = 40.0f;
	static constexpr float kSliderWidth = 400.0f;
	static constexpr float kSliderHeight = 16.0f;
	static constexpr float kKnobWidth = 20.0f;
	static constexpr float kKnobHeight = 36.0f;
	static constexpr float kSliderStartY = 140.0f;
	static constexpr float kSliderSpacing = 70.0f;
	static constexpr float kCheckboxRowY = 370.0f;
	static constexpr float kGroupGap = 120.0f;
	static constexpr float kDiffRowY = 450.0f;
	static constexpr float kDiffButtonW = 150.0f;
	static constexpr float kDiffButtonH = 80.0f;
	static constexpr float kDiffButtonGap = 20.0f;
	static constexpr float kHoverScale = 1.12f;
	static constexpr float kUnselectedBrightness = 0.45f; // 選ばれていない難易度ボタンの暗さ
	static constexpr float kTipWidth = 360.0f;            // tip画像の表示サイズ(この比率で描いてください)
	static constexpr float kTipHeight = 80.0f;
	static constexpr float kTipCursorGap = 28.0f; // カーソルからtip上端までの距離
};