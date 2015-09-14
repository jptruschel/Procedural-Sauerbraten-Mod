/* Implementation for Procedural Sections.
 *	A section is an individual cube, possibly with doors
 *	to other sections.
 *
 * Author:  Joao Paulo T Ruschel (joaopaulo.ruschel@gmail.com)
 *	2015
*/

#pragma region Includes

#include "cube.h"
#include "engine.h"
#include "game.h"

#pragma endregion

#pragma region Constants

const int DOOR_HEIGHT = 4;
const int DOOR_WIDTH = 4;
const int DOOR_Z_PADDING = DEFAULT_GRID;

#pragma endregion

#pragma region Instantialization helpers

float* vectorFromPos(float x, float y, float z) {
	float *v = new float[3];
	v[0] = x;
	v[1] = y;
	v[2] = z;
	return v;
}

int inline oppositeDirection(int v) {
	if (v == 0) return 2;
	if (v == 1) return 3;
	if (v == 2) return 0;
	if (v == 3) return 1;
}

int inCompound(int index, compound_info &compound) {
	loopi(COMPOUND_MAX_SIZE) {
		if (compound.indexes[i] == index)
			return index;
	}
	return -1;
}
void addToCompound(int index, compound_info *compound) {
	int added = 0;
	loopi(COMPOUND_MAX_SIZE * 4) if (compound->indexes[i] == index) added = 1;
	if (added == 0) {
		compound->indexes[compound->count] = index;
		compound->count++;
	}
}

// Return room index or -1
int proceduralSection::doorAt(float x, float y, float z) {
	int i = 0;//(type == Spawn) ? 0 : 1;
	while (i < 4) {
		if (sqrt(pow(x - doors[i][0], 2) + pow(y - doors[i][1], 2) + pow(z - (doors[i][2] + DOOR_Z_PADDING), 2)) < 2 * DEFAULT_GRID) {
			return proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1]);
		}
		i++;
	}
	return -1;
}

int proceduralSection::isConnectedTo(int room) {
	int i = 0;
	while (i < 4) {
		if (proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1]) == room)
			return i;
		i++;
	}
	return -1;
}

#pragma endregion

#pragma region Section Generation and Instantialization

// main constructor
proceduralSection::proceduralSection() {
	indexes[0] = -1; indexes[1] = -1;
	loopi(4) connections[i] = None;
	compound = NULL;
	isGenerated = false;
	isInstantialized = false;
}

// initializes to a specific grid position
void proceduralSection::init(int nindex_x, int nindex_y) {
	indexes[0] = nindex_x; indexes[1] = nindex_y;
}

// generates this section - the first step
//	this will choose height and connections do adjecent sections
//	the choice of being the exit is NOT made at this stage
int proceduralSection::generate(int parent) {
	if (isGenerated) return 0;
	isGenerated = true;

	int thisIndex = proceduralManager::indexFrom(indexes[0], indexes[1]);

	pos[0] = indexes[0] * SECTION_SIZE*DEFAULT_GRID; 
	pos[1] = indexes[1] * SECTION_SIZE*DEFAULT_GRID; 
	pos[2] = 504;
	size[0] = SECTION_SIZE/2; size[1] = SECTION_SIZE/2;

	// look for generated sections surrounding it
	loopi(4) {
		int li = proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1]);
		if (li >= 0) {
			if (proceduralManager::sections[li].isGenerated) {
				connections[i] = proceduralManager::sections[li].connections[oppositeDirection(i)];
				if (connections[i] == NoWall) {
					compound = proceduralManager::sections[li].compound;
				}
			}
			else {
				connections[i] = None;		// no limitation
			}
		} else
			connections[i] = Wall;
	}

	if (compound == NULL) {
		compound = new compound_info;
		compound->count = 0;
		compound->indexes = new int[COMPOUND_MAX_SIZE * 4];
		loopi(COMPOUND_MAX_SIZE * 4) compound->indexes[i] = -1;
	}
	addToCompound(thisIndex, compound);

	int localCreatedSections = 1;

	// Spawn - only 1 door (which will not be instantialized in code)
	if (parent == -1) {
		type = Spawn;

		textures[0] = 1;
		textures[1] = 9;
		textures[2] = 24;

		size[2] = (int)(SECTION_SIZE / 1.5f);

		loopi(4) connections[i] = Wall;
		connections[0] = Door;
	}
	// Normal - random doors or compound to every direction
	if (parent >= 0) {
		type = Normal;

		if (compound != NULL && compound->count == 1)
		{
			textures[0] = rand() % TEX_FLOOR_COUNT;
			textures[1] = rand() % TEX_WALL_COUNT;
			textures[2] = rand() % TEX_CEILING_COUNT;

			size[2] = (int)(SECTION_MIN_HEIGHT + rand() % (SECTION_MAX_HEIGHT- SECTION_MIN_HEIGHT));
		}
		else 
		{
			textures[0] = proceduralManager::sections[compound->indexes[0]].textures[0];
			textures[1] = proceduralManager::sections[compound->indexes[0]].textures[1];
			textures[2] = proceduralManager::sections[compound->indexes[0]].textures[2];

			size[2] = proceduralManager::sections[compound->indexes[0]].size[2];
		}
		

		int hd = 0;
		// loops through connections that were not set yet
		//	Note: it is remotely possible that it failed to create the final trigger
		//	and it's impossible to create new sections. 
		//	In this case, the game should call game::endsp();
		loopi(4) if (connections[i] == None) {
			// makes sure no sections are generated outside the map
			if (pos[0] > (DEFAULT_GRID * 2) + 1 &&
				pos[1] > (DEFAULT_GRID * 2) + 1 &&
				pos[0] + (DEFAULT_GRID * 2) < (MAP_SIZE*DEFAULT_GRID) - 1 &&
				pos[1] + (DEFAULT_GRID * 2) < (MAP_SIZE*DEFAULT_GRID) - 1)
			{
				if ((rand() % 101 <= probability_door && hd == 0) ||
					(proceduralManager::progression < progression_min &&
						proceduralManager::unknownSections <= 3)) {
					connections[i] = Door;
					proceduralManager::unknownSections++;
					hd = 1;
				}
				else {
					if (rand() % 101 <= probability_compound && compound->count + localCreatedSections < COMPOUND_MAX_SIZE) {
						connections[i] = NoWall;
						localCreatedSections += proceduralManager::sections[proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1])].generate(thisIndex);
					}
					else
						connections[i] = Wall;
				}
			}
		}
	}

	doors = new float*[4];
	loopi(4) {
		doors[i] = vectorFromPos(
			pos[0] + (DIRECTIONS[i][0] * size[0] * DEFAULT_GRID),
			pos[1] + (DIRECTIONS[i][1] * size[1] * DEFAULT_GRID),
			pos[2]);
	}

	loopi(4) if (connections[i] == None) connections[i] = Wall;

	return localCreatedSections;
}

// initializes this section - the second step
//	this will instantialize entities, raise and paint walls, ceilings, floors, etc
//	will also generate all adjacent sections
//	aand will also choose if must be the exit section
int proceduralSection::instantialize() {
	if ((!isGenerated) || (isInstantialized)) return -1;
	isInstantialized = true;

	proceduralManager::progression++;	// advanced another section

	// Generate all neighboors
	loopi(4) {
		if (connections[i] == Door)
		{
			proceduralManager::sections[proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1])].generate(proceduralManager::indexFrom(indexes[0], indexes[1]));
		}
		if (connections[i] == NoWall) {
			proceduralManager::sections[proceduralManager::indexFrom(indexes[0] + DIRECTIONS[i][0], indexes[1] + DIRECTIONS[i][1])].instantialize();
		}
	}

	// once minimum progression has been reached, start increasing chance of exit
	if (proceduralManager::progression >= progression_min)
		probability_exit += probability_exit_inc;

	// calculate if this section must be exit section
	if (proceduralManager::hasCreatedEnd == false && probability_exit > 0 &&
		rand() % 101 <= probability_exit)
	{
		type = Exit;
		proceduralManager::hasCreatedEnd = true;
	}

	// if this is the last possible room and no exit was generated yet, make this exit
	if (type == Normal && proceduralManager::unknownSections <= 2 && 
		proceduralManager::hasCreatedEnd == false)
	{
		type = Exit;
		proceduralManager::hasCreatedEnd = true;
	}

	// update the probabilities
	proceduralManager::updateProbabilities();

	// Build container (walls, doors)
	//	only if that direction was not yet instantialized
	if (type == Normal || type == Exit) {
		selinfo sel;
		sel.grid = 8; sel.orient = 5;
		sel.cx = 0; sel.cxs = 2; sel.cy = 0, sel.cys = 2;
		sel.corner = 0;

		// floor
		sel.o.x = pos[0] - (size[0] * DEFAULT_GRID) + ((connections[3] == NoWall) ? 0 : DEFAULT_GRID);
		sel.o.y = pos[1] - (size[1] * DEFAULT_GRID) + ((connections[2] == NoWall) ? 0 : DEFAULT_GRID);
		sel.o.z = pos[2];
		sel.corner = 1;
		sel.cx = 0; sel.cxs = 2; sel.cy = 0; sel.cys = 8;
		sel.s.x = (2 * size[0]) - ((connections[1] == NoWall) ? 0 : ((connections[3] == NoWall) ? 0 : 1));
		sel.s.y = (2 * size[1]) - ((connections[0] == NoWall) ? 0 : ((connections[2] == NoWall) ? 0 : 1));
		sel.s.z = 1;
		sel.orient = 5;
		mpedittex(TEX_FLOOR_ID[textures[0]], 0, sel, false);

		loop(d, 4) {
			if (connections[d] == Wall || connections[d] == Door) {
				sel.orient = 5;
				sel.corner = 0;
				sel.cx = 0; sel.cxs = 2; sel.cy = 0, sel.cys = 2;
				int li = proceduralManager::indexFrom(indexes[0] + DIRECTIONS[d][0], indexes[1] + DIRECTIONS[d][1]);
				bool build = (li == -1 || (li >= 0 && proceduralManager::sections[li].isInstantialized == false));

				// wall
				sel.o.x = pos[0] + (CORNERS[d][0] * size[0] * DEFAULT_GRID);
				sel.o.y = pos[1] + (CORNERS[d][1] * size[1] * DEFAULT_GRID);
				sel.o.z = pos[2]; 
				sel.s.x = 1+ (CORNERS_DIR[d][0] * 2 * size[0]); sel.s.y = 1+(CORNERS_DIR[d][1] * 2 * size[1]); sel.s.z = 1;
				if (sel.s.x == 0) sel.s.x = 1; if (sel.s.y == 0) sel.s.y = 1;
				if (build) loop(m, SECTION_MAX_HEIGHT) { mpeditface(-1, 1, sel, true); };

				sel.orient = NORMAL_ORIENTATION[d];
				sel.o.z = pos[2] + size[2];
				sel.s.z = size[2];
				mpedittex(TEX_WALL_ID[textures[1]], 0, sel, false);
				sel.orient = 5;

				// door
				if (build && connections[d] == Door) {
					sel.o.x = doors[d][0]; sel.o.y = doors[d][1]; sel.o.z = doors[d][2] + (DOOR_HEIGHT * DEFAULT_GRID);
					sel.s.x = 1; sel.s.y = 1; sel.s.z = 1;
					switch (d) {
					case 1:
						sel.s.y = DOOR_WIDTH; sel.o.y -= (DOOR_WIDTH*0.5f*DEFAULT_GRID);
						proceduralManager::createDoorAt(doors[d][0] + (DEFAULT_GRID / 2), doors[d][1], doors[d][2] + DOOR_Z_PADDING, 90);
						break;
					case 3:
						sel.s.y = DOOR_WIDTH; sel.o.y -= (DOOR_WIDTH*0.5f*DEFAULT_GRID);
						proceduralManager::createDoorAt(doors[d][0] + (DEFAULT_GRID / 2), doors[d][1], doors[d][2] + DOOR_Z_PADDING, 90);
						break;
					case 0:
						sel.s.x = DOOR_WIDTH; sel.o.x -= (DOOR_WIDTH*0.5f*DEFAULT_GRID);
						proceduralManager::createDoorAt(doors[d][0], doors[d][1] + (DEFAULT_GRID/2), doors[d][2] + DOOR_Z_PADDING, 0);
						break;
					case 2:
						sel.s.x = DOOR_WIDTH; sel.o.x -= (DOOR_WIDTH*0.5f*DEFAULT_GRID);
						proceduralManager::createDoorAt(doors[d][0], doors[d][1] + (DEFAULT_GRID / 2), doors[d][2] + DOOR_Z_PADDING, 0);
						break;
					}
					loop(m, DOOR_HEIGHT) { mpeditface(1, 1, sel, true); };
				}

			}
		}
		// ceiling
		sel.o.x = pos[0] - (size[0] * DEFAULT_GRID);
		sel.o.y = pos[1] - (size[1] * DEFAULT_GRID);
		sel.o.z = pos[2] + (size[2] * DEFAULT_GRID);
		sel.corner = 1;
		sel.cx = 0; sel.cxs = 2; sel.cy = 0; sel.cys = 8;
		sel.s.x = 2 * size[0]; sel.s.y = 1; sel.s.z = 1;
		sel.orient = 3;
		loop(m, size[1]*2) { mpeditface(-1, 1, sel, true); };
		sel.orient = 4;
		loop(m, size[1]*2) { mpedittex(TEX_CELIING_ID[textures[2]], 0, sel, false); sel.o.y -= DEFAULT_GRID; };

		// covers
		loopi(2)
		{
			sel.o.x = pos[0] + ((((rand() % 2) * 2) - 1) * (0.5f * size[0] * DEFAULT_GRID));
			sel.o.y = pos[1] + ((((rand() % 2) * 2) - 1) * (0.5f * size[1] * DEFAULT_GRID));
			sel.o.z = pos[2];
			int d = rand() % 4;
			sel.s.x = CORNERS_DIR[d][0] * size[0] * 0.5f;
			sel.s.y = CORNERS_DIR[d][1] * size[1] * 0.5f;
			sel.s.z = 1;
			if (sel.s.x == 0) sel.s.x = 1; if (sel.s.y == 0) sel.s.y = 1;
			sel.orient = 5;
			loop(m, 2) { mpeditface(-1, 1, sel, true); };
		}

	}

	// instantialize all compound
	loopi(compound->count) {
		proceduralManager::sections[compound->indexes[i]].instantialize();
		loopj(proceduralManager::sections[compound->indexes[i]].compound->count) {
			if (proceduralManager::sections[proceduralManager::sections[compound->indexes[i]].compound->indexes[j]].instantialize() == 0)
				proceduralManager::unknownSections--;
		}
	}

	// Spawns
	// Items and monsters (normal sections only)
	if (type == Normal) {
		int s = 0;
		// items (bonus probability if room with only 1 door)
		int bonus_item_prob = 30;
		bool d = false;
		loopi(4) { 
			if (connections[i] == Door) { 
				if (d) 
					bonus_item_prob = 0; 
				else 
					d = true; 
			} 
		}
		while (rand() % 101 <= item_spawn_probability && 
			s < items_max_persection) {
			int i = proceduralManager::sampleDistribution(ITEMS_COUNT, proceduralManager::itemsDist);
			proceduralManager::createEntityAt(ITEMS_INDEXES[i],
				pos[0] + (rand() % (int)((size[0] * 0.75f * DEFAULT_GRID))) - ((size[0]+1) * DEFAULT_GRID * 0.75f)+ DEFAULT_GRID,
				pos[1] + (rand() % (int)((size[1] * 0.75f * DEFAULT_GRID))) - ((size[1]+1) * DEFAULT_GRID * 0.75f)+ DEFAULT_GRID,
				pos[2] + (2*DEFAULT_GRID));
			s++;
		}
		// monsters
		s = 0;
		while (rand() % 101 <= monster_spawn_probability && 
			s < monsters_max_persection) {
			int i = proceduralManager::sampleDistribution(MONSTERS_COUNT, proceduralManager::monsterDist);
			proceduralManager::createEnemyAt(i,
				pos[0] + (rand() % (int)((size[0] * 0.75f * DEFAULT_GRID))) - ((size[0]+1) * DEFAULT_GRID * 0.75f) + DEFAULT_GRID,
				pos[1] + (rand() % (int)((size[1] * 0.75f * DEFAULT_GRID))) - ((size[1]+1) * DEFAULT_GRID * 0.75f) + DEFAULT_GRID,
				pos[2] + (2 * DEFAULT_GRID));
			s++;
		}
	}

	// End trigger (exit only)
	if (type == Exit)
	{
		proceduralManager::createEndOflevel(
			pos[0], pos[1], pos[2] + DEFAULT_GRID);
	}

	return 0;
}

/*char str[120];
sprintf(str, "newentity: %i, %i, %i, %i, %i, %i ; pos: %f, %f, %f", type, a1, a2, a3, a4, a5, pos.x, pos.y, pos.z);
conoutf(str);*/


#pragma endregion