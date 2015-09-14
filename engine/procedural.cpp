/* Implementation for basic Procedural Content Generation.
*	Makes the bridge between Sauerbraten and this procedural algorithm.
*
* Author:  Joao Paulo T Ruschel (joaopaulo.ruschel@gmail.com)
*	2015
*/

#pragma region Includes

#include "cube.h"
#include "engine.h"
#include "game.h"

#pragma endregion

#pragma region Variable Initilization

proceduralSection* proceduralManager::sections;
int *proceduralManager::monsterDist, *proceduralManager::itemsDist;
int proceduralManager::progression, proceduralManager::unknownSections;
bool proceduralManager::hasCreatedEnd;

#pragma endregion

#pragma region Procedural Manager

// Called when the procedural map is actually loaded
//	Use this method to initialize stuff, taking into account the map
//	now exists.
void startProcedural() {
	// initialize all sections (not generate nor instantialize)
	proceduralManager::sections = new proceduralSection[SECTIONS_COUNT];
	loop(x, SECTIONS_LINE) {
		loop(y, SECTIONS_LINE) {
			proceduralManager::sections[proceduralManager::indexFrom(x, y)].init(x, y);
		}
	}
	// initialize global entity index
	dct = 10;
	// initialize random number generator with a new seed (now, in seconds)
	srand(time(NULL));

	// initialize progression counters
	proceduralManager::progression = 0;
	proceduralManager::unknownSections = 1;
	proceduralManager::hasCreatedEnd = false;
	probability_exit = 0;

	// initialize basic monster and item distribution
	proceduralManager::monsterDist = proceduralManager::setupDistribution(MONSTERS_COUNT, monsters_probability, 0, monsters_probability_progression);
	proceduralManager::itemsDist = proceduralManager::setupDistribution(ITEMS_COUNT, items_probability, 0, items_probability_noob);

	// generate and instantialize the center room
	//	note that this room is already saved on the OGZ file, and therefore
	//	will not be actually instantialized (built or painted)
	int index = proceduralManager::indexFrom(SECTIONS_LINE/2, SECTIONS_LINE/2);	// the center room
	proceduralManager::sections[index].generate(-1);
	proceduralManager::sections[index].instantialize();
}

// New binds to get player APM
//	Note that this overrides any custom binds
//	If custom key binds are necessary, another method must be used
//	Also note that all callbacks are executing the actual action they
//	 are designed to do. This is necessary since the actions are overriden.
void adaptive_callback_jump() {
	executestr("jump");
	apm_count_jump++;
}
void adaptive_callback_forward() {
	executestr("forward");
	apm_count_movement++;
}
void adaptive_callback_backward() {
	executestr("backward");
	apm_count_movement++;
}
void adaptive_callback_left() {
	executestr("left");
	apm_count_movement++;
}
void adaptive_callback_right() {
	executestr("right");
	apm_count_movement++;
}
void adaptive_callback_attack() {
	executestr("attack");
	apm_count_attack++;
}

// Called from main - initialize the procedural generation
//	Use this method to make pre-map initializations.
//	Note that, at this stage, the map was not loaded yet.
void proceduralManager::initCommands() {
	addcommand("procstart", (void(*)())startProcedural, "");

	// overrides basic player binds for custom callbacks
	addcommand("adcb_jump", (void(*)())adaptive_callback_jump, "");
	executestr("bind SPACE adcb_jump");
	executestr("bind MOUSE2 adcb_jump");
	addcommand("adcb_forward", (void(*)())adaptive_callback_forward, "");
	executestr("bind W adcb_forward");
	addcommand("adcb_backward", (void(*)())adaptive_callback_backward, "");
	executestr("bind S adcb_backward");
	addcommand("adcb_left", (void(*)())adaptive_callback_left, "");
	executestr("bind A adcb_left");
	addcommand("adcb_right", (void(*)())adaptive_callback_right, "");
	executestr("bind D adcb_right");
	addcommand("adcb_attack", (void(*)())adaptive_callback_attack, "");
	executestr("bind MOUSE1 adcb_attack");

	// for information about the player, use:
	//	player->
	//		o: current origin
	//		yaw/pitch: look rotation
	//	game::player1->
	//		health: the health (int)
}

// Callback from entities.cpp: a door has been oppened at this position
//	returns true if it successfully instantialized the sections
bool proceduralManager::openDoorAt(float x, float y, float z) {
	bool r = false;
	loop(d, SECTIONS_COUNT) {
		if (proceduralManager::sections[d].isInstantialized) {
			int ns = proceduralManager::sections[d].doorAt(x, y, z);
			if (ns > 0) {
				if (proceduralManager::sections[ns].instantialize() == 0)
				{
					proceduralManager::unknownSections--;
					r = true;
				}
				break;
			}
		}
	}
	return r;
}

// Callback from monsters.cpp: a monster has been killed
void proceduralManager::killedMonster()
{

}

// Update the probabilities based on:
//		current progression
//		player skill (with basic APM & accuracy)
//		player health
void proceduralManager::updateProbabilities()
{
	// put together some information about the player's performance
	int accuracy = 0;
	int hp = 100;
	if (game::player1 != NULL)
	{
		accuracy = (game::player1->totaldamage * 100) / ((game::player1->totalshots > 0) ? game::player1->totalshots : 1);
		hp = game::player1->health;
	}

	// calculate noobness and progression
	float noobness = 0.0f;
	noobness += (hp < 75) ? ((hp < 50) ? ((hp < 25) ? 2.0f : 1.0f) : 0.5f) : 0.0f;
	float prog = ((float)progression / (float)progression_min) + ((float)accuracy / 150.0f);

	// more items if noob or high action count (make sure has enough ammo)
	item_spawn_probability = item_spawn_probability_default + (int)(5*noobness) + (int)(0.2f*apm_count_attack);
	// more monsters if good, less monsters if bad
	monster_spawn_probability = monster_spawn_probability_default + (int)(8*prog) - (int)(10*noobness);

	// monster progression is solely based on level progression
	proceduralManager::monsterDist =
		proceduralManager::setupDistribution(MONSTERS_COUNT, monsters_probability,
			prog, monsters_probability_progression);
	// items to make the game easier are based on noobness
	proceduralManager::itemsDist = proceduralManager::setupDistribution(ITEMS_COUNT, items_probability,
		noobness, items_probability_noob);

	// reset apm counters
	apm_count_jump = 0;
	apm_count_movement = 0;
	apm_count_attack = 0;

	char str[40];
	sprintf(str, "Progression: %.2f ; Noobness: %.2f", prog, noobness);
	conoutf(str);
	sprintf(str, "Items: %i ; Monsters: %i", item_spawn_probability, monster_spawn_probability);
	conoutf(str);
}

// Get the section index in a specific point
int proceduralManager::sectionAt(float px, float py, float ax, float ay) {
	int s = 0;
	while (s < SECTIONS_COUNT) {
		if (proceduralManager::sections[s].isGenerated) {
			if (px + ax >= proceduralManager::sections[s].pos[0] - proceduralManager::sections[s].size[0] &&
				px - ax <= proceduralManager::sections[s].pos[0] + proceduralManager::sections[s].size[0] &&
				py + ay >= proceduralManager::sections[s].pos[1] - proceduralManager::sections[s].size[1] &&
				py - ay <= proceduralManager::sections[s].pos[1] + proceduralManager::sections[s].size[1])
				return s;
		}
		s++;
	}
	return -1;
}

// Gets the absolute array index from a 2D point
int inline proceduralManager::indexFrom(int ix, int iy) {
	if (ix >= 0 && iy >= 0 && ix < SECTIONS_LINE && iy < SECTIONS_LINE)
		return (ix * SECTIONS_LINE) + iy;
	else
		return -1;
}
// Gets the 2D point from a specific array index
int* proceduralManager::indexTo(int index) {
	int i[2];
	i[0] = index % SECTIONS_LINE;
	i[1] = index - i[0];
	return i;
}

#pragma endregion

#pragma region Entity Management

void proceduralManager::createDoorAt(float x, float y, float z, int angle) {
	vec epos;
	epos.x = x;
	epos.y = y;
	epos.z = z;
	newMapModelEntity(epos, angle, ENT_DOOR_1, ENT_TRIGGER_OPEN_ONCE, dct++, 0);
	//proceduralManager::unknownSections++;
}
void proceduralManager::createEndOflevel(float x, float y, float z) {
	vec epos;
	epos.x = x;
	epos.y = y;
	epos.z = z;
	newMapModelEntity(epos, 0, ENT_LEVER, ENT_TRIGGER_END_LEVEL, 0, 0);
}
void proceduralManager::createEntityAt(int type, float x, float y, float z) {
	vec *epos = new vec(x, y, z);
	newentityat(type, *epos, 0, 0, 0, 0, 0, dct++);
}
void proceduralManager::createEnemyAt(int monsterIndex, float x, float y, float z) {
	vec *epos = new vec(x, y, z);
	extentity *t = newentityat(MONSTER_TYPE_INDEX, *epos, monsterIndex, 0, 0, 0, 0, dct++);
	game::initNewMonster(*t);
}

#pragma endregion

#pragma region Discrete Distribution
// Methods for sampling numbers with associated probabilities and progressions
//	Call setupDistribution on the array of relative probabilities
//	Call sampleDistribution to get 1 sample from distribution,
//		note: not many properties are guaranteed!

int *proceduralManager::setupDistribution(int n, int *prob, float mult, int *prob2)
{
	int* distr = new int[n];
	if (n > 0)
	{
		distr[0] = prob[0] + (int)(mult*(float)prob2[0]);
		for (int i = 1; i < n; i++)
			distr[i] = distr[i - 1] + prob[i] + (int)(mult*(float)prob2[i]);
	}
	return distr;
}

int proceduralManager::sampleDistribution(int n, int* distr)
{
	int s = rand() % distr[n - 1] + 1;
	int i = 0;
	while (distr[i] < s)
		i++;
	return i;
}

#pragma endregion