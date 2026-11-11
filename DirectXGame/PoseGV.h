#pragma once

#include "Global.h"
#include "GlobalVariables.h"
#include <array>
#include <string>

/// SRTOffset / コンボパターンをGlobalVariablesへ登録・適用する共通ヘルパー。
/// キー名: <key>Scale / <key>Rotation / <key>Translation
namespace PoseGV {

// コンボ4キーフレームのキー名(DebugMenuと共有)
inline constexpr const char* const kComboPhaseKeys[4] = {"AntStart", "AntEnd", "AtkEnd", "RecEnd"};

inline void Register(const char* group, const std::string& key, const SRTOffset& v) {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	gv->AddItem(group, key + "Scale", v.scale);
	gv->AddItem(group, key + "Rotation", v.rotation);
	gv->AddItem(group, key + "Translation", v.translation);
}

inline void Apply(const char* group, const std::string& key, SRTOffset& v) {
	GlobalVariables* gv = GlobalVariables::GetInstance();
	v.scale = gv->GetVector3Value(group, key + "Scale");
	v.rotation = gv->GetVector3Value(group, key + "Rotation");
	v.translation = gv->GetVector3Value(group, key + "Translation");
}

// キー: <prefix><コンボ番号><AntStart|AntEnd|AtkEnd|RecEnd><Scale|Rotation|Translation>
inline void RegisterCombo(const char* group, const std::string& prefix, const std::array<ComboAttackPattern, 3>& patterns) {
	for (size_t i = 0; i < 3; ++i) {
		const SRTOffset* poses[4] = {&patterns[i].anticipateStart, &patterns[i].anticipateEnd, &patterns[i].attackEnd, &patterns[i].recoveryEnd};
		for (size_t k = 0; k < 4; ++k) {
			Register(group, prefix + std::to_string(i) + kComboPhaseKeys[k], *poses[k]);
		}
	}
}

inline void ApplyCombo(const char* group, const std::string& prefix, std::array<ComboAttackPattern, 3>& patterns) {
	for (size_t i = 0; i < 3; ++i) {
		SRTOffset* poses[4] = {&patterns[i].anticipateStart, &patterns[i].anticipateEnd, &patterns[i].attackEnd, &patterns[i].recoveryEnd};
		for (size_t k = 0; k < 4; ++k) {
			Apply(group, prefix + std::to_string(i) + kComboPhaseKeys[k], *poses[k]);
		}
	}
}

// std::array<float,3> <-> Vector3(リーチ/高さを1行で編集するため)
inline KamataEngine::Vector3 ToVec3(const std::array<float, 3>& a) { return {a[0], a[1], a[2]}; }
inline void FromVec3(std::array<float, 3>& a, const KamataEngine::Vector3& v) { a = {v.x, v.y, v.z}; }

} // namespace PoseGV