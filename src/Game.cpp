#include "Game.hpp"
#include "GridCell.hpp"
#include "Snake.hpp"
#include "Utils/Constants.hpp"
#include "Texture.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <queue>
#include <sstream>

Game::Game() : 
	title_(constants::game_title), 
	screen_width_(constants::screen_width), 
	screen_height_(constants::screen_height), 
	is_running_(false), 
	game_over_(false), 
	paused_(false), 
	autopilot_toggle_(false), 
	shortest_path_toggle_(false), 
	wrapped_shortest_path_toggle_(false), 
	info_toggle_(false), 
	last_ms_(0), 
	tick_ms_(100), 
	score_(0), 
	grid_cell_side_(50), 
	score_info_(std::make_unique<Texture>()), 
	controls_info_(std::make_unique<Texture>()), 
	toggle_info_(std::make_unique<Texture>()), 
	toggled_controls_info_(std::make_unique<Texture>()), 
	game_over_info_(std::make_unique<Texture>()), 
	snake_(nullptr), 
	food_(nullptr), 
	mt_(std::random_device{}()), 
	random_x_(0, constants::screen_width - grid_cell_side_), 
	random_y_(0, constants::screen_height - grid_cell_side_), 
	window_(nullptr), 
	renderer_(nullptr), 
	font_(nullptr)
{
	assert(constants::screen_width % grid_cell_side_ == 0 && constants::screen_height % grid_cell_side_ == 0);
	grid_.resize((constants::screen_width / grid_cell_side_) * (constants::screen_height / grid_cell_side_));

	int y_pos = 0;
	int x_pos = 0;

	for (std::size_t i = 0; i < grid_.size(); ++i)
	{
		GridCell& grid_cell = grid_[i];

		grid_cell.box_.x = x_pos;
		grid_cell.box_.y = y_pos;
		grid_cell.box_.w = grid_cell_side_;
		grid_cell.box_.h = grid_cell_side_;

		//std::cout << "[" << x_pos << " " << y_pos << "] ";
		//std::cout << "[" << i << "] ";

		x_pos += grid_cell_side_;

		if (i != 0 && (i + 1) % (constants::screen_width / grid_cell_side_) == 0)
		{
			x_pos = 0;
			y_pos += grid_cell_side_;
			//std::cout << std::endl;
		}
	}

	snake_ = std::make_unique<Snake>(4, grid_cell_side_, this);

	SpawnFood();
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(title_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width_, screen_height_, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0xFF, 0xFF);

	constexpr int img_flags = IMG_INIT_PNG;

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not be initialized! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	return InitInfoTextures();
}

bool Game::InitInfoTextures()
{
	font_ = TTF_OpenFont("res/font/font.ttf", 28);

	if (font_ == nullptr)
	{
		printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
		return false;
	}

	SDL_Color text_color = { 0xFF, 0x00, 0x00, 0xFF };

	const std::string score_text = "Score: " + std::to_string(score_);

	std::stringstream ss;

	ss << "Autopilot: " << (autopilot_toggle_ ? "ON" : "OFF") << "        Regular A*: " << (shortest_path_toggle_ ? "ON" : "OFF") << "        Wrapped A*: " << (wrapped_shortest_path_toggle_ ? "ON" : "OFF");

	score_info_->LoadFromText(renderer_, font_, score_text.c_str(), text_color);
	controls_info_->LoadFromText(renderer_, font_, "Press to toggle: 'a' - autopilot       's' - A* path        'w' - wrapped A* 'ESC' - pause", text_color, 220);
	toggle_info_->LoadFromText(renderer_, font_, "Press 'i' to toggle info.", text_color);
	toggled_controls_info_->LoadFromText(renderer_, font_, ss.str().c_str(), text_color, 280);
	game_over_info_->LoadFromText(renderer_, font_, "Press SPACE to restart.", text_color);

	return true;
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;

	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	TTF_CloseFont(font_);
	font_ = nullptr;

	score_info_->FreeTexture();
	controls_info_->FreeTexture();
	toggle_info_->FreeTexture();
	toggled_controls_info_->FreeTexture();
	game_over_info_->FreeTexture();

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

void Game::Run()
{
	if (!Initialize())
	{
		return;
	}

	is_running_ = true;
	
	constexpr long double ms = 1.0 / 60.0;

	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (is_running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());
		
		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000)
		{
			timer += 1000;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::Stop()
{
	is_running_ = false;
}

void Game::GameOver()
{
	game_over_ = true;
}

void Game::Reset()
{
	score_ = 0;
	snake_.reset();
	snake_ = std::make_unique<Snake>(4, grid_cell_side_, this);
	tick_ms_ = 100;
	game_over_ = false;
	UpdateScore();
	SpawnFood();
}

void Game::HandleEvents()
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0)
	{
		if (e.type == SDL_QUIT)
		{
			Stop();
		}
		else if (game_over_ && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
		{
			autopilot_toggle_ = false;
			Reset();
			UpdateControlsStatus();
		}
		else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_i)
		{
			info_toggle_ = !info_toggle_;
		}
		else if (!game_over_)
		{
			if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_a)
			{
				shortest_path_toggle_ = false;
				wrapped_shortest_path_toggle_ = false;
				autopilot_toggle_ = !autopilot_toggle_;
				shortest_path_cells_.clear();
				UpdateControlsStatus();
			}
			else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_s)
			{
				autopilot_toggle_ = false;
				wrapped_shortest_path_toggle_ = false;
				shortest_path_toggle_ = !shortest_path_toggle_;
				shortest_path_cells_.clear();
				UpdateControlsStatus();
			}
			else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_w)
			{
				autopilot_toggle_ = false;
				shortest_path_toggle_ = false;
				wrapped_shortest_path_toggle_ = !wrapped_shortest_path_toggle_;
				shortest_path_cells_.clear();
				UpdateControlsStatus();
			}
			else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE)
			{
				paused_ = !paused_;
			}
		}

		snake_->HandleEvent(&e);
	}
}
	
void Game::Tick()
{
	const int current_ms = SDL_GetTicks();
	
	if (!paused_ && !game_over_ && current_ms - last_ms_ > tick_ms_)
	{
		last_ms_ = current_ms;

		GridCell* back_path_cell = nullptr;

		if (autopilot_toggle_ && !shortest_path_cells_.empty())
		{
			back_path_cell = shortest_path_cells_.back();
			shortest_path_cells_.pop_back();
		}

		snake_->Tick(back_path_cell);

		if ((autopilot_toggle_ && shortest_path_cells_.empty()) || (shortest_path_toggle_ || wrapped_shortest_path_toggle_))
		{
			FindAStarPath(snake_->GetHead(), &grid_.at(food_->ConvertCellToGridIndex()), wrapped_shortest_path_toggle_);
		}
	}
}

void Game::Render()
{
	SDL_RenderSetViewport(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer_);

	if (autopilot_toggle_ || shortest_path_toggle_ || wrapped_shortest_path_toggle_)
	{
		for (const GridCell* cell : shortest_path_cells_)
		{
			SDL_SetRenderDrawColor(renderer_, 0xFF, 0xFF, 0x00, 0xFF);
			SDL_RenderFillRect(renderer_, &(cell->box_));
		}
	}

	snake_->Render(renderer_);

	score_info_->Render(renderer_, constants::screen_width / 2 - (score_info_->Width() / 2), 0);

	if (game_over_)
	{
		game_over_info_->Render(renderer_, constants::screen_width / 2 - (game_over_info_->Width() / 2), constants::screen_height / 2 - (game_over_info_->Height() / 2));
	}

	toggle_info_->Render(renderer_, constants::screen_width / 2 - (toggle_info_->Width() / 2), constants::screen_height - toggle_info_->Height());

	if (info_toggle_)
	{
		controls_info_->Render(renderer_, 10, constants::screen_height - controls_info_->Height());
	}

	toggled_controls_info_->Render(renderer_, constants::screen_width - toggled_controls_info_->Width() + 50, constants::screen_height - toggled_controls_info_->Height());

	SDL_SetRenderDrawColor(renderer_, 0xFF, 0x00, 0x00, 0xFF);
	SDL_RenderFillRect(renderer_, &food_->box_);

	SDL_RenderPresent(renderer_);
}

void Game::SpawnFood()
{
	bool food_snake_collision = false;
	int random_x = 0;
	int random_y = 0;

	do
	{
		food_snake_collision = false;

		random_x = random_x_(mt_);
		random_y = random_y_(mt_);

		const int x_mod = random_x % grid_cell_side_;
		const int y_mod = random_y % grid_cell_side_;

		if (x_mod > (grid_cell_side_ / 2))
		{
			random_x = random_x - x_mod + grid_cell_side_;
		}
		else
		{
			random_x = random_x - x_mod;
		}

		if (y_mod > (grid_cell_side_ / 2))
		{
			random_y = random_y - y_mod + grid_cell_side_;
		}
		else
		{
			random_y = random_y - y_mod;
		}

		for (const GridCell* snake_segment : snake_->Segments())
		{
			if (snake_segment->box_.x == random_x && snake_segment->box_.y == random_y)
			{
				food_snake_collision = true;
				break;
			}
		}
	}
	while (food_snake_collision);

	food_ = &grid_.at(ConvertXYToGridIndex(random_x, random_y));
}

GridCell* Game::Food()
{
	return food_;
}

std::vector<GridCell>& Game::Grid()
{
	return grid_;
}

void Game::IncrementScore()
{
	score_ += 10;
}

void Game::UpdateScore()
{
	score_info_->FreeTexture();

	const SDL_Color text_color = { 0xFF, 0x00, 0x00, 0xFF };
	const std::string score_text = "Score: " + std::to_string(score_);

	score_info_->LoadFromText(renderer_, font_, score_text.c_str(), text_color);
}

void Game::UpdateControlsStatus()
{
	toggled_controls_info_->FreeTexture();

	SDL_Color text_color = { 0xFF, 0x00, 0x00, 0xFF };

	const std::string score_text = "Score: " + std::to_string(score_);

	std::stringstream ss;
	ss << "Autopilot: " << (autopilot_toggle_ ? "ON" : "OFF") << "        Regular A*: " << (shortest_path_toggle_ ? "ON" : "OFF") << "        Wrapped A*: " << (wrapped_shortest_path_toggle_ ? "ON" : "OFF");
	toggled_controls_info_->LoadFromText(renderer_, font_, ss.str().c_str(), text_color, 280);
}

void Game::SpeedUp()
{
	int minimum_speed = 50;

	if (tick_ms_ > minimum_speed)
	{
		--tick_ms_;
	}
}

int Game::ConvertXYToGridIndex(int x, int y)
{
	const int x_pos = (x / grid_cell_side_);
	const int y_pos = (y / grid_cell_side_);
	
	const int x_offset = constants::screen_width / grid_cell_side_;

	return y_pos * x_offset + x_pos;
}

bool Game::AutopilotToggled() const
{
	return autopilot_toggle_;
}

bool Game::WrappedShortestPathToggled() const
{
	return wrapped_shortest_path_toggle_;
}

bool Game::FindAStarPath(GridCell* start_cell, GridCell* target_cell, bool wrapped)
{
	for (GridCell& grid_cell : grid_)
	{
		grid_cell.graph_info_.parent_ = nullptr;
		grid_cell.graph_info_.visited_ = false;
		grid_cell.graph_info_.in_queue_ = false;
		grid_cell.graph_info_.global_cost_ = std::numeric_limits<int>::max();
		grid_cell.graph_info_.local_cost_ = std::numeric_limits<int>::max();
	}

	auto queue_cmp = [](GridCell* c1, GridCell* c2) { return c1->graph_info_.global_cost_ > c2->graph_info_.global_cost_; };
	std::priority_queue<GridCell*, std::vector<GridCell*>, decltype(queue_cmp)> min_heap{ queue_cmp };

	const std::vector<int> snake_indices = snake_->SnakeGridIndices();
		
	GridCell* current_cell = start_cell;
	current_cell->graph_info_.local_cost_ = 0;
	current_cell->graph_info_.global_cost_ = wrapped ? start_cell->GetShortestXYDistance(*target_cell) : start_cell->GetXYDistance(*target_cell);
	
	min_heap.push(start_cell);

	while (!min_heap.empty() && min_heap.top() != target_cell)
	{
		while (!min_heap.empty() && min_heap.top()->graph_info_.visited_)
		{
			min_heap.pop();
		}

		if (min_heap.empty())
		{
			break;
		}

		current_cell = min_heap.top();
		current_cell->graph_info_.visited_ = true;

		for (int index : current_cell->GetNeighboursIndices(wrapped))
		{
			GridCell* const neighbour_cell = &grid_.at(index);

			const bool is_snake_segment = std::find(std::begin(snake_indices), std::end(snake_indices), index) != std::end(snake_indices);	

			if (is_snake_segment)
			{
				continue;
			}

			const int lower_cost = current_cell->graph_info_.local_cost_ + (wrapped ? start_cell->GetShortestXYDistance(*target_cell) : start_cell->GetXYDistance(*target_cell));

			if (lower_cost < neighbour_cell->graph_info_.local_cost_)
			{
				neighbour_cell->graph_info_.parent_ = current_cell;
				neighbour_cell->graph_info_.local_cost_ = lower_cost;
				neighbour_cell->graph_info_.global_cost_ = neighbour_cell->graph_info_.local_cost_ + (wrapped ? start_cell->GetShortestXYDistance(*target_cell) : start_cell->GetXYDistance(*target_cell));
			}

			if (!neighbour_cell->graph_info_.visited_ && !is_snake_segment)
			{
				min_heap.push(neighbour_cell);
			}
		}
	}
	
	shortest_path_cells_.clear();
	
	GridCell* cell = target_cell;

	while (cell != start_cell)
	{
		shortest_path_cells_.push_back(cell);
		cell = cell->graph_info_.parent_;
		
		if (cell == nullptr)
		{
			shortest_path_cells_.clear();
			return false;
		}
	}

	return true;
}
