#include "GridCell.hpp"
#include "Utils/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

GridCell::GridCell()
{
	box_.x = 0;
	box_.y = 0;
	box_.w = 0;
	box_.h = 0;

	graph_info_.parent_ = nullptr;
	graph_info_.visited_ = false;
	graph_info_.in_queue_ = false;
	graph_info_.global_cost_ = std::numeric_limits<int>::max();
	graph_info_.local_cost_ = std::numeric_limits<int>::max();
}

SDL_Rect& GridCell::Box()
{
	return box_;
}

int GridCell::ConvertCellToGridIndex() const
{
	const int x_pos = box_.x / box_.w;
	const int y_pos = box_.y / box_.h;

	return y_pos * (constants::screen_width / box_.w) + x_pos;
}

int GridCell::GetXYDistance(const GridCell& target_cell) const
{
	const int x_distance = std::abs(target_cell.box_.x - box_.x) / box_.w;
	const int y_distance = std::abs(target_cell.box_.y - box_.y) / box_.w;

	return x_distance + y_distance;
}

int GridCell::GetShortestXYDistance(const GridCell& target_cell) const
{
	int min_distance = GetXYDistance(target_cell);
	
	GridCell target_copy = target_cell;
	target_copy.box_.x += constants::screen_width;
	min_distance = std::min(min_distance, GetXYDistance(target_copy));

	target_copy.box_.x -= 2 * constants::screen_width;
	min_distance = std::min(min_distance, GetXYDistance(target_copy));

	target_copy.box_.x += constants::screen_width;
	target_copy.box_.y += constants::screen_height;
	min_distance = std::min(min_distance, GetXYDistance(target_copy));

	target_copy.box_.y -= 2 * constants::screen_height;
	min_distance = std::min(min_distance, GetXYDistance(target_copy));

	target_copy.box_.y += constants::screen_height;

	return min_distance;
}

std::vector<int> GridCell::GetNeighboursIndices(bool wrap_around) const
{
	std::vector<int> neighbours_indices;

	GridCell left;
	left.box_ = { box_.x - box_.w, box_.y, box_.w, box_.h };
	
	if (left.box_.x < 0 && wrap_around)
	{
		left.box_.x += constants::screen_width;
		neighbours_indices.emplace_back(left.ConvertCellToGridIndex());
	}
	else if (left.box_.x >= 0)
	{
		neighbours_indices.emplace_back(left.ConvertCellToGridIndex());
	}

	GridCell right;
	right.box_ = { box_.x + box_.w, box_.y, box_.w, box_.h };

	if (right.box_.x == constants::screen_width && wrap_around)
	{
		right.box_.x -= constants::screen_width;
		neighbours_indices.emplace_back(right.ConvertCellToGridIndex());
	}
	else if (right.box_.x != constants::screen_width)
	{
		neighbours_indices.emplace_back(right.ConvertCellToGridIndex());
	}

	GridCell top;
	top.box_ = { box_.x, box_.y - box_.h, box_.w, box_.h };

	if (top.box_.y < 0 && wrap_around)
	{
		top.box_.y += constants::screen_height;
		neighbours_indices.emplace_back(top.ConvertCellToGridIndex());
	}
	else if (top.box_.y >= 0)
	{
		neighbours_indices.emplace_back(top.ConvertCellToGridIndex());
	}

	GridCell bottom;
	bottom.box_ = { box_.x, box_.y + box_.h, box_.w, box_.h };

	if (bottom.box_.y == constants::screen_height && wrap_around)
	{
		bottom.box_.y -= constants::screen_height;
		neighbours_indices.emplace_back(bottom.ConvertCellToGridIndex());
	}
	else if (bottom.box_.y != constants::screen_height)
	{
		neighbours_indices.emplace_back(bottom.ConvertCellToGridIndex());
	}

	return neighbours_indices;
}