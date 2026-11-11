#define NOMINMAX
#include "AimGuide.h"
#include "GlobalVariables.h"

#include <algorithm>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "AimGuide";
constexpr float kDt = 1.0f / 60.0f;
} // namespace

void AimGuide::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "MaxLength", kMaxLength);
	gv->AddItem(kGroupName, "StartOffset", kStartOffset);
	gv->AddItem(kGroupName, "DotSpacing", kDotSpacing);
	gv->AddItem(kGroupName, "DotSize", kDotSize);
	gv->AddItem(kGroupName, "EndMarkerSize", kEndMarkerSize);
	gv->AddItem(kGroupName, "ScrollSpeed", kScrollSpeed);
	gv->AddItem(kGroupName, "Alpha", kAlpha);
	gv->AddItem(kGroupName, "NotReadyAlphaScale", kNotReadyAlphaScale);
	gv->AddItem(kGroupName, "DepthOffset", kDepthOffset);
	gv->AddItem(kGroupName, "Color", kColor);
	gv->AddItem(kGroupName, "BlockedColor", kBlockedColor);
}

void AimGuide::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kMaxLength = gv->GetFloatValue(kGroupName, "MaxLength");
	kStartOffset = gv->GetFloatValue(kGroupName, "StartOffset");
	kDotSpacing = std::max(gv->GetFloatValue(kGroupName, "DotSpacing"), 0.1f);
	kDotSize = gv->GetFloatValue(kGroupName, "DotSize");
	kEndMarkerSize = gv->GetFloatValue(kGroupName, "EndMarkerSize");
	kScrollSpeed = gv->GetFloatValue(kGroupName, "ScrollSpeed");
	kAlpha = gv->GetFloatValue(kGroupName, "Alpha");
	kNotReadyAlphaScale = gv->GetFloatValue(kGroupName, "NotReadyAlphaScale");
	kDepthOffset = gv->GetFloatValue(kGroupName, "DepthOffset");
	kColor = gv->GetVector3Value(kGroupName, "Color");
	kBlockedColor = gv->GetVector3Value(kGroupName, "BlockedColor");
}

void AimGuide::Initialize(Model* model, uint32_t textureHandle, Camera* camera) {
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;
	for (Dot& dot : dots_) {
		dot.worldTransform.Initialize();
		dot.objectColor.Initialize();
	}
	endMarker_.worldTransform.Initialize();
	endMarker_.objectColor.Initialize();
	dotCount_ = 0;
	visible_ = false;
}

void AimGuide::Update(bool visible, const Vector3& origin, const Vector2& dir, float length, bool blocked, bool ready) {
	dotCount_ = 0;
	visible_ = visible && length > kStartOffset;
	if (!visible_) {
		return;
	}

	time_ += kDt;
	const float scroll = std::fmod(time_ * kScrollSpeed, kDotSpacing); // 点が目標へ流れる
	const float alphaScale = ready ? 1.0f : kNotReadyAlphaScale;
	const Vector3& color = blocked ? kBlockedColor : kColor;
	const float z = origin.z + kDepthOffset;

	for (float d = kStartOffset + scroll; d < length && dotCount_ < kMaxDots; d += kDotSpacing) {
		const float t = d / length; // 0=手元 / 1=終点
		Dot& dot = dots_[dotCount_++];
		const float size = kDotSize * (1.0f - 0.4f * t);
		dot.worldTransform.translation_ = {origin.x + dir.x * d, origin.y + dir.y * d, z};
		dot.worldTransform.scale_ = {size, size, size};
		dot.objectColor.SetColor({color.x, color.y, color.z, kAlpha * alphaScale * (1.0f - 0.5f * t)});
		WorldTransformUpdate(dot.worldTransform);
	}

	// 終端マーカー(照準が実際に届く場所)。ゆっくり脈打たせる
	const float pulse = 1.0f + 0.15f * std::sin(time_ * 8.0f);
	const float markerSize = kEndMarkerSize * pulse;
	endMarker_.worldTransform.translation_ = {origin.x + dir.x * length, origin.y + dir.y * length, z};
	endMarker_.worldTransform.scale_ = {markerSize, markerSize, markerSize};
	endMarker_.objectColor.SetColor({color.x, color.y, color.z, std::min(kAlpha + 0.2f, 1.0f) * alphaScale});
	WorldTransformUpdate(endMarker_.worldTransform);
}

void AimGuide::Draw() {
	if (!visible_ || !model_ || !camera_) {
		return;
	}
	// 加算合成・深度なし: 壁や敵に隠れず、常に読み取れる
	Model::PreDraw(Model::CullingMode::kNone, Model::BlendMode::kAdd, Model::DepthTestMode::kOff);
	for (size_t i = 0; i < dotCount_; ++i) {
		model_->Draw(dots_[i].worldTransform, *camera_, textureHandle_, &dots_[i].objectColor);
	}
	model_->Draw(endMarker_.worldTransform, *camera_, textureHandle_, &endMarker_.objectColor);
	Model::PostDraw();
}