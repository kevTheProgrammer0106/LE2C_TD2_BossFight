#pragma once
#include "BaseMelee.h"
#include "Global.h"

/// Heavy sabre. Shared melee logic lives in BaseMelee; this class holds its data plus the lunge.
/// (Top-down: positions are in the owner's yaw frame, see BaseMelee.)
class WeaponHeavySabre final : public BaseMelee {
public:
	WeaponHeavySabre() { equipDuration_ = 0.3f; }

	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	AABB GetAABB() const override;

	// ---------------- Enemy's HeavySword lunge ----------------
	void SetLungePositionGeneric(const KamataEngine::Vector3& ownerPosition, float ownerYaw, AttackPhase lungePhase, float t);
	void SetLungePositionGeneric(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, AttackPhase lungePhase, float t);

	AABB GetLungeAABBGeneric(const KamataEngine::Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth) const;
	AABB GetLungeAABBGeneric(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, float ownerHalfWidth) const;

	int GetLungeDamage() const { return kLungeDamage; }
	float GetLungeImpactPower() const { return kLungeImpactPower; }

	// --- SFX ---
	// Loads the heavy sabre sounds once (safe to call every scene start). Files that do not exist are skipped,
	// the weapon then falls back to the shared combo sounds / stays silent. Looked up in Resources/sfx_bgm/:
	//   swing : heavySabreSwing.wav   lunge : heavySabreLunge.wav
	static void LoadSounds();
	static void SetSwingSoundHandle(uint32_t handle) { sSwingSoundHandle_ = handle; }
	static void SetLungeSoundHandle(uint32_t handle) { sLungeSoundHandle_ = handle; }
	static void PlayLungeSound() {
		if (sLungeSoundHandle_ != 0) {
			AudioSettings::PlaySfx(sLungeSoundHandle_, false, sSfxVolume_);
		}
	}

	const SRTOffset& GetHolsterOffset() const override { return holsterOffset_; }
	const SRTOffset& GetReadyStanceOffset() const override { return readyStanceOffset_; }
	const SRTOffset& GetKnockbackOffset() const override { return kKnockbackOffset; }
	const SRTOffset& GetStunOffset() const override { return kStunOffset; }
	const SRTOffset& GetBlockOffset() const override { return kBlockOffset; }

	static float GetLungeReach() { return kLungeReach; }

	// ---------------- Player lunge. Separate values from the enemy lunge ----------------
	LungeSpec GetPlayerLungeSpec() const { return {kPlayerLungeAnticipate, kPlayerLungeAttack, kPlayerLungeRecovery, kPlayerLungeSpeed, kPlayerLungeDamage, kPlayerLungeImpact, kPlayerLungeCooldown}; }
	static float GetPlayerLungeReach() { return kPlayerLungeReach; }
	static float GetPlayerLungeHeight() { return kPlayerLungeHeight; }
	int GetPlayerLungeDamage() const { return kPlayerLungeDamage; }
	float GetPlayerLungeImpactPower() const { return kPlayerLungeImpact; }

	void SetPlayerLungePosition(const KamataEngine::Vector3& ownerPosition, float ownerYaw, AttackPhase phase, float t);
	void SetPlayerLungePosition(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, AttackPhase phase, float t);

	AABB GetPlayerLungeAABB(const KamataEngine::Vector3& ownerPosition, float ownerYaw, float ownerHalfWidth) const;
	AABB GetPlayerLungeAABB(const KamataEngine::Vector3& ownerPosition, const KamataEngine::Matrix4x4& ownerRotation, float ownerHalfWidth) const;

	// Uses the heavy-specific swing sound when set, otherwise the shared combo sounds.
	void PlaySwingSound(int comboIndex) override {
		if (sSwingSoundHandle_ != 0) {
			AudioSettings::PlaySfx(sSwingSoundHandle_, false, sSfxVolume_);
		} else {
			BaseMelee::PlaySwingSound(comboIndex);
		}
	}
	void PlayDefaultSwingSound() override {
		uint32_t handle = (sSwingSoundHandle_ != 0) ? sSwingSoundHandle_ : sDefaultSwingSoundHandle_;
		if (handle != 0) {
			AudioSettings::PlaySfx(handle, false, sSfxVolume_);
		}
	}

protected:
	const std::array<ComboAttackPattern, 3>& GetComboPatterns() const override { return comboPatterns_; }
	const std::array<float, 3>& GetComboReach() const override { return kComboReach; }
	const std::array<float, 3>& GetComboSwingHeight() const override { return kComboSwingHeight; }
	const std::array<float, 3>& GetUpDownReach() const override { return kUpDownReach; }
	const std::array<float, 3>& GetUpDownWidth() const override { return kUpDownWidth; }
	const SRTOffset& GetDownAttackOffset() const override { return kDownAttackOffset; }

private:
	friend class DebugMenu;

	static inline SRTOffset holsterOffset_{
	    {1.0f,   1.0f,    1.0f  },
        {0.4f,   1.560f,    0.070f},
        {-0.505f, -0.270f, 0.450f}
    };
	static inline SRTOffset readyStanceOffset_{
	    {1.0f,  1.0f, 1.0f },
        {2.39f, 1.67f, -3.12f },
        {0.565f, -0.03f, 0.46f}
    };

	static inline std::array<ComboAttackPattern, 3> comboPatterns_ = {
	    ComboAttackPattern{
	                       {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.4f}, {0.6f, -0.1f, 0.8f}},
	                       {{1.0f, 1.0f, 1.0f}, {-0.2f, -0.8f, 1.6f}, {0.4f, 0.2f, 0.5f}},
	                       {{1.0f, 1.0f, 1.0f}, {2.0f, -1.0f, 1.2f}, {0.4f, -0.2f, -0.5f}},
	                       {{1.0f, 1.0f, 1.0f}, {2.0f, -1.0f, 1.2f}, {0.4f, -0.2f, -0.5f}},
	                       },
	    ComboAttackPattern{
	                       {{1.0f, 1.0f, 1.0f}, {2.0f, -1.0f, 1.2f}, {0.4f, -0.2f, -0.5f}},
	                       {{1.0f, 1.0f, 1.0f}, {1.1f, -1.8f, 3.8f}, {0.25f, 0.35f, -0.3f}},
	                       {{1.0f, 1.0f, 1.0f}, {4.8f, -1.0f, 3.9f}, {0.25f, -0.25f, 0.5f}},
	                       {{1.0f, 1.0f, 1.0f}, {4.8f, -1.0f, 3.9f}, {0.25f, -0.25f, 0.5f}},
	                       },
	    ComboAttackPattern{
	                       {{1.0f, 1.0f, 1.0f}, {4.8f, -1.0f, 3.9f}, {0.25f, -0.25f, 0.5f}},
	                       {{1.0f, 1.0f, 1.0f}, {4.2f, -1.3f, 0.7f}, {-0.5f, 0.5f, 0.6f}},
	                       {{1.0f, 1.0f, 1.0f}, {5.4f, -0.1f, -0.4f}, {0.8f, -0.15f, 0.0f}},
	                       {{1.0f, 1.0f, 1.0f}, {5.4f, -0.1f, -0.4f}, {0.8f, -0.15f, 0.0f}},
	                       },
	};

	static inline SRTOffset kLungeAnticipate{
	    {1.0f, 1.0f,   1.0f},
        {0.0f, -0.3f,  0.6f},
        {0.3f, -0.05f, 0.5f}
    };
	static inline SRTOffset kLungeExtend{
	    {1.0f, 1.0f,  1.0f },
        {0.0f, -0.1f, 1.5f },
        {0.3f, -0.1f, -0.3f}
    };
	static inline SRTOffset kKnockbackOffset{
	    {1.0f,  1.0f, 1.0f },
        {-1.0f, 0.0f, 0.4f },
        {0.3f,  0.2f, 0.45f}
    };
	static inline SRTOffset kStunOffset{
	    {1.0f,  1.0f,   1.0f },
        {0.9f,  0.0f,   0.2f },
        {0.45f, -0.35f, 0.45f}
    };
	static inline SRTOffset kBlockOffset{
	    {1.0f,  1.0f,  1.0f },
        {-0.6f, 0.0f,  1.2f },
        {0.55f, 0.15f, 0.49f}
    };

	static inline std::array<float, 3> kComboReach = {2.0f, 2.0f, 2.4f};
	static inline std::array<float, 3> kComboSwingHeight = {1.8f, 1.8f, 2.0f};

	// (side-view down attack, kept for the existing GlobalVariables keys)
	static inline SRTOffset kDownAttackOffset{
	    {1.0f, 1.0f,  1.0f},
        {1.2f, 0.0f,  0.0f},
        {0.0f, -1.0f, 0.0f}
    };
	static inline std::array<float, 3> kUpDownReach = {1.2f, 1.4f, 1.8f};
	static inline std::array<float, 3> kUpDownWidth = {1.2f, 1.3f, 1.5f};

	static inline float kLungeReach = 4.5f;

	static inline int kLungeDamage = 25;
	static inline float kLungeImpactPower = 2.2f;

	static inline float kHalfWidthMin = 0.9f;
	static inline float kHalfHeightMin = 0.9f;
	static inline float kHalfWidthMax = 0.9f;
	static inline float kHalfHeightMax = 0.9f;
	static inline float kHalfDepthMin = 0.9f;
	static inline float kHalfDepthMax = 0.9f;

	// --- SFX ---
	static inline uint32_t sSwingSoundHandle_ = 0;
	static inline uint32_t sLungeSoundHandle_ = 0;

	// Player lunge values
	static inline SRTOffset kPlayerLungeAnticipatePose{
	    {1.0f, 1.0f,   1.0f},
        {0.0f, -0.3f,  0.6f},
        {0.3f, -0.05f, 0.5f}
    };
	static inline SRTOffset kPlayerLungeExtendPose{
	    {1.0f, 1.0f,  1.0f },
        {0.0f, -0.1f, 1.5f },
        {0.3f, -0.1f, -0.3f}
    };
	static inline float kPlayerLungeAnticipate = 21.0f; // frames
	static inline float kPlayerLungeAttack = 18.0f;
	static inline float kPlayerLungeRecovery = 15.0f;
	static inline float kPlayerLungeSpeed = 0.35f;
	static inline int kPlayerLungeDamage = 80;
	static inline float kPlayerLungeImpact = 2.2f;
	static inline float kPlayerLungeCooldown = 4.0f;
	static inline float kPlayerLungeReach = 4.5f;
	static inline float kPlayerLungeHeight = 1.4f; // hit area width
};