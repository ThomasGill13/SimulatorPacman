/* INCLUDES */
//////////////////////////////////////////////////////////////

// C/C++ Standard Libraries
#include <cstdint>
#include <stdio.h>
#include <cstdio>
#include <cmath>
#include <stack>

// MBED Libraries
#include "mbed.h"
#include "stm32f413h_discovery_ts.h"
#include "stm32f413h_discovery_lcd.h"

/* DEFINES */
//////////////////////////////////////////////////////////////
// Direction States
#define NORTH 0x1
#define EAST 0x2
#define SOUTH 0x4
#define WEST 0x8

// Game Engine defines
#define MAX_GAME_OBJECTS 16 // Max number of objects that can be added to the game engine

// Maze size (in tiles)
#define WIDTH 28 // Due to the tile state in the 'x' axis being stored in a 32 bit int, this cannot be greater than 32
#define HEIGHT 30 // Should be 31 in  a classic game of Pacman

// Maze tile states
#define FLOOR true
#define WALL false

#define TILE_SIZE 8

// AI States
#define BLINKY_AI 1
#define PINKY_AI 2
#define INKY_AI 3
#define CLYDE_AI 4

// Main Game World States
#define SPLASH_SCREEN 0
#define MAIN_MENU 1 // Not used
#define STARTUP 2
#define PLAY 3
#define CONTINUE 4
#define NEXT_LEVEL 5
#define DEAD 6
#define GAME_OVER 7

/* GLOBALS */
//////////////////////////////////////////////////////////////
char CurGameState = SPLASH_SCREEN; // Stores the current state of the game
char NextGameState = SPLASH_SCREEN; // Stores what the next state will be
TS_StateTypeDef TS_State = { 0 }; // Stores the state of the touchscreen input

/* STRUCTS */
//////////////////////////////////////////////////////////////

// Stuct used to store co-ordinate values 
struct Position
{
	int x;
	int y;
};

/* BASE GAME CLASS H */
//////////////////////////////////////////////////////////////

// This class is designed to be inherited by all objects used in the game engine
class BaseGameClass
{
public:
	Position position; // Stores the coordinate to draw the object on the screen
	bool Updating; // When true, the object's "Update" function will be called in the main game engine loop
	bool Visible; // When true, the object's "Draw" function will be called in the main game engine loop
    bool Destroy; // Used as a flag to remove the object from the game engine

    // Constructs the object, setting its position to (0, 0) and "Updating" and "Visible" flags to true
    BaseGameClass();

    // Constructs the object, setting its position to (x, y) and "Updating" and "Visible" flags to true
	BaseGameClass(int x, int y);

    // Virtual function to be overwritten by child classes
    // Called in the main game engine "Init()"
	virtual void Init();

    // Virtual function to be overwritten by child classes
    // Called in the main game engine "Update()"
	virtual void Update();

    // Virtual function to be overwritten by child classes
    // Called in the main game engine "Draw()"
	virtual void Draw();
};

/* BASE GAME CLASS CPP */
//////////////////////////////////////////////////////////////

// Constructs the object, setting its position to (0, 0) and "Updating" and "Visible" flags to true
BaseGameClass::BaseGameClass()
{
    Updating = true;
    Visible = true;
    Destroy = false;
	position.x = 0;
	position.y = 0;
}

// Constructs the object, setting its position to (x, y) and "Updating" and "Visible" flags to true
BaseGameClass::BaseGameClass(int x, int y)
{
    Updating = true;
    Visible = true;
    Destroy = false;
	position.x = x;
	position.y = y;
}

// Virtual function to be overwritten by child classes
// Called in the main game engine "Init()"
void BaseGameClass::Init() {}

// Virtual function to be overwritten by child classes
// Called in the main game engine "Update()"
void BaseGameClass::Update() {}

// Virtual function to be overwritten by child classes
// Called in the main game engine "Draw()"
void BaseGameClass::Draw() {}

/* BASE GAME SPRITE H */
//////////////////////////////////////////////////////////////

// This class is designed to be inherited by all sprite based objects used in the game engine
// Inherits from "BaseGameClass"
// Has extra functions for handling collisions and drawing basic single colour images stored as 1D char arrays
class BaseGameSprite :
	public BaseGameClass
{
// Protected variables/functions are accessible from child classes
protected:
    Position _startPosition; // Stores the initial position of the object (used for resetting the position)

    // Sets the object's position to "_startPosition"
    void MoveToStartPosition();

    // Moves the object one pixel in the given direction
	void UpdatePosition(char direction);

    // Draws a simple single colour image stored in a 1D char array to the object's current position
    // Array accessed like a 2D array, where each bit of the char stores whether the pixel should be drawn or not 
    void DrawSprite(char spriteImageArray[], uint16_t colour);

    // Draws a simple single colour image stored in a 1D char array to the object's current position
    // Array accessed like a 2D array, where each bit of the char stores whether the pixel should be drawn or not
    // The input image will be flipped horizontally on the display
    void DrawSpriteFlippedHorizontal(char spriteImageArray[], uint16_t colour);

    // Draws a simple single colour image stored in a 1D char array to the object's current position
    // Array accessed like a 2D array, where each bit of the char stores whether the pixel should be drawn or not
    // The input image will be rotated anti-clockwise 90 degrees on the display
    void DrawSpriteRotated90(char spriteImageArray[], uint16_t colour);

    // Draws a simple single colour image stored in a 1D char array to the object's current position
    // Array accessed like a 2D array, where each bit of the char stores whether the pixel should be drawn or not
    // The input image will be rotated anti-clockwise 270 degrees on the display
    void DrawSpriteRotated270(char spriteImageArray[], uint16_t colour);
public:
    // Constructs the object, setting its position to (x, y) and "Updating" and "Visible" flags to true
    // "_startPosition" will be set to (x, y)
	BaseGameSprite(int x, int y);

    // Checks if this object has collided with the object "sprite"
    // Collision is done using a simple bounding box algorithm, with the box dimensions of TILE_SIZE * TILE_SIZE
    bool HasCollided(BaseGameSprite *sprite);
};

/* BASE GAME SPRITE CPP */
//////////////////////////////////////////////////////////////
void BaseGameSprite::MoveToStartPosition()
{
    position = _startPosition;
}

void BaseGameSprite::UpdatePosition(char direction)
{
	if (direction == NORTH)
	{
		position.y--;
	}
	else if (direction == EAST)
	{
		position.x++;
	}
	else if (direction == SOUTH)
	{
		position.y++;
	}
	else if (direction == WEST)
	{
		position.x--;
	}

    // Teleport logic here
    if (position.x == 0)
    {
        position.x = (WIDTH - 1) * TILE_SIZE;
    }
    else if (position.x == (WIDTH - 1) * TILE_SIZE)
    {
        position.x = 0;
    }
}

void BaseGameSprite::DrawSprite(char *spriteImageArray, uint16_t colour)
{
    //BSP_LCD_SetTextColor(colour);
    for (int i = 0; i < TILE_SIZE; i++)
    {
        for (int j = 0; j < TILE_SIZE; j++)
        {
            // Get image bit
            if (((spriteImageArray[j] >> i) & 0x1))
            {
                BSP_LCD_DrawPixel(position.x + i, position.y + j, colour);
            }
            else 
            {
                //BSP_LCD_DrawPixel(position.x + i, position.y + j, LCD_COLOR_BLACK);
            }
        }
    }
}

void BaseGameSprite::DrawSpriteFlippedHorizontal(char spriteImageArray[], uint16_t colour)
{
    for (int i = 0; i < TILE_SIZE; i++)
    {
        for (int j = 0; j < TILE_SIZE; j++)
        {
            // Get image bit
            if (((spriteImageArray[j] >> i) & 0x1))
            {
                BSP_LCD_DrawPixel(position.x + (TILE_SIZE - 1 - i), position.y + j, colour);
            }
            else 
            {
                //BSP_LCD_DrawPixel(position.x + (TILE_SIZE - 1 - i), position.y + j, LCD_COLOR_BLACK);
            }
        }
    }
}

void BaseGameSprite::DrawSpriteRotated90(char spriteImageArray[], uint16_t colour)
{
    //BSP_LCD_SetTextColor(colour);
    for (int i = 0; i < TILE_SIZE; i++)
    {
        for (int j = 0; j < TILE_SIZE; j++)
        {
            // Get image bit
            if (((spriteImageArray[i] >> j) & 0x1))
            {
                BSP_LCD_DrawPixel(position.x + i, position.y + j, colour);
            }
            else 
            {
                //BSP_LCD_DrawPixel(position.x + i, position.y + j, LCD_COLOR_BLACK);
            }
        }
    }
}

void BaseGameSprite::DrawSpriteRotated270(char spriteImageArray[], uint16_t colour)
{
    //BSP_LCD_SetTextColor(colour);
    for (int i = 0; i < TILE_SIZE; i++)
    {
        for (int j = 0; j < TILE_SIZE; j++)
        {
            // Get image bit
            if (((spriteImageArray[i] >> (TILE_SIZE - 1 - j)) & 0x1))
            {
                BSP_LCD_DrawPixel(position.x + i, position.y + j, colour);
            }
            else 
            {
                //BSP_LCD_DrawPixel(position.x + i, position.y + j, LCD_COLOR_BLACK);
            }
        }
    }
}

BaseGameSprite::BaseGameSprite(int x, int y) : BaseGameClass(x, y)
{
    _startPosition.x = x;
    _startPosition.y = y;
}

bool BaseGameSprite::HasCollided(BaseGameSprite *sprite)
{
    return position.x < sprite->position.x + TILE_SIZE && position.x + TILE_SIZE > sprite->position.x && position.y < sprite->position.y + TILE_SIZE && position.y + TILE_SIZE > sprite->position.y;
}

/* GAME ENGINE H */
//////////////////////////////////////////////////////////////
class GameEngine
{
private:
	BaseGameClass* _GameObjects[MAX_GAME_OBJECTS];
	int _GameObjectCount;

	void Init();

	void Update();

	void Draw();

public:
	GameEngine();

	void AddGameObject(BaseGameClass* gameObject);

	void MainGameLoop();
};

/* GAME ENGINE CPP */
//////////////////////////////////////////////////////////////
void GameEngine::Init()
{
	for (int i = 0; i < _GameObjectCount; i++)
	{
		_GameObjects[i]->Init();
	}
}

void GameEngine::Update()
{
	for (int i = 0; i < _GameObjectCount; i++)
	{
		if (_GameObjects[i]->Updating)
		{
			_GameObjects[i]->Update();
		}
	}
}

void GameEngine::Draw()
{
	for (int i = 0; i < _GameObjectCount; i++)
	{
		if (_GameObjects[i]->Visible)
		{
			_GameObjects[i]->Draw();
		}
	}
}

GameEngine::GameEngine()
{
    _GameObjectCount = 0;
}

void GameEngine::AddGameObject(BaseGameClass* gameObject)
{
	_GameObjects[_GameObjectCount] = gameObject;
	_GameObjectCount++;
}

void GameEngine::MainGameLoop()
{
	Init();

	while (true)
	{
        BSP_TS_GetState(&TS_State);

		Update();
		Draw();

        CurGameState = NextGameState;

        wait_ms(10);
	}
}

/* MAZE H */
//////////////////////////////////////////////////////////////
class Maze :
	public BaseGameClass
{
private:
	//bool _maze[WIDTH][HEIGHT]; // 2D array of bools used to store the maze
    bool _initialDraw;

    int _maze[HEIGHT]; // Used like a 2D array to store the maze. Each bit of the int stores whether the tile is a floor or a wall
    int _pellets[HEIGHT]; // Used like a 2D array to store if there is a pellet on a given square

    void SetFloor(int x, int y);

    void SetWall(int x, int y);

    void SetTestMaze();

    void SetClassicMaze();

    void SetPelletsClassicMaze();

    void DrawTile(int x, int y);

    int GetPelletCount();
public:
    std::stack<Position> redrawStack;

    int maxPellets;

	Maze(int x, int y);

    bool IsInBounds(int x, int y);

	bool IsFloor(int x, int y);

	bool IsFloorAdjecent(int x, int y, char direction);

	bool IsFloorAdjecent(Position position, char direction);

    bool IsFloorAdjecentScreenPos(int x, int y, char direction);

    bool IsFloorAdjecentScreenPos(Position screenPos, char direction);

    bool IsPellet(int x, int y);

    bool TryRemovePellet(int x, int y);

    bool TryRemovePelletScreenPos(Position screenPos);

    Position ScreenPosToTilePos(int x, int y);

    Position ScreenPosToTilePos(Position screenPos);

    void Update();

	void Draw();
};

/* MAZE CPP */
//////////////////////////////////////////////////////////////
void Maze::SetFloor(int x, int y)
{
	//_maze[x][y] = FLOOR;
	_maze[y] |= 0x1 << x; // Set the x'th bit
}

void Maze::SetWall(int x, int y)
{
	//_maze[x][y] = WALL;
	_maze[y] &= ~(0x1 << x); // Clear the x'th bit
}

void Maze::SetTestMaze()
{
	for (int j = 0; j < HEIGHT; j++)
	{
		for (int i = 0; i < WIDTH; i++)
		{
			if (j % 2 == 1 || i % 4 == 3)
			{
				SetFloor(i, j);
			}
			else
			{
				SetWall(i, j);
			}
		}
	}
}

void Maze::SetClassicMaze()
{
    _maze[0] = 0x0;
    _maze[1] = 0x0;
    _maze[2] = 0x7FF9FFE;
    _maze[3] = 0x4209042;
    _maze[4] = 0x4209042;
    _maze[5] = 0x4209042;
    _maze[6] = 0x7FFFFFE;
    _maze[7] = 0x4240242;
    _maze[8] = 0x4240242;
    _maze[9] = 0x7E79E7E;
    _maze[10] = 0x0209040;
    _maze[11] = 0x0209040;
    _maze[12] = 0x027FE40;
    _maze[13] = 0x0240240;
    _maze[14] = 0xFFC03FF;
    _maze[15] = 0x0240240;
    _maze[16] = 0x027FE40;
    _maze[17] = 0x0240240;
    _maze[18] = 0x0240240;
    _maze[19] = 0x7FF9FFE;
    _maze[20] = 0x4209042;
    _maze[21] = 0x4209042;
    _maze[22] = 0x73FFFCE;
    _maze[23] = 0x1240248;
    _maze[24] = 0x1240248;
    _maze[25] = 0x7E79E7E;
    _maze[26] = 0x4009002;
    _maze[27] = 0x4009002;
    _maze[28] = 0x7FFFFFE;
    _maze[29] = 0x0;
}

void Maze::SetPelletsClassicMaze()
{
    _pellets[0] = 0x0;
    _pellets[1] = 0x0;
    _pellets[2] = 0x7FF9FFE;
    _pellets[3] = 0x4209042;
    _pellets[4] = 0x4209042;
    _pellets[5] = 0x4209042;
    _pellets[6] = 0x7FFFFFE;
    _pellets[7] = 0x4240242;
    _pellets[8] = 0x4240242;
    _pellets[9] = 0x7E79E7E;
    _pellets[10] = 0x0200040;
    _pellets[11] = 0x0200040;
    _pellets[12] = 0x0200040;
    _pellets[13] = 0x0200040;
    _pellets[14] = 0x0200040;
    _pellets[15] = 0x0200040;
    _pellets[16] = 0x0200040;
    _pellets[17] = 0x0200040;
    _pellets[18] = 0x0200040;
    _pellets[19] = 0x7FF9FFE;
    _pellets[20] = 0x4209042;
    _pellets[21] = 0x4209042;
    _pellets[22] = 0x73FDFCE;
    _pellets[23] = 0x1240248;
    _pellets[24] = 0x1240248;
    _pellets[25] = 0x7E79E7E;
    _pellets[26] = 0x4009002;
    _pellets[27] = 0x4009002;
    _pellets[28] = 0x7FFFFFE;
    _pellets[29] = 0x0;
}

int Maze::GetPelletCount()
{
    int count = 0;

    for (int i = 0; i < WIDTH; i++)
    {
        for (int j = 0; j < HEIGHT; j++)
        {
            count += IsPellet(i, j);
        }
    }

    return count;
}

Maze::Maze(int x, int y) : BaseGameClass(x, y)
{
    _initialDraw = true;
	//SetTestMaze();
	SetClassicMaze();
    SetPelletsClassicMaze();
    maxPellets = GetPelletCount();
}

bool Maze::IsInBounds(int x, int y)
{
    return x > -1 && x < WIDTH && y > -1 && y < HEIGHT;
}

bool Maze::IsFloor(int x, int y)
{
	//return _maze[x][y];
	return IsInBounds(x, y) && ((_maze[y] >> x) & 0x1);
}

bool Maze::IsFloorAdjecent(int x, int y, char direction)
{
	if (direction == NORTH && y > 0)
	{
		return IsFloor(x, y - 1);
	}
	else if (direction == EAST && x < WIDTH - 1)
	{
		return IsFloor(x + 1, y);
	}
	else if (direction == SOUTH && y < HEIGHT - 1)
	{
		return IsFloor(x, y + 1);
	}
	else if (direction == WEST && x > 0)
	{
		return IsFloor(x - 1, y);
	}
	else
	{
		return false;
	}
}

bool Maze::IsFloorAdjecent(Position position, char direction)
{
	return IsFloorAdjecent(position.x, position.y, direction);
}

bool Maze::IsFloorAdjecentScreenPos(int x, int y, char direction)
{

}

bool Maze::IsFloorAdjecentScreenPos(Position screenPos, char direction)
{
    Position adjecentPosA;
    Position adjecentPosB;

    if (direction == NORTH)
    {
        adjecentPosA.x = screenPos.x;
        adjecentPosA.y = screenPos.y - 1;

        adjecentPosB.x = screenPos.x + TILE_SIZE - 1;
        adjecentPosB.y = screenPos.y - 1;
    }
    else if (direction == EAST)
    {
        adjecentPosA.x = screenPos.x + TILE_SIZE;
        adjecentPosA.y = screenPos.y;

        adjecentPosB.x = screenPos.x + TILE_SIZE;
        adjecentPosB.y = screenPos.y + TILE_SIZE - 1;
    }
    else if (direction == SOUTH)
    {
        adjecentPosA.x = screenPos.x;
        adjecentPosA.y = screenPos.y + TILE_SIZE;

        adjecentPosB.x = screenPos.x + TILE_SIZE - 1;
        adjecentPosB.y = screenPos.y + TILE_SIZE;
    }
    else if (direction == WEST)
    {
        adjecentPosA.x = screenPos.x - 1;
        adjecentPosA.y = screenPos.y;

        adjecentPosB.x = screenPos.x - 1;
        adjecentPosB.y = screenPos.y + TILE_SIZE - 1;
    }

    Position tilePosA = ScreenPosToTilePos(adjecentPosA);
    Position tilePosB = ScreenPosToTilePos(adjecentPosB);

    return IsFloor(tilePosA.x, tilePosA.y) && IsFloor(tilePosB.x, tilePosB.y);
}

bool Maze::IsPellet(int x, int y)
{
	//return _maze[x][y];
	return IsInBounds(x, y) && ((_pellets[y] >> x) & 0x1);
}

bool Maze::TryRemovePellet(int x, int y) {
    bool hasPellet = IsPellet(x, y);

    if (hasPellet)
    {
        _pellets[y] &= ~(0x1 << x); // Clear the x'th bit
    }
    
    return hasPellet;
}

bool Maze::TryRemovePelletScreenPos(Position screenPos) {
  Position tilePos = ScreenPosToTilePos(screenPos);
  return TryRemovePellet(tilePos.x, tilePos.y);
}

Position Maze::ScreenPosToTilePos(int x, int y)
{
	int tileX = x / TILE_SIZE;
	int tileY = y / TILE_SIZE;
    
    Position tilePos;
    tilePos.x = tileX;
    tilePos.y = tileY;
    
    return tilePos;
	//return { tileX, tileY };
}

Position Maze::ScreenPosToTilePos(Position screenPos)
{
	return ScreenPosToTilePos(screenPos.x, screenPos.y);
}

void Maze::Update()
{
    switch (CurGameState) {
    case STARTUP:
        _initialDraw = true;
        Visible = true;
        SetPelletsClassicMaze();
        break;
    case CONTINUE:
        _initialDraw = true;
        break;
    case NEXT_LEVEL:
        _initialDraw = true;
        Visible = true;
        SetPelletsClassicMaze();
        break;
    case PLAY:
        break;
    case DEAD:
        break;
    default:
        Visible = false;
        break;
    }
}

void Maze::DrawTile(int x, int y)
{
    if (IsFloor(x, y) || !IsInBounds(x, y)) 
    {
        // BSP_LCD_DrawPixel(i, j, LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
        BSP_LCD_FillRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE - 1);

        if (IsInBounds(x, y) && IsPellet(x, y))
        {
            BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
            BSP_LCD_FillCircle((x * TILE_SIZE) + (TILE_SIZE / 2), (y * TILE_SIZE) + (TILE_SIZE / 2), 1);
        }
    } 
    else 
    {
        // BSP_LCD_DrawPixel(i, j, LCD_COLOR_BLUE);
        BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
        BSP_LCD_FillRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE - 1);
    }
}

void Maze::Draw()
{
    if (_initialDraw)
    {
        _initialDraw = false;

        // Print the maze to the console
        for (int j = 0; j < HEIGHT; j++) {
            for (int i = 0; i < WIDTH; i++) {
                DrawTile(i, j);
            }
        }
    }
    else 
    {
        while(!redrawStack.empty())
        {
            Position redrawPos = redrawStack.top();

            Position tilePos = ScreenPosToTilePos(redrawPos);

            // Redraw the surrounding tiles
            DrawTile(tilePos.x, tilePos.y);
            DrawTile(tilePos.x, tilePos.y + 1);
            DrawTile(tilePos.x + 1, tilePos.y);

            // BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
            // BSP_LCD_FillRect(redrawPos.x, redrawPos.y, TILE_SIZE, TILE_SIZE);
            redrawStack.pop();
        }
    }
}

/* PLAYER H */
//////////////////////////////////////////////////////////////
class Player : public BaseGameSprite
{
private:
	Maze* _maze;
	int _lives;
    int _score;
    int _level;

	char _nextDir;

    bool _mouthOpen;

    // 2D arrays to store simple, monocolour images
    char _closedMouthImage[TILE_SIZE];
    char _openMouthImage[TILE_SIZE];

    void SetImageArrays();

	void SetDirection();

public:
	char lastDir;

	Player(Maze* maze, int x, int y);

	void Init();

	void Update();

	void Draw();
};

/* PLAYER CPP */
//////////////////////////////////////////////////////////////
void Player::SetImageArrays()
{
    _closedMouthImage[0] = 0x18;
    _closedMouthImage[1] = 0x3C;
    _closedMouthImage[2] = 0x7E;
    _closedMouthImage[3] = 0xFF;
    _closedMouthImage[4] = 0xFF;
    _closedMouthImage[5] = 0x7E;
    _closedMouthImage[6] = 0x3C;
    _closedMouthImage[7] = 0x18;

    _openMouthImage[0] = 0x18;
    _openMouthImage[1] = 0x3C;
    _openMouthImage[2] = 0x7E;
    _openMouthImage[3] = 0xF0;
    _openMouthImage[4] = 0xE0;
    _openMouthImage[5] = 0x70;
    _openMouthImage[6] = 0x3E;
    _openMouthImage[7] = 0x18;
}

void Player::SetDirection()
{
    if(TS_State.touchDetected) {
        /* Get X and Y position of the first touch post calibrated */
        uint16_t x1 = TS_State.touchX[0];
        uint16_t y1 = TS_State.touchY[0];

        // Get difference in x and y of the player character and the player's input
        int xDiff = (position.x) - x1;
        int yDiff = (position.y) - y1;

        // Find which has direction the greatest absolute value
        if (abs(xDiff) >= abs(yDiff))
        {
            // More differece in the horizontal plane
            // Check if direction was EAST or WEST of the player
            if (xDiff < 0)
            {
                _nextDir = EAST;
            }
            else 
            {
                _nextDir = WEST;
            }
        }
        else 
        {
            // More differece in the vertical plane
            // Check if direction was NORTH or SOUTH of the player
            if (yDiff < 0)
            {
                _nextDir = SOUTH;
            }
            else
            {
                _nextDir = NORTH;
            }
        }
    }

}


Player::Player(Maze* maze, int x, int y) : BaseGameSprite(x * TILE_SIZE, y * TILE_SIZE)
{
	_maze = maze;
	_nextDir = 0x0;
	lastDir = EAST;
    _score = 0;
    _lives = 3;
    _level = 1;
    _mouthOpen = false;

    SetImageArrays();
}

void Player::Init()
{
    _score = 0;
    _lives = 10;
}

void Player::Update()
{
    switch (CurGameState) {
    case STARTUP:
        Visible = true;
        Init();
        MoveToStartPosition();
        _mouthOpen = false;

        if(TS_State.touchDetected) 
        {
            SetDirection();
            NextGameState = PLAY;
        }
        break;
    case CONTINUE:
        MoveToStartPosition();
        _mouthOpen = false;

        if(TS_State.touchDetected) 
        {
            SetDirection();
            NextGameState = PLAY;
        }
        break;
    case NEXT_LEVEL:
        MoveToStartPosition();

        if(TS_State.touchDetected) 
        {
            SetDirection();
            NextGameState = PLAY;
        }
        break;
    case PLAY:
        _maze->redrawStack.push(position);

        SetDirection();

        if (_maze->IsFloorAdjecentScreenPos(position, _nextDir))
        {
            UpdatePosition(_nextDir);
            _score += _maze->TryRemovePelletScreenPos(position);
            lastDir = _nextDir;
            _nextDir = 0x0;
        }
        else if (_maze->IsFloorAdjecentScreenPos(position, lastDir))
        {
            UpdatePosition(lastDir);
            _score += _maze->TryRemovePelletScreenPos(position);
        }

        if (_score == _maze->maxPellets * _level)
        {
            _level++;
            NextGameState = NEXT_LEVEL;
        }

        _mouthOpen = !_mouthOpen;

        break;
    case DEAD:
        NextGameState = CONTINUE;
        _lives--;
        printf("Score = %d\nLives = %d\n", _score, _lives);

        if (_lives == 0)
        {
            NextGameState = GAME_OVER;
        }
        break;
    case GAME_OVER:
        Visible = false;
        _level = 1;
    default:
        Visible = false;
        break;
    }
}

void Player::Draw()
{
    //BSP_LCD_DrawPixel(position.x, position.y, LCD_COLOR_YELLOW);
    // BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    // BSP_LCD_FillRect(position.x, position.y, TILE_SIZE, TILE_SIZE);

    if (!_mouthOpen)
    {
        DrawSprite(_closedMouthImage, LCD_COLOR_YELLOW);
    }
    else if (lastDir == NORTH)
    {
        DrawSpriteRotated90(_openMouthImage, LCD_COLOR_YELLOW);
    }
    else if (lastDir == WEST)
    {
        DrawSprite(_openMouthImage, LCD_COLOR_YELLOW);
    }
    else if (lastDir == SOUTH)
    {
        DrawSpriteRotated270(_openMouthImage, LCD_COLOR_YELLOW);
    }
    else if (lastDir == EAST)
    {
        DrawSpriteFlippedHorizontal(_openMouthImage, LCD_COLOR_YELLOW);
    }

    // Draw game state info at the top of the screen
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLUE);  

    char buffer[20];
    if (CurGameState == PLAY)
    {
        sprintf(buffer, "  LEVEL %d  SCORE %d  LIVES %d   ", _level, _score, _lives);
    }
    else 
    {
        sprintf(buffer, "     TOUCH SCREEN TO START...");
    }
    BSP_LCD_DisplayStringAtLine(0, (uint8_t *) buffer);
}

/* ENEMY H */
//////////////////////////////////////////////////////////////
class Enemy :
	public BaseGameSprite
{
private:
	Maze* _maze;
	Player* _player;
	Position _target;
	Enemy* _blinky;
	char _lastDir;
	char _nextDir;
	char _aiType;
    uint16_t _colour;

    char _horizontalLookImageA[TILE_SIZE];
    char _horizontalLookImageB[TILE_SIZE];

    char _northLookImageA[TILE_SIZE];
    char _northLookImageB[TILE_SIZE];

    char _southLookImageA[TILE_SIZE];
    char _southLookImageB[TILE_SIZE];

    bool _imageA;

    void SetImageArrays();

	int GetManhattanDist(int x0, int y0, int x1, int y1);

	int GetManhattanDist(Position start, Position target);

	int GetDistanceSq(int x0, int y0, int x1, int y1);

	int GetDistanceSq(int x0, int y0, Position target);

	void SetTargetToPlayer();

	void SetTargetInfrontOfPlayer();

	void SetTargetBlockPlayer();

	void SetTargetClyde();

	void SetTarget();

	void GetNextDir();

public:
	//char symbol = '~';

	Enemy(Maze* maze, Player* player, uint16_t colour, char aiType, int x, int y);

	Enemy(Maze* maze, Player* player, Enemy* blinky, uint16_t colour, char aiType, int x, int y);

	void Init();

	void Update();

	void Draw();
};

/* ENEMY CPP */
//////////////////////////////////////////////////////////////
void Enemy::SetImageArrays()
{
    _horizontalLookImageA[0] = 0x18;
    _horizontalLookImageA[1] = 0x3C;
    _horizontalLookImageA[2] = 0x7E;
    _horizontalLookImageA[3] = 0x6A;
    _horizontalLookImageA[4] = 0x6A;
    _horizontalLookImageA[5] = 0x7E;
    _horizontalLookImageA[6] = 0x7E;
    _horizontalLookImageA[7] = 0x2A;

    _horizontalLookImageB[0] = 0x18;
    _horizontalLookImageB[1] = 0x3C;
    _horizontalLookImageB[2] = 0x7E;
    _horizontalLookImageB[3] = 0x6A;
    _horizontalLookImageB[4] = 0x6A;
    _horizontalLookImageB[5] = 0x7E;
    _horizontalLookImageB[6] = 0x7E;
    _horizontalLookImageB[7] = 0x54;

    _northLookImageA[0] = 0x18;
    _northLookImageA[1] = 0x3C;
    _northLookImageA[2] = 0x5A;
    _northLookImageA[3] = 0x5A;
    _northLookImageA[4] = 0x7E;
    _northLookImageA[5] = 0x7E;
    _northLookImageA[6] = 0x7E;
    _northLookImageA[7] = 0x2A;

    _northLookImageB[0] = 0x18;
    _northLookImageB[1] = 0x3C;
    _northLookImageB[2] = 0x5A;
    _northLookImageB[3] = 0x5A;
    _northLookImageB[4] = 0x7E;
    _northLookImageB[5] = 0x7E;
    _northLookImageB[6] = 0x7E;
    _northLookImageB[7] = 0x54;

    _southLookImageA[0] = 0x18;
    _southLookImageA[1] = 0x3C;
    _southLookImageA[2] = 0x5A;
    _southLookImageA[3] = 0x5A;
    _southLookImageA[4] = 0x7E;
    _southLookImageA[5] = 0x7E;
    _southLookImageA[6] = 0x7E;
    _southLookImageA[7] = 0x2A;

    _southLookImageB[0] = 0x18;
    _southLookImageB[1] = 0x3C;
    _southLookImageB[2] = 0x7E;
    _southLookImageB[3] = 0x5A;
    _southLookImageB[4] = 0x5A;
    _southLookImageB[5] = 0x7E;
    _southLookImageB[6] = 0x7E;
    _southLookImageB[7] = 0x54;
}

int Enemy::GetManhattanDist(int x0, int y0, int x1, int y1)
{
	return abs(x0 - x1) + abs(y0 - y1);
}

int Enemy::GetManhattanDist(Position start, Position target)
{
	return GetManhattanDist(start.x, start.y, target.x, target.y);
}

int Enemy::GetDistanceSq(int x0, int y0, int x1, int y1)
{
	return ((x0 - x1) * (x0 - x1)) + ((y0 - y1) * (y0 - y1));
}

int Enemy::GetDistanceSq(int x0, int y0, Position target)
{
	return GetDistanceSq(x0, y0, target.x, target.y);
}

void Enemy::SetTargetToPlayer()
{
	_target = _player->position;
}

void Enemy::SetTargetInfrontOfPlayer()
{
	if (_player->lastDir == NORTH)
	{
		//_target = { _player->position.x, _player->position.y - 4 };
		_target.x = _player->position.x;
		_target.y = _player->position.y - (4 * TILE_SIZE);
	}
	else if (_player->lastDir == EAST)
	{
		//_target = { _player->position.x + 4, _player->position.y };
		_target.x = _player->position.x + (4 * TILE_SIZE);
		_target.y = _player->position.y;
	}
	else if (_player->lastDir == SOUTH)
	{
		//_target = { _player->position.x, _player->position.y + 4 };
		_target.x = _player->position.x;
		_target.y = _player->position.y + (4 * TILE_SIZE);
	}
	else if (_player->lastDir == WEST)
	{
		//_target = { _player->position.x - 4, _player->position.y };
		_target.x = _player->position.x - (4 * TILE_SIZE);
		_target.y = _player->position.y;
	}
}

void Enemy::SetTargetBlockPlayer()
{
	// Find two tiles ahed of the player
	if (_player->lastDir == NORTH)
	{
		//_target = { _player->position.x, _player->position.y - 2 };
		_target.x = _player->position.x;
		_target.y = _player->position.y - (2 * TILE_SIZE);
	}
	else if (_player->lastDir == EAST)
	{
		//_target = { _player->position.x + 2, _player->position.y };
		_target.x = _player->position.x + (2 * TILE_SIZE);
		_target.y = _player->position.y;
	}
	else if (_player->lastDir == SOUTH)
	{
		//_target = { _player->position.x, _player->position.y + 2 };
		_target.x = _player->position.x;
		_target.y = _player->position.y + (2 * TILE_SIZE);
	}
	else if (_player->lastDir == WEST)
	{
		//_target = { _player->position.x - 2, _player->position.y };
		_target.x = _player->position.x - (2 * TILE_SIZE);
		_target.y = _player->position.y;
	}

	// Rotate the target by 180 degrees in relation to blinky
	//int xDiff = _target.x - _blinky->position.x;
	//int yDiff = _target.y - _blinky->position.y;
    _target.x = _target.x + (_target.x - _blinky->position.x);
	_target.y = _target.y + (_target.y - _blinky->position.y);
	//_target = { _target.x + (_target.x - _blinky->position.x), _target.y + (_target.y - _blinky->position.y) };
}

void Enemy::SetTargetClyde()
{
	// If distance to the player is greater than 8 tiles
	if (GetManhattanDist(position, _player->position) > (8 * TILE_SIZE))
	{
		SetTargetToPlayer();
	}
	else
	{
		//_target = { 0, HEIGHT };
		_target.x = 0;
		_target.y = HEIGHT * TILE_SIZE;
		
	}

}

void Enemy::SetTarget()
{
	if (_aiType == BLINKY_AI)
	{
		SetTargetToPlayer();
	}
	else if (_aiType == PINKY_AI)
	{
		SetTargetInfrontOfPlayer();
	}
	else if (_aiType == INKY_AI)
	{
		SetTargetBlockPlayer();
	}
	else if (_aiType == CLYDE_AI)
	{
		SetTargetClyde();
	}
}

void Enemy::GetNextDir()
{
	// Find all possible tiles the enemy can move into, excluding the tile it has just moved away from
	bool north = _lastDir != SOUTH && _maze->IsFloorAdjecentScreenPos(position, NORTH);
	bool east = _lastDir != WEST && _maze->IsFloorAdjecentScreenPos(position, EAST);
	bool south = _lastDir != NORTH && _maze->IsFloorAdjecentScreenPos(position, SOUTH);
	bool west = _lastDir != EAST && _maze->IsFloorAdjecentScreenPos(position, WEST);

	// For each passable tile, get manhattan distance to target
	int northDist = -1;
	int eastDist = -1;
	int southDist = -1;
	int westDist = -1;

	char dirs[4] = { NORTH, SOUTH, EAST, WEST };
	int dists[4] = { -1, -1, -1, -1 };

	// Arrange dirs in order of priority
	/*
	if (abs(position.x - _target.x) >= abs(position.y - _target.y))
	{
		dirs[0] = EAST;
		dirs[1] = WEST;
		dirs[2] = NORTH;
		dirs[3] = SOUTH;
	}
	*/
	// Get distance from each neighbour to target and the smallest index
	int smallestIndex = 0;
	int smallestValue = -1;

	for (int i = 0; i < 4; i++)
	{
		int d = -1;

		if (north && dirs[i] == NORTH)
		{
			d = GetDistanceSq(position.x, position.y - 1, _target);
			dists[i] = d;
		}
		else if (east && dirs[i] == EAST)
		{
			d = GetDistanceSq(position.x + 1, position.y, _target);
			dists[i] = d;
		}
		else if (south && dirs[i] == SOUTH)
		{
			d = GetDistanceSq(position.x, position.y + 1, _target);
			dists[i] = d;
		}
		else if (west && dirs[i] == WEST)
		{
			d = GetDistanceSq(position.x - 1, position.y, _target);
			dists[i] = d;
		}

		if (d != -1 && (d < smallestValue || smallestValue == -1))
		{
			smallestValue = d;
			smallestIndex = i;
		}
	}

	_nextDir = dirs[smallestIndex];
}

Enemy::Enemy(Maze* maze, Player* player, uint16_t colour, char aiType, int x, int y) : BaseGameSprite(x * TILE_SIZE, y * TILE_SIZE)
{
	_maze = maze;
	_player = player;
	_colour = colour;
	_aiType = aiType;
	_lastDir = 0x0;
	_nextDir = 0x0;
    _imageA = true;
    SetImageArrays();
}

Enemy::Enemy(Maze* maze, Player* player, Enemy* blinky, uint16_t colour, char aiType, int x, int y) : BaseGameSprite(x * TILE_SIZE, y * TILE_SIZE)
{
	_maze = maze;
	_player = player;
	_colour = colour;
	_aiType = aiType;
	_blinky = blinky;
	_lastDir = 0x0;
	_nextDir = 0x0;
    _imageA = true;
    SetImageArrays();
}

void Enemy::Init()
{
}

void Enemy::Update()
{
    switch (CurGameState) {
    case STARTUP:
        Visible = true;
        MoveToStartPosition();
        break;
    case CONTINUE:
        MoveToStartPosition();
        break;
    case NEXT_LEVEL:
        MoveToStartPosition();
        break;
    case PLAY:
        _maze->redrawStack.push(position);

        SetTarget();
        GetNextDir();
        UpdatePosition(_nextDir);

        _lastDir = _nextDir;

        // Check for collision with the player
        if (HasCollided(_player))
        {
            printf("Collided with Player!\n");
            NextGameState = DEAD;
        }

        _imageA = !_imageA;
        break;
    case DEAD:
        break;
    default:
        Visible = false;
        break;
    }
	//_nextDir = 0x0;
}

void Enemy::Draw()
{
    //BSP_LCD_DrawPixel(position.x, position.y, _colour);
    BSP_LCD_SetTextColor(_colour);
    //BSP_LCD_FillRect(position.x, position.y, TILE_SIZE, TILE_SIZE);

    if (_lastDir == NORTH)
    {
        if (_imageA)
        {
            DrawSprite(_northLookImageA, _colour);
        }
        else 
        {
            DrawSprite(_northLookImageB, _colour);
        }
    }
    else if (_lastDir == EAST)
    {
        if (_imageA)
        {
            DrawSpriteFlippedHorizontal(_horizontalLookImageA, _colour);
        }
        else 
        {
            DrawSpriteFlippedHorizontal(_horizontalLookImageB, _colour);
        }
    }
    else if (_lastDir == SOUTH)
    {
        if (_imageA)
        {
            DrawSprite(_southLookImageA, _colour);
        }
        else 
        {
            DrawSprite(_southLookImageB, _colour);
        }
    }
    else
    {
        if (_imageA)
        {
            DrawSprite(_horizontalLookImageA, _colour);
        }
        else 
        {
            DrawSprite(_horizontalLookImageB, _colour);
        }
    }
}

/* SPLASH SCREEN H */
//////////////////////////////////////////////////////////////
class SplashScreen :
	public BaseGameClass
{
private:
    int _frameCount;
public:

    SplashScreen();

    void Update();

    void Draw();
};

/* SPLASH SCREEN CPP */
//////////////////////////////////////////////////////////////

SplashScreen::SplashScreen() : BaseGameClass(0, 0)
{
    _frameCount = 0;
}

void SplashScreen::Update()
{
    switch (CurGameState) {
    case SPLASH_SCREEN:
        Visible = true;
        _frameCount++;

        if (_frameCount >= 50)
        {
            NextGameState = STARTUP;
            _frameCount = 0;
        }
        break;
    default:
        Visible = false;
        break;
    }
}

void SplashScreen::Draw()
{
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2 - 8, (uint8_t *) "A Pacman-Like Game", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, (BSP_LCD_GetYSize() / 2), (uint8_t *) "for MBED Simulator", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, (BSP_LCD_GetYSize() / 2) + 16, (uint8_t *) "by Thomas Barnaby Gill", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, (BSP_LCD_GetYSize() / 2) + 24, (uint8_t *) "University of Leeds", CENTER_MODE);
}

/* GAME OVER SCREEN H */
//////////////////////////////////////////////////////////////
class GameOverScreen :
	public BaseGameClass
{
public:

    GameOverScreen();

    void Update();

    void Draw();
};

/* SPLASH SCREEN CPP */
//////////////////////////////////////////////////////////////

GameOverScreen::GameOverScreen() : BaseGameClass(0, 0)
{

}

void GameOverScreen::Update()
{
    switch (CurGameState) {
    case GAME_OVER:
        Visible = true;

        if (TS_State.touchDetected)
        {
            NextGameState = STARTUP;
        }
        break;
    default:
        Visible = false;
        break;
    }
}

void GameOverScreen::Draw()
{
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() / 2, (uint8_t *) "GAME OVER", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, (BSP_LCD_GetYSize() / 2) + 16, (uint8_t *) "Touch Screen to Play Again...", CENTER_MODE);
}

/* Other Functions */
//////////////////////////////////////////////////////////////
void LCDInit()
{
    BSP_LCD_Init();

    /* Touchscreen initialization */
    if (BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize()) == TS_ERROR) {
        printf("BSP_TS_Init error\n");
    }

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_Clear(LCD_COLOR_WHITE);
}




/* MAZE WORLD H */
//////////////////////////////////////////////////////////////
// class MazeWorld : BaseGameClass
// {
// private:
//     GameEngine *engine;
//     Maze maze;
//     Player player;
//     Enemy ghosts[4];

// public:
//     MazeWorld();

//     void Init();

//     void Update();
// };

// /* MAZE WORLD CPP */
// //////////////////////////////////////////////////////////////
// MazeWorld::MazeWorld() : BaseGameClass(0, 0)
// {
//     maze = new Maze(0, 0);
//     player(&maze, 13, 23);


// 	ghosts[1] = (&maze, &player, LCD_COLOR_RED, BLINKY_AI, 14, 11);
// 	Enemy enemy2(&maze, &player, LCD_COLOR_MAGENTA, PINKY_AI, 12, 11);
// 	Enemy enemy3(&maze, &player, &enemy1, LCD_COLOR_CYAN, INKY_AI, 10, 11);
// 	Enemy enemy4(&maze, &player, LCD_COLOR_ORANGE, CLYDE_AI, 16, 11);
// }

/* MAIN */
//////////////////////////////////////////////////////////////
int main()
{
    printf("Starting game...\n");

	GameEngine engine;
	
	Maze maze(0, 0);

	Player player(&maze, 13, 22);

    SplashScreen splash;
    GameOverScreen gameOver;

	Enemy enemy1(&maze, &player, LCD_COLOR_RED, BLINKY_AI, 14, 12);
	Enemy enemy2(&maze, &player, LCD_COLOR_MAGENTA, PINKY_AI, 12, 12);
	Enemy enemy3(&maze, &player, &enemy1, LCD_COLOR_CYAN, INKY_AI, 10, 12);
	Enemy enemy4(&maze, &player, LCD_COLOR_ORANGE, CLYDE_AI, 16, 12);

    engine.AddGameObject(&splash);
    engine.AddGameObject(&gameOver);
	engine.AddGameObject(&maze);
	engine.AddGameObject(&player);

	engine.AddGameObject(&enemy1);
	engine.AddGameObject(&enemy2);
	engine.AddGameObject(&enemy3);
	engine.AddGameObject(&enemy4);

    printf("Initialising LCD...\n");
    LCDInit();

    printf("Entering main game loop...\n");
	engine.MainGameLoop();
}