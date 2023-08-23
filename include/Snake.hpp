#ifndef SNAKE_HPP
#define SNAKE_HPP

#include <SDL2/SDL.h>

#include <vector>

enum class Direction
{
	LEFT, RIGHT, UP, DOWN
};

class Game;
class GridCell;

class Snake
{
private:
	int segment_side_size_;
	Direction direction_;
	std::vector<GridCell*> snake_segments_;
	bool moved_snake_;
	Game* game_;

	void MoveSnake(GridCell* next_cell = nullptr);

public:
	Snake(std::size_t segments_size, int segment_side_size, Game* game);

	std::vector<GridCell*>& Segments();

	GridCell* GetHead();
	
	void AddSegment();

	std::vector<int> SnakeGridIndices() const;

	void HandleEvent(SDL_Event* e);

	void Tick(GridCell* next_cell = nullptr);

	void Render(SDL_Renderer* renderer);
};

#endif