#pragma once

#define NOMINMAX

#include "BaseBehavior.h"
#include "BaseMelee.h"
#include "Global.h"

class Player;

/// Player state machine (top-down port of the old side-view PlayerBehavior).
///   kRoot      walking / dashing / rolling (Player::Move)
///   kAttack    heavy sabre combo (3 steps) or lunge. anticipate -> attack -> recovery
///   kKnockback pushed along the ground, friction until it settles
///   kStunned   frozen for a duration
/// Death, block, camera FX and the bayonet lunge were not ported (no HP / enemies / those weapons yet).
class PlayerBehavior : public BaseBehavior {
public:
	enum State : StateId { kRoot, kAttack, kKnockback, kStunned };
	enum class AttackKind { kCombo, kLunge };
	using AttackPhase = BaseMelee::AttackPhase; // same enum the weapon blends with
	using AttackAnimState = BaseMelee::GenericAttackAnimState;

	explicit PlayerBehavior(Player& owner) : owner_(owner) {}

	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();

	// ---- requests (applied on the next Update) ----
	void RequestAttack(); // first press starts a combo, presses during attack/recovery buffer the next step
	void RequestLunge();
	void RequestStun(float duration);
	void RequestHitKnockback(const KamataEngine::Vector3& direction, float power); // direction on the X-Z plane
	void ClearStun();
	void ClearKnockback();

	// ---- queries ----
	bool IsAttacking() const { return IsIn(kAttack); }
	bool IsLunge() const { return IsIn(kAttack) && attackKind_ == AttackKind::kLunge; }
	bool IsSwingActive() const { return IsIn(kAttack) && attackKind_ == AttackKind::kCombo; }
	bool IsHitboxActive() const { return IsIn(kAttack) && attackPhase_ == AttackPhase::kAttack; }
	bool IsStunned() const { return IsIn(kStunned); }
	bool IsKnockback() const { return IsIn(kKnockback); }
	float GetStunRemaining() const { return (IsIn(kStunned) && stunDuration_ > stunTimer_) ? (stunDuration_ - stunTimer_) : 0.0f; }
	int GetComboIndex() const { return comboIndex_; }
	AttackKind GetAttackKind() const { return attackKind_; }

	AttackAnimState GetAttackAnimState() const;
	SRTOffset GetAttackPoseOffset() const; // body pose for the current attack frame

protected:
	void OnEnter(StateId state, StateId previous) override;
	void OnUpdate(StateId state) override;

private:
	friend class DebugMenu;

	void UpdateAttack();
	void UpdateKnockback();
	void UpdateStunned();
	float PhaseDuration(AttackPhase phase) const; // frames
	void ResetAttack();

	Player& owner_;

	// attack
	bool isAttacking_ = false; // a request has been accepted (covers the frame before kAttack is entered)
	AttackKind attackKind_ = AttackKind::kCombo;
	AttackPhase attackPhase_ = AttackPhase::kAnticipate;
	int comboIndex_ = 0;
	bool comboInputBuffered_ = false;
	uint32_t attackParameter_ = 0; // frames spent in the current phase

	// knockback / stun
	uint32_t knockbackFrames_ = 0;
	float stunTimer_ = 0.0f;
	float stunDuration_ = 0.0f;

	static inline int32_t kKnockBackDuration = 25;    // frames before it may end
	static inline int32_t kKnockBackMaxDuration = 60; // frames before it must end
	static inline float kKnockbackGroundFriction = 0.88f;
	static inline float kKnockbackStopSpeed = 0.02f;
	static inline float kHitKnockbackSpeedScale = 0.1f;
	static inline float kStunFriction = 0.8f;

	// Body swing (same numbers the side-view player used; rotation.y = twist, x = pitch, z = lean)
	static inline BodyComboPoseSet kBodyCombo = {
	    {MakeBodyPose(-0.1f, -0.7f, 0.0f), MakeBodyPose(-0.1f, 1.0f, 0.0f), MakeBodyPose(-0.4f, -0.7f, 0.0f)},
	    {MakeBodyPose(0.2f, 0.2f, 0.0f), MakeBodyPose(0.3f, 0.0f, 0.0f, {1.1f, 1.0f, 1.0f}), MakeBodyPose(0.4f, -0.1f, 0.0f)},
	    {MakeBodyPose(0.3f, 1.0f, 0.0f), MakeBodyPose(0.4f, 0.0f, 0.0f), MakeBodyPose(0.5f, 0.0f, 0.0f)},
	};
	static inline BodyLungePoseSet kBodyLungeHeavy = {
	    MakeBodyPose(-0.3f, -0.5f, 0.0f, {0.97f, 1.03f, 1.0f}),
	    MakeBodyPose(0.55f, 0.35f, 0.0f, {1.05f, 0.95f, 1.0f}, {0.0f, -0.05f, 0.0f}),
	};
};