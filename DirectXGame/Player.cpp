#define NOMINMAX
#include "Player.h"
#include "AnimationManager.h"
#include "GlobalVariables.h"
#include "InputManager.h"
#include "MapChipField.h"
#include "PlayerBehavior.h"
#include "WeaponHeavySabre.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <limits>

using namespace KamataEngine;

namespace {
constexpr float kDt = 1.0f / 60.0f;
constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
constexpr const char* kPlayerGlobalGroup = "Player";

Matrix4x4 EulerMatrix(const Vector3& r) { return Multiply(MakeRotateXMatrix(r.x), Multiply(MakeRotateYMatrix(r.y), MakeRotateZMatrix(r.z))); }

// スケール行列 * 回転行列 * 平行移動行列の順で合成する。
Matrix4x4 BuildWorld(const Vector3& s, const Matrix4x4& rot, const Vector3& t) { return Multiply(MakeScaleMatrix(s), Multiply(rot, MakeTranslateMatrix(t))); }

void Commit(WorldTransform& wt, const Vector3& s, const Matrix4x4& rot, const Vector3& t) {
	wt.scale_ = s;
	wt.translation_ = t;
	wt.matWorld_ = BuildWorld(s, rot, t);
	wt.TransferMatrix();
}

// カメラ角度が±πをまたいでも360度回転しないよう、最短角度差を使ってフレームレート非依存で補間する。
float SmoothAngle(float current, float target, float speed) {
	const float delta = std::remainder(target - current, kTwoPi);
	const float factor = 1.0f - std::exp(-std::max(speed, 0.0f) * kDt);
	return current + delta * factor;
}

// AABBの8頂点を体の回転行列で変換し、ワールド空間のAABBを再計算する。
AABB TransformAABB(const AABB& source, const Vector3& pivot, const Matrix4x4& rotation) {

	const std::array<Vector3, 8> corners = {
	    Vector3{source.min.x, source.min.y, source.min.z},
        Vector3{source.max.x, source.min.y, source.min.z},
        Vector3{source.min.x, source.max.y, source.min.z},
	    Vector3{source.max.x, source.max.y, source.min.z},
        Vector3{source.min.x, source.min.y, source.max.z},
        Vector3{source.max.x, source.min.y, source.max.z},
	    Vector3{source.min.x, source.max.y, source.max.z},
        Vector3{source.max.x, source.max.y, source.max.z}
    };

	const float maxValue = std::numeric_limits<float>::max();

	AABB result{};
	result.min = {maxValue, maxValue, maxValue};
	result.max = {-maxValue, -maxValue, -maxValue};

	for (const Vector3& corner : corners) {
		// 回転中心からの相対位置を回転させてワールド座標へ戻す。
		const Vector3 transformed = pivot + TransformNormal(corner - pivot, rotation);

		result.min.x = std::min(result.min.x, transformed.x);
		result.min.y = std::min(result.min.y, transformed.y);
		result.min.z = std::min(result.min.z, transformed.z);

		result.max.x = std::max(result.max.x, transformed.x);
		result.max.y = std::max(result.max.y, transformed.y);
		result.max.z = std::max(result.max.z, transformed.z);
	}

	return result;
}
} // namespace

void Player::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kPlayerGlobalGroup);

	// ダッシュの移動パラメータと姿勢を登録する。
	gv->AddItem(kPlayerGlobalGroup, "DashSpeed", kDashSpeed);
	gv->AddItem(kPlayerGlobalGroup, "DashDuration", kDashDuration);
	gv->AddItem(kPlayerGlobalGroup, "DashAcceleration", kDashAccel);
	gv->AddItem(kPlayerGlobalGroup, "DashPoseScale", kDashPose.scale);
	gv->AddItem(kPlayerGlobalGroup, "DashPoseRotation", kDashPose.rotation);
	gv->AddItem(kPlayerGlobalGroup, "DashPoseTranslation", kDashPose.translation);

	// A/Dによる左右ロールの姿勢を登録する。
	gv->AddItem(kPlayerGlobalGroup, "HorizontalRollPoseScale", kHorizontalRollPose.scale);
	gv->AddItem(kPlayerGlobalGroup, "HorizontalRollPoseRotation", kHorizontalRollPose.rotation);
	gv->AddItem(kPlayerGlobalGroup, "HorizontalRollPoseTranslation", kHorizontalRollPose.translation);

	// W/Sによる前後ロールの姿勢を登録する。
	gv->AddItem(kPlayerGlobalGroup, "VerticalRollPoseScale", kVerticalRollPose.scale);
	gv->AddItem(kPlayerGlobalGroup, "VerticalRollPoseRotation", kVerticalRollPose.rotation);
	gv->AddItem(kPlayerGlobalGroup, "VerticalRollPoseTranslation", kVerticalRollPose.translation);

	gv->AddItem(kPlayerGlobalGroup, "RollSpeed", kRollSpeed);
	gv->AddItem(kPlayerGlobalGroup, "RollDuration", kRollDuration);
	gv->AddItem(kPlayerGlobalGroup, "RollRotationAngle", kRollRotationAngle);
}

void Player::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();

	// ダッシュの移動パラメータと姿勢を反映する。
	kDashSpeed = gv->GetFloatValue(kPlayerGlobalGroup, "DashSpeed");
	kDashDuration = gv->GetFloatValue(kPlayerGlobalGroup, "DashDuration");
	kDashAccel = gv->GetFloatValue(kPlayerGlobalGroup, "DashAcceleration");
	kDashPose.scale = gv->GetVector3Value(kPlayerGlobalGroup, "DashPoseScale");
	kDashPose.rotation = gv->GetVector3Value(kPlayerGlobalGroup, "DashPoseRotation");
	kDashPose.translation = gv->GetVector3Value(kPlayerGlobalGroup, "DashPoseTranslation");

	// A/Dによる左右ロールの姿勢を反映する。
	kHorizontalRollPose.scale = gv->GetVector3Value(kPlayerGlobalGroup, "HorizontalRollPoseScale");
	kHorizontalRollPose.rotation = gv->GetVector3Value(kPlayerGlobalGroup, "HorizontalRollPoseRotation");
	kHorizontalRollPose.translation = gv->GetVector3Value(kPlayerGlobalGroup, "HorizontalRollPoseTranslation");

	// W/Sによる前後ロールの姿勢を反映する。
	kVerticalRollPose.scale = gv->GetVector3Value(kPlayerGlobalGroup, "VerticalRollPoseScale");
	kVerticalRollPose.rotation = gv->GetVector3Value(kPlayerGlobalGroup, "VerticalRollPoseRotation");
	kVerticalRollPose.translation = gv->GetVector3Value(kPlayerGlobalGroup, "VerticalRollPoseTranslation");

	kRollSpeed = gv->GetFloatValue(kPlayerGlobalGroup, "RollSpeed");
	kRollDuration = gv->GetFloatValue(kPlayerGlobalGroup, "RollDuration");
	kRollRotationAngle = gv->GetFloatValue(kPlayerGlobalGroup, "RollRotationAngle");
}

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize(const std::array<PartResources, static_cast<size_t>(PartSlot::kCount)>& partResources, Camera* camera, const Vector3& position) {
#ifdef _DEBUG
	assert(camera);
#endif
	RegisterGlobalVariables();
	ApplyGlobalVariables();

	for (size_t i = 0; i < parts_.size(); ++i) {
		parts_[i].model = partResources[i].model;
		parts_[i].textureHandle = partResources[i].textureHandle;
		parts_[i].transform.Initialize();
	}
	camera_ = camera;
	velocity_ = {};
	moveMode_ = MoveMode::kNormal;
	modeTimer_ = 0.0f;
	rollDirectionIsWorld_ = false;
	rollIsHorizontal_ = false;
	currentPose_ = {};
	currentActionPose_ = {};
	lungeCooldown_ = 0.0f;
	lungeHit_ = false;
	controlLocked_ = false;

	facingYaw_ = camera_->rotation_.y; // プレイヤーの背面をカメラ側へ向ける。
	lockedYaw_ = facingYaw_;
	yawLocked_ = false;
	yawReleasing_ = false;
	yawDetached_ = false;

	hp_ = kMaxHP;
	stamina_ = kMaxStamina;
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.rotation_.y = facingYaw_;
	worldTransform_.translation_ = position;

	behavior_ = std::make_unique<PlayerBehavior>(*this);
	behavior_->Start(PlayerBehavior::kRoot);

	UpdateBodyParts();
}

#pragma region Weapon
void Player::EquipHeavySabre(Model* model, uint32_t textureHandle) {
	heavySabre_ = std::make_unique<WeaponHeavySabre>();
	heavySabre_->Initialize(model, textureHandle, camera_, worldTransform_.translation_);
	heavySabre_->SnapWeaponState(WeaponState::kEquipped);
	UpdateWeapon();
}

BaseMelee* Player::GetMelee() const { return heavySabre_.get(); }

bool Player::IsWeaponEquipped() const { return heavySabre_ && heavySabre_->GetWeaponState() == WeaponState::kEquipped; }

void Player::SetWeaponEquipped(bool equipped) {
	if (heavySabre_) {
		heavySabre_->SetWeaponState(equipped ? WeaponState::kEquipped : WeaponState::kHolstered);
	}
}

LungeSpec Player::GetLungeSpec() const { return heavySabre_ ? heavySabre_->GetPlayerLungeSpec() : LungeSpec{}; }

void Player::BeginLungeCooldown() { lungeCooldown_ = GetLungeSpec().cooldown; }

bool Player::GetAttackHitbox(AABB& outBox) const {
	if (!heavySabre_ || !behavior_ || !behavior_->IsHitboxActive()) {
		return false;
	}

	const float halfWidth = kWidth * 0.5f;
	const Vector3 bodyPos = parts_[body].transform.translation_;

	if (behavior_->IsLunge()) {
		outBox = heavySabre_->GetPlayerLungeAABB(worldTransform_.translation_, GetYaw(), halfWidth);
	} else {
		// Yaw = 0で体のローカル空間にあるスイング判定を取得する。
		const AABB localSwingAABB = heavySabre_->GetSwingAABB(bodyPos, 0.0f, halfWidth, behavior_->GetComboIndex());

		// 保存した体の回転を適用し、ワールド空間のAABBに変換する。
		outBox = TransformAABB(localSwingAABB, bodyPos, bodyRotation_);
	}

	return true;
}

// Q：納刀／抜刀。左クリック：コンボ攻撃。右クリック：突進攻撃。
void Player::UpdateAttackInput() {
	if (!heavySabre_ || controlLocked_ || !behavior_) {
		return;
	}
	InputManager* input = InputManager::GetInstance();

	if (input->IsWeaponSwitchTriggered() && !behavior_->IsAttacking()) {
		SetWeaponEquipped(!IsWeaponEquipped());
	}
	if (!IsWeaponEquipped() || moveMode_ == MoveMode::kRoll) {
		return;
	}

	if (input->IsPrimaryTriggered()) {
		if (moveMode_ == MoveMode::kDash && behavior_->IsIn(PlayerBehavior::kRoot)) {
			ResetMoveMode(); // 攻撃を開始した場合はダッシュを中断する。
		}
		behavior_->RequestAttack();
	} else if (input->IsSecondaryTriggered() && CanLunge()) {
		if (moveMode_ == MoveMode::kDash && behavior_->IsIn(PlayerBehavior::kRoot)) {
			ResetMoveMode();
		}
		behavior_->RequestLunge();
	}
}

void Player::UpdateWeapon() {
	if (!heavySabre_ || !behavior_) {
		return;
	}

	ReactionPose reaction = ReactionPose::kNone;

	if (behavior_->IsKnockback()) {
		reaction = ReactionPose::kKnockback;
	} else if (behavior_->IsStunned()) {
		reaction = ReactionPose::kStun;
	}

	heavySabre_->SetReactionPose(reaction);

	// 武器の位置と回転を体と共有する。
	const Vector3 bodyPos = parts_[body].transform.translation_;

	if (behavior_->IsLunge()) {
		const PlayerBehavior::AttackAnimState anim = behavior_->GetAttackAnimState();

		heavySabre_->SetPlayerLungePosition(bodyPos, GetYaw(), anim.phase, anim.t);
	} else if (behavior_->IsSwingActive()) {
		const PlayerBehavior::AttackAnimState anim = behavior_->GetAttackAnimState();

		heavySabre_->SetMeleePosition(bodyPos, bodyRotation_, &anim);
	} else {
		heavySabre_->SetMeleePosition(bodyPos, bodyRotation_, nullptr);
	}

	heavySabre_->Update();
}
#pragma endregion

#pragma region Debuffs
void Player::SetStunned(float duration) {
	if (behavior_) {
		behavior_->RequestStun(duration);
	}
}
void Player::SetKnockback(const Vector3& direction, float power) {
	if (behavior_) {
		behavior_->RequestHitKnockback(direction, power);
	}
}
void Player::ClearStun() {
	if (behavior_) {
		behavior_->ClearStun();
	}
}
void Player::ClearKnockback() {
	if (behavior_) {
		behavior_->ClearKnockback();
	}
}
void Player::ClearAllDebuffs() {
	ClearStun();
	ClearKnockback();
	controlLocked_ = false;
}
bool Player::HasAnyDebuff() const { return behavior_ && (behavior_->IsStunned() || behavior_->IsKnockback() || controlLocked_); }
#pragma endregion

Vector3 Player::GetForward() const {
	const float yaw = GetYaw();
	return {std::sin(yaw), 0.0f, std::cos(yaw)};
}

#pragma region Facing
// ダッシュ中と攻撃中はYawを固定する。バレルロール中や通常時は再びカメラへ追従させる。
// 攻撃を要求したフレームからYawを固定し、攻撃開始時にモデルが回転しないようにする。
bool Player::ShouldLockYaw() const {
	if (moveMode_ == MoveMode::kDash) {
		return true;
	}
	return behavior_ && (behavior_->IsAttacking() || behavior_->IsRequested(PlayerBehavior::kAttack));
}

void Player::UpdateFacing() {
	const float cameraYaw = camera_ ? camera_->rotation_.y : facingYaw_;
	const bool lock = ShouldLockYaw();

	// フリーカーソル中は右クリックの有無に関係なくプレイヤーの向きを維持する。
	// カメラを旋回しても、ダッシュや攻撃の向きがカメラに引っ張られないようにする。
	InputManager* input = InputManager::GetInstance();
	const bool freeCursor = input->IsFreeCursorMode();
	if (freeCursor) {
		yawDetached_ = true;
	} else if (yawDetached_) {
		yawDetached_ = false;
		yawReleasing_ = true;
	}

	if (lock && !yawLocked_) {
		// フリーカーソル中は現在のプレイヤーYaw、それ以外はカメラYawを固定する。
		lockedYaw_ = freeCursor ? facingYaw_ : cameraYaw;
		yawReleasing_ = false;
	} else if (!lock && yawLocked_) {
		yawReleasing_ = true; // 固定が終了したため、急に切り替えずカメラの向きへ補間する。
	}
	yawLocked_ = lock;

	float target = cameraYaw;
	float speed = kYawFollowSpeed;
	if (lock) {
		target = lockedYaw_;
		speed = kYawLockSpeed;
	} else if (yawDetached_) {
		target = facingYaw_; // 現在の向きを維持する。
	} else if (yawReleasing_) {
		if (std::abs(std::remainder(cameraYaw - facingYaw_, kTwoPi)) < 0.02f) {
			yawReleasing_ = false; // カメラの向きに追いついたため、通常の追従速度に戻す。
		} else {
			speed = kYawReleaseSpeed;
		}
	}

	facingYaw_ = SmoothAngle(facingYaw_, target, speed);
	worldTransform_.rotation_.y = facingYaw_;
}
#pragma endregion

void Player::Update() {
	// デバッグ中にGlobalVariablesで変更した値を毎フレーム反映する。
	ApplyGlobalVariables();

	lungeCooldown_ = std::max(lungeCooldown_ - kDt, 0.0f);
	if (heavySabre_) {
		heavySabre_->UpdateWeaponState();
	}

	UpdateAttackInput();

	// 攻撃入力の後に更新することで、要求された攻撃のYaw固定を同じフレームから有効にする。
	// 状態機械の更新前に処理し、攻撃／ダッシュ方向が最新の向きを使えるようにする。
	UpdateFacing();

	// 現在の状態に応じて、このフレームのvelocity_を更新する（RootならMove()、Attackなら前方への押し出しなど）。
	if (behavior_) {
		behavior_->Update();
	}

	MapChipCollisionUpdate();
	UpdateBodyParts();
	UpdateWeapon();
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

// 通常移動はカメラ基準、フリーカーソル中のダッシュ／ロールはプレイヤー基準で行う。Shift：ダッシュ／ダッシュ中にShift：バレルロール。

void Player::Move() {
	InputManager* input = InputManager::GetInstance();

	const float cameraYaw = camera_ ? camera_->rotation_.y : 0.0f;
	const bool freeCursor = input->IsFreeCursorMode();

	// 通常移動は従来どおりカメラ基準で行う。
	const float cameraSin = std::sin(cameraYaw);
	const float cameraCos = std::cos(cameraYaw);
	const Vector3 cameraForward = {cameraSin, 0.0f, cameraCos};
	const Vector3 cameraRight = {cameraCos, 0.0f, -cameraSin};

	// フリーカーソル中のダッシュ／ロールはプレイヤー自身のYawを基準にする。
	const float actionYaw = freeCursor ? facingYaw_ : cameraYaw;
	const float actionSin = std::sin(actionYaw);
	const float actionCos = std::cos(actionYaw);
	const Vector3 actionForward = {actionSin, 0.0f, actionCos};
	const Vector3 actionRight = {actionCos, 0.0f, -actionSin};

	const Vector2 moveAxis = input->GetMoveAxis();

	// 入力方向をカメラ基準のローカル空間で取得する。
	Vector3 localDir = {moveAxis.x, 0.0f, moveAxis.y};

	const float len = std::sqrt(localDir.x * localDir.x + localDir.z * localDir.z);
	const bool hasInput = len > 0.0001f;

	if (hasInput) {
		localDir.x /= len;
		localDir.z /= len;
	}

	// 入力がない場合のアクション方向は、プレイヤーの後方を基準にする。
	const Vector3 actionDirLocal = hasInput ? localDir : Vector3{0.0f, 0.0f, -1.0f};

	// 通常移動用：カメラ基準のローカル方向をワールド方向へ変換する。
	auto ToWorldDirection = [&](const Vector3& local) -> Vector3 { return {cameraRight.x * local.x + cameraForward.x * local.z, 0.0f, cameraRight.z * local.x + cameraForward.z * local.z}; };

	// アクション用：ロック中はカメラ基準、フリーカーソル中はプレイヤー基準で変換する。
	auto ToActionWorldDirection = [&](const Vector3& local) -> Vector3 { return {actionRight.x * local.x + actionForward.x * local.z, 0.0f, actionRight.z * local.x + actionForward.z * local.z}; };

	const Vector3 dir = ToWorldDirection(localDir);

	// ロール中にカーソルを解除した場合も、その時点の方向をワールド方向として固定する。
	if (moveMode_ == MoveMode::kRoll && freeCursor && !rollDirectionIsWorld_) {
		rollDir_ = ToWorldDirection(rollDir_);
		rollDirectionIsWorld_ = true;
	}

	// ---- 状態遷移 ----
	if (input->IsDashTriggered()) {
		if (moveMode_ == MoveMode::kNormal) {
			// ダッシュ方向は開始時にワールド方向へ変換し、以降のカメラ旋回の影響を受けない。
			EnterDash(ToActionWorldDirection(actionDirLocal));

			// ロール用の入力方向は保持しておき、ロール開始時に必要ならワールド方向へ固定する。
			rollDir_ = actionDirLocal;
			rollDirectionIsWorld_ = false;
		} else if (moveMode_ == MoveMode::kDash) {
			// 2回目のShift時に方向入力があれば優先し、なければダッシュ開始時の方向を使う。
			const Vector3 rollLocalDir = hasInput ? localDir : rollDir_;

			// 入力の強い軸から、左右ロール（A/D）か前後ロール（W/S）かを判定する。
			rollIsHorizontal_ = std::abs(rollLocalDir.x) >= std::abs(rollLocalDir.z);
			const float rollAxisInput = rollIsHorizontal_ ? rollLocalDir.x : rollLocalDir.z;
			// 前後ロールの回転方向を反転する。
			float sign = rollAxisInput < -0.1f ? 1.0f : -1.0f;
			if (!rollIsHorizontal_) {
				sign *= -1.0f;
			}

			// フリーカーソル中はプレイヤー基準のワールド方向を保存し、カメラ旋回から切り離す。
			if (freeCursor) {
				EnterRoll(ToActionWorldDirection(rollLocalDir), sign);
				rollDirectionIsWorld_ = true;
			} else {
				EnterRoll(rollLocalDir, sign);
				rollDirectionIsWorld_ = false;
			}
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

		// フリーカーソル中に開始したロールは保存済みのワールド方向を使う。
		// ロック中に開始したロールは従来どおりカメラ基準のローカル方向を使う。
		const Vector3 rollWorldDir = rollDirectionIsWorld_ ? rollDir_ : ToWorldDirection(rollDir_);

		velocity_ = {rollWorldDir.x * kRollSpeed, 0.0f, rollWorldDir.z * kRollSpeed};

		if (modeTimer_ >= kRollDuration) {
			moveMode_ = MoveMode::kNormal;
			modeTimer_ = 0.0f;
			velocity_ = {}; // ロール終了時に速度を完全に0にし、通常移動へ慣性を持ち越さない。
		}
		return;
	}

	case MoveMode::kNormal:
		break;
	}

	// ---- 通常移動 ----
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

	// マップの外周はブロックとは別に扱う。CSVに境界ブロックがなくてもプレイヤーが外へ出ないようにする。
	worldTransform_.translation_.x = std::clamp(worldTransform_.translation_.x, mapChipField_->GetWorldMinX() + kWidth * 0.5f, mapChipField_->GetWorldMaxX() - kWidth * 0.5f);
	worldTransform_.translation_.z = std::clamp(worldTransform_.translation_.z, mapChipField_->GetWorldMinZ() + kDepth * 0.5f, mapChipField_->GetWorldMaxZ() - kDepth * 0.5f);

	// 壁に当たった軸の速度だけを止め、壁に沿って滑れるようにする。
	if (std::abs(info.move.x) < std::abs(velocity_.x))
		velocity_.x = 0.0f;
	if (std::abs(info.move.z) < std::abs(velocity_.z))
		velocity_.z = 0.0f;
}

void Player::UpdateBodyParts() {
	WorldTransformUpdate(worldTransform_);

	// ---- アニメーション：移動姿勢の補間＋攻撃姿勢の補間＋バレルロールの回転 ----
	SRTOffset target{};
	float rollAngle = 0.0f;
	if (moveMode_ == MoveMode::kDash) {
		target = kDashPose;
	} else if (moveMode_ == MoveMode::kRoll) {
		// A/Dは左右用、W/Sは前後用のSRT姿勢を使い分ける。
		target = rollIsHorizontal_ ? kHorizontalRollPose : kVerticalRollPose;
		const float t = std::clamp(modeTimer_ / std::max(kRollDuration, 0.0001f), 0.0f, 1.0f);
		rollAngle = rollSign_ * kRollRotationAngle * EaseInOut(0.0f, 1.0f, t);
	}
	currentPose_ = AnimationManager::SmoothSRT(currentPose_, target);

	SRTOffset actionTarget{};
	if (behavior_ && behavior_->IsAttacking()) {
		actionTarget = behavior_->GetAttackPoseOffset();
	}
	currentActionPose_ = AnimationManager::SmoothSRT(currentActionPose_, actionTarget);

	// 左右ロールは前方向の軸（Z）を中心に、前後ロールは左右の軸（X）を中心に回転させる。
	const bool isRolling = moveMode_ == MoveMode::kRoll;
	const float pitch = currentPose_.rotation.x + currentActionPose_.rotation.x + (isRolling && !rollIsHorizontal_ ? rollAngle : 0.0f);
	const float roll = currentPose_.rotation.z + currentActionPose_.rotation.z + (isRolling && rollIsHorizontal_ ? rollAngle : 0.0f);
	const float yaw = worldTransform_.rotation_.y + currentActionPose_.rotation.y; // 攻撃動作による体のひねりを加える。

	// 回転順はPitch（前傾）→前方向を軸にしたRoll→Yaw（カメラ方向）。通常のEuler X-Y-Zでは、Rollが次の行のように
	// ワールドZ軸回りになり、プレイヤーの向きが変わると前方向と一致しないため、行列を個別に合成する。
	const Matrix4x4 bodyRot = Multiply(MakeRotateXMatrix(pitch), Multiply(MakeRotateZMatrix(roll), MakeRotateYMatrix(yaw)));

	// 武器と攻撃判定から同じ体の回転を参照できるように保存する。
	bodyRotation_ = bodyRot;

	const Vector3 poseScale = {
	    currentPose_.scale.x * currentActionPose_.scale.x,
	    currentPose_.scale.y * currentActionPose_.scale.y,
	    currentPose_.scale.z * currentActionPose_.scale.z,
	};
	const Vector3 bodyScale = {worldTransform_.scale_.x * poseScale.x, worldTransform_.scale_.y * poseScale.y, worldTransform_.scale_.z * poseScale.z};
	const Vector3 bodyPos = worldTransform_.translation_ + currentPose_.translation + currentActionPose_.translation;
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

	if (heavySabre_) {
		heavySabre_->Draw();
	}
}

AABB Player::GetAABB() const {
	const Vector3 p = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {p.x - kWidth / 2.0f, p.y - kHeight / 2.0f, p.z - kDepth / 2.0f};
	aabb.max = {p.x + kWidth / 2.0f, p.y + kHeight / 2.0f, p.z + kDepth / 2.0f};
	return aabb;
}

AABB Player::GetFootprintAABB() const {
	const Vector3 p = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {p.x - kWidth / 2.0f, p.y - 0.05f, p.z - kDepth / 2.0f};
	aabb.max = {p.x + kWidth / 2.0f, p.y + 0.05f, p.z + kDepth / 2.0f};
	return aabb;
}