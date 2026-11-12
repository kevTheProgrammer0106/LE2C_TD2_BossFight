#pragma once

#define NOMINMAX

#include "Global.h"
#include <KamataEngine.h>
#include <Xinput.h>

/// <summary>
/// 実際の入力を扱う唯一のクラス（キーボード・マウス・XInputコントローラー）。
/// KamataEngine::Inputへの直接アクセスをこのクラスに集約し、Player／敵／UIMenuなどの各システムは
/// このクラス経由で入力状態を取得する。
/// 毎フレームの先頭でUpdate()を1回だけ呼び、そのフレーム中の問い合わせにはキャッシュした値を返す。
///
/// 既存API（IsMoveLeftHeld、IsJumpTriggered、IsPrimaryTriggered、IsBlockHeldなど）は
/// コントローラー入力にも対応している。ゲームプレイ側はこれらのAPIを呼ぶだけでパッド入力を扱える。
///
/// ---- 標準ボタン割り当て ----
///  移動              ：A / D / 左 / 右             | 左スティック / 十字キー
///  ジャンプ          ：Space                       | Aボタン
///  攻撃／射撃        ：マウス左                     | RT
///  ガード            ：マウス右                     | LT / LB
///  武器切り替え      ：Q                           | Yボタン
///  インタラクト      ：F                           | Xボタン
///  下攻撃／すり抜け  ：S                           | 左スティック下 / 十字キー下
///  ポーズ            ：Esc                         | Start
///  エイム            ：マウス座標                   | 右スティック
///  メニュー上下      ：W,S / 上,下                 | 十字キー / 左右いずれかのスティック
///  メニュー決定      ：Space                       | Aボタン（マウスクリックはUIMenuのAABBで判定）
///
/// TPSのカーソル操作：
///  ・ゲームプレイ中のロック状態：カーソルを非表示にして画面中央へ固定し、マウス移動量でカメラを回す。
///  ・ゲームプレイ中のフリー状態：AltでOSカーソルを表示し、自由に移動できるようにする。
///  ・フリー状態で右クリックを押している間：マウス移動量をカメラ旋回用として取得する。
///  ・UI操作中：OSカーソルは非表示・固定なし。UIMenu側で独自のカーソルスプライトを描画する。
/// </summary>
class InputManager {
public:
	static InputManager* GetInstance();

	void Update();

	// --- 移動 ---
	bool IsMoveLeftHeld() const { return moveLeftHeld_; }
	bool IsMoveRightHeld() const { return moveRightHeld_; }
	// -1.0 / 0.0 / 1.0（左右同時押しの場合は0）。
	float GetMoveAxisX() const;

	// TPS用の移動軸。xは右が正、yは前が正。
	KamataEngine::Vector2 GetMoveAxis() const { return moveAxis_; }

	// 旧名称（互換用エイリアス）。
	bool PushLeftKey() const { return moveLeftHeld_; }
	bool PushRightKey() const { return moveRightHeld_; }
	bool TriggerLeftKey() const { return moveLeftTriggered_; }
	bool TriggerRightKey() const { return moveRightTriggered_; }

	// --- ジャンプ ---
	bool IsJumpHeld() const { return jumpHeld_; }           // Space / Aボタン
	bool IsJumpTriggered() const { return jumpTriggered_; } // 押した瞬間

	// --- アクション ---
	bool IsWeaponSwitchTriggered() const { return weaponSwitchTriggered_; } // Q / Yボタン
	bool IsPauseTriggered() const { return pauseTriggered_; }               // Esc / Start
	bool IsInteractTriggered() const { return interactTriggered_; }         // F / Xボタン
	bool IsAttackUpHeld() const { return attackUpHeld_; }                    // W / スティック上
	bool IsAttackDownHeld() const { return attackDownHeld_; }                // S / スティック下（下攻撃＋すり抜け）

	// --- 攻撃／射撃（マウス左／RT） ---
	bool IsPrimaryTriggered() const { return primaryTriggered_; }
	bool IsPrimaryHeld() const { return primaryHeld_; }
	// 旧名称（互換用エイリアス）。Qは武器切り替えなので攻撃入力には含めない。
	bool TriggerAttackKey() const { return primaryHeld_; }
	bool TriggerAttackKeyEdge() const { return primaryTriggered_; }
	bool TriggerMouseLeft() const { return primaryMouseTriggered_; }

	// --- ガード（マウス右／LT／LB） ---
	bool IsBlockHeld() const { return blockHeld_; }
	bool IsSecondaryTriggered() const { return secondaryTriggered_; }

	// --- 決定 ---
	// Space / マウス左 / Aボタン
	bool TriggerDecisionKey() const { return jumpHeld_ || primaryMouseHeld_; }
	bool TriggerDecisionKeyEdge() const { return jumpTriggered_ || primaryMouseTriggered_; }
	// Space / コントローラーAのみ（マウスクリックはUIMenuがAABBで判定）。
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

	// 右スティックのエイム方向（デッドゾーン外の場合）。xは右が正、yは上が正、長さは0～1。
	KamataEngine::Vector2 GetAimStick() const { return aimStick_; }
	bool IsAimStickActive() const { return aimStickActive_; }

	// --- マウス ---
	int GetMouseX() const { return mouseX_; }
	int GetMouseY() const { return mouseY_; }
	KamataEngine::Vector2 GetMouseScreenPosition() const { return mouseMovement_; }

	// TPSカメラ用の相対マウス移動量。ロック中、またはフリーカーソル中の右クリック旋回時に値が入る。
	KamataEngine::Vector2 GetMouseDelta() const { return mouseDelta_; }

	// ゲームプレイ中のみAltによるカーソルロック切り替えを許可する。
	// false：UI用（OSカーソル非表示・自由移動）、true：ゲームプレイ用（初期状態はロック）。
	void SetCursorLockAllowed(bool allowed);
	bool IsDashTriggered() const { return dashTriggered_; } // Shift / Bボタン

	// ゲームプレイ中にAltでロックを解除している状態かどうか。
	bool IsFreeCursorMode() const { return cursorLockAllowed_ && !lockCursor_; }
	// フリーカーソル中にマウス右ボタンを押している状態（カメラ旋回中）。
	bool IsFreeOrbiting() const { return IsFreeCursorMode() && secondaryMouseHeld_; }

private:
	InputManager() = default;
	~InputManager();
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	// カーソル表示状態とマウスの相対移動量を管理する。
	void UpdateMouseCapture(bool lockCursor);
	void ReleaseMouseCapture();
	void SetSystemCursorVisible(bool visible);

	// 移動入力
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

	// マウス入力
	KamataEngine::Vector2 mouseMovement_{};
	int mouseX_ = 0;
	int mouseY_ = 0;
	bool primaryMouseTriggered_ = false;
	bool primaryMouseHeld_ = false;
	bool secondaryMouseHeld_ = false;
	bool secondaryMouseTriggered_ = false;

	// カーソルモードの状態
	bool lockCursor_ = false;
	bool cursorLockAllowed_ = false;
	bool cursorModeInitialized_ = false;
	bool systemCursorVisible_ = true;
	bool mouseCaptured_ = false;
	void* capturedWindow_ = nullptr;

	// フリーカーソルで右クリック旋回を開始した直後の急な移動量を防ぐための前回座標。
	KamataEngine::Vector2 previousFreeOrbitMousePosition_{};
	bool freeOrbitMouseInitialized_ = false;

	// Altによる連続切り替えを防ぐクールダウン（60fpsで約0.17秒）。
	int cursorToggleCooldownFrames_ = 0;
	static constexpr int kCursorToggleCooldownFrames_ = 10;

	bool dashTriggered_ = false;

	// TPSカメラ用のマウス移動量
	KamataEngine::Vector2 mouseDelta_ = {};

	// マウス入力とパッド入力を統合した状態
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

	// コントローラーの現在フレームと前フレームの状態
	XINPUT_STATE controllerState_{};
	XINPUT_STATE previousControllerState_{};

	static constexpr SHORT kStickDeadZone_ = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	static constexpr SHORT kAimDeadZone_ = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	static constexpr BYTE kTriggerThreshold_ = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
};
