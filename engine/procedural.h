/* Header for all Procedural Generation code.
 *	See procedural.cpp, proceduralsection.cpp
 * 
 * Author:  Joao Paulo T Ruschel (joaopaulo.ruschel@gmail.com)
 *	2015
*/

#ifndef __PROC_H__
#define __PROC_H__

#include "cube.h"
#include "engine.h"
#include "game.h"
#include <stdlib.h>
#include <time.h>

// Section Type
enum SectionType { Spawn=0, Normal, Exit };
enum SectionConnection { 
	Wall = 0,			// build a wall
	Door = 1,			// build a wall with a door in the middle
	NoWall = 2,			// build nothing (treat section as the same)
	None = 3,			// not computed yet
	NOP = 4				// don't do anything
};		

// A compound (list of sections)
struct compound_info {
	int count;
	int* indexes;
}typedef compound_info;

// Map and Section Sizes
const int MAP_SIZE = 128;
const int SECTION_MIN_HEIGHT = 5;
const int SECTION_MAX_HEIGHT = 10;
const int SECTION_SIZE = 10;
const int SECTIONS_LINE = MAP_SIZE / SECTION_SIZE;
const int SECTIONS_COUNT = SECTIONS_LINE * SECTIONS_LINE;
const int DEFAULT_GRID = 8;
const int COMPOUND_MAX_SIZE = 8;

// Indexed multipliers
//									+Y		+X			-Y			-X
static int DIRECTIONS[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
static int CORNERS[4][2] = { { -1, 1 }, { 1, -1 }, { -1, -1 }, { -1, -1 } };
//								TL			BR			BL			BL
static int CORNERS_DIR[4][2] = { { 1, 0 },{ 0, 1 },{ 1, 0 }, { 0, 1 } };

// Orientations
static int DIRECTION_ORIENTATION[4] = { 3, 1, 2, 0 };
static int NORMAL_ORIENTATION[4] = { 2, 0, 3, 1 };

// Global entity index
//	this number is incremented everytime a new entity is created
//	this is needed to make sure all entities are valid
static int dct = 10;

// Entities
static int ENT_DOOR_1 = 26;
static int ENT_DOOR_2 = 27;
static int ENT_DOOR_3 = 28;
static int ENT_LEVER = 24;
static int ENT_TRIGGER_OPEN_ONCE = 9;
static int ENT_TRIGGER_END_LEVEL = 29;

// Action count (for AP)
static int apm_count_jump = 0;
static int apm_count_movement = 0;
static int apm_count_attack = 0;

// Probabilities (all /100)
static int probability_door = 20;
static int probability_compound = 30;
static int item_spawn_probability = 50;
static int item_spawn_probability_default = 50;
static int items_max_persection = 6;
static int monster_spawn_probability = 60;
static int monster_spawn_probability_default = 60;
static int monsters_max_persection = 8;
static int progression_min = 6;			// min progression to end level
static int probability_exit = 0;
static int probability_exit_inc = 8;		// once progression has reached min, add prob by

// Elements
/*
8: shells
9: bullets
10: rockets
11: riflerounds
12: granades
13: cartridges
14: health
15: healthboost
16: armour
17: yellow armor
18: quaddamage
*/
const int ITEMS_COUNT = 11;
const int ITEMS_INDEXES[ITEMS_COUNT] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
static int items_probability[ITEMS_COUNT] = { 35, 25, 10, 10, 10, 5, 10, 5, 5, 5, 2 };
static int items_probability_noob[ITEMS_COUNT] = {10, 0, 10, 0, 0, 0, 20, 15, 10, 8, 8};

const int MONSTERS_COUNT = 9;
const int MONSTER_TYPE_INDEX = 21;
static int monsters_probability[MONSTERS_COUNT] = { 25, 20, 15, 4, 0, 12, 4, 12, 2 };
static int monsters_probability_progression[MONSTERS_COUNT] = { 2, 2, 5, 10, 6, 8, 10, 5, 10};

// Textures
const int TEX_FLOOR_COUNT = 6;
const int TEX_FLOOR_ID[TEX_FLOOR_COUNT] = { 2,3,4,5,6,7 };
const int TEX_WALL_COUNT = 11;
const int TEX_WALL_ID[TEX_WALL_COUNT] = { 9, 10, 11, 12, 13, 25, 26, 27, 31, 32, 33 };
const int TEX_CEILING_COUNT = 2;
const int TEX_CELIING_ID[TEX_CEILING_COUNT] = { 24, 19 };

// A single section
class proceduralSection {

	private:

	public:
		SectionType type;
		compound_info* compound;
		int indexes[2];		// x and y indexes
		float pos[3];
		float size[3];

		int textures[3];	// ground, walls, celiing
		
		SectionConnection connections[4];	// same order as DIRECTIONS
		float** doors;

		bool isGenerated;
		bool isInstantialized;

		proceduralSection();

		void proceduralSection::init(int nindex_x, int nindex_y);
		int proceduralSection::generate(int parent);
		int proceduralSection::instantialize();

		int proceduralSection::doorAt(float x, float y, float z);
		int proceduralSection::isConnectedTo(int room);
		
};

// Procedural map manager - singleton
static class proceduralManager {
private:
	
public:
	// All the sections
	//	Note: although its an unidimensional array, it is
	//	organized as a 2D section array.
	//	Always use indexFrom() and indexTo() to modify
	//	indexes in this array!
	static proceduralSection* sections;

	// current probability distribution for monster and items
	static int *monsterDist, *itemsDist;

	// current state of the procedural generation
	static int progression;			// player progression (instantialized sections)
	static int unknownSections;		// all generated, not instantialized, sections	
	static bool hasCreatedEnd;		// has created the end section

	static void proceduralManager::initCommands();
	static void proceduralManager::updateProbabilities();
	static bool proceduralManager::openDoorAt(float x, float y, float z);
	static void proceduralManager::killedMonster();

	static int proceduralManager::sectionAt(float px, float py, float ax, float ay);
	static int proceduralManager::indexFrom(int ix, int iy);
	static int* proceduralManager::indexTo(int index);

	static void createDoorAt(float x, float y, float z, int angle);
	static void createEndOflevel(float x, float y, float z);
	static void createEntityAt(int type, float x, float y, float z);
	static void createEnemyAt(int monsterIndex, float x, float y, float z);

	static int *setupDistribution(int n, int *prob, float mult, int *prob2);
	static int sampleDistribution(int n, int* distr);
};

#endif