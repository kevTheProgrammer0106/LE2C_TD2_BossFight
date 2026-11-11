#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>
#include <Xinput.h>

/// <summary>
/// 実際の入力を扱う唯一のクラス (キーボード + マウス + XInputコントローラー)。
/// KamataEngine::Inputへの直接アクセスはここに集約し、Player/敵/UIMenuなど他のシステムは
/// 全てこのクラス経由で入力状態を問い合わせる。
/// 毎フレーム先頭で1回だけUpdate()を呼び、そのフレーム中の問い合わせは全てキャッシュされた値を返す。
///
/// 既存API(IsMoveLeftHeld, IsJumpTriggered, IsPrimaryTriggered, IsBlockHeld ...)は
/// 全てコントローラー入力も含む。ゲームプレイ側はこれらを呼ぶだけでパッド対応になる。
///
/// ---- Default bindings ----
///  Move            : A / D / Left / Right      | Left stick / D-Pad
///  Jump            : Space                     | A
///  Attack / Fire   : Mouse Left                | RT
///  Block           : Mouse Right               | LT / LB
///  Weapon switch   : Q                         | Y
///  Interact        : F                         | X
///  Down attack / drop through : S              | Left stick down / D-Pad down
///  Pause           : Esc                       | Start
///  Aim             : Mouse position            | Right stick
///  Menu up/down    : W,S / Up,Down             | D-Pad / either stick
///  Menu confirm    : Space                     | A   (mouse click handled by UIMenu AABB)
/// </summary>
class InputManager {
public:
	static InputManager* GetInstance();

	void Update();

	// --- 移動 ---
	bool IsMoveLeftHeld() const { return moveLeftHeld_; }
	bool IsMoveRightHeld() const { return moveRightHeld_; }
	// -1.0 / 0.0 / 1.0 (左右同時押しは0)
	float GetMoveAxisX() const;

	// 旧名(エイリアス)
	bool PushLeftKey() const { return moveLeftHeld_; }
	bool PushRightKey() const { return moveRightHeld_; }
	bool TriggerLeftKey() const { return moveLeftTriggered_; }
	bool TriggerRightKey() const { return moveRightTriggered_; }

	// --- ジャンプ ---
	bool IsJumpHeld() const { return jumpHeld_; }           // Space / A
	bool IsJumpTriggered() const { return jumpTriggered_; } // 立ち上がり

	// --- アクション ---
	bool IsWeaponSwitchTriggered() const { return weaponSwitchTriggered_; } // Q / Y
	bool IsPauseTriggered() const { return pauseTriggered_; }               // Esc / Start
	bool IsInteractTriggered() const { return interactTriggered_; }         // F / X
	bool IsAttackUpHeld() const { return attackUpHeld_; }                   // W / stick up
	bool IsAttackDownHeld() const { return attackDownHeld_; }               // S / stick down (下攻撃 + ドロップスルー)

	// --- 攻撃 / 発砲 (マウス左 / RT) ---
	bool IsPrimaryTriggered() const { return primaryTriggered_; }
	bool IsPrimaryHeld() const { return primaryHeld_; }
	// 旧名(エイリアス)。Qは武器切替なので攻撃には含めない。
	bool TriggerAttackKey() const { return primaryHeld_; }
	bool TriggerAttackKeyEdge() const { return primaryTriggered_; }
	bool TriggerMouseLeft() const { return primaryMouseTriggered_; }

	// --- ガード (マウス右 / LT / LB) ---
	bool IsBlockHeld() const { return blockHeld_; }
	bool IsSecondaryTriggered() const { return secondaryTriggered_; }

	// --- 決定 ---
	// Space / マウス左 / A
	bool TriggerDecisionKey() const { return jumpHeld_ || primaryMouseHeld_; }
	bool TriggerDecisionKeyEdge() const { return jumpTriggered_ || primaryMouseTriggered_; }
	// Space / Controller A のみ(マウスクリックはUIMenuがAABBで判定)
	bool IsMenuConfirmTriggered() const { return jumpTriggered_; }

	// --- メニュー操作 ---
	bool IsMenuUpTriggered() const { return menuUpTriggered_; }
	bool IsMenuDownTriggered() const { return menuDownTriggered_; }
	bool IsMenuLeftTriggered() const { return menuLeftTriggered_; }
	bool IsMenuRightTriggered() const { return menuRightTriggered_; }

	// --- コントローラー ---
	bool IsControllerButtonTriggered(WORD button) const;
	bool IsControllerButtonHeld(WORD button) const;
	bool MoveStickUp() const { return menuUpTriggered_; }
	bool MoveStickDown() const { return menuDownTriggered_; }
	bool MoveStickLeft() const { return menuLeftTriggered_; }
	bool MoveStickRight() const { return menuRightTriggered_; }

	// 右スティックのエイム方向(デッドゾーン外のとき。x右+, y上+、長さ0〜1)
	KamataEngine::Vector2 GetAimStick() const { return aimStick_; }
	bool IsAimStickActive() const { return aimStickActive_; }

	// --- マウス ---
	int GetMouseX() const { return mouseX_; }
	int GetMouseY() const { return mouseY_; }
	KamataEngine::Vector2 GetMouseScreenPosition() const { return mouseMovement_; }

private:
	InputManager() = default;
	~InputManager() = default;
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	// movement
	bool moveLeftHeld_ = false;
	bool moveRightHeld_ = false;
	bool moveLeftTriggered_ = false;
	bool moveRightTriggered_ = false;

	bool jumpHeld_ = false;
	bool jumpTriggered_ = false;

	bool weaponSwitchTriggered_ = false;
	bool pauseTriggered_ = false;
	bool interactTriggered_ = false;
	bool attackUpHeld_ = false;
	bool attackDownHeld_ = false;

	// mouse
	KamataEngine::Vector2 mouseMovement_{};
	int mouseX_ = 0;
	int mouseY_ = 0;
	bool primaryMouseTriggered_ = false;
	bool primaryMouseHeld_ = false;
	bool secondaryMouseHeld_ = false;
	bool secondaryMouseTriggered_ = false;

	// combined (mouse | pad)
	bool primaryTriggered_ = false;
	bool primaryHeld_ = false;
	bool blockHeld_ = false;
	bool secondaryTriggered_ = false;

	bool menuUpTriggered_ = false;
	bool menuDownTriggered_ = false;
	bool menuLeftTriggered_ = false;
	bool menuRightTriggered_ = false;

	KamataEngine::Vector2 aimStick_{};
	bool aimStickActive_ = false;

	// controller
	XINPUT_STATE controllerState_{};
	XINPUT_STATE previousControllerState_{};

	static constexpr SHORT kStickDeadZone_ = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	static constexpr SHORT kAimDeadZone_ = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	static constexpr BYTE kTriggerThreshold_ = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
};