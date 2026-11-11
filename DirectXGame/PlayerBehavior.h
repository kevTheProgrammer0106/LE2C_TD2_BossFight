#pragma once

#include "BaseBehavior.h"

class Player;

// Player behavior placeholder: base only, intentionally empty.
// (Attack / block / knockback / lunge behaviors from the side-view game were NOT ported.)
class PlayerBehavior : public BaseBehavior {
public:
	explicit PlayerBehavior(Player* player) : player_(player) {}

private:
	Player* player_ = nullptr;
};
