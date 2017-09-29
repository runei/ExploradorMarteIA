#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <functional>
#include <memory>
#include <algorithm>

#define ROW_SIZE 10
#define COL_SIZE (ROW_SIZE * 2)

/*=========================================================================================*/

enum Type
{
	EMPTY,
	SAMPLE,
	OBSTACLE,
	AGENT,
	SPACE_SHIP
};

enum Direction
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	LAST_VALUE
};

struct Pos {
	int x;
	int y;

	Pos(int _x, int _y) : x(_x), y(_y) {};
	Pos() {};
	bool operator==(Pos a) { return a.x == x && a.y == y; };
	bool operator!=(Pos a) { return a.x != x && a.y != y; };
	Pos operator=(Pos a) { x = a.x; y = a.y; };
};

/*========================================================================================*/

#ifdef _WIN32

#include <windows.h>
#include <time.h>

void cls()
{

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	// Get the number of character cells in the current buffer. 

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
		dwConSize, coordScreen, &cCharsWritten))
		return;

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
		dwConSize, coordScreen, &cCharsWritten))
		return;

	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition(hConsole, coordScreen);
}

#else

#include <unistd.h>

void cls()
{
	std::cout << "\x1B[2J\x1B[H";
}

#endif 

void mySleep(int sleepMs)
{
#ifdef _WIN32
    Sleep(sleepMs);
#else
    usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
}

/*=========================================================================================*/

class Object {
	std::string display;
	Type type;
public:
	Object() : display(" "), type(Type::EMPTY) {}
	Object(std::string _d, Type _t) : display(_d), type(_t) {}
	Type getType() { return type; };
	void print();
};

void Object::print()
{
	std::cout << display;
}

/*=========================================================================================*/

class Agent : public Object {

	int n_samples;
public:
	Agent() : Object("A", Type::AGENT), n_samples(0) {};
	bool isLoaded() { return n_samples > 0; };
	int unload();
	void load(int n) { n_samples += n; };
	int getNSamples() { return n_samples; };
};

int Agent::unload()
{
	int result = n_samples;
	n_samples = 0;
	return result;
}

/*=========================================================================================*/

class Obstacle : public Object {
public:
	Obstacle() : Object("O", Type::OBSTACLE) {};
};

class Sample : public Object {
public:
	Sample() : Object("S", Type::SAMPLE) {}
};

class SpaceShip : public Object {
	int n_samples;
public:
	SpaceShip() : Object("N", Type::SPACE_SHIP) {}
	void addSamples(int n) { n_samples += n; };
	int getSamples() { return n_samples; };
};

/*=========================================================================================*/

class Planet {
	std::vector<std::vector<Object>> planet;	
	Pos agent_position;
	SpaceShip *space_ship;
	int agent_samples;
	std::default_random_engine rng;

	Pos getNewPos(Direction dir, Pos old_pos);
	Agent *getAgent();
	void moveAgent(Pos);
public:
	Planet(int _r, int _c);
	void print();
	void run();
};

void Planet::moveAgent(Pos new_pos)
{
	if (new_pos == agent_position ||
		planet[new_pos.x][new_pos.y].getType() == Type::SPACE_SHIP)
	{
		return;
	}
	planet[new_pos.x][new_pos.y] = planet[agent_position.x][agent_position.y];
	planet[agent_position.x][agent_position.y] = Object();
	agent_position = new_pos;
}

Agent *Planet::getAgent()
{
	return static_cast<Agent*> (&planet[agent_position.x][agent_position.y]);
}

Planet::Planet(int _r, int _c) : planet(_r, std::vector<Object>(_c, Object())), agent_samples(0)
{
	std::mt19937::result_type seed = time(0);
	auto random_ij = std::bind(std::uniform_int_distribution<int>(0, _c - 1), std::mt19937(seed));
	for (int i = 0; i < 11; ++i)
	{
		int r = random_ij() % _r;
		int c = random_ij();

		if (i < 5)
		{
			planet[r][c] = Sample();
		}
		else if (i < 10)
		{
			planet[r][c] = Obstacle();
		}
		else
		{
			planet[r][c] = Agent();
			agent_position = Pos(r, c);
		}
	}

	planet[_r-1][_c-1] = SpaceShip();
	space_ship = static_cast<SpaceShip*> (&planet[_r-1][_c-1]);
	rng = std::default_random_engine {};
}

void Planet::print()
{
	std::cout << "Amostras coletadas: " << space_ship->getSamples() << "\n\n";
	for (auto r : planet)
	{
		for (Object c : r)
		{
			c.print();
			// std::cout << "\t";
		}
		std::cout << std::endl;
	}
}

Pos Planet::getNewPos(Direction dir, Pos old_pos)
{
	Pos result = old_pos;
	switch (dir)
	{
		case Direction::LEFT: result.y--; break;
		case Direction::RIGHT: result.y++; break;
		case Direction::UP: result.x--; break;
		case Direction::DOWN: result.x++; break;
	}

	if (result.x < 0 || result.x >= planet.size() || result.y < 0 || result.y >= planet[0].size())
	{
		return old_pos;
	}
	return result;
}

void Planet::run()
{
	std::mt19937::result_type seed = time(0);
	auto rd = std::bind(std::uniform_int_distribution<int>(0, Direction::LAST_VALUE), std::mt19937(seed));

	Agent *agent = getAgent();
	Direction dir;
	std::vector<Direction> dirs = { Direction::RIGHT, Direction::DOWN };
	std::shuffle(std::begin(dirs), std::end(dirs), rng); 

	Pos new_pos;
	do
	{
		new_pos = agent_position;
		if (agent_samples && dirs.size() > 0)
		{
			while (dirs.size() > 0 && new_pos == agent_position)
			{
				new_pos = getNewPos(dirs.back(), agent_position);
				dirs.pop_back();
			} 
		}
		else 
		{
			Direction dir = static_cast<Direction>(rd());
			new_pos = getNewPos(dir, agent_position);
		}
	} while (planet[new_pos.x][new_pos.y].getType() == Type::OBSTACLE ||
			  new_pos == agent_position ||
			  (agent_samples && planet[new_pos.x][new_pos.y].getType() == Type::SAMPLE)); 
	
	if (agent_samples)
	{
		if (planet[new_pos.x][new_pos.y].getType() == Type::SPACE_SHIP)
		{
			space_ship->addSamples(agent_samples);
			agent_samples=0;
		}
		else
		{
			moveAgent(new_pos);
		}
	}
	else 
	{
		if (agent_position.x < planet.size()-1 && 
			planet[agent_position.x+1][agent_position.y].getType() == Type::SAMPLE)
		{
			new_pos = getNewPos(Direction::DOWN, agent_position);
		}
		else if (agent_position.x > 0 && 
				 planet[agent_position.x-1][agent_position.y].getType() == Type::SAMPLE)
		{
			new_pos = getNewPos(Direction::UP, agent_position);
		}
		else if (agent_position.y < planet[0].size()-1 && 
				 planet[agent_position.x][agent_position.y+1].getType() == Type::SAMPLE)
		{
			new_pos = getNewPos(Direction::RIGHT, agent_position);
		}
		else if (agent_position.y > 0 && 
				 planet[agent_position.x][agent_position.y-1].getType() == Type::SAMPLE)
		{
			new_pos = getNewPos(Direction::LEFT, agent_position);
		}
		if (planet[new_pos.x][new_pos.y].getType() == Type::SAMPLE)
		{
			agent_samples++;
		}
		moveAgent(new_pos);
	}
}

/*=========================================================================================*/

int main()
{
	Planet planet = Planet(ROW_SIZE, COL_SIZE);

	std::mt19937::result_type seed = time(0);
	auto rd = std::bind(std::uniform_int_distribution<int>(0, Direction::LAST_VALUE - 1), std::mt19937(seed));

	do {
		cls();
		planet.print();

		planet.run();
		mySleep(200);
	} while (true);

	// cls();

	return 0;
}