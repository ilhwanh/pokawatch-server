//
// CNH 2016.05.19
// unit class
//

#include "unit.h"
#include "resource.h"
#include "draw.h"
#include "spline.h"

void Unit::init() {
	switch (p.dep) {
	case DEP_CSE:  healthMax = 3; break;
	case DEP_PHYS: healthMax = 3; break;
	case DEP_LIFE: healthMax = 3; break;
	case DEP_ME:   healthMax = 5; break;
	case DEP_CHEM: healthMax = 3; break;
	default: error("Invalid department"); break;
	}

	moveStun = 0;
	p.health = healthMax;
	p.stun = 0;
	p.cooltime = 0;
	p.x = orgx;
	p.y = orgy;
}

Unit::Unit(int x, int y, protocol_team team) : Unit(team, "Unnamed unit") {}

Unit::Unit(int x, int y, protocol_team team, const char* name) {
	p.team = team;
	p.state = STATE_NULL;
	p.respawn = 1;
	orgx = x;
	orgy = y;
	p.x = orgx;
	p.y = orgy;
	death = 0;
	this->name = name;
}

void Unit::spawn(protocol_dep dep) {
	p.dep = dep;
}

void Unit::move(protocol_direction direction) {
	if (p.state == STATE_NULL)
		return;

	if (moveStun > 0) {
		error("Cannot move in consecutive turns");
		std::cout << moveStun << " turns left to move" << std::endl;
		return;
	}
	if (p.state == STATE_STUN) {
		error("Tried to move when stunned");
		return;
	}

	int dx = 0;
	int dy = 0;

	switch (direction) {
	case DIRECTION_RIGHT: dx = 1;  break;
	case DIRECTION_UP:    dy = 1;  break;
	case DIRECTION_LEFT:  dx = -1; break;
	case DIRECTION_DOWN:  dy = -1; break;
	default: error("Invalid input"); break;
	}

	p.x += dx;
	p.y += dy;
	moveOffPhase = 1.0;

	if (p.x < 0 || p.x >= MAP_WIDTH || p.y < 0 || p.y >= MAP_HEIGHT) {
		p.x -= dx;
		p.y -= dy;
		moveOffDirection = DIRECTION_NULL;
		error("Tried to move to the outside of the map");
	}
	else {
		moveOffDirection = direction;
		if (p.dep == DEP_ME) {
			moveStun = 2;
		}
	}
}

void Unit::attack(protocol_direction direction) {
	if (p.state == STATE_NULL)
		return;

	if (p.state == STATE_DEAD) {
		error("Tried to attack when dead");
		return;
	}
	if (p.state == STATE_STUN) {
		error("Tried to attack when stunned");
		return;
	}
	if (p.cooltime > 0) {
		error("Tried to attack in cooltime");
		return;
	}
	if (p.dep == DEP_ME) {
		error("ME cannot attack");
		return;
	}

	switch (direction) {
	case DIRECTION_RIGHT:	p.state = STATE_ATTACK_RIGHT;	break;
	case DIRECTION_UP:		p.state = STATE_ATTACK_UP;		break;
	case DIRECTION_LEFT:	p.state = STATE_ATTACK_LEFT;	break;
	case DIRECTION_DOWN:	p.state = STATE_ATTACK_DOWN;	break;
	default: std::cerr << name << ": Invalid input" << std::endl; break;
	}

	switch (p.dep) {
	case DEP_CSE:  p.cooltime = 6;  break;
	case DEP_PHYS: p.cooltime = 0;  break;
	case DEP_LIFE: p.cooltime = 10; break;
	case DEP_ME:   p.cooltime = 0;  break;
	case DEP_CHEM: p.cooltime = 4;  break;
	default: std::cerr << name << ": Invalid department" << std::endl; break;
	}
}

void Unit::skill(protocol_direction direction) {
	if (p.state == STATE_NULL)
		return;

	if (p.state == STATE_DEAD) {
		error("Tried to use hero skill when dead");
		return;
	}
	if (p.state == STATE_STUN) {
		error("Tried to use hero skill when stunned");
		return;
	}
	if (!p.hero) {
		error("Non-hero tried to use hero skill");
		return;
	}

	switch (direction) {
	case DIRECTION_RIGHT:	p.state = STATE_SKILL_RIGHT;	break;
	case DIRECTION_UP:		p.state = STATE_SKILL_UP;		break;
	case DIRECTION_LEFT:	p.state = STATE_SKILL_LEFT;	break;
	case DIRECTION_DOWN:	p.state = STATE_SKILL_DOWN;	break;
	default: error("Invalid input"); break;
	}

	p.hero = false;
}

void Unit::damage(int h) {
	if (p.state == STATE_NULL)
		return;

	p.health -= h;

	if (p.health <= 0) {
		p.state = STATE_DEAD;
		p.respawn = RESPAWN_COOLTIME;
	}
}

void Unit::heal(int h) {
	if (p.state == STATE_NULL)
		return;

	p.health += h;

	if (p.health > healthMax) {
		p.health = healthMax;
	}
}

void Unit::stun(int s) {
	if (p.state == STATE_NULL)
		return;

	p.stun = s;
	if (s > 0) {
		p.state = STATE_STUN;
	}
}

void Unit::turn() {
	if (p.state == STATE_NULL)
		return;

	p.state = STATE_IDLE;

	if (p.respawn > 0) {
		p.respawn--;
		if (p.respawn > 0) {
			p.state = STATE_DEAD;
			p.health = 0;
			return;
		}
		else {
			init();
		}
	}

	if (p.cooltime > 0) {
		p.cooltime--;
	}

	if (moveStun > 0) {
		moveStun--;
	}

	if (p.stun > 0) {
		p.stun--;
		if (p.stun > 0) {
			p.state = STATE_STUN;
			return;
		}
	}
}

void Unit::update() {
	if (p.state == STATE_NULL)
		return;

	// Animation by moving
	if (moveOffPhase > 0.0) {
		moveOffPhase -= DELTA_PER_TURN;
		
		float dx = 0.0, dy = 0.0;
		switch (moveOffDirection) {
		case DIRECTION_RIGHT: dx = -1.0; break;
		case DIRECTION_UP: dy = -1.0; break;
		case DIRECTION_LEFT: dx = 1.0; break;
		case DIRECTION_DOWN: dy = 1.0; break;
		}

		float aniPhase;
		if (moveOffPhase > 0.5)
			aniPhase = 1.0;
		else
			aniPhase = moveOffPhase * 2.0;

		float mag = Spline::accandfric(aniPhase);
		dx *= mag;
		dy *= mag;

		moveOffX = dx;
		moveOffY = dy;
		moveOffY += Spline::accjump(aniPhase) * 0.2;
	}
	else {
		moveOffPhase = 0.0;
		moveOffX = 0.0;
		moveOffY = 0.0;
	}
	// End of animation by moving
}

void Unit::draw() const {
	if (p.state == STATE_NULL)
		return;

	if (p.state == STATE_DEAD)
		return;

	float drawx = (float)p.x + moveOffX;
	float drawy = (float)p.y + moveOffY;

	switch (p.dep) {
	case DEP_CSE:
		Draw::onmap(Rspr::unitCSE, 0.0, drawx, drawy);
		break;
	case DEP_CHEM:
		Draw::onmap(Rspr::unitCHEM, 0.0, drawx, drawy);
		break;
	case DEP_ME:
		Draw::onmap(Rspr::unitME, 0, 0.0, drawx, drawy, 1.0, 1.0, 0.0, moveStun > 1 ? Color(1.0, 0.5, 0.5) : Color(1.0, 1.0, 1.0), 1.0);
		break;
	case DEP_LIFE:
		Draw::onmap(Rspr::unitLIFE, 0.0, drawx, drawy);
		break;
	case DEP_PHYS:
		Draw::onmap(Rspr::unitPHYS, 0.0, drawx, drawy);
		break;
	}

	float ddx = 20 / GUI_CELL_WIDTH;
	float dx = -(float)(p.health - 1) / 2.0 * ddx;

	for (int i = 0; i < p.health; i++) {
		Draw::onmap(Rspr::unitHeart, 2.0, drawx + dx, drawy + 2.0);
		dx += ddx;
	}
}

void Flag::turn() {

}

void Flag::update() {

}

void Flag::draw() const {
	Sprite& n = Rspr::flagNull;
	Sprite& f = Rspr::flagFlag;
	Draw::onmap(p.team == TEAM_NULL ? n : f, -1.0, (float)p.x, (float)p.y);
}

Poison::Poison() {
	p.valid = false;
	name = "Poison";
}

void Poison::spawn(protocol_team team, int x, int y) {
	p.team = team;
	p.x = x;
	p.y = y;
	p.span = 4;
	p.valid = true;
}

void Poison::turn() {
	if (!p.valid)
		return;

	if (p.span > 0) {
		p.span--;
	}
	else {
		p.valid = false;
	}
}

void Poison::update() {
	if (!p.valid)
		return;

}

void Poison::draw() const {
	if (!p.valid)
		return;

}

Petal::Petal() {
	p.valid = false;
	name = "Petal";
}

void Petal::spawn(protocol_team team, int x, int y, protocol_direction direction) {
	p.team = team;
	p.x = x;
	p.y = y;
	p.direction = direction;
	p.valid = true;
}

void Petal::turn() {
	if (!p.valid)
		return;

	int dx = 0;
	int dy = 0;

	switch (p.direction) {
	case DIRECTION_RIGHT: dx = 1;  break;
	case DIRECTION_UP:    dy = 1;  break;
	case DIRECTION_LEFT:  dx = -1; break;
	case DIRECTION_DOWN:  dy = -1; break;
	default: error("Invalid direction"); break;
	}

	p.x += dx;
	p.y += dy;

	if (p.x < 0 || p.x >= MAP_WIDTH || p.y < 0 || p.y >= MAP_HEIGHT) {
		p.valid = false;
	}
}

void Petal::update() {
	if (!p.valid)
		return;

}

void Petal::draw() const {
	if (!p.valid)
		return;

}

Mushroom::Mushroom() {
	p.valid = false;
	name = "Mushroom";
}

void Mushroom::spawn(protocol_team team, int x, int y) {
	p.team = team;
	p.x = x;
	p.y = y;
	p.valid = true;
}

void Mushroom::turn() {
	if (!p.valid)
		return;

}

void Mushroom::update() {
	if (!p.valid)
		return;

}

void Mushroom::draw() const {
	if (!p.valid)
		return;

}