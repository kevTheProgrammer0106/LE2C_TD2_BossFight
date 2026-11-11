#define NOMINMAX
#include "SettingsPanel.h"

#include "AudioSettings.h"
#include "InputManager.h"

#include <algorithm>
#include <fstream>
#include <string>

using namespace KamataEngine;

namespace {
// 画像の置き場所(ここだけ変えれば全部の画像パスが変わる)
constexpr const char* kImageDir = "./Resources/UserInterface/Settings/";
} // namespace

bool SettingsPanel::LoadIfExists(const char* fileName, uint32_t& outTex) {
	const std::string path = std::string(kImageDir) + fileName;
	std::ifstream f(path);
	if (!f.good())
		return false; // 画像がまだ無い
	outTex = TextureManager::Load(path);
	return true;
}

void SettingsPanel::MakeElement(Element& e, const char* fileName, const Rect& rect, const Vector4& placeholder) {
	uint32_t tex = whiteTex_;
	e.hasTexture = LoadIfExists(fileName, tex);
	e.rect = rect;
	e.placeholder = placeholder;
	e.sprite.reset(Sprite::Create(tex, {rect.x, rect.y}));
	e.sprite->SetSize({rect.w, rect.h});
}

void SettingsPanel::LoadTip(int tip, const char* fileName) {
	uint32_t tex = 0;
	if (!LoadIfExists(fileName, tex)) {
		tipSprites_[tip].reset(); // 画像が無ければ説明は出さない
		return;
	}
	tipSprites_[tip].reset(Sprite::Create(tex, {0.0f, 0.0f}));
	tipSprites_[tip]->SetSize({kTipWidth, kTipHeight});
}

void SettingsPanel::ApplyElement(Element& e, bool hover, float alpha, float brightness) {
	if (!e.sprite)
		return;
	// ホバー中は中心を保ったまま少し拡大する
	const float scale = hover ? kHoverScale : 1.0f;
	const float w = e.rect.w * scale;
	const float h = e.rect.h * scale;
	e.sprite->SetSize({w, h});
	e.sprite->SetPosition({e.rect.x - (w - e.rect.w) * 0.5f, e.rect.y - (h - e.rect.h) * 0.5f});
	if (e.hasTexture)
		e.sprite->SetColor({brightness, brightness, brightness, alpha});
	else
		e.sprite->SetColor({e.placeholder.x * brightness, e.placeholder.y * brightness, e.placeholder.z * brightness, e.placeholder.w * alpha});
}

void SettingsPanel::Initialize(uint32_t whiteTexture, bool showDifficulty) {
	whiteTex_ = whiteTexture;
	showDifficulty_ = showDifficulty;
	hoveredTip_ = kTipNone;
	tipVisible_ = false;

	const float centerX = static_cast<float>(WinApp::kWindowWidth) * 0.5f;

	// ---- 音量スライダー(Master / Music / SFX) + アイコン ----
	struct SliderDef {
		float (*get)();
		void (*set)(float);
		const char* icon;
		const char* tip;
		Vector4 color;
	};
	const SliderDef defs[3] = {
	    {&AudioSettings::GetMasterVolume, &AudioSettings::SetMasterVolume, "iconButtonMaster.png", "tipMaster.png", {1.0f, 0.85f, 0.3f, 1.0f}},
	    {&AudioSettings::GetMusicSetting, &AudioSettings::SetMusicVolume,  "iconButtonMusic.png",  "tipMusic.png",  {0.4f, 0.8f, 1.0f, 1.0f} },
	    {&AudioSettings::GetSfxSetting,   &AudioSettings::SetSfxVolume,    "iconButtonSfx.png",    "tipSFX.png",    {1.0f, 0.5f, 0.4f, 1.0f} },
	};
	for (int i = 0; i < 3; ++i) {
		Slider& s = sliders_[i];
		s.get = defs[i].get;
		s.set = defs[i].set;
		s.dragging = false;
		s.bar = {centerX - kSliderWidth * 0.5f, kSliderStartY + static_cast<float>(i) * kSliderSpacing, kSliderWidth, kSliderHeight};
		s.hit = {s.bar.x - kKnobWidth, s.bar.y - kKnobHeight * 0.5f, s.bar.w + kKnobWidth * 2.0f, s.bar.h + kKnobHeight};

		s.trackSprite.reset(Sprite::Create(whiteTex_, {s.bar.x, s.bar.y}));
		s.trackSprite->SetSize({s.bar.w, s.bar.h});
		s.fillSprite.reset(Sprite::Create(whiteTex_, {s.bar.x, s.bar.y}));
		s.fillSprite->SetSize({s.bar.w, s.bar.h});
		s.knobSprite.reset(Sprite::Create(whiteTex_, {s.bar.x, s.bar.y}));
		s.knobSprite->SetSize({kKnobWidth, kKnobHeight});

		// アイコンはスライダーの左隣
		MakeElement(s.icon, defs[i].icon, {s.bar.x - kIconGap - kIconSize, s.bar.y + s.bar.h * 0.5f - kIconSize * 0.5f, kIconSize, kIconSize}, defs[i].color);
		LoadTip(i, defs[i].tip);
	}

	// ---- チェックボックス(画面揺れ / 画面フラッシュ)。スライダーの下の行に横並び ----
	struct CheckDef {
		bool (*get)();
		void (*set)(bool);
		const char* icon;
		int tip;
		const char* tipFile;
		Vector4 color;
	};
	const CheckDef checks[2] = {
	    {&GameSettings::IsScreenShakeEnabled, &GameSettings::SetScreenShakeEnabled, "iconButtonShake.png", kTipShake, "tipShake.png", {0.8f, 0.6f, 1.0f, 1.0f}},
	    {&GameSettings::IsScreenFlashEnabled, &GameSettings::SetScreenFlashEnabled, "iconButtonFlash.png", kTipFlash, "tipFlash.png", {1.0f, 1.0f, 0.6f, 1.0f}},
	};
	const float groupW = kIconSize + kIconGap + kBoxSize;
	const float rowStartX = centerX - (groupW * 2.0f + kGroupGap) * 0.5f;
	for (int i = 0; i < 2; ++i) {
		Checkbox& c = checkboxes_[i];
		c.get = checks[i].get;
		c.set = checks[i].set;
		c.tip = checks[i].tip;

		const float gx = rowStartX + static_cast<float>(i) * (groupW + kGroupGap);
		const float boxY = kCheckboxRowY + (kIconSize - kBoxSize) * 0.5f;
		c.hit = {gx, kCheckboxRowY, groupW, kIconSize}; // アイコンとボックスの両方がクリック/ホバー範囲

		MakeElement(c.icon, checks[i].icon, {gx, kCheckboxRowY, kIconSize, kIconSize}, checks[i].color);
		MakeElement(c.boxOff, "iconButtonCheckbox.png", {gx + kIconSize + kIconGap, boxY, kBoxSize, kBoxSize}, {0.15f, 0.15f, 0.15f, 0.9f});
		MakeElement(c.boxOn, "iconButtonCheckmark.png", {gx + kIconSize + kIconGap, boxY, kBoxSize, kBoxSize}, {0.25f, 0.55f, 0.95f, 1.0f});
		LoadTip(checks[i].tip, checks[i].tipFile);
	}
}

void SettingsPanel::CancelDrag() {
	for (Slider& s : sliders_) {
		if (s.dragging) {
			s.dragging = false;
			AudioSettings::SaveToFile();
		}
	}
}

void SettingsPanel::Update(float alpha, bool interactive) {
	InputManager* input = InputManager::GetInstance();
	const float mx = static_cast<float>(input->GetMouseX());
	const float my = static_cast<float>(input->GetMouseY());
	const bool canUse = interactive && alpha > 0.01f;
	const bool clicked = canUse && input->IsPrimaryTriggered();

	hoveredTip_ = kTipNone;

	// ---- スライダー ----
	for (int i = 0; i < 3; ++i) {
		Slider& s = sliders_[i];
		float value = s.get();

		if (interactive) {
			if (clicked && Inside(s.hit, mx, my))
				s.dragging = true;
			if (s.dragging) {
				if (input->IsPrimaryHeld()) {
					value = std::clamp((mx - s.bar.x) / s.bar.w, 0.0f, 1.0f);
					s.set(value);
				} else {
					s.dragging = false;
					AudioSettings::SaveToFile(); // 離した時に1回だけ保存
				}
			}
		}

		const bool hover = canUse && (Inside(s.hit, mx, my) || Inside(s.icon.rect, mx, my));
		if (hover || s.dragging)
			hoveredTip_ = i;

		s.trackSprite->SetColor({0.15f, 0.15f, 0.15f, 0.9f * alpha});
		s.fillSprite->SetSize({s.bar.w * value, s.bar.h});
		s.fillSprite->SetColor({0.25f, 0.55f, 0.95f, alpha});
		s.knobSprite->SetPosition({s.bar.x + s.bar.w * value - kKnobWidth * 0.5f, s.bar.y + s.bar.h * 0.5f - kKnobHeight * 0.5f});
		s.knobSprite->SetColor({1.0f, 1.0f, 1.0f, alpha});
		ApplyElement(s.icon, hover || s.dragging, alpha, 1.0f);
	}

	// ---- チェックボックス ----
	for (Checkbox& c : checkboxes_) {
		const bool hover = canUse && Inside(c.hit, mx, my);
		if (hover)
			hoveredTip_ = c.tip;
		if (clicked && hover) {
			c.set(!c.get());
			GameSettings::SaveToFile();
		}
		c.checked = c.get();
		ApplyElement(c.icon, hover, alpha, 1.0f);
		ApplyElement(c.boxOff, hover, alpha, 1.0f);
		ApplyElement(c.boxOn, hover, alpha, 1.0f);
	}

	// ---- ホバー中の説明画像: カーソルの真下(画面下に収まらなければ上) ----
	tipVisible_ = (hoveredTip_ != kTipNone && tipSprites_[hoveredTip_] != nullptr);
	if (tipVisible_) {
		const float screenW = static_cast<float>(WinApp::kWindowWidth);
		const float screenH = static_cast<float>(WinApp::kWindowHeight);
		const float x = std::clamp(mx - kTipWidth * 0.5f, 4.0f, std::max(4.0f, screenW - kTipWidth - 4.0f));
		float y = my + kTipCursorGap;
		if (y + kTipHeight > screenH - 4.0f)
			y = my - kTipCursorGap - kTipHeight;
		tipSprites_[hoveredTip_]->SetPosition({x, y});
		tipSprites_[hoveredTip_]->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
}

void SettingsPanel::Draw() {
	Sprite::PreDraw();
	for (Slider& s : sliders_) {
		s.trackSprite->Draw();
		s.fillSprite->Draw();
		s.knobSprite->Draw();
		s.icon.sprite->Draw();
	}
	for (Checkbox& c : checkboxes_) {
		c.icon.sprite->Draw();
		c.boxOff.sprite->Draw();
		if (c.checked)
			c.boxOn.sprite->Draw();
	}
	if (tipVisible_)
		tipSprites_[hoveredTip_]->Draw(); // 最後に描いて一番手前に出す
	Sprite::PostDraw();
}