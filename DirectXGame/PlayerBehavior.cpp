#define NOMINMAX
#include "PlayerBehavior.h"

#include "GlobalVariables.h"
#include "Player.h"
#include "WeaponHeavySabre.h"

#include <algorithm>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr const char* kGroupName = "PlayerBehavior";
}

void PlayerBehavior::RegisterGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->CreateGroup(kGroupName);
	gv->AddItem(kGroupName, "KnockBackDuration", kKnockBackDuration);
	gv->AddItem(kGroupName, "KnockBackMaxDuration", kKnockBackMaxDuration);
	gv->AddItem(kGroupName, "KnockbackGroundFriction", kKnockbackGroundFriction);
	gv->AddItem(kGroupName, "KnockbackStopSpeed", kKnockbackStopSpeed);
	gv->AddItem(kGroupName, "HitKnockbackSpeedScale", kHitKnockbackSpeedScale);
	gv->AddItem(kGroupName, "StunFriction", kStunFriction);
	RegisterBodyComboPoseSet(kGroupName, "BodyCombo", kBodyCombo);
	RegisterBodyLungePoseSet(kGroupName, "BodyLungeHeavy", kBodyLungeHeavy);
}

void PlayerBehavior::ApplyGlobalVariables() {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	kKnockBackDuration = gv->GetIntValue(kGroupName, "KnockBackDuration");
	kKnockBackMaxDuration = gv->GetIntValue(kGroupName, "KnockBackMaxDuration");
	kKnockbackGroundFriction = gv->GetFloatValue(kGroupName, "KnockbackGroundFriction");
	kKnockbackStopSpeed = gv->GetFloatValue(kGroupName, "KnockbackStopSpeed");
	kHitKnockbackSpeedScale = gv->GetFloatValue(kGroupName, "HitKnockbackSpeedScale");
	kStunFriction = gv->GetFloatValue(kGroupName, "StunFriction");
	ApplyBodyComboPoseSet(kGroupName, "BodyCombo", kBodyCombo);
	ApplyBodyLungePoseSet(kGroupName, "BodyLungeHeavy", kBodyLungeHeavy);
}

#pragma region Requests
void PlayerBehavior::RequestAttack() {
	if (owner_.IsControlLocked()) {
		return;
	}
	if (IsIn(kKnockback) || IsIn(kStunned) || IsRequested(kKnockback) || IsRequested(kStunned)) {
		return;
	}
	if (!isAttacking_) {
		comboIndex_ = 0;
		comboInputBuffered_ = false;
		isAttacking_ = true;
		attackKind_ = AttackKind::kCombo;
		Request(kAttack);
	} else if (attackPhase_ != AttackPhase::kAnticipate) {
		comboInputBuffered_ = true; // chain into the next step when recovery ends
	}
}

void PlayerBehavior::RequestLunge() {
	if (owner_.IsControlLocked() || isAttacking_) {
		return;
	}
	if (IsIn(kKnockback) || IsIn(kStunned) || IsRequested(kKnockback) || IsRequested(kStunned)) {
		return;
	}
	comboIndex_ = 0;
	comboInputBuffered_ = false;
	isAttacking_ = true;
	attackKind_ = AttackKind::kLunge;
	Request(kAttack);
}

void PlayerBehavior::RequestStun(float duration) {
	stunDuration_ = duration;
	isAttacking_ = false;
	Request(kStunned);
}

void PlayerBehavior::RequestHitKnockback(const Vector3& direction, float power) {
	const float scale = power * kHitKnockbackSpeedScale;
	owner_.velocity_ = {direction.x * scale, 0.0f, direction.z * scale};
	isAttacking_ = false;
	Request(kKnockback);
}

void PlayerBehavior::ClearStun() {
	if (IsIn(kStunned)) {
		Request(kRoot);
	} else if (IsRequested(kStunned)) {
		ClearRequest();
	}
}

void PlayerBehavior::ClearKnockback() {
	if (IsIn(kKnockback)) {
		Request(kRoot);
	} else if (IsRequested(kKnockback)) {
		ClearRequest();
	}
}
#pragma endregion

#pragma region State hooks
void PlayerBehavior::ResetAttack() {
	isAttacking_ = false;
	attackKind_ = AttackKind::kCombo;
	attackPhase_ = AttackPhase::kAnticipate;
	comboIndex_ = 0;
	comboInputBuffered_ = false;
	attackParameter_ = 0;
}

void PlayerBehavior::OnEnter(StateId state, StateId /*previous*/) {
	BaseMelee* melee = owner_.GetMelee();

	if (state != kAttack) {
		if (melee) {
			melee->SetBehavior(BaseMelee::MeleeBehavior::kIdle);
		}
		ResetAttack();
	}

	switch (state) {
	case kRoot:
	default:
		break;

	case kAttack:
		attackParameter_ = 0;
		attackPhase_ = AttackPhase::kAnticipate;
		if (attackKind_ == AttackKind::kLunge) {
			owner_.ResetLungeHits();
			owner_.BeginLungeCooldown(); // cooldown starts the moment the lunge is launched
			if (melee) {
				melee->SetBehavior(BaseMelee::MeleeBehavior::kAttacking);
			}
		} else if (melee) {
			melee->SetBehavior(BaseMelee::MeleeBehavior::kAttacking);
			melee->ResetSwingHits();
			melee->PlaySwingSound(comboIndex_);
		}
		break;

	case kKnockback:
		knockbackFrames_ = 0;
		owner_.ResetMoveMode();
		break;

	case kStunned:
		stunTimer_ = 0.0f;
		owner_.velocity_ = {};
		owner_.ResetMoveMode();
		break;
	}
}

void PlayerBehavior::OnUpdate(StateId state) {
	switch (state) {
	case kRoot:
	default:
		if (!owner_.IsControlLocked()) {
			owner_.Move();
		} else {
			owner_.velocity_.x *= (1.0f - Player::kAttenuation);
			owner_.velocity_.z *= (1.0f - Player::kAttenuation);
		}
		break;
	case kAttack:
		UpdateAttack();
		break;
	case kKnockback:
		UpdateKnockback();
		break;
	case kStunned:
		UpdateStunned();
		break;
	}
}
#pragma endregion

#pragma region Attack
float PlayerBehavior::PhaseDuration(AttackPhase phase) const {
	if (attackKind_ == AttackKind::kLunge) {
		const LungeSpec spec = owner_.GetLungeSpec();
		return (phase == AttackPhase::kAnticipate) ? spec.anticipateFrames : (phase == AttackPhase::kAttack) ? spec.attackFrames : spec.recoveryFrames;
	}
	const BaseMelee* melee = owner_.GetMelee();
	switch (phase) {
	case AttackPhase::kAnticipate:
		return melee ? melee->GetComboAnticipateDuration(comboIndex_, Owner::kPlayer) : 10.0f;
	case AttackPhase::kAttack:
		return melee ? melee->GetComboAttackDuration(comboIndex_, Owner::kPlayer) : 8.0f;
	case AttackPhase::kRecovery:
	default:
		return melee ? melee->GetComboRecoveryDuration(comboIndex_, Owner::kPlayer) : 15.0f;
	}
}

void PlayerBehavior::UpdateAttack() {
	++attackParameter_;

	BaseMelee* melee = owner_.GetMelee();
	const bool lunge = (attackKind_ == AttackKind::kLunge);
	const Vector3 forward = owner_.GetForward();
	float speed = 0.0f; // units per frame along the facing direction

	switch (attackPhase_) {
	case AttackPhase::kAnticipate:
	default:
		if (static_cast<float>(attackParameter_) >= PhaseDuration(AttackPhase::kAnticipate)) {
			attackPhase_ = AttackPhase::kAttack;
			attackParameter_ = 0;
			if (lunge) {
				WeaponHeavySabre::PlayLungeSound();
			}
		}
		break;

	case AttackPhase::kAttack:
		if (lunge) {
			// stop pushing forward once the lunge has connected
			speed = owner_.HasLungeHits() ? 0.0f : owner_.GetLungeSpec().speed;
		} else {
			speed = melee ? melee->GetComboAttackSpeed(comboIndex_, Owner::kPlayer) : 0.25f;
		}
		if (static_cast<float>(attackParameter_) >= PhaseDuration(AttackPhase::kAttack)) {
			attackPhase_ = AttackPhase::kRecovery;
			attackParameter_ = 0;
		}
		break;

	case AttackPhase::kRecovery:
		if (static_cast<float>(attackParameter_) >= PhaseDuration(AttackPhase::kRecovery)) {
			const int maxCombo = melee ? melee->GetMaxCombo() : 1;
			if (!lunge && comboInputBuffered_ && comboIndex_ + 1 < maxCombo) {
				++comboIndex_;
				comboInputBuffered_ = false;
				attackPhase_ = AttackPhase::kAnticipate;
				attackParameter_ = 0;
				if (melee) {
					melee->ResetSwingHits();
					melee->PlaySwingSound(comboIndex_);
				}
			} else {
				Request(kRoot); // OnEnter(kRoot) clears the attack state
			}
		}
		break;
	}

	owner_.velocity_ = {forward.x * speed, 0.0f, forward.z * speed};
}

PlayerBehavior::AttackAnimState PlayerBehavior::GetAttackAnimState() const {
	const float duration = PhaseDuration(attackPhase_);
	const float t = (duration > 0.0f) ? static_cast<float>(attackParameter_) / duration : 0.0f;
	return {attackPhase_, std::clamp(t, 0.0f, 1.0f), comboIndex_};
}

SRTOffset PlayerBehavior::GetAttackPoseOffset() const {
	const AttackAnimState s = GetAttackAnimState();
	const BodyAttackPhase phase = static_cast<BodyAttackPhase>(static_cast<int>(s.phase));
	if (attackKind_ == AttackKind::kLunge) {
		return EvaluateBodyLungePose(kBodyLungeHeavy, phase, s.t);
	}
	return EvaluateBodyComboPose(kBodyCombo, phase, s.t, s.comboIndex);
}
#pragma endregion

#pragma region Knockback / Stun
void PlayerBehavior::UpdateKnockback() {
	++knockbackFrames_;
	owner_.velocity_.x *= kKnockbackGroundFriction;
	owner_.velocity_.z *= kKnockbackGroundFriction;

	const float speed = std::sqrt(owner_.velocity_.x * owner_.velocity_.x + owner_.velocity_.z * owner_.velocity_.z);
	const bool settled = speed < kKnockbackStopSpeed || static_cast<int32_t>(knockbackFrames_) >= kKnockBackMaxDuration;
	if (static_cast<int32_t>(knockbackFrames_) >= kKnockBackDuration && settled) {
		Request(kRoot);
	}
}

void PlayerBehavior::UpdateStunned() {
	stunTimer_ += 1.0f / 60.0f;
	owner_.velocity_.x *= kStunFriction;
	owner_.velocity_.z *= kStunFriction;
	if (stunTimer_ >= stunDuration_) {
		Request(kRoot);
	}
}
#pragma endregion