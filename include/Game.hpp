#ifndef GAME_HPP
#define GAME_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <memory>
#include <random>

class Snake;
class Texture;
class GridCell;

class Game
{
private:
	const char* title_;
	int screen_width_;
	int screen_height_;
	bool is_running_;
	bool game_over_;
	bool paused_;
	bool autopilot_toggle_;
	bool shortest_path_toggle_;
	bool wrapped_shortest_path_toggle_;
	bool info_toggle_;
	int last_ms_;
	int tick_ms_;
	int score_;
	int grid_cell_side_;

	std::unique_ptr<Texture> score_info_;
	std::unique_ptr<Texture> controls_info_;
	std::unique_ptr<Texture> toggle_info_;
	std::unique_ptr<Texture> toggled_controls_info_;
	std::unique_ptr<Texture> game_over_info_;

	std::vector<GridCell> grid_;
	std::unique_ptr<Snake> snake_;
	std::vector<GridCell*> shortest_path_cells_;
	GridCell* food_;

	std::mt19937_64 mt_;
	std::uniform_int_distribution<int> random_x_;
  	std::uniform_int_distribution<int> random_y_;

	SDL_Window* window_;
	SDL_Renderer* renderer_;
	TTF_Font* font_;

	bool Initialize();
	
	bool InitInfoTextures();

	void Finalize();

public:
	Game();
	
	~Game();

	void Run();

	void Stop();

	void GameOver();
	
	void Reset();

	void HandleEvents();
	
	void Tick();
	
	void Render();

	void SpawnFood();

	GridCell* Food();

	std::vector<GridCell>& Grid();

	void IncrementScore();

	void UpdateScore();

	void UpdateControlsStatus();

	void SpeedUp();

	int ConvertXYToGridIndex(int x, int y);

	bool AutopilotToggled() const;
	
	bool WrappedShortestPathToggled() const;

	bool FindAStarPath(GridCell* start_cell, GridCell* target_cell, bool wrapped = false);
};

#endif