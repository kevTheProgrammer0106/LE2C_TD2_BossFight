
#define NOMINMAX
#include "CameraController.h"
#include "GlobalVariables.h"
#include "Player.h"
#include "InputManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "CameraController";

// 角度差を最短経路で補間する
float LerpAngle(float current, float target, float t) {
	const float twoPi = 2.0f * std::numbers::pi_v<float>;
	const float delta = std::remainder(target - current, twoPi);
	return current + delta * t;
}
} // namespace

void CameraController::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);

	// カメラの初期設定をGlobalVariablesに登録
	gv->AddItem(kGroupName, "TargetOffset", targetOffset_);
	gv->AddItem(kGroupName, "RotationOffset", rotationOffset_);
	gv->AddItem(kGroupName, "InterpolationRate", kInterpolationRate);
	gv->AddItem(kGroupName, "MouseSensitivity", kMouseSensitivity_);
}

void CameraController::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();

	// GlobalVariablesの値をカメラ設定に反映
	targetOffset_ = gv->GetVector3Value(kGroupName, "TargetOffset");
	rotationOffset_ = gv->GetVector3Value(kGroupName, "RotationOffset");
	kInterpolationRate = gv->GetFloatValue(kGroupName, "InterpolationRate");
	kMouseSensitivity_ = gv->GetFloatValue(kGroupName, "MouseSensitivity");
}

void CameraController::Initialize(Camera* camera) {
	// カメラ受け取り
	assert(camera);
	camera_ = camera;
	camera_->Initialize();

	// GlobalVariablesが未設定でもデフォルト値で動作する
	needsSnap_ = true;
}
void CameraController::InitializeOrbit() {
	if (!target_ || orbitInitialized_) {
		return;
	}

	// 初期カメラ方向をプレイヤーの向きから決定
	orbitYaw_ = target_->GetWorldTransform().rotation_.y + rotationOffset_.y;

	appliedRotationOffsetY_ = rotationOffset_.y;
	lookPitchOffset_ = 0.0f;

	orbitInitialized_ = true;
}
void CameraController::UpdateLook() {
	if (!camera_ || !target_ || !thirdPersonEnabled_) {
		return;
	}

	InitializeOrbit();

	// GlobalVariablesで変更されたYawオフセットを反映
	orbitYaw_ += rotationOffset_.y - appliedRotationOffsetY_;
	appliedRotationOffsetY_ = rotationOffset_.y;

	InputManager* input = InputManager::GetInstance();

	// マウス移動量を常時カメラの回転に反映
	// RMB中はInputManagerがmouseDeltaを0にする
	const Vector2 mouseDelta = input->GetMouseDelta();

	orbitYaw_ += mouseDelta.x * kMouseSensitivity_;
	lookPitchOffset_ += mouseDelta.y * kMouseSensitivity_;

	// カメラの上下回転を制限
	const float pitch = std::clamp(rotationOffset_.x + lookPitchOffset_, -1.2f, 1.2f);

	lookPitchOffset_ = pitch - rotationOffset_.x;

	// プレイヤー移動が最新のカメラYawを使用できるようにする
	camera_->rotation_ = {pitch, orbitYaw_, rotationOffset_.z};
}
Vector3 CameraController::ComputeTargetPosition() const {
	const WorldTransform& targetWT = target_->GetWorldTransform();

	// カメラのPitchに合わせてオフセットを上下に回転
	const float sinPitch = std::sin(lookPitchOffset_);
	const float cosPitch = std::cos(lookPitchOffset_);

	const Vector3 pitchedOffset = {targetOffset_.x, targetOffset_.y * cosPitch - targetOffset_.z * sinPitch, targetOffset_.y * sinPitch + targetOffset_.z * cosPitch};

	// カメラのYawを基準にオフセットをワールド座標へ変換
	const float sinYaw = std::sin(orbitYaw_);
	const float cosYaw = std::cos(orbitYaw_);

	const Vector3 offset = {pitchedOffset.x * cosYaw + pitchedOffset.z * sinYaw, pitchedOffset.y, -pitchedOffset.x * sinYaw + pitchedOffset.z * cosYaw};

	return targetWT.translation_ + offset;
}

void CameraController::ApplyThirdPersonView() {
	// 設定された回転をカメラに反映
	camera_->rotation_ = {std::clamp(rotationOffset_.x + lookPitchOffset_, -1.2f, 1.2f), orbitYaw_, rotationOffset_.z};

	camera_->UpdateMatrix();
}

void CameraController::Update() {
	if (!camera_) {
		return;
	}

	if (!thirdPersonEnabled_ || !target_) {
		camera_->UpdateMatrix();
		return;
	}

	InitializeOrbit();

	const Vector3 desiredPos = ComputeTargetPosition();

	// 初回は即座に配置し、その後は滑らかに追従
	if (needsSnap_) {
		camera_->translation_ = desiredPos;
		needsSnap_ = false;
		currentInterpolation_ = std::clamp(kInterpolationRate, 0.0f, 1.0f);
	} else {
		// raise the follow rate while dashing / rolling so the camera keeps up, then ease it back
		const bool boosting = target_->IsDashing() || target_->IsRolling();
		const float wanted = std::clamp(boosting ? kBoostInterpolationRate : kInterpolationRate, 0.0f, 1.0f);
		currentInterpolation_ += (wanted - currentInterpolation_) * 0.2f;

		camera_->translation_ = Lerp(camera_->translation_, desiredPos, currentInterpolation_);
	}

	ApplyThirdPersonView();
}

void CameraController::Reset() {
	if (!camera_ || !target_ || !thirdPersonEnabled_) {
		return;
	}

	InitializeOrbit();

	camera_->translation_ = ComputeTargetPosition();

	ApplyThirdPersonView();

	needsSnap_ = false;
}
