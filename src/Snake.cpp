#include "Game.hpp"
#include "GridCell.hpp"
#include "Snake.hpp"
#include "Utils/Constants.hpp"

#include <SDL2/SDL.h>

#include <iostream>
#include <cassert>

Snake::Snake(std::size_t segments_size, int segment_side_size, Game* game) : 
	segment_side_size_(segment_side_size),
	direction_(Direction::RIGHT),  
	moved_snake_(false),
	game_(game)
{
	snake_segments_.resize(segments_size);

	int x_half = constants::screen_width / 100;
	int y_half = constants::screen_height / 100;

	for (std::size_t i = 0; i < snake_segments_.size(); ++i)
	{
		snake_segments_[i] = &(game_->Grid()[y_half * (constants::screen_width / segment_side_size_) + x_half - i]);
	}
}

std::vector<GridCell*>& Snake::Segments()
{
	return snake_segments_;
}

GridCell* Snake::GetHead()
{
	return snake_segments_[0];
}

void Snake::AddSegment()
{
	snake_segments_.emplace_back(snake_segments_.back());
}

std::vector<int> Snake::SnakeGridIndices() const
{
	std::vector<int> snake_grid_indices;

	for (const GridCell* snake_segment : snake_segments_)
	{
		snake_grid_indices.emplace_back(snake_segment->ConvertCellToGridIndex());
	}

	return snake_grid_indices;
}

void Snake::MoveSnake(GridCell* next_cell)
{
	for (std::size_t i = snake_segments_.size() - 1; i > 0; --i)
	{
		snake_segments_[i] = snake_segments_[i - 1];
	}

	GridCell*& snake_head = snake_segments_[0];

	const int x_offset = constants::screen_width / segment_side_size_;
	const int y_offset = constants::screen_height / segment_side_size_;

	int new_head_index = game_->ConvertXYToGridIndex(snake_head->box_.x, snake_head->box_.y);

	if (next_cell != nullptr)
	{
		const SDL_Rect diff = { next_cell->box_.x - snake_head->box_.x, next_cell->box_.y - snake_head->box_.y, snake_head->box_.w, snake_head->box_.h };

		if (diff.x < 0)
		{
			direction_ = Direction::LEFT;
			new_head_index -= 1;
		}
		else if (diff.x > 0)
		{
			direction_ = Direction::RIGHT;
			new_head_index += 1;
		}
		else if (diff.y > 0)
		{
			direction_ = Direction::DOWN;
			new_head_index += x_offset;
		}
		else if (diff.y < 0)
		{
			direction_ = Direction::UP;
			new_head_index -= x_offset;
		}

		snake_head = &game_->Grid()[new_head_index];
		return;
	}
	
	switch (direction_)
	{
		case Direction::LEFT:
			if (snake_head->box_.x == 0)
			{
				new_head_index += x_offset;
			}

			new_head_index -= 1;
			break;

		case Direction::RIGHT:
			if (snake_head->box_.x == constants::screen_width - segment_side_size_)
			{
				new_head_index -= x_offset;
			}

			new_head_index += 1;
			break;

		case Direction::UP:
			if (snake_head->box_.y == 0)
			{
				new_head_index += x_offset * y_offset;
			}

			new_head_index -= x_offset;
			break;

		case Direction::DOWN:
			if (snake_head->box_.y == constants::screen_height - segment_side_size_)
			{
				new_head_index -= x_offset * y_offset;
			}
			
			new_head_index += x_offset;
			break;
	}

	assert(new_head_index >= 0 && new_head_index < x_offset * y_offset);
	snake_head = &game_->Grid()[new_head_index];
}

void Snake::HandleEvent(SDL_Event* e)
{
	if (!moved_snake_ && e->type == SDL_KEYDOWN && e->key.repeat == 0)
	{
		switch (e->key.keysym.sym)
		{
			case SDLK_LEFT:
				if (direction_ != Direction::RIGHT)
				{
					direction_ = Direction::LEFT;
				}
				break;
			case SDLK_RIGHT:
				if (direction_ != Direction::LEFT)
				{
					direction_ = Direction::RIGHT;
				}
				break;
			case SDLK_UP:
				if (direction_ != Direction::DOWN)
				{
					direction_ = Direction::UP;
				}
				break;
			case SDLK_DOWN:
				if (direction_ != Direction::UP)
				{
					direction_ = Direction::DOWN;
				}
				break;
		}

		moved_snake_ = true;
	}
}

void Snake::Tick(GridCell* next_cell)
{
	MoveSnake(next_cell);
	moved_snake_ = false;

	if (SDL_HasIntersection(&GetHead()->box_, &(game_->Food()->box_)))
	{
		game_->SpawnFood();
		game_->IncrementScore();
		game_->UpdateScore();
		AddSegment();
		game_->SpeedUp();

		if (game_->AutopilotToggled())
		{
			game_->FindAStarPath(GetHead(), &game_->Grid().at(game_->Food()->ConvertCellToGridIndex()), game_->WrappedShortestPathToggled());
		}
	}
	else
	{
		for (std::size_t i = 1; i < snake_segments_.size(); ++i)
		{
			if (SDL_HasIntersection(&GetHead()->box_, &snake_segments_[i]->box_))
			{
				game_->GameOver();
				break;
			}
		}
	}
}

void Snake::Render(SDL_Renderer* renderer)
{
	SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);

	for (std::size_t i = 1; i < snake_segments_.size(); ++i)
	{
		SDL_RenderFillRect(renderer, &snake_segments_[i]->box_);
	}

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
	SDL_RenderFillRect(renderer, &GetHead()->box_);
}