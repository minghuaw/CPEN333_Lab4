#include "maze_runner_common.h"

#include <cpen333/process/shared_memory.h>
#include <cpen333/process/mutex.h>
#include <cstring>
#include <chrono>
#include <thread>

#define PASSED_CHAR 'x'

class MazeRunner {

	cpen333::process::shared_object<SharedData> memory_;
	cpen333::process::mutex mutex_;

	// local copy of maze
	MazeInfo minfo_;
	
	// runner info
	size_t idx_;   // runner index
	int loc_[2];   // current location
	
public:
	
	MazeRunner() : memory_(MAZE_MEMORY_NAME), mutex_(MAZE_MUTEX_NAME),
		minfo_(), idx_(0), loc_() {

		// check if magic number match
		if (memory_->magic != MAGIC_NUM)
			memory_.unlink();

		// copy maze contents
		minfo_ = memory_->minfo;

		{
			// protect access of number of runners
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			idx_ = memory_->rinfo.nrunners;
			memory_->rinfo.nrunners++;
		}

		// get current location
		loc_[COL_IDX] = memory_->rinfo.rloc[idx_][COL_IDX];
		loc_[ROW_IDX] = memory_->rinfo.rloc[idx_][ROW_IDX];

	}

	/**
	* Solves the maze, taking time between each step so we can actually see progress in the UI
	* @return 1 for success, 0 for failure, -1 to quit
	*/

	int go() {
		// current location
		int c = loc_[COL_IDX];
		int r = loc_[ROW_IDX];

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		//==========================================================
		// TODO: NAVIGATE MAZE
		int cur_r = r, cur_c = c;
		int last_r = r, last_c = c;
		while (!memory_->quit) {
			last_c = cur_c;
			last_r = cur_r;
			cur_c = c;
			cur_r = r;

			int wall_count = _is_dead_end(c, r);
			if (wall_count == 4) {
				if (!_remove_PASSED_CHAR(c, r))
					return 0;
			}
			if (wall_count >= 3)
				minfo_.maze[c][r] = PASSED_CHAR;

			// try right
			if (c + 1 < minfo_.cols && (minfo_.maze[c + 1][r] != WALL_CHAR && minfo_.maze[c + 1][r] != PASSED_CHAR)) {
				c++;
				if (_go_helper(c, r))
					return 1;
			}
			// try down
			else if (r + 1 < minfo_.rows && (minfo_.maze[c][r + 1] != WALL_CHAR && minfo_.maze[c][r + 1] != PASSED_CHAR)) {
				r++;
				if (_go_helper(c, r))
					return 1;
			}
			// try left
			else if (c - 1 >= 0 && (minfo_.maze[c - 1][r] != WALL_CHAR && minfo_.maze[c - 1][r] != PASSED_CHAR)) {
				c--;
				if (_go_helper(c, r))
					return 1;
			}
			// try up
			else if (r - 1 >= 0 && (minfo_.maze[c][r - 1] != WALL_CHAR && minfo_.maze[c][r - 1] != PASSED_CHAR)) {
				r--;
				if (_go_helper(c, r))
					return 1;
			}
			else {

				return 0;
			}

			if (r == last_r && c == last_c)
				minfo_.maze[c][r] = PASSED_CHAR;

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		if (memory_->quit)
			return -1;
		//==========================================================

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// failed to find exit
		return 0;		
	}

	// Update runner's location, return true if runner
	// is at Exit location
	bool _go_helper(int c, int r) {
		try {
			mutex_.lock(); // lock mutex
			
			memory_->rinfo.rloc[idx_][ROW_IDX] = r;
			memory_->rinfo.rloc[idx_][COL_IDX] = c;
		}
		catch (std::exception& e) {}

		mutex_.unlock(); // unlock mutex

		// return true if at Exit
		if (minfo_.maze[c][r] == EXIT_CHAR) {
			return true;
		}

		return false;
	}

	// check if runner is at dead end
	int _is_dead_end(int c, int r) {
		int wall_count = 0;
		//  right
		if (c + 1 < minfo_.cols && (minfo_.maze[c + 1][r] == WALL_CHAR || minfo_.maze[c + 1][r] == PASSED_CHAR)) {
			wall_count++;
		}
		//  down
		if (r + 1 < minfo_.rows && (minfo_.maze[c][r + 1] == WALL_CHAR || minfo_.maze[c][r + 1] == PASSED_CHAR)) {
			wall_count++;
		}
		//  left
		if (c - 1 >= 0 && (minfo_.maze[c - 1][r] == WALL_CHAR || minfo_.maze[c - 1][r] == PASSED_CHAR)) {
			wall_count++;
		}
		//  up
		if (r - 1 >= 0 && (minfo_.maze[c][r - 1] == WALL_CHAR || minfo_.maze[c][r - 1] == PASSED_CHAR)) {
			wall_count++;
		}

		return wall_count;
	}
	
	// remove PASSED_CHAR if stuck
	bool _remove_PASSED_CHAR(int c, int r) {
		bool char_removed = false;

		if (c + 1 < minfo_.cols && minfo_.maze[c + 1][r] == PASSED_CHAR) {
			minfo_.maze[c + 1][r] = ' ';
			char_removed = true;
		}
		//  down
		if (r + 1 < minfo_.rows && minfo_.maze[c][r + 1] == PASSED_CHAR) {
			minfo_.maze[c][r + 1] = ' ';
			char_removed = true;
		}
		//  left
		if (c - 1 >= 0 && minfo_.maze[c - 1][r] == PASSED_CHAR) {
			minfo_.maze[c - 1][r] = ' ';
			char_removed = true;
		}
		//  up
		if (r - 1 >= 0 && minfo_.maze[c][r - 1] == PASSED_CHAR) {
			minfo_.maze[c][r - 1] = ' ';
			char_removed = true;
		}

		return char_removed;
	}
};



int main() {

	MazeRunner runner;
	runner.go();

	return 0;
}