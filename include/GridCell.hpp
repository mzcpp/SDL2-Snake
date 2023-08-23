#ifndef GRID_CELL_HPP
#define GRID_CELL_HPP

#include <SDL2/SDL.h>

#include <vector>

class GridCell;

struct GraphInfo
{
	GridCell* parent_;
	bool visited_;
	bool in_queue_;
	int global_cost_;
	int local_cost_;
};

class GridCell
{
public:
	SDL_Rect box_;
	GraphInfo graph_info_;

	GridCell();

	~GridCell() = default;

	SDL_Rect& Box();

	int ConvertCellToGridIndex() const;

	int GetXYDistance(const GridCell& target_cell) const;

	int GetShortestXYDistance(const GridCell& target_cell) const;

	std::vector<int> GetNeighboursIndices(bool wrap_around = false) const;
};

#endif