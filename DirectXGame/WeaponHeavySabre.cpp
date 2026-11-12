#define NOMINMAX
#include "WeaponHeavySabre.h"
#include "GlobalVariables.h"
#include "PoseGV.h"

#include <filesystem>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "WeaponHeavySabre";

// ready -> anticipate -> extend -> ready
SRTOffset BlendLunge(const SRTOffset& ready, const SRTOffset& anticipate, const SRTOffset& extend, BaseMelee::AttackPhase phase, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	switch (phase) {
	case BaseMelee::AttackPhase::kAnticipate:
		return LerpSRT(ready, anticipate, t);
	case BaseMelee::AttackPhase::kAttack:
		return LerpSRT(anticipate, extend, t);
	case BaseMelee::AttackPhase::kRecovery:
	default:
		return LerpSRT(extend, ready, t);
	}
}

// Loads Resources/sfx_bgm/<fileName> when it exists. Returns 0 when it does not (no assert on a missing sound).
uint32_t LoadWaveIfExists(const char* fileName) {
	const std::string relative = std::string("Resources/sfx_bgm/") + fileName;
	if (!std::filesystem::exists(relative)) {
		return 0;
	}
	return Audio::GetInstance()->LoadWave("./" + relative);
}
} // namespace

void WeaponHeavySabre::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "HalfWidthMin", kHalfWidthMin);
	gv->AddItem(kGroupName, "HalfWidthMax", kHalfWidthMax);
	gv->AddItem(kGroupName, "HalfHeightMin", kHalfHeightMin);
	gv->AddItem(kGroupName, "HalfHeightMax", kHalfHeightMax);
	gv->AddItem(kGroupName, "HalfDepthMin", kHalfDepthMin);
	gv->AddItem(kGroupName, "HalfDepthMax", kHalfDepthMax);
	gv->AddItem(kGroupName, "LungeReach", kLungeReach);
	gv->AddItem(kGroupName, "HolsterOffsetScale", holsterOffset_.scale);
	gv->AddItem(kGroupName, "HolsterOffsetTranslation", holsterOffset_.translation);
	gv->AddItem(kGroupName, "HolsterOffsetRotation", holsterOffset_.rotation);
	gv->AddItem(kGroupName, "LungeDamage", kLungeDamage);
	gv->AddItem(kGroupName, "LungeImpactPower", kLungeImpactPower);
	gv->AddItem(kGroupName, "PlayerLungeAnticipate", kPlayerLungeAnticipate);
	gv->AddItem(kGroupName, "PlayerLungeAttack", kPlayerLungeAttack);
	gv->AddItem(kGroupName, "PlayerLungeRecovery", kPlayerLungeRecovery);
	gv->AddItem(kGroupName, "PlayerLungeSpeed", kPlayerLungeSpeed);
	gv->AddItem(kGroupName, "PlayerLungeDamage", kPlayerLungeDamage);
	gv->AddItem(kGroupName, "PlayerLungeImpact", kPlayerLungeImpact);
	gv->AddItem(kGroupName, "PlayerLungeCooldown", kPlayerLungeCooldown);
	gv->AddItem(kGroupName, "PlayerLungeReach", kPlayerLungeReach);
	gv->AddItem(kGroupName, "PlayerLungeHeight", kPlayerLungeHeight);

	// ---- ポーズ(構え / リアクション / 下攻撃) ----
	PoseGV::Register(kGroupName, "Ready", readyStanceOffset_);
	PoseGV::Register(kGroupName, "Knockback", kKnockbackOffset);
	PoseGV::Register(kGroupName, "Stun", kStunOffset);
	PoseGV::Register(kGroupName, "Block", kBlockOffset);
	PoseGV::Register(kGroupName, "DownAttack", kDownAttackOffset);
	// ---- コンボ(3段 x 4キーフレーム) ----
	PoseGV::RegisterCombo(kGroupName, "Combo", comboPatterns_);
	// ---- ランジのポーズ(敵 / プレイヤー) ----
	PoseGV::Register(kGroupName, "LungePoseAnticipate", kLungeAnticipate);
	PoseGV::Register(kGroupName, "LungePoseExtend", kLungeExtend);
	PoseGV::Register(kGroupName, "PlayerLungePoseAnticipate", kPlayerLungeAnticipatePose);
	PoseGV::Register(kGroupName, "PlayerLungePoseExtend", kPlayerLungeExtendPose);
	// ---- コンボの射程 / 高さ(3段分を1行で) ----
	gv->AddItem(kGroupName, "ComboReach", PoseGV::ToVec3(kComboReach));
	gv->AddItem(kGroupName, "ComboSwingHeight", PoseGV::ToVec3(kComboSwingHeight));
}

void WeaponHeavySabre::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kHalfWidthMin = gv->GetFloatValue(kGroupName, "HalfWidthMin");
	kHalfWidthMax = gv->GetFloatValue(kGroupName, "HalfWidthMax");
	kHalfHeightMin = gv->GetFloatValue(kGroupName, "HalfHeightMin");
	kHalfHeightMax = gv->GetFloatValue(kGroupName, "HalfHeightMax");
	kHalfDepthMin = gv->GetFloatValue(kGroupName, "HalfDepthMin");
	kHalfDepthMax = gv->GetFloatValue(kGroupName, "HalfDepthMax");
	kLungeReach = gv->GetFloatValue(kGroupName, "LungeReach");
	holsterOffset_.scale = gv->GetVector3Value(kGroupName, "HolsterOffsetScale");
	holsterOffset_.translation = gv->GetVector3Value(kGroupName, "HolsterOffsetTranslation");
	holsterOffset_.rotation = gv->GetVector3Value(kGroupName, "HolsterOffsetRotation");
	kLungeDamage = gv->GetIntValue(kGroupName, "LungeDamage");
	kLungeImpactPower = gv->GetFloatValue(kGroupName, "LungeImpactPower");
	kPlayerLungeAnticipate = gv->GetFloatValue(kGroupName, "PlayerLungeAnticipate");
	kPlayerLungeAttack = gv->GetFloatValue(kGroupName, "PlayerLungeAttack");
	kPlayerLungeRecovery = gv->GetFloatValue(kGroupName, "PlayerLungeRecovery");
	kPlayerLungeSpeed = gv->GetFloatValue(kGroupName, "PlayerLungeSpeed");
	kPlayerLungeDamage = gv->GetIntValue(kGroupName, "PlayerLungeDamage");
	kPlayerLungeImpact = gv->GetFloatValue(kGroupName, "PlayerLungeImpact");
	kPlayerLungeCooldown = gv->GetFloatValue(kGroupName, "PlayerLungeCooldown");
	kPlayerLungeReach = gv->GetFloatValue(kGroupName, "PlayerLungeReach");
	kPlayerLungeHeight = gv->GetFloatValue(kGroupName, "PlayerLungeHeight");

	PoseGV::Apply(kGroupName, "Ready", readyStanceOffset_);
	PoseGV::Apply(kGroupName, "Knockback", kKnockbackOffset);
	PoseGV::Apply(kGroupName, "Stun", kStunOffset);
	PoseGV::Apply(kGroupName, "Block", kBlockOffset);
	PoseGV::Apply(kGroupName, "DownAttack", kDownAttackOffset);
	PoseGV::ApplyCombo(kGroupName, "Combo", comboPatterns_);
	PoseGV::Apply(kGroupName, "LungePoseAnticipate", kLungeAnticipate);
	PoseGV::Apply(kGroupName, "LungePoseExtend", kLungeExtend);
	PoseGV::Apply(kGroupName, "PlayerLungePoseAnticipate", kPlayerLungeAnticipatePose);
	PoseGV::Apply(kGroupName, "PlayerLungePoseExtend", kPlayerLungeExtendPose);
	PoseGV::FromVec3(kComboReach, gv->GetVector3Value(kGroupName, "ComboReach"));
	PoseGV::FromVec3(kComboSwingHeight, gv->GetVector3Value(kGroupName, "ComboSwingHeight"));
}

void WeaponHeavySabre::LoadSounds() {
	// the handles are static, so loading once is enough even though every GameScene start calls this
	static bool loaded = false;
	if (loaded) {
		return;
	}
	loaded = true;
	sSwingSoundHandle_ = LoadWaveIfExists("heavySabreSwing.wav");
	sLungeSoundHandle_ = LoadWaveIfExists("heavySabreLunge.wav");
}

AABB WeaponHeavySabre::GetAABB() const {
	Vector3 pos = worldTransform_.translation_;
	AABB aabb;
	aabb.min = {pos.x - kHalfWidthMin / 2.0f, pos.y - kHalfHeightMin / 2.0f, pos.z - kHalfDepthMin / 2.0f};
	aabb.max = {pos.x + kHalfWidthMax / 2.0f, pos.y + kHalfHeightMax / 2.0f, pos.z + kHalfDepthMax / 2.0f};
	return aabb;
}

void WeaponHeavySabre::SetLungePositionGeneric(const Vector3& ownerPosition, float ownerYaw, AttackPhase lungePhase, float t) {
	SetLungePositionGeneric(ownerPosition, MakeRotateYMatrix(ownerYaw), lungePhase, t);
}

void WeaponHeavySabre::SetLungePositionGeneric(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, AttackPhase lungePhase, float t) {
	ApplyPoseToWorld(BlendLunge(readyStanceOffset_, kLungeAnticipate, kLungeExtend, lungePhase, t), ownerPosition, ownerRotation);
}

AABB WeaponHeavySabre::GetLungeAABBGeneric(const Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth) const {
	return GetLungeAABBGeneric(ownerPosition, MakeRotateYMatrix(ownerYaw), ownerHalfWidth);
}

AABB WeaponHeavySabre::GetLungeAABBGeneric(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, float ownerHalfWidth) const {
	constexpr float kLungeHeightRatio = 0.875f;

	return MakeForwardBox(ownerPosition, ownerRotation, ownerHalfWidth, kLungeReach, kSwingBoxHeight * kLungeHeightRatio, kSwingBoxHeight);
}

void WeaponHeavySabre::SetPlayerLungePosition(const Vector3& ownerPosition, float ownerYaw, AttackPhase phase, float t) {
	SetPlayerLungePosition(ownerPosition, MakeRotateYMatrix(ownerYaw), phase, t);
}

void WeaponHeavySabre::SetPlayerLungePosition(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, AttackPhase phase, float t) {
	ApplyPoseToWorld(BlendLunge(readyStanceOffset_, kPlayerLungeAnticipatePose, kPlayerLungeExtendPose, phase, t), ownerPosition, ownerRotation);
}

AABB WeaponHeavySabre::GetPlayerLungeAABB(const Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth) const {
	return GetPlayerLungeAABB(ownerPosition, MakeRotateYMatrix(ownerYaw), ownerHalfWidth);
}

AABB WeaponHeavySabre::GetPlayerLungeAABB(const Vector3& ownerPosition, const Matrix4x4& ownerRotation, float ownerHalfWidth) const {
	return MakeForwardBox(ownerPosition, ownerRotation, ownerHalfWidth, kPlayerLungeReach, kPlayerLungeHeight, kSwingBoxHeight);
}