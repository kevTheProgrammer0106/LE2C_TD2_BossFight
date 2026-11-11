#pragma once

#define NOMINMAX

#include "Global.h"

/// <summary>
/// BaseEnemyの見た目・アニメーション（体パーツの追従、移動モーションのブレンド）を担当するクラス。
/// BaseEnemy本体の物理/衝突/振る舞いロジックからは独立しており、
/// 毎フレームBaseEnemy側から現在の物理状態(AnimationContext)を渡してUpdate/ApplyToPartsを呼ぶ想定。
/// </summary>
class AnimationManager {
public:
	static void RegisterGlobalVariables();
	static void ApplyGlobalVariables();
	enum class MovementState { kIdle, kRun, kJump, kFall, kLanding, kKnockback, kStunned, kBlock };

	// 毎フレームBaseEnemyから渡される、アニメーション判定に必要な最小限の物理状態
	struct AnimationContext {
		KamataEngine::Vector3 velocity = {};
		bool onGround = true;
		bool isDead = false;
		bool isKnockedBack = false; 
		bool isStunned = false;    
		bool isBlocking = false;    
		bool facingRight = true;    // used to mirror knockback/stun/block poses
		bool isAttacking = false;   // melee in progress: movement pose eases to neutral so the attack pose reads cleanly

		// 移動アニメーションとは別の、攻撃・照準・リロード等による体のポーズ
		SRTOffset actionPose = {};
	};

	struct MovementEvents {
		bool justJumped = false;   // ground -> kJump this frame (a real jump, not walking off a ledge)
		bool justLanded = false;   // kFall -> kLanding this frame
		bool footstepTick = false; // periodic beat while actively running on the ground
	};

	virtual MovementEvents UpdateMovementState(const AnimationContext& context);

	virtual SRTOffset GetMovementAnimOffset() const { return currentPose_; }

	// [NEW] frame-rate-independent exponential smoothing toward a target pose. Public so Player (or anything else) can reuse it.
	static SRTOffset SmoothSRT(const SRTOffset& current, const SRTOffset& target);

	MovementState GetCurrentMovementState() const { return movementState_; }

	virtual void UpdateBodyParts(const KamataEngine::WorldTransform& worldTransform, const AnimationContext& context, std::array<BodyPart, static_cast<size_t>(PartSlot::kCount)>& parts);

	virtual ~AnimationManager() = default;

protected:
	friend class DebugMenu;
	static constexpr size_t kPartCount = static_cast<size_t>(PartSlot::kCount);
	static constexpr size_t body = static_cast<size_t>(PartSlot::kBody);
	static constexpr size_t head = static_cast<size_t>(PartSlot::kHead);
	static constexpr size_t hat = static_cast<size_t>(PartSlot::kHat);

	float footstepTimer_ = 0.0f;
	static inline float kFootstepInterval = 0.32f;

	SRTOffset headOffset_ = {
	    {1.0f,    1.0f,   1.0f},
        {0.0f,    0.0f,   0.0f},
        {-0.050f, 0.720f, 0.0f}
    };
	SRTOffset hatOffset_ = {
	    {1.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };

	MovementState movementState_ = MovementState::kIdle;
	float movementStateTimer_ = 0.0f;
	float deadTimer_ = 0.0f;  // seconds spent in the death behavior
	bool facingRight_ = true; // last facing passed in via AnimationContext

	// Smoothed poses. Targets jump between states; these chase them so transitions blend instead of snapping.
	SRTOffset currentPose_ = {};
	SRTOffset currentActionPose_ = {};
	// the un-smoothed pose for the current movement state (old GetMovementAnimOffset body)
	SRTOffset ComputeTargetPose() const;
	// higher = snappier. ~14 => ~0.07s time constant
	static inline float kPoseTransitionSpeed = 14.0f;

	// --- Idle ---
	static inline float kIdleBreatheAmplitude = 0.03f;
	static inline float kIdleBreatheSpeed = 2.0f;

	// --- Run ---
	static inline float kRunBounceAmplitude = 0.08f;
	static inline float kRunBounceSpeed = 10.0f;
	static inline float kRunTiltAmplitude = 0.05f;

	// --- Jump / Fall ---
	static inline float kJumpStretchY = 1.1f;
	static inline float kJumpSquashX = 0.95f;
	static inline float kFallSquashY = 0.95f;
	static inline float kFallStretchX = 1.05f;

	// --- Landing ---
	static inline float kLandingDuration = 0.15f;
	static inline float kLandingSquashY = 0.7f;
	static inline float kLandingSquashX = 1.2f;

	static inline float kRunSpeedThreshold = 0.01f;
	static inline float kIdleCycleSpeed = 2.0f;
	static inline float kRunCycleSpeed = 10.0f;
	static inline float kAirPoseBlendSpeed = 8.0f;

	// --- [NEW] Knockback / Stun ---
	static inline float kKnockbackPoseBlendSpeed = 12.0f; // pose reaches 100% after 1/speed seconds
	static inline float kStunPoseBlendSpeed = 10.0f;
	static inline float kStunWobbleAmplitude = 0.08f; // radians of Z wobble while stunned
	static inline float kStunWobbleSpeed = 18.0f;
	static inline float kBlockPoseBlendSpeed = 12.0f;

	// --- [NEW] Dead body ---
	static inline float kDeadBodyDropDistance = 0.3f;   // how far the body sinks while dead
	static inline float kDeadBodyDropBlendTime = 0.25f; // seconds to reach full drop

	static inline SRTOffset kIdlePoseLow = {
	    {1.0f, 1.0f, 1.0f},
        {0,    0,    0   },
        {0.0f, 0.0f, 0.0f}
    };
	static inline SRTOffset kIdlePoseHigh = {
	    {1.0f, 1.0f,  1.0f},
        {0,    0,     0   },
        {0.0f, 0.03f, 0.0f}
    };
	static inline SRTOffset kRunPoseLow = {
	    {1.0f, 1.0f, 1.0f  },
        {0,    0,    -0.05f},
        {0.0f, 0.0f, 0.0f  }
    };
	static inline SRTOffset kRunPoseHigh = {
	    {1.0f, 1.0f,  1.0f },
        {0,    0,     0.05f},
        {0.0f, 0.08f, 0.0f }
    };
	static inline SRTOffset kJumpPose = {
	    {0.95f, 1.1f, 1.0f},
        {0,     0,    0   },
        {0.0f,  0.0f, 0.0f}
    };
	static inline SRTOffset kFallPose = {
	    {1.05f, 0.95f, 1.0f},
        {0,     0,     0   },
        {0.0f,  0.0f,  0.0f}
    };
	// [NEW] hit-reel: slightly squashed/stretched, leaning back
	static inline SRTOffset kKnockbackPose = {
	    {0.9f, 1.1f,  1.0f  },
        {0,    0,     -0.25f},
        {0.0f, 0.05f, 0.0f  }
    };
	// [NEW] dazed slump: shorter body, pitched forward (Z wobble is added on top)
	static inline SRTOffset kStunPose = {
	    {1.05f, 0.9f,  1.0f},
        {0.2f,  0,     0   },
        {0.0f,  -0.1f, 0.0f}
    };
	// [NEW] guard stance: braced / slightly crouched, leaning back (authored for a RIGHT-facing entity)
	static inline SRTOffset kBlockPose = {
	    {1.0f, 0.95f,  1.0f },
        {0.1f, 0,      -0.1f},
        {0.0f, -0.05f, 0.0f }
    };
	static inline SRTOffset kNeutralPose = {};

	// [NEW] Poses are authored facing RIGHT. For a left-facing entity, mirror the world-space parts
	// (rotation.z roll and translation.x). rotation.x pitch is applied in model space before the yaw, so it mirrors by itself.
	static SRTOffset MirrorForFacing(const SRTOffset& pose, bool facingRight);
};