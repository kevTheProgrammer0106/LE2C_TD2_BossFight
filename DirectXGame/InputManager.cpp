#include "InputManager.h"

using namespace KamataEngine;

namespace {
constexpr SHORT kDead = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

bool Btn(const XINPUT_STATE& s, WORD b) { return (s.Gamepad.wButtons & b) != 0; }

// Left stick or D-Pad
bool PadLeft(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_LEFT); }
bool PadRight(const XINPUT_STATE& s) { return s.Gamepad.sThumbLX > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_RIGHT); }

// Menu navigation: either stick or D-Pad
bool MenuUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || s.Gamepad.sThumbRY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }
bool MenuDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || s.Gamepad.sThumbRY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool MenuLeft(const XINPUT_STATE& s) { return PadLeft(s) || s.Gamepad.sThumbRX < -kDead; }
bool MenuRight(const XINPUT_STATE& s) { return PadRight(s) || s.Gamepad.sThumbRX > kDead; }

// Gameplay down (left stick / D-Pad only, so the right/aim stick never triggers drop-through)
bool PadDown(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY < -kDead || Btn(s, XINPUT_GAMEPAD_DPAD_DOWN); }
bool PadUp(const XINPUT_STATE& s) { return s.Gamepad.sThumbLY > kDead || Btn(s, XINPUT_GAMEPAD_DPAD_UP); }
} // namespace

InputManager* InputManager::GetInstance() {
	static InputManager instance;
	return &instance;
}

void InputManager::Update() {
	Input* input = Input::GetInstance();

	// --- Controller snapshot (we keep our own previous frame so edges don't depend on the engine) ---
	previousControllerState_ = controllerState_;
	XINPUT_STATE polled{};
	if (input->GetJoystickState(0, polled)) {
		controllerState_ = polled;
	} else {
		controllerState_ = XINPUT_STATE{}; // disconnected: everything released
	}
	const XINPUT_STATE& cur = controllerState_;
	const XINPUT_STATE& prev = previousControllerState_;

	auto padEdge = [&](WORD b) { return Btn(cur, b) && !Btn(prev, b); };
	const bool rtNow = cur.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool rtPrev = prev.Gamepad.bRightTrigger > kTriggerThreshold_;
	const bool ltNow = cur.Gamepad.bLeftTrigger > kTriggerThreshold_;
	const bool ltPrev = prev.Gamepad.bLeftTrigger > kTriggerThreshold_;

	// --- Movement ---
	moveLeftHeld_ = input->PushKey(DIK_A) || input->PushKey(DIK_LEFT) || PadLeft(cur);
	moveRightHeld_ = input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT) || PadRight(cur);
	moveLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (PadLeft(cur) && !PadLeft(prev));
	moveRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (PadRight(cur) && !PadRight(prev));

	// --- Jump ---
	jumpHeld_ = input->PushKey(DIK_SPACE) || Btn(cur, XINPUT_GAMEPAD_A);
	jumpTriggered_ = input->TriggerKey(DIK_SPACE) || padEdge(XINPUT_GAMEPAD_A);

	// --- Actions ---
	weaponSwitchTriggered_ = input->TriggerKey(DIK_Q) || padEdge(XINPUT_GAMEPAD_Y);
	pauseTriggered_ = input->TriggerKey(DIK_ESCAPE) || padEdge(XINPUT_GAMEPAD_START);
	interactTriggered_ = input->TriggerKey(DIK_F) || padEdge(XINPUT_GAMEPAD_X);
	attackUpHeld_ = input->PushKey(DIK_W) || PadUp(cur);
	attackDownHeld_ = input->PushKey(DIK_S) || PadDown(cur);

	// --- Mouse ---
	mouseMovement_ = input->GetMousePosition();
	mouseX_ = static_cast<int>(mouseMovement_.x);
	mouseY_ = static_cast<int>(mouseMovement_.y);
	primaryMouseTriggered_ = input->IsTriggerMouse(0);
	primaryMouseHeld_ = input->IsPressMouse(0);
	secondaryMouseHeld_ = input->IsPressMouse(1);
	secondaryMouseTriggered_ = input->IsTriggerMouse(1);

	// --- Combined mouse + pad ---
	primaryTriggered_ = primaryMouseTriggered_ || (rtNow && !rtPrev);
	primaryHeld_ = primaryMouseHeld_ || rtNow;
	blockHeld_ = secondaryMouseHeld_ || ltNow || Btn(cur, XINPUT_GAMEPAD_LEFT_SHOULDER);
	secondaryTriggered_ = secondaryMouseTriggered_ || (ltNow && !ltPrev) || padEdge(XINPUT_GAMEPAD_LEFT_SHOULDER);

	// --- Menu navigation edges ---
	menuUpTriggered_ = input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP) || (MenuUp(cur) && !MenuUp(prev));
	menuDownTriggered_ = input->TriggerKey(DIK_S) || input->TriggerKey(DIK_DOWN) || (MenuDown(cur) && !MenuDown(prev));
	menuLeftTriggered_ = input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || (MenuLeft(cur) && !MenuLeft(prev));
	menuRightTriggered_ = input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || (MenuRight(cur) && !MenuRight(prev));

	// --- Right stick aim ---
	const float ax = static_cast<float>(cur.Gamepad.sThumbRX);
	const float ay = static_cast<float>(cur.Gamepad.sThumbRY);
	const float len = std::sqrt(ax * ax + ay * ay);
	if (len > static_cast<float>(kAimDeadZone_)) {
		aimStick_ = {ax / len, ay / len};
		aimStickActive_ = true;
	} else {
		aimStick_ = {0.0f, 0.0f};
		aimStickActive_ = false;
	}
}

float InputManager::GetMoveAxisX() const {
	if (moveLeftHeld_ == moveRightHeld_) {
		return 0.0f; // どちらも押していない、または両方押している
	}
	return moveRightHeld_ ? 1.0f : -1.0f;
}

bool InputManager::IsControllerButtonTriggered(WORD button) const { return Btn(controllerState_, button) && !Btn(previousControllerState_, button); }

bool InputManager::IsControllerButtonHeld(WORD button) const { return Btn(controllerState_, button); }