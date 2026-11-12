#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>
#include <algorithm>
#include <array>
#include <vector>

enum class WeaponState { kHolstered, kEquipped };

/// Melee weapon base (top-down version).
/// Equip state, pose blending and swing AABBs are implemented HERE once.
/// Derived weapons (WeaponHeavySabre) only supply their data through the pure-virtual getters.
///
/// Pose frame: every SRTOffset pose is expressed in the OWNER's yaw frame, exactly like the old side-view poses
/// were expressed in screen space (the camera sits behind the player, so screen = owner frame):
///   translation.x = right, .y = up, .z = forward      rotation.z = roll in the screen plane (the swing)
/// A pose with rotation (0,0,0) points the blade to the owner's right (the old "facing right").
class BaseMelee {
public:
	enum class AttackPhase { kAnticipate, kAttack, kRecovery };
	struct GenericAttackAnimState {
		AttackPhase phase = AttackPhase::kAnticipate;
		float t = 0.0f;
		int comboIndex = 0;
	};

	enum class MeleeBehavior { kIdle, kAttacking };
	MeleeBehavior behavior_ = MeleeBehavior::kIdle;

	void ResetSwingHits() { hitTargetsThisSwing_.clear(); }
	bool HasHit(const void* target) const { return std::find(hitTargetsThisSwing_.begin(), hitTargetsThisSwing_.end(), target) != hitTargetsThisSwing_.end(); }
	void MarkHit(const void* target) { hitTargetsThisSwing_.push_back(target); }

	virtual void BehaviorIdleInitialize() {}
	virtual void BehaviorIdleUpdate() {}
	virtual void BehaviorAttackingInitialize() {}
	virtual void BehaviorAttackingUpdate() {}

	// Does not play the swing sound; callers call PlaySwingSound(comboIndex) themselves.
	void SetBehavior(MeleeBehavior behavior) {
		if (behavior_ == behavior)
			return;
		behavior_ = behavior;
		switch (behavior_) {
		case MeleeBehavior::kIdle:
			BehaviorIdleInitialize();
			break;
		case MeleeBehavior::kAttacking:
			BehaviorAttackingInitialize();
			break;
		}
	}

	// ---------------- Equip state ----------------
	virtual void SetWeaponState(WeaponState state);
	virtual void UpdateWeaponState();
	virtual void SnapWeaponState(WeaponState state);
	void SetEquipDuration(float seconds);
	float GetEquipDuration() const { return equipDuration_; }
	WeaponState GetWeaponState() const { return weaponState_; }

	// ---------------- Pose / hitbox ----------------
	// ---------------- Pose / hitbox ----------------
	virtual void SetMeleePosition(const KamataEngine::Vector3& ownerPosition, float ownerYaw, const GenericAttackAnimState* attackState);

	// プレイヤーの回転行列を受け取り、Yaw・Pitch・Rollを反映する
	virtual void SetMeleePosition(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, const GenericAttackAnimState* attackState);

	virtual AABB GetSwingAABB(const KamataEngine::Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth, int comboIndex) const;

	// プレイヤーの回転に合わせた攻撃判定
	virtual AABB GetSwingAABB(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, float ownerHalfWidth, int comboIndex) const;

	// Dropped mid-swing = on cooldown. Picking it up again restarts the draw animation instead of resuming the swing.
	virtual DropItemState GetDropState() const { return IsAttacking() ? DropItemState::kOnCooldown : DropItemState::kReady; }

	// ---- Animation offsets: override per weapon ----
	virtual const SRTOffset& GetHolsterOffset() const { return kDefaultHolsterOffset; }
	virtual const SRTOffset& GetReadyStanceOffset() const { return kDefaultReadyStanceOffset; }
	virtual const SRTOffset& GetKnockbackOffset() const { return kDefaultKnockbackOffset; }
	virtual const SRTOffset& GetStunOffset() const { return kDefaultStunOffset; }
	virtual const SRTOffset& GetBlockOffset() const { return kDefaultBlockOffset; }

	static inline SRTOffset kDefaultHolsterOffset{
	    {1,     1,     1    },
        {0.0f,  3.14f, 0.5f },
        {0.25f, -0.1f, 0.49f}
    };
	static inline SRTOffset kDefaultReadyStanceOffset{
	    {1,     1,    1    },
        {-0.6f, 0.0f, 0.0f },
        {0.5f,  0.0f, 0.49f}
    };
	static inline SRTOffset kDefaultKnockbackOffset{
	    {1,     1,    1    },
        {-1.0f, 0.0f, 0.4f },
        {0.3f,  0.2f, 0.49f}
    };
	static inline SRTOffset kDefaultStunOffset{
	    {1,     1,      1    },
        {0.9f,  0.0f,   0.2f },
        {0.45f, -0.35f, 0.49f}
    };
	static inline SRTOffset kDefaultBlockOffset{
	    {1,     1,     1    },
        {-0.6f, 0.0f,  1.2f },
        {0.55f, 0.15f, 0.49f}
    };

	static inline float kDebuffBlendSpeed = 0.2f;

	void SetReactionPose(ReactionPose pose) {
		debuffPose_ = pose;
		if (pose != ReactionPose::kNone)
			lastReactionPose_ = pose;
	}

	bool IsAttacking() const { return behavior_ == MeleeBehavior::kAttacking; }

	virtual ~BaseMelee() = default;

	virtual void Initialize(KamataEngine::Model* model, uint32_t textureHandle, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	virtual void Update();
	virtual void Draw();

	bool HasModel() const { return model_ != nullptr; }
	bool IsDead() const { return isDead_; }
	virtual AABB GetAABB() const;

	virtual void OnCollision() { isDead_ = true; }

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::Vector3 GetWorldPosition() const { return worldTransform_.translation_; }
	virtual void SetWorldPosition(KamataEngine::Vector3& TargetPosition) { worldTransform_.translation_ = TargetPosition; }

	float GetHalfWidthMin() { return kHalfWidthMin; }
	float GetHalfHeightMin() { return kHalfHeightMin; }
	float GetHalfDepthMin() { return kHalfDepthMin; }
	float GetHalfWidthMax() { return kHalfWidthMax; }
	float GetHalfHeightMax() { return kHalfHeightMax; }
	float GetHalfDepthMax() { return kHalfDepthMax; }

	void SetHalfWidthMin(float& v) { kHalfWidthMin = v; }
	void SetHalfHeightMin(float& v) { kHalfHeightMin = v; }
	void SetHalfDepthMin(float& v) { kHalfDepthMin = v; }
	void SetHalfWidthMax(float& v) { kHalfWidthMax = v; }
	void SetHalfHeightMax(float& v) { kHalfHeightMax = v; }
	void SetHalfDepthMax(float& v) { kHalfDepthMax = v; }

	int GetMaxCombo() const { return kMaxCombo; }

	// ---- コンボの前進速度・各フェーズ時間(フレーム): プレイヤー用/敵用を分離 ----
	float GetComboAttackSpeed(int comboIndex, Owner owner) const { return (owner == Owner::kPlayer) ? kComboAttackSpeedPlayer[comboIndex] : kComboAttackSpeedEnemy[comboIndex]; }
	float GetComboAnticipateDuration(int comboIndex, Owner owner) const { return (owner == Owner::kPlayer) ? kComboAnticipateDurationPlayer[comboIndex] : kComboAnticipateDurationEnemy[comboIndex]; }
	float GetComboAttackDuration(int comboIndex, Owner owner) const { return (owner == Owner::kPlayer) ? kComboAttackDurationPlayer[comboIndex] : kComboAttackDurationEnemy[comboIndex]; }
	float GetComboRecoveryDuration(int comboIndex, Owner owner) const { return (owner == Owner::kPlayer) ? kComboRecoveryDurationPlayer[comboIndex] : kComboRecoveryDurationEnemy[comboIndex]; }

	// 旧シグネチャ(Ownerなし)は敵用として残す。プレイヤー側の呼び出しは必ずOwner::kPlayerを渡すこと
	float GetComboAttackSpeed(int comboIndex) const { return GetComboAttackSpeed(comboIndex, Owner::kEnemy); }
	float GetComboAnticipateDuration(int comboIndex) const { return GetComboAnticipateDuration(comboIndex, Owner::kEnemy); }
	float GetComboAttackDuration(int comboIndex) const { return GetComboAttackDuration(comboIndex, Owner::kEnemy); }
	float GetComboRecoveryDuration(int comboIndex) const { return GetComboRecoveryDuration(comboIndex, Owner::kEnemy); }

	// 衝撃力もプレイヤー用/敵用で分離
	virtual float GetImpactPower(int comboIndex, Owner owner) const {
		return (owner == Owner::kPlayer) ? kBaseImpactPowerPlayer + static_cast<float>(comboIndex) * kImpactPowerStepPlayer
		                                 : kBaseImpactPowerEnemy + static_cast<float>(comboIndex) * kImpactPowerStepEnemy;
	}
	float GetImpactPower(int comboIndex) const { return GetImpactPower(comboIndex, Owner::kEnemy); }

	virtual int GetDamage(int comboIndex) const { return kComboDamageEnemy[comboIndex]; }
	virtual int GetDamage(int comboIndex, Owner owner) const { return (owner == Owner::kPlayer) ? kComboDamagePlayer[comboIndex] : kComboDamageEnemy[comboIndex]; }

	// Which slash/thrust texture the swing effect uses for this combo step. Weapons override this.
	virtual AttackEffectType GetComboEffectType(int comboIndex) const {
		static constexpr AttackEffectType kTable[3] = {AttackEffectType::kSlash, AttackEffectType::kSlashOpposite, AttackEffectType::kSlashDownward};
		return kTable[std::clamp(comboIndex, 0, 2)];
	}

	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	// 剣先のワールド座標(トレイル用)。刃はモデルのローカル+Z方向と仮定。ずれる場合は GlobalVariables の TrailTipOffset で調整
	virtual KamataEngine::Vector3 GetTrailTipLocalOffset() const { return kTrailTipOffset; }
	KamataEngine::Vector3 GetTipWorldPosition() const { return TransformCoordinate(GetTrailTipLocalOffset(), worldTransform_.matWorld_); }
	static inline KamataEngine::Vector3 kTrailTipOffset = {0.0f, 0.0f, 1.3f};

	// --- SFX ---
	static void SetDefaultSwingSoundHandle(uint32_t handle) { sDefaultSwingSoundHandle_ = handle; }
	static void SetSwingSoundHandle(int comboIndex, uint32_t handle) {
		if (comboIndex < 0 || comboIndex >= kMaxCombo) {
			return;
		}
		sComboSwingHandle[comboIndex] = handle;
	}
	static void SetSfxVolume(float volume) { sSfxVolume_ = volume; }
	virtual void PlayDefaultSwingSound() {
		if (sDefaultSwingSoundHandle_ != 0) {
			AudioSettings::PlaySfx(sDefaultSwingSoundHandle_, false, sSfxVolume_);
		}
	}
	// Per-combo sound, falling back to the (overridable) default sound.
	virtual void PlaySwingSound(int comboIndex) {
		if (comboIndex < 0 || comboIndex >= kMaxCombo) {
			return;
		}
		uint32_t handle = sComboSwingHandle[comboIndex];
		if (handle == 0) {
			PlayDefaultSwingSound();
			return;
		}
		AudioSettings::PlaySfx(handle, false, sSfxVolume_);
	}

protected:
	friend class DebugMenu;

	// ---- Data every melee weapon must provide ----
	virtual const std::array<ComboAttackPattern, 3>& GetComboPatterns() const = 0;
	virtual const std::array<float, 3>& GetComboReach() const = 0;       // forward reach of each combo step
	virtual const std::array<float, 3>& GetComboSwingHeight() const = 0; // side-to-side width of each combo step's hit area
	virtual const std::array<float, 3>& GetUpDownReach() const = 0;      // (side-view down attack; unused top-down)
	virtual const std::array<float, 3>& GetUpDownWidth() const = 0;
	virtual const SRTOffset& GetDownAttackOffset() const = 0;

	// Holster <-> ready blend for the current equip state, with the reaction (knockback/stun/block) pose on top.
	SRTOffset ComputeEquipPose();
	static SRTOffset BlendComboPose(const ComboAttackPattern& pattern, AttackPhase phase, float t);


	// Writes pose into worldTransform_.
	void ApplyPoseToWorld(const SRTOffset& pose, const KamataEngine::Vector3& basePos, float baseYaw);

	// 回転行列を受け取る共通処理
	void ApplyPoseToWorld(const SRTOffset& pose, const KamataEngine::Vector3& basePos, const KamataEngine::Matrix4x4& ownerRotation);

	// Axis-aligned bounds of a box in front of the owner.
	static AABB MakeForwardBox(const KamataEngine::Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth, float reach, float width, float height);

	// 回転行列に合わせて攻撃判定の8頂点を変換する
	static AABB MakeForwardBox(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, float ownerHalfWidth, float reach, float width, float height);

	// weight: 1 = fully equipped, 0 = holstered (so a holstered weapon doesn't jump to a combat pose)
	SRTOffset ApplyReactionPose(const SRTOffset& base, float weight = 1.0f) {
		float target = (debuffPose_ != ReactionPose::kNone) ? 1.0f : 0.0f;
		reactionBlend_ = Lerp(reactionBlend_, target, kDebuffBlendSpeed);
		if (reactionBlend_ < 0.001f) {
			reactionBlend_ = 0.0f;
			return base;
		}
		const SRTOffset* dp = &GetKnockbackOffset();
		if (lastReactionPose_ == ReactionPose::kStun)
			dp = &GetStunOffset();
		else if (lastReactionPose_ == ReactionPose::kBlock)
			dp = &GetBlockOffset();
		return LerpSRT(base, *dp, reactionBlend_ * std::clamp(weight, 0.0f, 1.0f));
	}
	ReactionPose debuffPose_ = ReactionPose::kNone;
	ReactionPose lastReactionPose_ = ReactionPose::kKnockback;
	float reactionBlend_ = 0.0f;

	// ---- Equip state (shared) ----
	WeaponState weaponState_ = WeaponState::kEquipped;
	float equipTimer_ = 1.0f;
	float equipDuration_ = 0.25f;
	bool isTransitioning_ = false;

	// ---- 衝撃力: プレイヤー用 / 敵用 ----
	static inline float kBaseImpactPowerPlayer = 1.0f;
	static inline float kImpactPowerStepPlayer = 0.4f;
	static inline float kBaseImpactPowerEnemy = 1.0f;
	static inline float kImpactPowerStepEnemy = 0.4f;

	static inline std::array<int, 3> kComboDamagePlayer = {45, 45, 75};
	static inline std::array<int, 3> kComboDamageEnemy = {10, 10, 20};

	static inline int kMaxCombo = 3;
	// ---- コンボ時間(フレーム)・前進速度: プレイヤー用 ----
	static inline std::array<float, 3> kComboAttackSpeedPlayer = {0.25f, 0.25f, 0.7f};
	static inline std::array<float, 3> kComboAnticipateDurationPlayer = {10.0f, 10.0f, 15.0f};
	static inline std::array<float, 3> kComboAttackDurationPlayer = {8.0f, 8.0f, 15.0f};
	static inline std::array<float, 3> kComboRecoveryDurationPlayer = {15.0f, 15.0f, 30.0f};
	// ---- コンボ時間(フレーム)・前進速度: 敵用 ----
	static inline std::array<float, 3> kComboAttackSpeedEnemy = {0.25f, 0.25f, 0.7f};
	static inline std::array<float, 3> kComboAnticipateDurationEnemy = {10.0f, 10.0f, 15.0f};
	static inline std::array<float, 3> kComboAttackDurationEnemy = {8.0f, 8.0f, 15.0f};
	static inline std::array<float, 3> kComboRecoveryDurationEnemy = {15.0f, 15.0f, 30.0f};

	// vertical size of the swing hit areas (they are centred on the owner's height)
	static inline float kSwingBoxHeight = 1.6f;
	// the blade points to the owner's right when the pose rotation is zero
	static constexpr float kBladeYaw = 1.570796f;

	std::vector<const void*> hitTargetsThisSwing_;

	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;

	KamataEngine::Vector3 velocity_{};

	static inline int32_t kLifeTime = 60;
	int32_t deathTimer_ = kLifeTime;

	bool isDead_ = false;

	float kHalfWidthMin = 0.2f;
	float kHalfHeightMin = 0.2f;
	float kHalfWidthMax = 0.2f;
	float kHalfHeightMax = 0.2f;
	float kHalfDepthMin = 0.2f;
	float kHalfDepthMax = 0.2f;

	// --- SFX ---
	static inline uint32_t sDefaultSwingSoundHandle_ = 0;
	static inline std::array<uint32_t, 3> sComboSwingHandle = {0, 0, 0};
	static inline float sSfxVolume_ = 1.0f;
};