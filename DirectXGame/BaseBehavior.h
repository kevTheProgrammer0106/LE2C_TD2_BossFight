#pragma once

// Empty behavior base. Nothing is ported from the old behaviors (player/enemy attack, block, knockback, AI...).
// A concrete behavior derives from this and overrides what it needs; every hook is a no-op by default.
class BaseBehavior {
public:
	virtual ~BaseBehavior() = default;

	virtual void Enter() {}  // called when the behavior becomes active
	virtual void Update() {} // called every frame while it is active
	virtual void Exit() {}   // called when it stops being active
};
