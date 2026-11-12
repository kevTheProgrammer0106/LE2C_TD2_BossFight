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
///
/// TPS cursor behavior:
///  - Gameplay locked: cursor hidden and recentered; mouse delta rotates the camera.
///  - Gameplay free: Alt toggles to a visible, freely moving system cursor.
///  - UI: system cursor hidden but not locked; UIMenu draws its own cursor sprite.
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

	// TPS用の移動軸。x=右+, y=前+
	KamataEngine::Vector2 GetMoveAxis() const { return moveAxis_; }

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

	// TPSカメラ用の相対マウス移動量。ロック中のみ値が入る。
	KamataEngine::Vector2 GetMouseDelta() const { return mouseDelta_; }

	// ゲームプレイ中のみAltによるカーソルロック切り替えを許可する。
	// false: UI用(システムカーソル非表示・自由移動)、true: gameplay(初期ロック)。
	void SetCursorLockAllowed(bool allowed);
	bool IsDashTriggered() const { return dashTriggered_; } // Shift / B
private:
	InputManager() = default;
	~InputManager();
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	// カーソルとマウス相対移動量の管理
	void UpdateMouseCapture(bool lockCursor);
	void ReleaseMouseCapture();
	void SetSystemCursorVisible(bool visible);

	// movement
	bool moveLeftHeld_ = false;
	bool moveRightHeld_ = false;
	bool moveLeftTriggered_ = false;
	bool moveRightTriggered_ = false;
	KamataEngine::Vector2 moveAxis_ = {};

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

	// Cursor mode state
	bool lockCursor_ = false;
	bool cursorLockAllowed_ = false;
	bool cursorModeInitialized_ = false;
	bool systemCursorVisible_ = true;
	bool mouseCaptured_ = false;
	void* capturedWindow_ = nullptr;

	// Alt入力の連続切り替えを防ぐクールダウン(60fpsで約0.17秒)
	int cursorToggleCooldownFrames_ = 0;
	static constexpr int kCursorToggleCooldownFrames_ = 10;

	bool dashTriggered_ = false;

	// TPS camera input
	KamataEngine::Vector2 mouseDelta_ = {};

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
