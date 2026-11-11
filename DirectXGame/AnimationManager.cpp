#include "AnimationManager.h"
#include "GlobalVariables.h"

#include <cmath>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "AnimationManager";
}

void AnimationManager::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "IdleBreatheAmplitude", kIdleBreatheAmplitude);
	gv->AddItem(kGroupName, "IdleBreatheSpeed", kIdleBreatheSpeed);
	gv->AddItem(kGroupName, "RunBounceAmplitude", kRunBounceAmplitude);
	gv->AddItem(kGroupName, "RunBounceSpeed", kRunBounceSpeed);
	gv->AddItem(kGroupName, "RunTiltAmplitude", kRunTiltAmplitude);
	gv->AddItem(kGroupName, "JumpStretchY", kJumpStretchY);
	gv->AddItem(kGroupName, "JumpSquashX", kJumpSquashX);
	gv->AddItem(kGroupName, "FallSquashY", kFallSquashY);
	gv->AddItem(kGroupName, "FallStretchX", kFallStretchX);
	gv->AddItem(kGroupName, "LandingDuration", kLandingDuration);
	gv->AddItem(kGroupName, "LandingSquashY", kLandingSquashY);
	gv->AddItem(kGroupName, "LandingSquashX", kLandingSquashX);
	gv->AddItem(kGroupName, "RunSpeedThreshold", kRunSpeedThreshold);
	gv->AddItem(kGroupName, "IdleCycleSpeed", kIdleCycleSpeed);
	gv->AddItem(kGroupName, "RunCycleSpeed", kRunCycleSpeed);
	gv->AddItem(kGroupName, "AirPoseBlendSpeed", kAirPoseBlendSpeed);
	// [NEW]
	gv->AddItem(kGroupName, "PoseTransitionSpeed", kPoseTransitionSpeed);
	gv->AddItem(kGroupName, "KnockbackPoseBlendSpeed", kKnockbackPoseBlendSpeed);
	gv->AddItem(kGroupName, "StunPoseBlendSpeed", kStunPoseBlendSpeed);
	gv->AddItem(kGroupName, "BlockPoseBlendSpeed", kBlockPoseBlendSpeed);
	gv->AddItem(kGroupName, "StunWobbleAmplitude", kStunWobbleAmplitude);
	gv->AddItem(kGroupName, "StunWobbleSpeed", kStunWobbleSpeed);
	gv->AddItem(kGroupName, "DeadBodyDropDistance", kDeadBodyDropDistance);
	gv->AddItem(kGroupName, "DeadBodyDropBlendTime", kDeadBodyDropBlendTime);
}
void AnimationManager::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kIdleBreatheAmplitude = gv->GetFloatValue(kGroupName, "IdleBreatheAmplitude");
	kIdleBreatheSpeed = gv->GetFloatValue(kGroupName, "IdleBreatheSpeed");
	kRunBounceAmplitude = gv->GetFloatValue(kGroupName, "RunBounceAmplitude");
	kRunBounceSpeed = gv->GetFloatValue(kGroupName, "RunBounceSpeed");
	kRunTiltAmplitude = gv->GetFloatValue(kGroupName, "RunTiltAmplitude");
	kJumpStretchY = gv->GetFloatValue(kGroupName, "JumpStretchY");
	kJumpSquashX = gv->GetFloatValue(kGroupName, "JumpSquashX");
	kFallSquashY = gv->GetFloatValue(kGroupName, "FallSquashY");
	kFallStretchX = gv->GetFloatValue(kGroupName, "FallStretchX");
	kLandingDuration = gv->GetFloatValue(kGroupName, "LandingDuration");
	kLandingSquashY = gv->GetFloatValue(kGroupName, "LandingSquashY");
	kLandingSquashX = gv->GetFloatValue(kGroupName, "LandingSquashX");
	kRunSpeedThreshold = gv->GetFloatValue(kGroupName, "RunSpeedThreshold");
	kIdleCycleSpeed = gv->GetFloatValue(kGroupName, "IdleCycleSpeed");
	kRunCycleSpeed = gv->GetFloatValue(kGroupName, "RunCycleSpeed");
	kAirPoseBlendSpeed = gv->GetFloatValue(kGroupName, "AirPoseBlendSpeed");
	// [NEW]
	kPoseTransitionSpeed = gv->GetFloatValue(kGroupName, "PoseTransitionSpeed");
	kKnockbackPoseBlendSpeed = gv->GetFloatValue(kGroupName, "KnockbackPoseBlendSpeed");
	kStunPoseBlendSpeed = gv->GetFloatValue(kGroupName, "StunPoseBlendSpeed");
	kBlockPoseBlendSpeed = gv->GetFloatValue(kGroupName, "BlockPoseBlendSpeed");
	kStunWobbleAmplitude = gv->GetFloatValue(kGroupName, "StunWobbleAmplitude");
	kStunWobbleSpeed = gv->GetFloatValue(kGroupName, "StunWobbleSpeed");
	kDeadBodyDropDistance = gv->GetFloatValue(kGroupName, "DeadBodyDropDistance");
	kDeadBodyDropBlendTime = gv->GetFloatValue(kGroupName, "DeadBodyDropBlendTime");
}

SRTOffset AnimationManager::SmoothSRT(const SRTOffset& current, const SRTOffset& target) {
	// frame-rate independent: 1 - e^(-speed * dt)
	float factor = 1.0f - std::exp(-std::max(kPoseTransitionSpeed, 0.0f) * (1.0f / 60.0f));
	return LerpSRT(current, target, factor);
}

SRTOffset AnimationManager::MirrorForFacing(const SRTOffset& pose, bool facingRight) {
	if (facingRight)
		return pose;
	SRTOffset mirrored = pose;
	mirrored.rotation.z = -mirrored.rotation.z;
	mirrored.translation.x = -mirrored.translation.x;
	return mirrored;
}

AnimationManager::MovementEvents AnimationManager::UpdateMovementState(const AnimationContext& context) {
	MovementEvents events;
	MovementState previous = movementState_;

	if (context.isDead) {
		deadTimer_ += 1.0f / 60.0f;
	} else {
		deadTimer_ = 0.0f;
	}

	facingRight_ = context.facingRight;

	if (context.isKnockedBack) {
		movementState_ = MovementState::kKnockback;
	} else if (context.isStunned) {
		movementState_ = MovementState::kStunned;
	} else if (context.isBlocking) {
		movementState_ = MovementState::kBlock;
	} else if (!context.onGround) {
		movementState_ = (context.velocity.y > 0.0f) ? MovementState::kJump : MovementState::kFall;
	} else if (movementState_ == MovementState::kFall && context.onGround) {
		movementState_ = MovementState::kLanding;
		movementStateTimer_ = 0.0f;
	} else if (movementState_ == MovementState::kLanding) {
		movementStateTimer_ += 1.0f / 60.0f;
		if (movementStateTimer_ >= kLandingDuration) {
			movementState_ = (std::abs(context.velocity.x) > kRunSpeedThreshold) ? MovementState::kRun : MovementState::kIdle;
		}
	} else {
		movementState_ = (std::abs(context.velocity.x) > kRunSpeedThreshold) ? MovementState::kRun : MovementState::kIdle;
	}

	if (movementState_ != previous) {
		movementStateTimer_ = 0.0f;
		footstepTimer_ = 0.0f;
	} else {
		movementStateTimer_ += 1.0f / 60.0f;
	}

	// leaving knockback/stun in mid-air lands in kJump; that is not a real jump, so no jump SFX
	bool wasDebuffed = (previous == MovementState::kKnockback || previous == MovementState::kStunned || previous == MovementState::kBlock);
	events.justJumped = !wasDebuffed && (previous != MovementState::kJump && movementState_ == MovementState::kJump);
	events.justLanded = (previous != MovementState::kLanding && movementState_ == MovementState::kLanding);

	if (movementState_ == MovementState::kRun) {
		footstepTimer_ += 1.0f / 60.0f;
		if (footstepTimer_ >= kFootstepInterval) {
			footstepTimer_ -= kFootstepInterval;
			events.footstepTick = true;
		}
	} else {
		footstepTimer_ = 0.0f;
	}

	SRTOffset target = (context.isDead || context.isAttacking) ? kNeutralPose : ComputeTargetPose();
	currentPose_ = SmoothSRT(currentPose_, target);

	return events;
}

SRTOffset AnimationManager::ComputeTargetPose() const {
	switch (movementState_) {
	case MovementState::kIdle: {
		float t = TriangleWave(movementStateTimer_, kIdleCycleSpeed);
		return LerpSRT(kIdlePoseLow, kIdlePoseHigh, t);
	}
	case MovementState::kRun: {
		float t = TriangleWave(movementStateTimer_, kRunCycleSpeed);
		return LerpSRT(kRunPoseLow, kRunPoseHigh, t);
	}
	case MovementState::kJump: {
		float t = std::clamp(movementStateTimer_ * kAirPoseBlendSpeed, 0.0f, 1.0f);
		return LerpSRT(kNeutralPose, kJumpPose, t);
	}
	case MovementState::kFall: {
		float t = std::clamp(movementStateTimer_ * kAirPoseBlendSpeed, 0.0f, 1.0f);
		return LerpSRT(kNeutralPose, kFallPose, t);
	}
	case MovementState::kLanding: {
		float t = std::clamp(movementStateTimer_ / kLandingDuration, 0.0f, 1.0f);
		SRTOffset landPose = {
		    {kLandingSquashX, kLandingSquashY, 1.0f},
            {0,               0,               0   },
            {0,               0,               0   }
        };
		return LerpSRT(landPose, kNeutralPose, t);
	}
	case MovementState::kKnockback: {
		float t = std::clamp(movementStateTimer_ * kKnockbackPoseBlendSpeed, 0.0f, 1.0f);
		return MirrorForFacing(LerpSRT(kNeutralPose, kKnockbackPose, t), facingRight_);
	}
	case MovementState::kStunned: {
		float t = std::clamp(movementStateTimer_ * kStunPoseBlendSpeed, 0.0f, 1.0f);
		SRTOffset pose = LerpSRT(kNeutralPose, kStunPose, t);
		pose.rotation.z += std::sin(movementStateTimer_ * kStunWobbleSpeed) * kStunWobbleAmplitude * t;
		return MirrorForFacing(pose, facingRight_);
	}
	case MovementState::kBlock: {
		float t = std::clamp(movementStateTimer_ * kBlockPoseBlendSpeed, 0.0f, 1.0f);
		return MirrorForFacing(LerpSRT(kNeutralPose, kBlockPose, t), facingRight_);
	}
	default:
		return kNeutralPose;
	}
}

void AnimationManager::UpdateBodyParts(const WorldTransform& worldTransform, const AnimationContext& context, std::array<BodyPart, kPartCount>& parts) {

	// [CHANGED] movement pose is already smoothed in UpdateMovementState (and eases to neutral when dead).
	SRTOffset anim = GetMovementAnimOffset();

	// [NEW] action pose (aim / fire / reload etc.) is smoothed too
	SRTOffset actionTarget = context.isDead ? SRTOffset{} : context.actionPose;
	currentActionPose_ = SmoothSRT(currentActionPose_, actionTarget);

	anim.translation += currentActionPose_.translation;
	anim.rotation += currentActionPose_.rotation;
	anim.scale = {anim.scale.x * currentActionPose_.scale.x, anim.scale.y * currentActionPose_.scale.y, anim.scale.z * currentActionPose_.scale.z};

	// [NEW] dead body sinks toward the floor
	if (context.isDead) {
		float t = std::clamp(deadTimer_ / std::max(kDeadBodyDropBlendTime, 0.0001f), 0.0f, 1.0f);
		anim.translation.y -= kDeadBodyDropDistance * EaseOut(0.0f, 1.0f, t);
	}

	parts[body].transform.translation_ = worldTransform.translation_ + anim.translation;

	parts[body].transform.rotation_ = worldTransform.rotation_ + anim.rotation;

	parts[body].transform.scale_ = {
	    worldTransform.scale_.x * anim.scale.x,
	    worldTransform.scale_.y * anim.scale.y,
	    worldTransform.scale_.z * anim.scale.z,
	};

	WorldTransformUpdate(parts[body].transform);

	// head は最終的な body transform を親として計算する
	Matrix4x4 rotMatrix =
	    Multiply(MakeRotateXMatrix(parts[body].transform.rotation_.x), Multiply(MakeRotateYMatrix(parts[body].transform.rotation_.y), MakeRotateZMatrix(parts[body].transform.rotation_.z)));

	Vector3 rotatedHeadOffset = TransformNormal(headOffset_.translation, rotMatrix);

	parts[head].transform.translation_ = parts[body].transform.translation_ + rotatedHeadOffset;

	parts[head].transform.rotation_ = parts[body].transform.rotation_ + headOffset_.rotation;

	parts[head].transform.scale_ = {
	    parts[body].transform.scale_.x * headOffset_.scale.x,
	    parts[body].transform.scale_.y * headOffset_.scale.y,
	    parts[body].transform.scale_.z * headOffset_.scale.z,
	};

	WorldTransformUpdate(parts[head].transform);

	// hat は最終的な head transform を親として計算する
	Matrix4x4 headRotMatrix =
	    Multiply(MakeRotateXMatrix(parts[head].transform.rotation_.x), Multiply(MakeRotateYMatrix(parts[head].transform.rotation_.y), MakeRotateZMatrix(parts[head].transform.rotation_.z)));

	Vector3 rotatedHatOffset = TransformNormal(hatOffset_.translation, headRotMatrix);

	parts[hat].transform.translation_ = parts[head].transform.translation_ + rotatedHatOffset;

	parts[hat].transform.rotation_ = parts[head].transform.rotation_ + hatOffset_.rotation;

	parts[hat].transform.scale_ = {
	    parts[head].transform.scale_.x * hatOffset_.scale.x,
	    parts[head].transform.scale_.y * hatOffset_.scale.y,
	    parts[head].transform.scale_.z * hatOffset_.scale.z,
	};

	WorldTransformUpdate(parts[hat].transform);
}