//#define NOMINMAX
//
//#include "DebugMenu.h"
//
//#include "GameScene.h"
//#include "GlobalVariables.h"
//#include "PoseGV.h"
//#include "StageManager.h"
//
//#include <array>
//#include <cstdio>
//#include <initializer_list>
//#include <string>
//#include <unordered_map>
//
//using namespace KamataEngine;
//
//// Player debuff section needs Player::DebuffStatus / GetDebuffStatus / HasAnyDebuff / ClearAllDebuffs /
//// ClearStun / ClearKnockback / ApplySlow / ClearSlow. Set to 0 if your Player doesn't have them.
//#ifndef DEBUGMENU_PLAYER_DEBUFFS
//#define DEBUGMENU_PLAYER_DEBUFFS 1
//#endif
//
//#ifdef _DEBUG
//
//namespace {
//
//// First-seen value of every edited SRTOffset, so "Reset" works.
//std::unordered_map<const void*, SRTOffset> sOriginals;
//
//bool DragF(const char* label, float& v, float speed) { return ImGui::DragFloat(label, &v, speed); }
//bool DragF3Arr(const char* label, std::array<float, 3>& a, float speed) { return ImGui::DragFloat3(label, a.data(), speed); }
//
//bool EditSRT(const char* label, SRTOffset& v) {
//	bool changed = false;
//	auto it = sOriginals.try_emplace(&v, v).first;
//
//	ImGui::PushID(&v);
//	if (ImGui::TreeNode(label)) {
//		changed |= ImGui::DragFloat3("Scale", &v.scale.x, 0.01f);
//		changed |= ImGui::DragFloat3("Rotation (rad)", &v.rotation.x, 0.01f);
//		changed |= ImGui::DragFloat3("Translation", &v.translation.x, 0.005f);
//		if (ImGui::SmallButton("Reset")) {
//			v = it->second;
//			changed = true;
//		}
//		ImGui::TreePop();
//	}
//	ImGui::PopID();
//	return changed;
//}
//
//bool EditComboPatterns(const char* id, std::array<ComboAttackPattern, 3>& patterns) {
//	static const char* kPhaseNames[4] = {"anticipateStart", "anticipateEnd", "attackEnd", "recoveryEnd"};
//	bool changed = false;
//
//	ImGui::PushID(id);
//	for (int i = 0; i < 3; ++i) {
//		char name[32];
//		std::snprintf(name, sizeof(name), "Combo %d", i);
//		if (ImGui::TreeNode(name)) {
//			ComboAttackPattern& p = patterns[i];
//			SRTOffset* poses[4] = {&p.anticipateStart, &p.anticipateEnd, &p.attackEnd, &p.recoveryEnd};
//			for (int k = 0; k < 4; ++k)
//				changed |= EditSRT(kPhaseNames[k], *poses[k]);
//			ImGui::TreePop();
//		}
//	}
//	ImGui::PopID();
//	return changed;
//}
//
//// ============================================================
//// GlobalVariables直接編集ヘルパー
//// 値はGVの中にあるので、ここで変えた値は次フレームのApplyGlobalVariables()で各クラスへ反映される。
//// ============================================================
//struct GvOriginal {
//	Vector3 s{};
//	Vector3 r{};
//	Vector3 t{};
//};
//std::unordered_map<const void*, GvOriginal> sGvOriginals;
//
//Vector3* GvVec(const char* group, const char* key) {
//	if (!key || !*key)
//		return nullptr;
//	return GlobalVariables::GetInstance()->GetVector3Ptr(group, key);
//}
//
//// 1つのポーズ(Scale/Rotation/Translation)。キーがnullptr/未登録の行は出さない。折りたたみ式 + Reset付き
//bool GvPoseKeys(const char* label, const char* group, const char* scaleKey, const char* rotKey, const char* transKey) {
//	Vector3* s = GvVec(group, scaleKey);
//	Vector3* r = GvVec(group, rotKey);
//	Vector3* t = GvVec(group, transKey);
//	if (!s && !r && !t) {
//		ImGui::TextDisabled("%s : (not registered)", label);
//		return false;
//	}
//	const void* id = t ? static_cast<const void*>(t) : (r ? static_cast<const void*>(r) : static_cast<const void*>(s));
//
//	auto it = sGvOriginals.find(id);
//	if (it == sGvOriginals.end()) {
//		GvOriginal o;
//		if (s)
//			o.s = *s;
//		if (r)
//			o.r = *r;
//		if (t)
//			o.t = *t;
//		it = sGvOriginals.emplace(id, o).first;
//	}
//
//	bool changed = false;
//	ImGui::PushID(id);
//	if (ImGui::TreeNode(label)) {
//		if (s)
//			changed |= ImGui::DragFloat3("Scale", &s->x, 0.01f);
//		if (r)
//			changed |= ImGui::DragFloat3("Rotation (rad)", &r->x, 0.01f);
//		if (t)
//			changed |= ImGui::DragFloat3("Translation", &t->x, 0.005f);
//		if (ImGui::SmallButton("Reset")) {
//			if (s)
//				*s = it->second.s;
//			if (r)
//				*r = it->second.r;
//			if (t)
//				*t = it->second.t;
//			changed = true;
//		}
//		ImGui::TreePop();
//	}
//	ImGui::PopID();
//	return changed;
//}
//
//// <base>Scale / <base>Rotation / <base>Translation 形式のキー
//bool GvPose(const char* label, const char* group, const std::string& base) {
//	const std::string s = base + "Scale";
//	const std::string r = base + "Rotation";
//	const std::string t = base + "Translation";
//	return GvPoseKeys(label, group, s.c_str(), r.c_str(), t.c_str());
//}
//
//bool GvFloat(const char* label, const char* group, const char* key, float speed = 0.01f) {
//	float* p = GlobalVariables::GetInstance()->GetFloatPtr(group, key);
//	if (!p) {
//		ImGui::TextDisabled("%s : (not registered)", label);
//		return false;
//	}
//	ImGui::PushID(group);
//	ImGui::PushID(key);
//	const bool changed = ImGui::DragFloat(label, p, speed);
//	ImGui::PopID();
//	ImGui::PopID();
//	return changed;
//}
//
//bool GvInt(const char* label, const char* group, const char* key) {
//	int32_t* p = GlobalVariables::GetInstance()->GetIntPtr(group, key);
//	if (!p) {
//		ImGui::TextDisabled("%s : (not registered)", label);
//		return false;
//	}
//	ImGui::PushID(group);
//	ImGui::PushID(key);
//	const bool changed = ImGui::DragInt(label, p, 0.2f);
//	ImGui::PopID();
//	ImGui::PopID();
//	return changed;
//}
//
//bool GvVec3(const char* label, const char* group, const char* key, float speed = 0.01f) {
//	Vector3* p = GvVec(group, key);
//	if (!p) {
//		ImGui::TextDisabled("%s : (not registered)", label);
//		return false;
//	}
//	ImGui::PushID(group);
//	ImGui::PushID(key);
//	const bool changed = ImGui::DragFloat3(label, &p->x, speed);
//	ImGui::PopID();
//	ImGui::PopID();
//	return changed;
//}
//
//// 表示中のグループをまとめてjsonへ保存する
//void SaveGroups(std::initializer_list<const char*> groups) {
//	if (ImGui::Button("Save this tab (json)")) {
//		for (const char* g : groups)
//			GlobalVariables::GetInstance()->SaveFile(g);
//	}
//	ImGui::SameLine();
//	ImGui::TextDisabled("Edits apply live. Save writes every group used in this tab.");
//	ImGui::Separator();
//}
//
//// 近接武器のコンボ(3段 x 4キーフレーム)。キー: Combo<i><AntStart|AntEnd|AtkEnd|RecEnd>
//void GvWeaponCombos(const char* group) {
//	static const char* kLabels[4] = {"1 Anticipate start", "2 Anticipate end (wind-up)", "3 Attack end (swing)", "4 Recovery end"};
//	ImGui::TextDisabled("Tip: a combo's Recovery end is usually the next combo's Anticipate start.");
//	ImGui::PushID(group);
//	for (int i = 0; i < 3; ++i) {
//		char name[32];
//		std::snprintf(name, sizeof(name), "Combo %d", i + 1);
//		if (ImGui::CollapsingHeader(name)) {
//			for (int k = 0; k < 4; ++k)
//				GvPose(kLabels[k], group, "Combo" + std::to_string(i) + PoseGV::kComboPhaseKeys[k]);
//		}
//	}
//	ImGui::PopID();
//}
//
//// 体のコンボポーズ(RegisterBodyComboPoseSet): キー <prefix><i><Anticipate|Peak|Recovery>
//void GvBodyCombo(const char* title, const char* group, const char* prefix) {
//	static const char* kPhases[3] = {"Anticipate", "Peak", "Recovery"};
//	ImGui::PushID(group);
//	ImGui::PushID(prefix);
//	for (int i = 0; i < 3; ++i) {
//		char name[64];
//		std::snprintf(name, sizeof(name), "%s combo %d", title, i + 1);
//		if (ImGui::CollapsingHeader(name)) {
//			for (int k = 0; k < 3; ++k)
//				GvPose(kPhases[k], group, std::string(prefix) + std::to_string(i) + kPhases[k]);
//		}
//	}
//	ImGui::PopID();
//	ImGui::PopID();
//}
//
//// 体の突き/叩きつけポーズ(RegisterBodyLungePoseSet): キー <prefix>WindUp / <prefix>Thrust
//void GvBodyLunge(const char* title, const char* group, const char* prefix) {
//	ImGui::PushID(group);
//	ImGui::PushID(prefix);
//	if (ImGui::CollapsingHeader(title)) {
//		GvPose("Wind-up", group, std::string(prefix) + "WindUp");
//		GvPose("Thrust / slam", group, std::string(prefix) + "Thrust");
//	}
//	ImGui::PopID();
//	ImGui::PopID();
//}
//
//// 3つのリアクションの構え(ノックバック / スタン / ブロック)
//void GvReactionPoses(const char* group) {
//	GvPose("Knockback", group, "Knockback");
//	GvPose("Stun", group, "Stun");
//	GvPose("Block (guard)", group, "Block");
//}
//
//// 銃の構え。キー名は既存のOffset*(Rotateの綴り)を使う
//void GvFirearmPoses(const char* group, bool dual) {
//	if (ImGui::CollapsingHeader("Weapon poses", ImGuiTreeNodeFlags_DefaultOpen)) {
//		GvPoseKeys("Combat (aim / fire)", group, "OffsetScale", "OffsetRotate", "OffsetTranslate");
//		if (dual) {
//			GvPoseKeys("Holster - pistol 1", group, "OffsetHolsterScale", "OffsetHolsterRotate", "OffsetHolsterTranslate");
//			GvPose("Holster - pistol 2", group, "Holster2");
//		} else {
//			GvPoseKeys("Holster", group, "OffsetHolsterScale", "OffsetHolsterRotate", "OffsetHolsterTranslate");
//		}
//		GvPoseKeys("Reload (stowed pose while reloading)", group, "OffsetReloadScale", "OffsetReloadRotate", "OffsetReloadTranslate");
//	}
//	if (ImGui::CollapsingHeader("Reaction poses"))
//		GvReactionPoses(group);
//}
//
//// 全ての銃で共通の撃った時の動き(BaseFirearm)
//void GvFireFeel() {
//	if (ImGui::CollapsingHeader("Fire feel (shared by ALL firearms)")) {
//		GvFloat("Recoil kick", "BaseFirearm", "RecoilKick", 0.005f);
//		GvInt("Recoil frames", "BaseFirearm", "RecoilDuration");
//		GvFloat("Aim lean", "BaseFirearm", "AimLeanAmount", 0.005f);
//		GvVec3("Muzzle offset (gun local)", "BaseFirearm", "MuzzleOffset", 0.01f);
//	}
//}
//
//// 敵ガンナー(Rifleman / Shotgunner)の体のポーズ。キーはTranslation/Rotationのみ登録
//void GvGunnerBodyPoses(const char* group) {
//	GvPoseKeys("Aim pose (body)", group, nullptr, "AimPoseRotation", "AimPoseTranslation");
//	GvPoseKeys("Fire pose (body)", group, nullptr, "FirePoseRotation", "FirePoseTranslation");
//	GvPoseKeys("Reload pose (body)", group, nullptr, "ReloadPoseRotation", "ReloadPoseTranslation");
//	GvFloat("Fire pose duration (s)", group, "FirePoseDuration", 0.005f);
//}
//
//const char* EnemyTypeName(BaseEnemy* e) {
//	if (dynamic_cast<EnemySapper*>(e))
//		return "Sapper";
//	if (dynamic_cast<EnemyGrenadier*>(e))
//		return "Grenadier";
//	if (dynamic_cast<EnemyOfficer*>(e))
//		return "Officer";
//	if (dynamic_cast<EnemyCarabinier*>(e))
//		return "Carabinier";
//	if (dynamic_cast<EnemyShotgunner*>(e))
//		return "Shotgunner";
//	if (dynamic_cast<EnemyFighter*>(e))
//		return "Fighter";
//	if (dynamic_cast<EnemyCuirassier*>(e))
//		return "Cuirassier";
//	if (dynamic_cast<EnemyRifleman*>(e))
//		return "Rifleman";
//	return "Enemy";
//}
//
//const char* PhaseName(GameScene::Phase p) {
//	switch (p) {
//	case GameScene::Phase::kFadeIn:
//		return "FadeIn";
//	case GameScene::Phase::kPlay:
//		return "Play";
//	case GameScene::Phase::kPause:
//		return "Pause";
//	case GameScene::Phase::kDeath:
//		return "Death";
//	case GameScene::Phase::kRetry:
//		return "Retry";
//	case GameScene::Phase::kFadeOut:
//		return "FadeOut";
//	case GameScene::Phase::kVictory:
//		return "Victory";
//	case GameScene::Phase::kGoalSequence:
//		return "GoalSequence";
//	}
//	return "?";
//}
//
//const char* ArenaStateName(ArenaManager::State s) {
//	switch (s) {
//	case ArenaManager::State::kDormant:
//		return "Dormant";
//	case ArenaManager::State::kActive:
//		return "Active";
//	case ArenaManager::State::kCleared:
//		return "Cleared";
//	}
//	return "?";
//}
//
//constexpr ImVec4 kWarn = {1.0f, 0.75f, 0.2f, 1.0f};
//
//} // namespace
//
//// ------------------------------------------------------------------ Main window
//void DebugMenu::Draw(GameScene* gs) {
//	if (!gs)
//		return;
//
//	gs->reloadRequested_ = false;
//
//	Input* input = Input::GetInstance();
//	if (input->TriggerKey(DIK_F3))
//		isOpen_ = !isOpen_;
//	if (input->TriggerKey(DIK_F2)) {
//		bool both = BaseEnemyBehavior::IsDebugDecisionFreeze() && BaseEnemyBehavior::IsDebugMovementFreeze();
//		BaseEnemyBehavior::SetDebugDecisionFreeze(!both);
//		BaseEnemyBehavior::SetDebugMovementFreeze(!both);
//	}
//
//	// Runs even while the window is closed so newly spawned enemies keep your head/hat edits.
//	PushBodyOffsetsToEnemies(gs);
//
//	if (!isOpen_)
//		return;
//
//	if (!ImGui::Begin("Debug Menu (F3)", &isOpen_)) {
//		ImGui::End();
//		return;
//	}
//
//	ImGui::TextDisabled("F1 Reload field | F2 freeze enemies | F3 this window | F4 collision boxes | P debug camera");
//	ImGui::TextDisabled("Weapon tabs edit GlobalVariables directly (live). 'Save this tab' writes the json.");
//
//	if (ImGui::BeginTabBar("DebugMenuTabs")) {
//		if (ImGui::BeginTabItem("Scene")) {
//			DrawSceneTab(gs);
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Player")) {
//			DrawPlayerTab(gs);
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Spawn")) {
//			DrawSpawnTab(gs);
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Enemies")) {
//			DrawEnemiesTab(gs);
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Sabre (Fighter)")) {
//			DrawSabreTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Heavy Sabre")) {
//			DrawHeavySabreTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Axe")) {
//			DrawAxeTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Spontoon")) {
//			DrawSpearTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Bayonet Musket")) {
//			DrawBayonetTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Shotgun")) {
//			DrawShotgunTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Pistols")) {
//			DrawPistolsTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Body / Anim")) {
//			DrawBodyTab();
//			ImGui::EndTabItem();
//		}
//		if (ImGui::BeginTabItem("Combat")) {
//			DrawCombatTab();
//			ImGui::EndTabItem();
//		}
//		ImGui::EndTabBar();
//	}
//	ImGui::End();
//}
//
//// ------------------------------------------------------------------ Scene
//void DebugMenu::DrawSceneTab(GameScene* gs) {
//	StageManager* sm = gs->stageManager_;
//
//	ImGui::Text("Phase: %s", PhaseName(gs->phase_));
//	if (sm)
//		ImGui::Text("Stage: %s (index %d)", sm->GetCurrentStageData().name.c_str(), sm->GetCurrentStageIndex());
//	ImGui::Text("Debug camera (P): %s", gs->isDebugCameraActive_ ? "ON" : "off");
//
//	if (ImGui::Button("Reload Scene") || Input::GetInstance()->TriggerKey(DIK_F1))
//		gs->reloadRequested_ = true;
//
//	if (sm) {
//		ImGui::Text("Load stage:");
//		int count = sm->GetStageCount();
//		for (int i = 0; i < count; ++i) {
//			if (i > 0)
//				ImGui::SameLine();
//			char label[96];
//			std::snprintf(label, sizeof(label), "%s##stage%d", sm->GetStageData(i).name.c_str(), i);
//			if (ImGui::Button(label)) {
//				sm->SetCurrentStageIndex(i);
//				gs->reloadRequested_ = true;
//			}
//		}
//	}
//
//	ImGui::Separator();
//	ImGui::Checkbox("Show collision boxes (F1)", &gs->showCollisionDebug_);
//
//	if (gs->player_) {
//		Vector3 p = gs->player_->GetWorldPosition();
//		if (ImGui::Button("Spawn Guard Effect"))
//			gs->CreateGuardEffect(p);
//		ImGui::SameLine();
//		if (ImGui::Button("Spawn Hit Effect"))
//			gs->CreateHitEffect(p);
//	}
//
//	if (ImGui::CollapsingHeader("Arenas")) {
//		if (gs->arenas_.empty())
//			ImGui::TextDisabled("No arenas registered");
//		for (const auto& arena : gs->arenas_) {
//			if (!arena)
//				continue;
//			ImGui::Text(
//			    "Arena %d: %s  remaining %d %s", static_cast<int>(arena->GetArenaID()), ArenaStateName(arena->GetState()), arena->GetRemainingEnemyCount(gs),
//			    arena->IsWaitingForNextWave() ? "[waiting for next wave]" : "");
//		}
//	}
//
//	if (ImGui::CollapsingHeader("Camera") && gs->cameraController_)
//		gs->cameraController_->DrawDebugControls();
//}
//
//// ------------------------------------------------------------------ Player
//void DebugMenu::DrawPlayerTab(GameScene* gs) {
//	Player* player = gs->player_.get();
//	if (!player) {
//		ImGui::Text("No player");
//		return;
//	}
//
//	int hp = player->GetHP();
//	if (ImGui::SliderInt("HP", &hp, 0, player->GetMaxHP()))
//		player->SetHP(hp);
//
//	if (ImGui::Button("Reset HP (100)")) {
//		player->SetMaxHP(100);
//		player->SetHP(100);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Debug HP (9999)")) {
//		player->SetMaxHP(9999);
//		player->SetHP(9999);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Kill player"))
//		player->KillInstant();
//
//	ImGui::Separator();
//	bool firearm = player->GetWeaponMode() == Player::WeaponMode::kFirearm;
//	if (ImGui::RadioButton("Melee", !firearm))
//		player->SetWeaponMode(Player::WeaponMode::kMelee);
//	ImGui::SameLine();
//	if (ImGui::RadioButton("Firearm", firearm))
//		player->SetWeaponMode(Player::WeaponMode::kFirearm);
//
//	bool locked = player->IsControlLocked();
//	if (ImGui::Checkbox("Control Locked", &locked))
//		player->SetControlLocked(locked);
//
//#if DEBUGMENU_PLAYER_DEBUFFS
//	if (ImGui::CollapsingHeader("Debuffs", ImGuiTreeNodeFlags_DefaultOpen)) {
//		const ImVec4 kOn = {1.0f, 0.45f, 0.25f, 1.0f};
//		const ImVec4 kOff = {0.55f, 0.55f, 0.55f, 1.0f};
//		Player::DebuffStatus s = player->GetDebuffStatus();
//
//		ImGui::Text("Active: %s", player->HasAnyDebuff() ? "YES" : "none");
//		ImGui::Separator();
//
//		if (s.stunned)
//			ImGui::TextColored(kOn, "Stun: %.2fs left", s.stunRemaining);
//		else
//			ImGui::TextColored(kOff, "Stun: -");
//
//		ImGui::TextColored(s.knockedBack ? kOn : kOff, "Knockback: %s", s.knockedBack ? "ACTIVE" : "-");
//		ImGui::TextColored(s.slowedByTerrain ? kOn : kOff, "Slow (terrain): %s (x%.2f)", s.slowedByTerrain ? "ACTIVE" : "-", MapChipField::kSlowSpeedFactor);
//
//		if (s.slowedByEffect) {
//			if (s.slowUntilCleared)
//				ImGui::TextColored(kOn, "Slow (effect): x%.2f until cleared", s.slowFactor);
//			else
//				ImGui::TextColored(kOn, "Slow (effect): x%.2f, %.2fs left", s.slowFactor, s.slowRemaining);
//		} else {
//			ImGui::TextColored(kOff, "Slow (effect): -");
//		}
//
//		ImGui::TextColored(s.controlLocked ? kOn : kOff, "Control Locked: %s", s.controlLocked ? "ACTIVE" : "-");
//
//		if (s.beingPushed)
//			ImGui::TextColored(kOn, "Scroll Push: ACTIVE (vx %.3f, squish %u)", s.pushVelocityX, s.squishTimer);
//		else
//			ImGui::TextColored(kOff, "Scroll Push: -");
//
//		ImGui::Separator();
//		ImGui::Text("Disable");
//		if (ImGui::Button("Clear ALL Debuffs"))
//			player->ClearAllDebuffs();
//		if (ImGui::Button("Clear Stun"))
//			player->ClearStun();
//		ImGui::SameLine();
//		if (ImGui::Button("Clear Knockback"))
//			player->ClearKnockback();
//		ImGui::SameLine();
//		if (ImGui::Button("Clear Slow"))
//			player->ClearSlow();
//
//		ImGui::Separator();
//		ImGui::Text("Apply");
//
//		static float stunDuration = 1.5f;
//		ImGui::SliderFloat("Stun Duration", &stunDuration, 0.1f, 10.0f, "%.2f s");
//		if (ImGui::Button("Apply Stun"))
//			player->SetStunned(stunDuration);
//
//		static float knockbackPower = 5.0f;
//		ImGui::SliderFloat("Knockback Power", &knockbackPower, 0.5f, 20.0f);
//		if (ImGui::Button("Knockback Left"))
//			player->SetKnockback({-1.0f, 0.4f, 0.0f}, knockbackPower);
//		ImGui::SameLine();
//		if (ImGui::Button("Knockback Right"))
//			player->SetKnockback({1.0f, 0.4f, 0.0f}, knockbackPower);
//
//		static float slowFactor = 0.45f;
//		static float slowDuration = 5.0f;
//		static bool slowUntilCleared = false;
//		ImGui::SliderFloat("Slow Factor", &slowFactor, 0.05f, 1.0f);
//		ImGui::Checkbox("Slow until cleared", &slowUntilCleared);
//		if (!slowUntilCleared)
//			ImGui::SliderFloat("Slow Duration", &slowDuration, 0.1f, 30.0f, "%.1f s");
//		if (ImGui::Button("Apply Slow"))
//			player->ApplySlow(slowFactor, slowUntilCleared ? 0.0f : slowDuration);
//	}
//#endif
//}
//
//// ------------------------------------------------------------------ Spawn
//void DebugMenu::DrawSpawnTab(GameScene* gs) {
//	Player* player = gs->player_.get();
//	if (!player) {
//		ImGui::Text("No player");
//		return;
//	}
//	Vector3 p = player->GetWorldPosition();
//
//	ImGui::DragFloat("Spawn distance from player", &spawnOffsetX_, 0.1f, 0.0f, 30.0f);
//
//	// enemy type ids: 0 = Rifleman, 1 = Cuirassier, 2 = Fighter
//	Vector3 right = {p.x + spawnOffsetX_, p.y, p.z};
//	Vector3 left = {p.x - spawnOffsetX_, p.y, p.z};
//
//	if (ImGui::Button("Spawn Rifleman (right)"))
//		gs->SpawnEnemyAt(right, 0);
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn Cuirassier (left)"))
//		gs->SpawnEnemyAt(left, 1);
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn Fighter (left)"))
//		gs->SpawnEnemyAt(left, 2);
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn Carabiner (right)"))
//		gs->SpawnEnemyAt(right, 3);
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn Shotgunner (right)"))
//		gs->SpawnEnemyAt(right, 4);
//
//	if (ImGui::Button("Spawn Grenadier (left)"))
//		gs->SpawnEnemyAt(left, 5);
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn Officer (left)"))
//		gs->SpawnEnemyAt(left, 6);
//	if (ImGui::Button("Spawn Sapper (left)"))
//		gs->SpawnEnemyAt(left, 7);
//
//	if (ImGui::Button("Spawn one of each")) {
//		gs->SpawnEnemyAt(right, 0);
//		gs->SpawnEnemyAt(left, 2);
//		Vector3 farthest = {p.x - spawnOffsetX_ * 2.0f, p.y, p.z};
//		gs->SpawnEnemyAt(farthest, 1);
//		gs->SpawnEnemyAt(farthest, 3);
//		gs->SpawnEnemyAt(right, 4);
//		gs->SpawnEnemyAt(left, 5);
//		gs->SpawnEnemyAt(farthest, 6);
//		gs->SpawnEnemyAt(left, 7);
//	}
//
//	ImGui::Separator();
//	ImGui::Text("Spawners at player position");
//	if (ImGui::Button("Limited Rifleman (x3, every 4s)"))
//		gs->AddEnemySpawner(p, 0, 4.0f, EnemySpawner::SpawnMode::kLimited, 3, false);
//	if (ImGui::Button("Endless Cuirassier (every 8s, omniscient)"))
//		gs->AddEnemySpawner(p, 1, 8.0f, EnemySpawner::SpawnMode::kEndless, -1, true);
//	ImGui::Text("Active spawners: %d", static_cast<int>(gs->enemySpawners_.size()));
//
//	// ---- アイテムスポーナー(武器ドロップ) ----
//	ImGui::Separator();
//	ImGui::Text("Item spawner");
//
//	// 武器の種類と状態の選択(static: 選択を保持する)
//	static int itemKindIndex = 0;
//	static int itemStateIndex = 0;
//	static float itemSpawnOffsetX = 1.5f;
//	static float itemSpawnOffsetY = 1.0f;
//	static bool itemSpawnFront = true;
//	static int itemSpawnCount = 1;
//
//	struct ItemEntry {
//		const char* label;
//		DropItemCategory category;
//		DropWeaponKind kind;
//	};
//	static const ItemEntry kItemTable[] = {
//	    {"Sabre (light)",          DropItemCategory::kMelee,   DropWeaponKind::kSabreLight   },
//	    {"Sabre (heavy)",          DropItemCategory::kMelee,   DropWeaponKind::kSabreHeavy   },
//	    {"Axe",	                DropItemCategory::kMelee,   DropWeaponKind::kLongAxe      },
//	    {"Spontoon",               DropItemCategory::kMelee,   DropWeaponKind::kSpear        },
//	    {"Musket (long)",          DropItemCategory::kFirearm, DropWeaponKind::kMusketLong   },
//	    {"Musket (bayonet)",       DropItemCategory::kFirearm, DropWeaponKind::kMusketBayonet},
//	    {"Shotgun",                DropItemCategory::kFirearm, DropWeaponKind::kShotgun      },
//	    {"Dual Artillery Pistols", DropItemCategory::kFirearm, DropWeaponKind::kDualPistols  },
//	};
//	constexpr int kItemCount = static_cast<int>(sizeof(kItemTable) / sizeof(kItemTable[0]));
//
//	if (ImGui::BeginCombo("Weapon", kItemTable[itemKindIndex].label)) {
//		for (int i = 0; i < kItemCount; ++i) {
//			if (ImGui::Selectable(kItemTable[i].label, itemKindIndex == i))
//				itemKindIndex = i;
//		}
//		ImGui::EndCombo();
//	}
//
//	// 銃: Ready=装填済み / OnCooldown=未装填、近接: Ready=通常 / OnCooldown=クールダウン中(拾い直しでリセットされる)
//	static const char* kStateLabels[] = {"Ready (loaded / ready)", "OnCooldown (unloaded / cooling)"};
//	ImGui::Combo("State", &itemStateIndex, kStateLabels, 2);
//
//	ImGui::DragFloat("Offset X", &itemSpawnOffsetX, 0.05f, 0.0f, 20.0f);
//	ImGui::DragFloat("Offset Y", &itemSpawnOffsetY, 0.05f, -5.0f, 10.0f);
//	ImGui::Checkbox("In front of player (facing)", &itemSpawnFront);
//	ImGui::SliderInt("Count", &itemSpawnCount, 1, 10);
//
//	// プレイヤーの位置(向きに応じて前方 or 常に右)からスポーン位置を決める
//	auto itemSpawnPosition = [&]() {
//		float dir = 1.0f;
//		if (itemSpawnFront && player->isLeft())
//			dir = -1.0f;
//		return Vector3{p.x + itemSpawnOffsetX * dir, p.y + itemSpawnOffsetY, p.z};
//	};
//	const DropItemState itemState = (itemStateIndex == 0) ? DropItemState::kReady : DropItemState::kOnCooldown;
//
//	if (ImGui::Button("Spawn item")) {
//		const ItemEntry& e = kItemTable[itemKindIndex];
//		for (int i = 0; i < itemSpawnCount; ++i)
//			gs->CreateDroppableItem(itemSpawnPosition(), e.category, e.kind, itemState);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Spawn one of each")) {
//		for (int i = 0; i < kItemCount; ++i)
//			gs->CreateDroppableItem(itemSpawnPosition(), kItemTable[i].category, kItemTable[i].kind, itemState);
//	}
//	ImGui::Text("Droppable items: %d", static_cast<int>(gs->droppableItems_.size()));
//	ImGui::SameLine();
//	if (ImGui::SmallButton("Clear items"))
//		gs->droppableItems_.clear();
//}
//
//// ------------------------------------------------------------------ Enemies
//void DebugMenu::DrawEnemiesTab(GameScene* gs) {
//	bool freezeDecisions = BaseEnemyBehavior::IsDebugDecisionFreeze();
//	if (ImGui::Checkbox("Freeze Enemy Decisions", &freezeDecisions))
//		BaseEnemyBehavior::SetDebugDecisionFreeze(freezeDecisions);
//	bool freezeMovement = BaseEnemyBehavior::IsDebugMovementFreeze();
//	if (ImGui::Checkbox("Freeze Enemy Movement", &freezeMovement))
//		BaseEnemyBehavior::SetDebugMovementFreeze(freezeMovement);
//	ImGui::TextDisabled("F2 toggles both. Frozen enemies still animate their current attack.");
//
//	ImGui::Separator();
//	const auto& enemies = gs->GetEnemies();
//	ImGui::Text("Live enemies: %d", static_cast<int>(enemies.size()));
//
//	if (ImGui::Button("Heal all")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->SetHP(e->GetMaxHP());
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Kill all")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->KillInstant();
//	}
//
//	ImGui::Separator();
//	ImGui::Text("Debuffs (all live enemies)");
//	static float stunDuration = 1.5f;
//	static float knockbackPower = 5.0f;
//	ImGui::SliderFloat("Stun Duration", &stunDuration, 0.1f, 10.0f, "%.2f s");
//	ImGui::SliderFloat("Knockback Power", &knockbackPower, 0.5f, 20.0f);
//
//	if (ImGui::Button("Stun all")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->RequestStun(stunDuration);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Knockback all left")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->RequestKnockback({-1.0f, 0.4f, 0.0f}, knockbackPower);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Knockback all right")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->RequestKnockback({1.0f, 0.4f, 0.0f}, knockbackPower);
//	}
//	ImGui::SameLine();
//	if (ImGui::Button("Clear debuffs")) {
//		for (const auto& e : enemies)
//			if (e && !e->IsDead())
//				e->ClearDebuffs();
//	}
//
//	ImGui::Separator();
//	int index = 0;
//	for (const auto& e : enemies) {
//		if (!e)
//			continue;
//		Vector3 p = e->GetWorldPosition();
//		const BaseEnemyBehavior& b = e->GetBehaviorModule();
//		ImGui::Text(
//		    "#%d %-10s HP %d/%d  pos(%.1f, %.1f) arena %d %s%s%s", index++, EnemyTypeName(e.get()), e->GetHP(), e->GetMaxHP(), p.x, p.y, static_cast<int>(e->GetArenaID()),
//		    e->IsPlayerSpotted() ? "[SPOTTED] " : "", b.IsStunned() ? "[STUN] " : "", b.IsKnockedBack() ? "[KNOCKBACK]" : "");
//	}
//}
//
//// ------------------------------------------------------------------ Sabre (EnemyFighter)
//void DebugMenu::DrawSabreTab() {
//	ImGui::TextColored(kWarn, "WeaponSabre statics are shared with the PLAYER's sabre. (not in GlobalVariables: lost on restart)");
//	EditSRT("readyStanceOffset_", WeaponSabre::readyStanceOffset_);
//
//	if (ImGui::CollapsingHeader("Combo poses (visible while attacking)", ImGuiTreeNodeFlags_DefaultOpen))
//		EditComboPatterns("sabreCombos", WeaponSabre::comboPatterns_);
//
//	if (ImGui::CollapsingHeader("Reach / height (enemy hitbox)")) {
//		DragF3Arr("kComboReach", WeaponSabre::kComboReach, 0.02f);
//		DragF3Arr("kComboSwingHeight", WeaponSabre::kComboSwingHeight, 0.02f);
//	}
//
//	if (ImGui::CollapsingHeader("BaseMelee timing (shared by ALL melee weapons: sabre / heavy / axe / spontoon)")) {
//		ImGui::TextDisabled("Player");
//		DragF3Arr("P Attack speed", WeaponSabre::kComboAttackSpeedPlayer, 0.01f);
//		DragF3Arr("P Anticipate (frames)", WeaponSabre::kComboAnticipateDurationPlayer, 0.2f);
//		DragF3Arr("P Attack (frames)", WeaponSabre::kComboAttackDurationPlayer, 0.2f);
//		DragF3Arr("P Recovery (frames)", WeaponSabre::kComboRecoveryDurationPlayer, 0.2f);
//		ImGui::TextDisabled("Enemy");
//		DragF3Arr("E Attack speed", WeaponSabre::kComboAttackSpeedEnemy, 0.01f);
//		DragF3Arr("E Anticipate (frames)", WeaponSabre::kComboAnticipateDurationEnemy, 0.2f);
//		DragF3Arr("E Attack (frames)", WeaponSabre::kComboAttackDurationEnemy, 0.2f);
//		DragF3Arr("E Recovery (frames)", WeaponSabre::kComboRecoveryDurationEnemy, 0.2f);
//	}
//}
//
//// ------------------------------------------------------------------ Heavy sabre (EnemyCuirassier)
//void DebugMenu::DrawHeavySabreTab() {
//	const char* g = "WeaponHeavySabre";
//	SaveGroups({g, "EnemyCuirassier", "PlayerBehavior"});
//
//	if (ImGui::CollapsingHeader("Weapon poses", ImGuiTreeNodeFlags_DefaultOpen)) {
//		GvPose("Holster", g, "HolsterOffset");
//		GvPose("Ready stance", g, "Ready");
//		GvPose("Down attack (player)", g, "DownAttack");
//	}
//	if (ImGui::CollapsingHeader("Reaction poses"))
//		GvReactionPoses(g);
//
//	if (ImGui::CollapsingHeader("Combo poses (3 steps x 4 keyframes)", ImGuiTreeNodeFlags_DefaultOpen))
//		GvWeaponCombos(g);
//
//	if (ImGui::CollapsingHeader("Lunge poses")) {
//		GvPose("Enemy lunge - wind-up", g, "LungePoseAnticipate");
//		GvPose("Enemy lunge - extend", g, "LungePoseExtend");
//		GvPose("Player lunge - wind-up", g, "PlayerLungePoseAnticipate");
//		GvPose("Player lunge - extend", g, "PlayerLungePoseExtend");
//	}
//
//	if (ImGui::CollapsingHeader("Reach / lunge stats")) {
//		GvVec3("Combo reach (x3 steps)", g, "ComboReach", 0.02f);
//		GvVec3("Combo swing height (x3 steps)", g, "ComboSwingHeight", 0.02f);
//		GvFloat("Enemy lunge reach", g, "LungeReach", 0.05f);
//		GvFloat("Player lunge reach", g, "PlayerLungeReach", 0.05f);
//		GvFloat("Player lunge height", g, "PlayerLungeHeight", 0.05f);
//		GvFloat("Player lunge anticipate (frames)", g, "PlayerLungeAnticipate", 0.2f);
//		GvFloat("Player lunge attack (frames)", g, "PlayerLungeAttack", 0.2f);
//		GvFloat("Player lunge recovery (frames)", g, "PlayerLungeRecovery", 0.2f);
//		GvFloat("Player lunge speed", g, "PlayerLungeSpeed", 0.005f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Cuirassier = enemy. PlayerBehavior values are shared with the player's other weapons.");
//		GvBodyCombo("Cuirassier", "EnemyCuirassier", "BodyCombo");
//		GvBodyLunge("Cuirassier lunge (body)", "EnemyCuirassier", "BodyLunge");
//		GvBodyLunge("Player heavy lunge (body)", "PlayerBehavior", "BodyLungeHeavy");
//		GvBodyCombo("Player (shared)", "PlayerBehavior", "BodyCombo");
//	}
//}
//
//// ------------------------------------------------------------------ Axe (EnemySapper)
//void DebugMenu::DrawAxeTab() {
//	const char* g = "WeaponAxe";
//	SaveGroups({g, "EnemySapper", "PlayerBehavior"});
//
//	if (ImGui::CollapsingHeader("Weapon poses", ImGuiTreeNodeFlags_DefaultOpen)) {
//		GvPose("Holster", g, "HolsterOffset");
//		GvPose("Ready stance", g, "Ready");
//		GvPose("Down attack (player)", g, "DownAttack");
//	}
//	if (ImGui::CollapsingHeader("Reaction poses"))
//		GvReactionPoses(g);
//
//	if (ImGui::CollapsingHeader("Combo poses (3 steps x 4 keyframes)", ImGuiTreeNodeFlags_DefaultOpen))
//		GvWeaponCombos(g);
//
//	if (ImGui::CollapsingHeader("Smash poses (Sapper leap)")) {
//		GvPoseKeys("Raise (overhead)", g, nullptr, "SmashRaiseRotation", "SmashRaiseTranslation");
//		GvPoseKeys("Slam (ground)", g, nullptr, "SmashSlamRotation", "SmashSlamTranslation");
//		GvFloat("Smash half width", g, "SmashHalfWidth", 0.05f);
//		GvFloat("Smash below", g, "SmashBelow", 0.05f);
//		GvFloat("Smash above", g, "SmashAbove", 0.05f);
//	}
//
//	if (ImGui::CollapsingHeader("Reach / damage")) {
//		GvVec3("Combo reach (x3 steps)", g, "ComboReach", 0.02f);
//		GvVec3("Combo swing height (x3 steps)", g, "ComboSwingHeight", 0.02f);
//		ImGui::TextDisabled("Enemy");
//		GvInt("Damage 1", g, "ComboDamage0");
//		GvInt("Damage 2", g, "ComboDamage1");
//		GvInt("Damage 3", g, "ComboDamage2");
//		GvFloat("Base impact", g, "BaseImpact", 0.05f);
//		GvFloat("Impact step", g, "ImpactStep", 0.05f);
//		GvInt("Smash damage", g, "SmashDamage");
//		GvFloat("Smash impact", g, "SmashImpact", 0.05f);
//		ImGui::TextDisabled("Player");
//		GvInt("P Damage 1", g, "PlayerComboDamage0");
//		GvInt("P Damage 2", g, "PlayerComboDamage1");
//		GvInt("P Damage 3", g, "PlayerComboDamage2");
//		GvFloat("P Base impact", g, "PlayerBaseImpact", 0.05f);
//		GvFloat("P Impact step", g, "PlayerImpactStep", 0.05f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Sapper = enemy. Player combo body poses are shared with the player's other melee weapons.");
//		GvBodyCombo("Sapper", "EnemySapper", "BodyCombo");
//		GvBodyLunge("Sapper smash (body)", "EnemySapper", "BodySmash");
//		GvBodyCombo("Player (shared)", "PlayerBehavior", "BodyCombo");
//	}
//}
//
//// ------------------------------------------------------------------ Spontoon (EnemyOfficer)
//void DebugMenu::DrawSpearTab() {
//	const char* g = "WeaponSpear";
//	SaveGroups({g, "EnemyOfficer", "EnemyFighter", "PlayerBehavior"});
//
//	if (ImGui::CollapsingHeader("Weapon poses", ImGuiTreeNodeFlags_DefaultOpen)) {
//		GvPose("Holster", g, "HolsterOffset");
//		GvPose("Ready stance", g, "Ready");
//		GvPose("Down attack (player)", g, "DownAttack");
//	}
//	if (ImGui::CollapsingHeader("Reaction poses"))
//		GvReactionPoses(g);
//
//	if (ImGui::CollapsingHeader("Combo poses (3 steps x 4 keyframes)", ImGuiTreeNodeFlags_DefaultOpen))
//		GvWeaponCombos(g);
//
//	if (ImGui::CollapsingHeader("Reach")) {
//		GvFloat("Reach (all 3 steps)", g, "Reach", 0.05f);
//		GvVec3("Combo swing height (x3 steps)", g, "ComboSwingHeight", 0.02f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Officer swings with the Fighter's body combo (EnemyFighter, shared with the sabre Fighter).");
//		GvPoseKeys("Officer buff pose (body)", "EnemyOfficer", "BuffPoseScale", "BuffPoseRotation", "BuffPoseTranslation");
//		GvFloat("Buff anim duration (s)", "EnemyOfficer", "BuffAnimDuration", 0.02f);
//		GvFloat("Buff apply time (s)", "EnemyOfficer", "BuffApplyTime", 0.02f);
//		GvBodyCombo("Fighter/Officer", "EnemyFighter", "BodyCombo");
//		GvBodyCombo("Player (shared)", "PlayerBehavior", "BodyCombo");
//	}
//}
//
//// ------------------------------------------------------------------ Bayonet musket (EnemyRifleman / Carabinier)
//void DebugMenu::DrawBayonetTab() {
//	const char* g = "FirearmMusketBayonet";
//	SaveGroups({g, "EnemyRifleman", "Player", "PlayerBehavior", "BaseFirearm"});
//
//	GvFirearmPoses(g, false);
//
//	if (ImGui::CollapsingHeader("Bayonet lunge poses")) {
//		ImGui::TextDisabled("rotation.y must stay -1.5708. translation.z = forward, .x = camera side, rotation.x = barrel pitch.");
//		GvPose("Enemy lunge - wind-up", g, "LungeWindUp");
//		GvPose("Enemy lunge - thrust", g, "LungeThrust");
//		GvPose("Player lunge - wind-up", g, "PlayerLungeWindUp");
//		GvPose("Player lunge - thrust", g, "PlayerLungeThrust");
//	}
//
//	GvFireFeel();
//
//	if (ImGui::CollapsingHeader("Lunge timing / hitbox")) {
//		ImGui::TextDisabled("Enemy (frames @60fps)");
//		GvFloat("Anticipate", g, "BayonetAnticipateDuration", 0.2f);
//		GvFloat("Attack", g, "BayonetAttackDuration", 0.2f);
//		GvFloat("Recovery", g, "BayonetRecoveryDuration", 0.2f);
//		GvFloat("Advance speed", g, "BayonetAttackSpeed", 0.005f);
//		GvFloat("Reach", g, "BayonetReach", 0.05f);
//		GvFloat("Swing height", g, "BayonetSwingHeight", 0.05f);
//		ImGui::TextDisabled("Player");
//		GvFloat("P Anticipate", g, "PlayerBayonetAnticipate", 0.2f);
//		GvFloat("P Attack", g, "PlayerBayonetAttack", 0.2f);
//		GvFloat("P Recovery", g, "PlayerBayonetRecovery", 0.2f);
//		GvFloat("P Advance speed", g, "PlayerBayonetSpeed", 0.005f);
//		GvFloat("P Reach", g, "PlayerBayonetReach", 0.05f);
//		GvFloat("P Height", g, "PlayerBayonetHeight", 0.05f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Rifleman (and Carabinier, same statics). Player offsets are shared with the player's other firearms.");
//		GvGunnerBodyPoses("EnemyRifleman");
//		GvBodyLunge("Rifleman bayonet lunge (body)", "EnemyRifleman", "BodyLunge");
//		EditSRT("Rifleman firearmOffset_ (extra, not in GV)", EnemyRifleman::firearmOffset_);
//		GvBodyLunge("Player bayonet lunge (body)", "PlayerBehavior", "BodyLungeBayonet");
//		GvPose("Player firearm offset (shared)", "Player", "FirearmOffset");
//	}
//}
//
//// ------------------------------------------------------------------ Shotgun (EnemyShotgunner)
//void DebugMenu::DrawShotgunTab() {
//	const char* g = "FirearmShotgun";
//	SaveGroups({g, "EnemyShotgunner", "Player", "BaseFirearm"});
//
//	GvFirearmPoses(g, false);
//	GvFireFeel();
//
//	if (ImGui::CollapsingHeader("Fire pattern / recoil")) {
//		GvInt("Pellet count", g, "PelletCount");
//		GvFloat("Spread step (rad)", g, "SpreadStep", 0.005f);
//		GvInt("Pellet life (frames)", g, "PelletLifeFrames");
//		GvFloat("Fire knockback power", g, "FireKnockbackPower", 0.1f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Shotgunner = enemy. Player offsets are shared with the player's other firearms.");
//		GvGunnerBodyPoses("EnemyShotgunner");
//		EditSRT("Shotgunner firearmOffset_ (extra, not in GV)", EnemyShotgunner::firearmOffset_);
//		GvPose("Player firearm offset (shared)", "Player", "FirearmOffset");
//	}
//}
//
//// ------------------------------------------------------------------ Dual pistols (player only)
//void DebugMenu::DrawPistolsTab() {
//	const char* g = "FirearmPistols";
//	SaveGroups({g, "Player", "BaseFirearm"});
//
//	GvFirearmPoses(g, true);
//	GvFireFeel();
//
//	if (ImGui::CollapsingHeader("Second pistol / reload")) {
//		GvVec3("2nd pistol delta (combat, parent-local)", g, "SecondPistolDelta", 0.01f);
//		GvFloat("Reload per pistol (player, s)", g, "ReloadDurationPlayer", 0.05f);
//		GvFloat("Reload hit bonus (player, s)", g, "ReloadHitBonusPlayer", 0.01f);
//		GvFloat("Reload min (player, s)", g, "ReloadDurationMinPlayer", 0.05f);
//	}
//
//	if (ImGui::CollapsingHeader("Entity (body) offsets")) {
//		ImGui::TextColored(kWarn, "Pistols are player-only. This offset is shared with the player's other firearms.");
//		GvPose("Player firearm offset (shared)", "Player", "FirearmOffset");
//	}
//}
//
//// ------------------------------------------------------------------ Body / Anim
//void DebugMenu::PushBodyOffsetsToEnemies(GameScene* gs) {
//	const auto& enemies = gs->GetEnemies();
//
//	if (!bodyInitialized_) {
//		for (const auto& e : enemies) {
//			if (!e)
//				continue;
//			bodyHead_ = e->GetAnimationManager().headOffset_;
//			bodyHat_ = e->GetAnimationManager().hatOffset_;
//			bodyInitialized_ = true;
//			break;
//		}
//		return; // nothing to push until real values are captured
//	}
//
//	if (!pushBodyOffsets_)
//		return;
//	for (const auto& e : enemies) {
//		if (!e)
//			continue;
//		e->GetAnimationManager().headOffset_ = bodyHead_;
//		e->GetAnimationManager().hatOffset_ = bodyHat_;
//	}
//}
//
//void DebugMenu::DrawBodyTab() {
//	ImGui::TextColored(kWarn, "AnimationManager statics are shared with the PLAYER too.");
//
//	ImGui::Checkbox("Push head/hat offsets to all enemies every frame", &pushBodyOffsets_);
//	if (!bodyInitialized_) {
//		ImGui::TextDisabled("Waiting for an enemy to exist to capture head/hat offsets...");
//	} else {
//		EditSRT("headOffset_ (per-instance -> all enemies)", bodyHead_);
//		EditSRT("hatOffset_ (per-instance -> all enemies)", bodyHat_);
//	}
//
//	if (ImGui::CollapsingHeader("Movement poses", ImGuiTreeNodeFlags_DefaultOpen)) {
//		EditSRT("kIdlePoseLow", AnimationManager::kIdlePoseLow);
//		EditSRT("kIdlePoseHigh", AnimationManager::kIdlePoseHigh);
//		EditSRT("kRunPoseLow", AnimationManager::kRunPoseLow);
//		EditSRT("kRunPoseHigh", AnimationManager::kRunPoseHigh);
//		EditSRT("kJumpPose", AnimationManager::kJumpPose);
//		EditSRT("kFallPose", AnimationManager::kFallPose);
//	}
//
//	DragF("FootstepInterval", AnimationManager::kFootstepInterval, 0.005f);
//}
//
//// ------------------------------------------------------------------ Combat / movement
//void DebugMenu::DrawCombatTab() {
//	DragF("ChaseSpeed (all enemies)", BaseEnemy::kChaseSpeed, 0.002f);
//
//	if (ImGui::CollapsingHeader("Cuirassier AI")) {
//		DragF("Lunge anticipate (s)", CuirassierBehavior::kLungeAnticipateDuration, 0.01f);
//		DragF("Lunge attack (s)", CuirassierBehavior::kLungeAttackDuration, 0.01f);
//		DragF("Lunge recovery (s)", CuirassierBehavior::kLungeRecoveryDuration, 0.01f);
//		DragF("Lunge speed", CuirassierBehavior::kLungeSpeed, 0.005f);
//		DragF("Melee engage range", CuirassierBehavior::kMeleeEngageRange, 0.05f);
//	}
//	if (ImGui::CollapsingHeader("Fighter AI")) {
//		DragF("Melee engage range", FighterBehavior::kMeleeEngageRange, 0.05f);
//		DragF("Cooldown (s)", FighterBehavior::kCooldownDuration, 0.01f);
//		DragF("Combo reset (s)", FighterBehavior::kComboResetDuration, 0.05f);
//		DragF("Block chance when hit", FighterBehavior::kBlockChanceWhenHit, 0.01f);
//	}
//	if (ImGui::CollapsingHeader("Rifleman AI")) {
//		DragF("Aim duration (s)", RiflemanBehavior::kAimDuration, 0.01f);
//		DragF("Retreat speed", RiflemanBehavior::kRetreatSpeed, 0.002f);
//		DragF("Retreat duration (s)", RiflemanBehavior::kRetreatDuration, 0.01f);
//		DragF("Retreat range", RiflemanBehavior::kRetreatRange, 0.05f);
//		DragF("Bayonet range", RiflemanBehavior::kBayonetRange, 0.05f);
//		DragF("Firing range", RiflemanBehavior::kFiringRange, 0.05f);
//	}
//}
//
//#else // Release: compiles to nothing
//
//void DebugMenu::Draw(GameScene*) {}
//
//#endif