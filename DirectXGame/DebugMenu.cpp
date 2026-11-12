#define NOMINMAX

#include "DebugMenu.h"

#include "GameScene.h"
#include "GlobalVariables.h"
#include "MapChipField.h"
#include "PlayerBehavior.h"
#include "PoseGV.h"
#include "StageManager.h"
#include "WeaponHeavySabre.h"

#include <cstdio>
#include <initializer_list>
#include <string>
#include <unordered_map>

using namespace KamataEngine;

#ifdef _DEBUG

namespace {

// ============================================================
// Plain static editing (Player statics that are not in GlobalVariables: lost on restart)
// ============================================================

// First-seen value of every edited SRTOffset, so "Reset" works.
std::unordered_map<const void*, SRTOffset> sOriginals;

bool DragF(const char* label, float& v, float speed) { return ImGui::DragFloat(label, &v, speed); }

bool EditSRT(const char* label, SRTOffset& v) {
	bool changed = false;
	auto it = sOriginals.try_emplace(&v, v).first;

	ImGui::PushID(&v);
	if (ImGui::TreeNode(label)) {
		changed |= ImGui::DragFloat3("Scale", &v.scale.x, 0.01f);
		changed |= ImGui::DragFloat3("Rotation (rad)", &v.rotation.x, 0.01f);
		changed |= ImGui::DragFloat3("Translation", &v.translation.x, 0.005f);
		if (ImGui::SmallButton("Reset")) {
			v = it->second;
			changed = true;
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
	return changed;
}

// ============================================================
// GlobalVariables direct editing helpers
// The value lives in the GV, so what is changed here reaches the owning class through the next ApplyGlobalVariables().
// ============================================================
struct GvOriginal {
	Vector3 s{};
	Vector3 r{};
	Vector3 t{};
};
std::unordered_map<const void*, GvOriginal> sGvOriginals;

Vector3* GvVec(const char* group, const char* key) {
	if (!key || !*key)
		return nullptr;
	return GlobalVariables::GetInstance()->GetVector3Ptr(group, key);
}

// One pose (Scale / Rotation / Translation). A null / unregistered key row is not shown. Collapsible, with Reset.
bool GvPoseKeys(const char* label, const char* group, const char* scaleKey, const char* rotKey, const char* transKey) {
	Vector3* s = GvVec(group, scaleKey);
	Vector3* r = GvVec(group, rotKey);
	Vector3* t = GvVec(group, transKey);
	if (!s && !r && !t) {
		ImGui::TextDisabled("%s : (not registered)", label);
		return false;
	}
	const void* id = t ? static_cast<const void*>(t) : (r ? static_cast<const void*>(r) : static_cast<const void*>(s));

	auto it = sGvOriginals.find(id);
	if (it == sGvOriginals.end()) {
		GvOriginal o;
		if (s)
			o.s = *s;
		if (r)
			o.r = *r;
		if (t)
			o.t = *t;
		it = sGvOriginals.emplace(id, o).first;
	}

	bool changed = false;
	ImGui::PushID(id);
	if (ImGui::TreeNode(label)) {
		if (s)
			changed |= ImGui::DragFloat3("Scale", &s->x, 0.01f);
		if (r)
			changed |= ImGui::DragFloat3("Rotation (rad)", &r->x, 0.01f);
		if (t)
			changed |= ImGui::DragFloat3("Translation", &t->x, 0.005f);
		if (ImGui::SmallButton("Reset")) {
			if (s)
				*s = it->second.s;
			if (r)
				*r = it->second.r;
			if (t)
				*t = it->second.t;
			changed = true;
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
	return changed;
}

// <base>Scale / <base>Rotation / <base>Translation keys
bool GvPose(const char* label, const char* group, const std::string& base) {
	const std::string s = base + "Scale";
	const std::string r = base + "Rotation";
	const std::string t = base + "Translation";
	return GvPoseKeys(label, group, s.c_str(), r.c_str(), t.c_str());
}

bool GvFloat(const char* label, const char* group, const char* key, float speed = 0.01f) {
	float* p = GlobalVariables::GetInstance()->GetFloatPtr(group, key);
	if (!p) {
		ImGui::TextDisabled("%s : (not registered)", label);
		return false;
	}
	ImGui::PushID(group);
	ImGui::PushID(key);
	const bool changed = ImGui::DragFloat(label, p, speed);
	ImGui::PopID();
	ImGui::PopID();
	return changed;
}

bool GvInt(const char* label, const char* group, const char* key) {
	int32_t* p = GlobalVariables::GetInstance()->GetIntPtr(group, key);
	if (!p) {
		ImGui::TextDisabled("%s : (not registered)", label);
		return false;
	}
	ImGui::PushID(group);
	ImGui::PushID(key);
	const bool changed = ImGui::DragInt(label, p, 0.2f);
	ImGui::PopID();
	ImGui::PopID();
	return changed;
}

bool GvVec3(const char* label, const char* group, const char* key, float speed = 0.01f) {
	Vector3* p = GvVec(group, key);
	if (!p) {
		ImGui::TextDisabled("%s : (not registered)", label);
		return false;
	}
	ImGui::PushID(group);
	ImGui::PushID(key);
	const bool changed = ImGui::DragFloat3(label, &p->x, speed);
	ImGui::PopID();
	ImGui::PopID();
	return changed;
}

// Saves every group the tab shows to its json
void SaveGroups(std::initializer_list<const char*> groups) {
	if (ImGui::Button("Save this tab (json)")) {
		for (const char* g : groups)
			GlobalVariables::GetInstance()->SaveFile(g);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Edits apply live. Save writes every group used in this tab.");
	ImGui::Separator();
}

// Melee combo (3 steps x 4 keyframes). Keys: Combo<i><AntStart|AntEnd|AtkEnd|RecEnd>
void GvWeaponCombos(const char* group) {
	static const char* kLabels[4] = {"1 Anticipate start", "2 Anticipate end (wind-up)", "3 Attack end (swing)", "4 Recovery end"};
	ImGui::TextDisabled("Tip: a combo's Recovery end is usually the next combo's Anticipate start.");
	ImGui::PushID(group);
	for (int i = 0; i < 3; ++i) {
		char name[32];
		std::snprintf(name, sizeof(name), "Combo %d", i + 1);
		if (ImGui::CollapsingHeader(name)) {
			for (int k = 0; k < 4; ++k)
				GvPose(kLabels[k], group, "Combo" + std::to_string(i) + PoseGV::kComboPhaseKeys[k]);
		}
	}
	ImGui::PopID();
}

// Body combo poses (RegisterBodyComboPoseSet): keys <prefix><i><Anticipate|Peak|Recovery>
void GvBodyCombo(const char* title, const char* group, const char* prefix) {
	static const char* kPhases[3] = {"Anticipate", "Peak", "Recovery"};
	ImGui::PushID(group);
	ImGui::PushID(prefix);
	for (int i = 0; i < 3; ++i) {
		char name[64];
		std::snprintf(name, sizeof(name), "%s combo %d", title, i + 1);
		if (ImGui::CollapsingHeader(name)) {
			for (int k = 0; k < 3; ++k)
				GvPose(kPhases[k], group, std::string(prefix) + std::to_string(i) + kPhases[k]);
		}
	}
	ImGui::PopID();
	ImGui::PopID();
}

// Body thrust / slam poses (RegisterBodyLungePoseSet): keys <prefix>WindUp / <prefix>Thrust
void GvBodyLunge(const char* title, const char* group, const char* prefix) {
	ImGui::PushID(group);
	ImGui::PushID(prefix);
	if (ImGui::CollapsingHeader(title)) {
		GvPose("Wind-up", group, std::string(prefix) + "WindUp");
		GvPose("Thrust / slam", group, std::string(prefix) + "Thrust");
	}
	ImGui::PopID();
	ImGui::PopID();
}

// The three reaction stances (knockback / stun / block)
void GvReactionPoses(const char* group) {
	GvPose("Knockback", group, "Knockback");
	GvPose("Stun", group, "Stun");
	GvPose("Block (guard)", group, "Block");
}

const char* MoveModeName(Player::MoveMode mode) {
	switch (mode) {
	case Player::MoveMode::kNormal:
		return "Normal";
	case Player::MoveMode::kDash:
		return "Dash";
	case Player::MoveMode::kRoll:
		return "Roll";
	}
	return "?";
}

const char* BehaviorName(BaseBehavior::StateId state) {
	switch (state) {
	case PlayerBehavior::kRoot:
		return "Root";
	case PlayerBehavior::kAttack:
		return "Attack";
	case PlayerBehavior::kKnockback:
		return "Knockback";
	case PlayerBehavior::kStunned:
		return "Stunned";
	default:
		return "-";
	}
}

constexpr ImVec4 kWarn = {1.0f, 0.75f, 0.2f, 1.0f};
constexpr ImVec4 kOn = {1.0f, 0.45f, 0.25f, 1.0f};
constexpr ImVec4 kOff = {0.55f, 0.55f, 0.55f, 1.0f};

} // namespace

// ------------------------------------------------------------------ Main window
void DebugMenu::Draw(GameScene* gs) {
	if (!gs)
		return;

	Input* input = Input::GetInstance();
	if (input->TriggerKey(DIK_F1))
		gs->reloadRequested_ = true;
	if (input->TriggerKey(DIK_F3))
		isOpen_ = !isOpen_;
	if (input->TriggerKey(DIK_F4))
		gs->showCollisionDebug_ = !gs->showCollisionDebug_;

	if (!isOpen_)
		return;

	if (!ImGui::Begin("Debug Menu (F3)", &isOpen_)) {
		ImGui::End();
		return;
	}

	ImGui::TextDisabled("F1 reload scene | F3 this window | F4 collision boxes");
	ImGui::TextDisabled("Alt frees the mouse (clicks no longer attack). Hold right click to orbit the camera. Alt again to lock.");
	ImGui::TextDisabled("Heavy Sabre / Camera tabs edit GlobalVariables directly (live). 'Save this tab' writes the json.");

	if (ImGui::BeginTabBar("DebugMenuTabs")) {
		if (ImGui::BeginTabItem("Scene")) {
			DrawSceneTab(gs);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Player")) {
			DrawPlayerTab(gs);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Movement")) {
			DrawMovementTab(gs);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Heavy Sabre")) {
			DrawHeavySabreTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Camera")) {
			DrawCameraTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

// ------------------------------------------------------------------ Scene
void DebugMenu::DrawSceneTab(GameScene* gs) {
	StageManager* sm = gs->stageManager_;

	if (sm && sm->GetStageCount() > 0) {
		ImGui::Text("Stage: %s (index %d)", sm->GetCurrentStageData().name.c_str(), sm->GetCurrentStageIndex());
	}
	if (gs->mapChipField_) {
		ImGui::Text(
		    "Chunks: %u x %u  (cells %u x %u)", gs->mapChipField_->GetChunkCountX(), gs->mapChipField_->GetChunkCountZ(), gs->mapChipField_->GetNumBlockHorizontal(),
		    gs->mapChipField_->GetNumBlockDepth());
	}
	ImGui::Text("Blocks: %d", static_cast<int>(gs->blocks_.size()));

	if (ImGui::Button("Reload Scene (F1)"))
		gs->reloadRequested_ = true;

	if (sm) {
		ImGui::Text("Load stage:");
		const int count = sm->GetStageCount();
		for (int i = 0; i < count; ++i) {
			if (i > 0)
				ImGui::SameLine();
			char label[96];
			std::snprintf(label, sizeof(label), "%s##stage%d", sm->GetStageData(i).name.c_str(), i);
			if (ImGui::Button(label)) {
				sm->SetCurrentStageIndex(i);
				gs->reloadRequested_ = true;
			}
		}
	}

	ImGui::Separator();
	ImGui::Checkbox("Show collision boxes (F4)", &gs->showCollisionDebug_);
	ImGui::TextDisabled("green = player | blue = block footprint / nearby blocks | red = active attack | purple = weapon | arrow = facing");
}

// ------------------------------------------------------------------ Player
void DebugMenu::DrawPlayerTab(GameScene* gs) {
	Player* player = gs->player_.get();
	if (!player) {
		ImGui::Text("No player");
		return;
	}

	// ---- live state ----
	ImGui::DragFloat3("Position", &player->worldTransform_.translation_.x, 0.05f);
	const Vector3& v = player->velocity_;
	ImGui::Text("Velocity: (%.3f, %.3f)  speed %.3f", v.x, v.z, std::sqrt(v.x * v.x + v.z * v.z));
	ImGui::Text("Move mode: %s", MoveModeName(player->GetMoveMode()));
	ImGui::Text("Behavior: %s   combo %d", BehaviorName(player->GetBehavior().GetState()), player->GetBehavior().GetComboIndex());

	// ---- facing: the lock / release is what keeps the model steady in dash + attack and eases it back in a roll ----
	ImGui::Separator();
	ImGui::Text("Facing");
	const float cameraYaw = gs->camera_.rotation_.y;
	ImGui::Text("Model yaw %.3f | camera yaw %.3f | locked yaw %.3f", player->facingYaw_, cameraYaw, player->lockedYaw_);
	ImGui::TextColored(player->yawLocked_ ? kOn : kOff, "Yaw lock: %s", player->yawLocked_ ? "LOCKED (dash / attack)" : "following camera");
	ImGui::TextColored(player->yawReleasing_ ? kOn : kOff, "Releasing: %s", player->yawReleasing_ ? "easing back to camera" : "-");
	ImGui::TextColored(player->yawDetached_ ? kOn : kOff, "Free-look: %s", player->yawDetached_ ? "DETACHED (re-lock with Alt to ease back)" : "-");

	// ---- weapon ----
	ImGui::Separator();
	bool equipped = player->IsWeaponEquipped();
	if (ImGui::Checkbox("Weapon equipped (Q)", &equipped))
		player->SetWeaponEquipped(equipped);
	ImGui::Text("Lunge cooldown: %.2fs", player->GetLungeCooldownRemaining());
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset cooldown"))
		player->lungeCooldown_ = 0.0f;

	bool locked = player->IsControlLocked();
	if (ImGui::Checkbox("Control Locked", &locked))
		player->SetControlLocked(locked);

	// ---- debuffs ----
	ImGui::Separator();
	ImGui::Text("Debuffs");
	ImGui::TextColored(player->GetBehavior().IsStunned() ? kOn : kOff, "Stun: %s (%.2fs left)", player->GetBehavior().IsStunned() ? "ACTIVE" : "-", player->GetBehavior().GetStunRemaining());
	ImGui::TextColored(player->GetBehavior().IsKnockback() ? kOn : kOff, "Knockback: %s", player->GetBehavior().IsKnockback() ? "ACTIVE" : "-");

	ImGui::SliderFloat("Stun Duration", &stunDuration_, 0.1f, 10.0f, "%.2f s");
	if (ImGui::Button("Apply Stun"))
		player->SetStunned(stunDuration_);

	ImGui::SliderFloat("Knockback Power", &knockbackPower_, 0.5f, 20.0f);
	const Vector3 forward = player->GetForward();
	if (ImGui::Button("Knockback backward"))
		player->SetKnockback({-forward.x, 0.0f, -forward.z}, knockbackPower_);
	ImGui::SameLine();
	if (ImGui::Button("Knockback forward"))
		player->SetKnockback({forward.x, 0.0f, forward.z}, knockbackPower_);

	if (ImGui::Button("Clear ALL debuffs"))
		player->ClearAllDebuffs();
}

// ------------------------------------------------------------------ Movement
void DebugMenu::DrawMovementTab(GameScene*) {
	ImGui::TextColored(kWarn, "Player statics are not in GlobalVariables: edits are lost on restart.");

	if (ImGui::CollapsingHeader("Walking", ImGuiTreeNodeFlags_DefaultOpen)) {
		DragF("Acceleration", Player::kAcceleration, 0.001f);
		DragF("Attenuation (friction)", Player::kAttenuation, 0.001f);
		DragF("Max run speed", Player::kLimitRunSpeed, 0.002f);
	}
	if (ImGui::CollapsingHeader("Dash / Roll", ImGuiTreeNodeFlags_DefaultOpen)) {
		DragF("Dash speed", Player::kDashSpeed, 0.005f);
		DragF("Dash duration (s)", Player::kDashDuration, 0.01f);
		DragF("Dash accel", Player::kDashAccel, 0.005f);
		DragF("Roll speed", Player::kRollSpeed, 0.005f);
		DragF("Roll duration (s)", Player::kRollDuration, 0.01f);
	}
	if (ImGui::CollapsingHeader("Yaw smoothing", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextDisabled("1/seconds, exponential. Higher = snappier. Release speed is what smooths the barrel roll.");
		DragF("Follow speed (walking)", Player::kYawFollowSpeed, 0.1f);
		DragF("Lock speed (dash / attack start)", Player::kYawLockSpeed, 0.1f);
		DragF("Release speed (after lock / roll)", Player::kYawReleaseSpeed, 0.05f);
	}
	if (ImGui::CollapsingHeader("Body poses")) {
		EditSRT("kDashPose", Player::kDashPose);
		EditSRT("kRollPose", Player::kHorizontalRollPose);
		EditSRT("kRollPose", Player::kVerticalRollPose);
		EditSRT("headOffset_", Player::headOffset_);
		EditSRT("hatOffset_", Player::hatOffset_);
	}
}

// ------------------------------------------------------------------ Heavy sabre
void DebugMenu::DrawHeavySabreTab() {
	const char* g = "WeaponHeavySabre";
	SaveGroups({g, "BaseMelee", "PlayerBehavior"});

	if (ImGui::CollapsingHeader("Weapon poses", ImGuiTreeNodeFlags_DefaultOpen)) {
		GvPose("Holster", g, "HolsterOffset");
		GvPose("Ready stance", g, "Ready");
	}
	if (ImGui::CollapsingHeader("Reaction poses"))
		GvReactionPoses(g);

	if (ImGui::CollapsingHeader("Combo poses (3 steps x 4 keyframes)", ImGuiTreeNodeFlags_DefaultOpen))
		GvWeaponCombos(g);

	if (ImGui::CollapsingHeader("Lunge poses")) {
		GvPose("Player lunge - wind-up", g, "PlayerLungePoseAnticipate");
		GvPose("Player lunge - extend", g, "PlayerLungePoseExtend");
		GvPose("Enemy lunge - wind-up", g, "LungePoseAnticipate");
		GvPose("Enemy lunge - extend", g, "LungePoseExtend");
	}

	if (ImGui::CollapsingHeader("Hitboxes (reach / width)")) {
		GvVec3("Combo reach (x3 steps)", g, "ComboReach", 0.02f);
		GvVec3("Combo swing width (x3 steps)", g, "ComboSwingHeight", 0.02f);
		GvFloat("Swing box height", "BaseMelee", "SwingBoxHeight", 0.02f);
		GvFloat("Player lunge reach", g, "PlayerLungeReach", 0.05f);
		GvFloat("Player lunge width", g, "PlayerLungeHeight", 0.05f);
		GvFloat("Enemy lunge reach", g, "LungeReach", 0.05f);
		GvFloat("Weapon box half width", g, "HalfWidthMin", 0.02f);
		GvFloat("Weapon box half height", g, "HalfHeightMin", 0.02f);
		GvFloat("Weapon box half depth", g, "HalfDepthMin", 0.02f);
		ImGui::TextDisabled("Turn on F4 to see the red attack box while swinging.");
	}

	if (ImGui::CollapsingHeader("Player combo timing / damage")) {
		ImGui::TextDisabled("One value per combo step. Durations are frames @60fps; speed is units per frame.");
		GvVec3("Attack speed", "BaseMelee", "PlayerComboAttackSpeed", 0.005f);
		GvVec3("Anticipate (frames)", "BaseMelee", "PlayerComboAnticipate", 0.2f);
		GvVec3("Attack (frames)", "BaseMelee", "PlayerComboAttack", 0.2f);
		GvVec3("Recovery (frames)", "BaseMelee", "PlayerComboRecovery", 0.2f);
		GvInt("Damage 1", "BaseMelee", "ComboDamagePlayer0");
		GvInt("Damage 2", "BaseMelee", "ComboDamagePlayer1");
		GvInt("Damage 3", "BaseMelee", "ComboDamagePlayer2");
		GvFloat("Base impact", "BaseMelee", "BaseImpactPowerPlayer", 0.05f);
		GvFloat("Impact step", "BaseMelee", "ImpactPowerStepPlayer", 0.05f);
	}

	if (ImGui::CollapsingHeader("Player lunge")) {
		GvFloat("Anticipate (frames)", g, "PlayerLungeAnticipate", 0.2f);
		GvFloat("Attack (frames)", g, "PlayerLungeAttack", 0.2f);
		GvFloat("Recovery (frames)", g, "PlayerLungeRecovery", 0.2f);
		GvFloat("Advance speed", g, "PlayerLungeSpeed", 0.005f);
		GvInt("Damage", g, "PlayerLungeDamage");
		GvFloat("Impact", g, "PlayerLungeImpact", 0.05f);
		GvFloat("Cooldown (s)", g, "PlayerLungeCooldown", 0.05f);
	}

	if (ImGui::CollapsingHeader("Enemy values (for later)")) {
		GvVec3("Attack speed", "BaseMelee", "EnemyComboAttackSpeed", 0.005f);
		GvVec3("Anticipate (frames)", "BaseMelee", "EnemyComboAnticipate", 0.2f);
		GvVec3("Attack (frames)", "BaseMelee", "EnemyComboAttack", 0.2f);
		GvVec3("Recovery (frames)", "BaseMelee", "EnemyComboRecovery", 0.2f);
		GvInt("Damage 1", "BaseMelee", "ComboDamageEnemy0");
		GvInt("Damage 2", "BaseMelee", "ComboDamageEnemy1");
		GvInt("Damage 3", "BaseMelee", "ComboDamageEnemy2");
		GvInt("Lunge damage", g, "LungeDamage");
		GvFloat("Lunge impact", g, "LungeImpactPower", 0.05f);
	}

	if (ImGui::CollapsingHeader("Trail")) {
		GvVec3("Blade tip (weapon local)", "BaseMelee", "TrailTipOffset", 0.02f);
	}

	if (ImGui::CollapsingHeader("Player body poses (shared with every future player weapon)")) {
		GvBodyCombo("Player", "PlayerBehavior", "BodyCombo");
		GvBodyLunge("Player heavy lunge (body)", "PlayerBehavior", "BodyLungeHeavy");
	}

	if (ImGui::CollapsingHeader("Knockback / stun feel")) {
		GvInt("Knockback min frames", "PlayerBehavior", "KnockBackDuration");
		GvInt("Knockback max frames", "PlayerBehavior", "KnockBackMaxDuration");
		GvFloat("Knockback friction", "PlayerBehavior", "KnockbackGroundFriction", 0.005f);
		GvFloat("Knockback stop speed", "PlayerBehavior", "KnockbackStopSpeed", 0.002f);
		GvFloat("Hit knockback speed scale", "PlayerBehavior", "HitKnockbackSpeedScale", 0.005f);
		GvFloat("Stun friction", "PlayerBehavior", "StunFriction", 0.005f);
	}
}

// ------------------------------------------------------------------ Camera
void DebugMenu::DrawCameraTab() {
	const char* g = "CameraController";
	SaveGroups({g});

	GvVec3("Target offset (x right, y up, z back)", g, "TargetOffset", 0.05f);
	GvVec3("Rotation offset (pitch, yaw, roll)", g, "RotationOffset", 0.01f);
	GvFloat("Follow interpolation", g, "InterpolationRate", 0.005f);
	GvFloat("Mouse sensitivity", g, "MouseSensitivity", 0.0001f);
}

#else // Release: compiles to nothing

void DebugMenu::Draw(GameScene*) {}

#endif