#define NOMINMAX
#include "Player.h"
#include "AnimationManager.h"
#include "InputManager.h"
#include "MapChipField.h"
#include "PlayerBehavior.h"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

namespace {
constexpr float kDt = 1.0f / 60.0f;

Matrix4x4 EulerMatrix(const Vector3& r) { return Multiply(MakeRotateXMatrix(r.x), Multiply(MakeRotateYMatrix(r.y), MakeRotateZMatrix(r.z))); }

// S * R * T
Matrix4x4 BuildWorld(const Vector3& s, const Matrix4x4& rot, const Vector3& t) { return Multiply(MakeScaleMatrix(s), Multiply(rot, MakeTranslateMatrix(t))); }

void Commit(WorldTransform& wt, const Vector3& s, const Matrix4x4& rot, const Vector3& t) {
	wt.scale_ = s;
	wt.translation_ = t;
	wt.matWorld_ = BuildWorld(s, rot, t);
	wt.TransferMatrix();
}
} // namespace

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, Camera* camera, const Vector3& position) {
#ifdef _DEBUG
	assert(camera);
#endif
	for (size_t i = 0; i < parts_.size(); ++i) {
		parts_[i].model = partResources[i].model;
		parts_[i].textureHandle = partResources[i].textureHandle;
		parts_[i].transform.Initialize();
	}
	camera_ = camera;
	velocity_ = {};
	moveMode_ = MoveMode::kNormal;
	modeTimer_ = 0.0f;
	currentPose_ = {};

	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_.y = camera_->rotation_.y; // back to the camera
	worldTransform_.translation_ = position;

	behavior_ = std::make_unique<PlayerBehavior>(this);
	behavior_->Enter();

	UpdateBodyParts();
}

void Player::Update() {
	if (behavior_) {
		behavior_->Update();
	}
	// The player never turns toward its move direction: its back always faces the camera.
	if (camera_) {
		worldTransform_.rotation_.y = camera_->rotation_.y;
	}
	Move();
	MapChipCollisionUpdate();
	UpdateBodyParts();
}

void Player::EnterDash(const Vector3& dir) {
	moveMode_ = MoveMode::kDash;
	modeTimer_ = 0.0f;

	// ダッシュ開始時の方向を固定する
	dashDir_ = dir;
}

void Player::EnterRoll(const Vector3& dir, float sign) {
	moveMode_ = MoveMode::kRoll;
	modeTimer_ = 0.0f;
	rollDir_ = dir;
	rollSign_ = sign;
}

// カメラの向きを基準にX-Z平面上を移動する。Shift: ダッシュ / ダッシュ中にもう一度: バレルロール

void Player::Move() {
	InputManager* input = InputManager::GetInstance();

	const float cameraYaw = camera_ ? camera_->rotation_.y : 0.0f;

	const float sinY = std::sin(cameraYaw);
	const float cosY = std::cos(cameraYaw);

	// カメラ基準の前方・右方向を取得
	const Vector3 forward = {sinY, 0.0f, cosY};
	const Vector3 right = {cosY, 0.0f, -sinY};

	const Vector2 moveAxis = input->GetMoveAxis();

	// 入力方向をカメラローカル空間で取得
	Vector3 localDir = {moveAxis.x, 0.0f, moveAxis.y};

	const float len = std::sqrt(localDir.x * localDir.x + localDir.z * localDir.z);
	const bool hasInput = len > 0.0001f;

	if (hasInput) {
		localDir.x /= len;
		localDir.z /= len;
	}

	// 入力がない場合はカメラ基準の後方をデフォルトにする
	const Vector3 actionDirLocal = hasInput ? localDir : Vector3{0.0f, 0.0f, -1.0f};

	// カメラローカル方向を現在のYawでワールド方向に変換
	auto ToWorldDirection = [&](const Vector3& local) -> Vector3 { return {right.x * local.x + forward.x * local.z, 0.0f, right.z * local.x + forward.z * local.z}; };

	const Vector3 dir = ToWorldDirection(localDir);

	// ---- state transitions ----
	if (input->IsDashTriggered()) {
		if (moveMode_ == MoveMode::kNormal) {
			// ダッシュ方向を開始時のワールド方向で固定
			EnterDash(ToWorldDirection(actionDirLocal));

			// ロールは従来どおりカメラ基準の方向を保持
			rollDir_ = actionDirLocal;
		} else if (moveMode_ == MoveMode::kDash) {
			// ダッシュ方向をそのままロールに引き継ぐ
			const float side = rollDir_.x;
			EnterRoll(rollDir_, side < -0.1f ? 1.0f : -1.0f);
		}
	}

	switch (moveMode_) {
	case MoveMode::kDash: {
		modeTimer_ += kDt;

		// 開始時に固定したワールド方向を維持。カメラ回転の影響を受けない
		velocity_.x += (dashDir_.x * kDashSpeed - velocity_.x) * kDashAccel;
		velocity_.z += (dashDir_.z * kDashSpeed - velocity_.z) * kDashAccel;

		if (modeTimer_ >= kDashDuration) {
			moveMode_ = MoveMode::kNormal;
			modeTimer_ = 0.0f;
		}
		return;
	}

	case MoveMode::kRoll: {
		modeTimer_ += kDt;

		// 固定したローカル方向にカメラの回転を反映
		const Vector3 rollWorldDir = ToWorldDirection(rollDir_);

		velocity_ = {rollWorldDir.x * kRollSpeed, 0.0f, rollWorldDir.z * kRollSpeed};

		if (modeTimer_ >= kRollDuration) {
			moveMode_ = MoveMode::kNormal;
			modeTimer_ = 0.0f;
			velocity_ = {}; // stop dead at the end of the roll, no momentum leaks into walking
		}
		return;
	}

	case MoveMode::kNormal:
		break;
	}

	// ---- normal walking ----
	if (hasInput) {
		velocity_.x += dir.x * kAcceleration;
		velocity_.z += dir.z * kAcceleration;

		const float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);

		if (speed > kLimitRunSpeed) {
			velocity_.x *= kLimitRunSpeed / speed;
			velocity_.z *= kLimitRunSpeed / speed;
		}
	} else {
		// 入力がない場合は慣性を減衰させる
		velocity_.x *= (1.0f - kAttenuation);
		velocity_.z *= (1.0f - kAttenuation);

		if (std::abs(velocity_.x) < 0.0005f) {
			velocity_.x = 0.0f;
		}
		if (std::abs(velocity_.z) < 0.0005f) {
			velocity_.z = 0.0f;
		}
	}
}


void Player::MapChipCollisionUpdate() {
	if (!mapChipField_) {
		worldTransform_.translation_ += Vector3(velocity_.x, 0.0f, velocity_.z);
		return;
	}
	blockDetector_.SetMapChipField(mapChipField_);
	blockDetector_.SetSize(kWidth, kDepth);
	blockDetector_.SetWorldTransform(worldTransform_);

	CollisionInfo info{};
	info.move = {velocity_.x, 0.0f, velocity_.z};
	blockDetector_.MapChipCollision(info);

	worldTransform_.translation_ += info.move;

	// the world edge is not a block: keep the player inside it even where the CSV has no border
	worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, mapChipField_->GetWorldMinX() + kWidth * 0.5f, mapChipField_->GetWorldMaxX() - kWidth * 0.5f);
	worldTransform_.translation_.z = std::clamp(worldTransform_.translation_.z, mapChipField_->GetWorldMinZ() + kDepth * 0.5f, mapChipField_->GetWorldMaxZ() - kDepth * 0.5f);

	// a wall stops the velocity on that axis only, so the player slides along it
	if (std::abs(info.move.x) < std::abs(velocity_.x))
		velocity_.x = 0.0f;
	if (std::abs(info.move.z) < std::abs(velocity_.z))
		velocity_.z = 0.0f;
}

void Player::UpdateBodyParts() {
	WorldTransformUpdate(worldTransform_);

	// ---- animation: smoothed pose (AnimationManager::SmoothSRT) + barrel roll spin ----
	SRTOffset target{};
	float rollAngle = 0.0f;
	if (moveMode_ == MoveMode::kDash) {
		target = kDashPose;
	} else if (moveMode_ == MoveMode::kRoll) {
		target = kRollPose;
		const float t = std::clamp(modeTimer_ / std::max(kRollDuration, 0.0001f), 0.0f, 1.0f);
		rollAngle = rollSign_ * 2.0f * std::numbers::pi_v<float> * EaseInOut(0.0f, 1.0f, t);
	}
	currentPose_ = AnimationManager::SmoothSRT(currentPose_, target);

	// Order: pitch (lean) -> roll around the forward axis -> yaw (camera). A plain Euler X-Y-Z would roll around
	// the WORLD Z axis, which is not "forward" once the player is turned, so the matrix is built by hand.
	const Matrix4x4 bodyRot = Multiply(MakeRotateXMatrix(currentPose_.rotation.x), Multiply(MakeRotateZMatrix(currentPose_.rotation.z + rollAngle), MakeRotateYMatrix(worldTransform_.rotation_.y)));

	const Vector3 bodyScale = {worldTransform_.scale_.x * currentPose_.scale.x, worldTransform_.scale_.y * currentPose_.scale.y, worldTransform_.scale_.z * currentPose_.scale.z};
	const Vector3 bodyPos = worldTransform_.translation_ + currentPose_.translation;
	Commit(parts_[body].transform, bodyScale, bodyRot, bodyPos);

	const Matrix4x4 headRot = Multiply(EulerMatrix(headOffset_.rotation), bodyRot);
	const Vector3 headScale = {bodyScale.x * headOffset_.scale.x, bodyScale.y * headOffset_.scale.y, bodyScale.z * headOffset_.scale.z};
	const Vector3 headPos = bodyPos + TransformNormal(headOffset_.translation, bodyRot);
	Commit(parts_[head].transform, headScale, headRot, headPos);

	const Matrix4x4 hatRot = Multiply(EulerMatrix(hatOffset_.rotation), headRot);
	const Vector3 hatScale = {headScale.x * hatOffset_.scale.x, headScale.y * hatOffset_.scale.y, headScale.z * hatOffset_.scale.z};
	const Vector3 hatPos = headPos + TransformNormal(hatOffset_.translation, headRot);
	Commit(parts_[hat].transform, hatScale, hatRot, hatPos);
}

void Player::Draw() {
	Model::PreDraw();
	for (auto& part : parts_) {
		if (part.model) {
			part.model->Draw(part.transform, *camera_, part.textureHandle);
		}
	}
	Model::PostDraw();
}

AABB Player::GetAABB() const {
	const Vector3 p = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {p.x - kWidth / 2.0f, p.y - kHeight / 2.0f, p.z - kDepth / 2.0f};
	aabb.max = {p.x + kWidth / 2.0f, p.y + kHeight / 2.0f, p.z + kDepth / 2.0f};
	return aabb;
}