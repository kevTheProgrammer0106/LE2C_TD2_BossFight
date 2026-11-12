#define NOMINMAX
#include "BaseMelee.h"
#include "GlobalVariables.h"
#include "PoseGV.h"

#include <cmath>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "BaseMelee";
}

void BaseMelee::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "BaseImpactPowerPlayer", kBaseImpactPowerPlayer);
	gv->AddItem(kGroupName, "ImpactPowerStepPlayer", kImpactPowerStepPlayer);
	gv->AddItem(kGroupName, "BaseImpactPowerEnemy", kBaseImpactPowerEnemy);
	gv->AddItem(kGroupName, "ImpactPowerStepEnemy", kImpactPowerStepEnemy);
	gv->AddItem(kGroupName, "ComboDamagePlayer0", kComboDamagePlayer[0]);
	gv->AddItem(kGroupName, "ComboDamagePlayer1", kComboDamagePlayer[1]);
	gv->AddItem(kGroupName, "ComboDamagePlayer2", kComboDamagePlayer[2]);
	gv->AddItem(kGroupName, "ComboDamageEnemy0", kComboDamageEnemy[0]);
	gv->AddItem(kGroupName, "ComboDamageEnemy1", kComboDamageEnemy[1]);
	gv->AddItem(kGroupName, "ComboDamageEnemy2", kComboDamageEnemy[2]);
	gv->AddItem(kGroupName, "SwingBoxHeight", kSwingBoxHeight);
	gv->AddItem(kGroupName, "TrailTipOffset", kTrailTipOffset);

	// combo timing (3 combo steps per row; frames @60fps, speed = units per frame)
	gv->AddItem(kGroupName, "PlayerComboAttackSpeed", PoseGV::ToVec3(kComboAttackSpeedPlayer));
	gv->AddItem(kGroupName, "PlayerComboAnticipate", PoseGV::ToVec3(kComboAnticipateDurationPlayer));
	gv->AddItem(kGroupName, "PlayerComboAttack", PoseGV::ToVec3(kComboAttackDurationPlayer));
	gv->AddItem(kGroupName, "PlayerComboRecovery", PoseGV::ToVec3(kComboRecoveryDurationPlayer));
	gv->AddItem(kGroupName, "EnemyComboAttackSpeed", PoseGV::ToVec3(kComboAttackSpeedEnemy));
	gv->AddItem(kGroupName, "EnemyComboAnticipate", PoseGV::ToVec3(kComboAnticipateDurationEnemy));
	gv->AddItem(kGroupName, "EnemyComboAttack", PoseGV::ToVec3(kComboAttackDurationEnemy));
	gv->AddItem(kGroupName, "EnemyComboRecovery", PoseGV::ToVec3(kComboRecoveryDurationEnemy));
}
void BaseMelee::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kBaseImpactPowerPlayer = gv->GetFloatValue(kGroupName, "BaseImpactPowerPlayer");
	kImpactPowerStepPlayer = gv->GetFloatValue(kGroupName, "ImpactPowerStepPlayer");
	kBaseImpactPowerEnemy = gv->GetFloatValue(kGroupName, "BaseImpactPowerEnemy");
	kImpactPowerStepEnemy = gv->GetFloatValue(kGroupName, "ImpactPowerStepEnemy");
	kComboDamagePlayer[0] = gv->GetIntValue(kGroupName, "ComboDamagePlayer0");
	kComboDamagePlayer[1] = gv->GetIntValue(kGroupName, "ComboDamagePlayer1");
	kComboDamagePlayer[2] = gv->GetIntValue(kGroupName, "ComboDamagePlayer2");
	kComboDamageEnemy[0] = gv->GetIntValue(kGroupName, "ComboDamageEnemy0");
	kComboDamageEnemy[1] = gv->GetIntValue(kGroupName, "ComboDamageEnemy1");
	kComboDamageEnemy[2] = gv->GetIntValue(kGroupName, "ComboDamageEnemy2");
	kSwingBoxHeight = gv->GetFloatValue(kGroupName, "SwingBoxHeight");
	kTrailTipOffset = gv->GetVector3Value(kGroupName, "TrailTipOffset");

	PoseGV::FromVec3(kComboAttackSpeedPlayer, gv->GetVector3Value(kGroupName, "PlayerComboAttackSpeed"));
	PoseGV::FromVec3(kComboAnticipateDurationPlayer, gv->GetVector3Value(kGroupName, "PlayerComboAnticipate"));
	PoseGV::FromVec3(kComboAttackDurationPlayer, gv->GetVector3Value(kGroupName, "PlayerComboAttack"));
	PoseGV::FromVec3(kComboRecoveryDurationPlayer, gv->GetVector3Value(kGroupName, "PlayerComboRecovery"));
	PoseGV::FromVec3(kComboAttackSpeedEnemy, gv->GetVector3Value(kGroupName, "EnemyComboAttackSpeed"));
	PoseGV::FromVec3(kComboAnticipateDurationEnemy, gv->GetVector3Value(kGroupName, "EnemyComboAnticipate"));
	PoseGV::FromVec3(kComboAttackDurationEnemy, gv->GetVector3Value(kGroupName, "EnemyComboAttack"));
	PoseGV::FromVec3(kComboRecoveryDurationEnemy, gv->GetVector3Value(kGroupName, "EnemyComboRecovery"));
}

void BaseMelee::Initialize(Model* model, uint32_t textureHandle, Camera* camera, const Vector3& position) {
	// model may be null (asset missing): the weapon still animates and produces hitboxes, it just isn't drawn
	model_ = model;
	textureHandle_ = textureHandle;
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.translation_ = position;
}

void BaseMelee::Update() {
	switch (behavior_) {
	case MeleeBehavior::kIdle:
		BehaviorIdleUpdate();
		break;
	case MeleeBehavior::kAttacking:
		BehaviorAttackingUpdate();
		break;
	}
	// the world matrix is written by ApplyPoseToWorld, so it is not rebuilt here
}

void BaseMelee::Draw() {
	if (!model_ || !camera_) {
		return;
	}
	Model::PreDraw();
	model_->Draw(worldTransform_, *camera_, textureHandle_);
	Model::PostDraw();
}

AABB BaseMelee::GetAABB() const {
	Vector3 pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kHalfWidthMin / 2.0f, pos.y - kHalfHeightMin / 2.0f, pos.z - kHalfDepthMin / 2.0f};
	aabb.max = {pos.x + kHalfWidthMax / 2.0f, pos.y + kHalfHeightMax / 2.0f, pos.z + kHalfDepthMax / 2.0f};
	return aabb;
}

#pragma region Equip state
void BaseMelee::SetWeaponState(WeaponState state) {
	if (weaponState_ == state) {
		return;
	}
	weaponState_ = state;
	equipTimer_ = 0.0f;
	isTransitioning_ = true;
}

void BaseMelee::UpdateWeaponState() {
	if (!isTransitioning_) {
		return;
	}
	equipTimer_ += (1.0f / 60.0f) / equipDuration_;
	if (equipTimer_ >= 1.0f) {
		equipTimer_ = 1.0f;
		isTransitioning_ = false;
	}
}

void BaseMelee::SnapWeaponState(WeaponState state) {
	weaponState_ = state;
	equipTimer_ = 1.0f;
	isTransitioning_ = false;
}

void BaseMelee::SetEquipDuration(float seconds) { equipDuration_ = std::max(seconds, 0.01f); }
#pragma endregion

#pragma region Pose
SRTOffset BaseMelee::ComputeEquipPose() {
	const SRTOffset& holster = GetHolsterOffset();
	const SRTOffset& ready = GetReadyStanceOffset();

	SRTOffset pose = (weaponState_ == WeaponState::kHolstered) ? LerpSRT(ready, holster, equipTimer_) : LerpSRT(holster, ready, equipTimer_);
	float equipWeight = (weaponState_ == WeaponState::kEquipped) ? equipTimer_ : (1.0f - equipTimer_);
	return ApplyReactionPose(pose, equipWeight);
}

SRTOffset BaseMelee::BlendComboPose(const ComboAttackPattern& pattern, AttackPhase phase, float t) {
	switch (phase) {
	case AttackPhase::kAnticipate:
		return LerpSRT(pattern.anticipateStart, pattern.anticipateEnd, t);
	case AttackPhase::kAttack:
		return LerpSRT(pattern.anticipateEnd, pattern.attackEnd, t);
	case AttackPhase::kRecovery:
	default:
		return LerpSRT(pattern.attackEnd, pattern.recoveryEnd, t);
	}
}

// Same Euler order the old side-view code used (pitch, then yaw, then roll about the screen axis), followed by the owner's yaw.
// The roll therefore happens in the owner's screen plane, which is where the combo poses were authored.
void BaseMelee::ApplyPoseToWorld(const SRTOffset& pose, const Vector3& basePos, float baseYaw) { ApplyPoseToWorld(pose, basePos, MakeRotateYMatrix(baseYaw)); }

void BaseMelee::ApplyPoseToWorld(const SRTOffset& pose, const Vector3& basePos, const Matrix4x4& ownerRotation) {
	// 武器自身のポーズを作成する
	const Matrix4x4 local = Multiply(MakeRotateXMatrix(pose.rotation.x), Multiply(MakeRotateYMatrix(pose.rotation.y + kBladeYaw), MakeRotateZMatrix(pose.rotation.z)));

	// プレイヤーの回転を武器に適用する
	const Matrix4x4 rotation = Multiply(local, ownerRotation);

	// 武器の位置もプレイヤーの回転に合わせる
	const Vector3 position = basePos + TransformNormal(pose.translation, ownerRotation);

	worldTransform_.scale_ = pose.scale;

	// matWorld_を使って描画するため、こちらが実際の回転を保持する
	worldTransform_.rotation_ = {pose.rotation.x, pose.rotation.y + kBladeYaw, pose.rotation.z};

	worldTransform_.translation_ = position;
	worldTransform_.matWorld_ = Multiply(MakeScaleMatrix(pose.scale), Multiply(rotation, MakeTranslateMatrix(position)));

	worldTransform_.TransferMatrix();
}

void BaseMelee::SetMeleePosition(const Vector3& ownerPosition, float ownerYaw, const GenericAttackAnimState* attackState) { SetMeleePosition(ownerPosition, MakeRotateYMatrix(ownerYaw), attackState); }

void BaseMelee::SetMeleePosition(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, const GenericAttackAnimState* attackState) {
	// リアクションブレンドを毎フレーム更新する
	SRTOffset pose = ComputeEquipPose();

	if (attackState) {
		const int idx = std::clamp(attackState->comboIndex, 0, 2);
		pose = BlendComboPose(GetComboPatterns()[idx], attackState->phase, attackState->t);
	}

	ApplyPoseToWorld(pose, ownerPosition, ownerRotation);
}
#pragma endregion

#pragma region Hitboxes
AABB BaseMelee::MakeForwardBox(const Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth, float reach, float width, float height) {
	return MakeForwardBox(ownerPosition, MakeRotateYMatrix(ownerYaw), ownerHalfWidth, reach, width, height);
}

AABB BaseMelee::MakeForwardBox(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, float ownerHalfWidth, float reach, float width, float height) {
	constexpr float kOverlapMargin = 0.1f;

	const float nearDist = ownerHalfWidth - kOverlapMargin;
	const float farDist = ownerHalfWidth + reach;
	const float halfSide = width * 0.5f;
	const float halfHeight = height * 0.5f;

	AABB aabb;
	aabb.min = {1.0e9f, 1.0e9f, 1.0e9f};
	aabb.max = {-1.0e9f, -1.0e9f, -1.0e9f};

	// プレイヤーのローカル空間にある8頂点をワールド空間へ変換する
	for (float dist : {nearDist, farDist}) {
		for (float side : {-halfSide, halfSide}) {
			for (float up : {-halfHeight, halfHeight}) {
				const Vector3 localPosition = {side, up, dist};
				const Vector3 worldPosition = ownerPosition + TransformNormal(localPosition, ownerRotation);

				aabb.min.x = std::min(aabb.min.x, worldPosition.x);
				aabb.min.y = std::min(aabb.min.y, worldPosition.y);
				aabb.min.z = std::min(aabb.min.z, worldPosition.z);

				aabb.max.x = std::max(aabb.max.x, worldPosition.x);
				aabb.max.y = std::max(aabb.max.y, worldPosition.y);
				aabb.max.z = std::max(aabb.max.z, worldPosition.z);
			}
		}
	}

	return aabb;
}

AABB BaseMelee::GetSwingAABB(const Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth, int comboIndex) const {
	return GetSwingAABB(ownerPosition, MakeRotateYMatrix(ownerYaw), ownerHalfWidth, comboIndex);
}

AABB BaseMelee::GetSwingAABB(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, float ownerHalfWidth, int comboIndex) const {
	comboIndex = std::clamp(comboIndex, 0, 2);

	return MakeForwardBox(ownerPosition, ownerRotation, ownerHalfWidth, GetComboReach()[comboIndex], GetComboSwingHeight()[comboIndex], kSwingBoxHeight);
}
#pragma endregion