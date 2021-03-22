#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include <unordered_map>
#include <condition_variable>

#include <thread>
#include <sstream>


#include "node_ref.h"


struct MutexPairs {
	MutexPairs();
	~MutexPairs();

	std::mutex method_entry_mutex;
	std::mutex async_mutex;
	std::condition_variable cond;
	std::atomic<bool> ready;
};





class MutexSetManager {

private:
	std::mutex mutex_pairs_mutex;
	std::vector<MutexPairs*> variable_condition_source;

public:

	MutexSetManager();
	~MutexSetManager();

	//MutexSetManager(const MutexSetManager& arg) = delete;
	MutexSetManager& operator=(const MutexSetManager&) = delete;


	void  release(MutexPairs* pairs)
	{
		std::lock_guard<std::mutex> lk(this->mutex_pairs_mutex);
		this->variable_condition_source.push_back(pairs);
	}


	MutexPairs* getMutexPairs()
	{
		std::lock_guard<std::mutex> lk(this->mutex_pairs_mutex);


		if (this->variable_condition_source.size() > 0) {
			auto *back = this->variable_condition_source.back();
			this->variable_condition_source.pop_back();
			back->ready = false;

			/*
			if (back->method_entry_mutex.try_lock()) {
				back->method_entry_mutex.unlock();
			}
			else {
				DCHECK(false);
			}
			*/

			return back;
		}

		auto *mtx = new MutexPairs();
		mtx->ready = false;

		return mtx;
	}
};


class with_thread_mutex_set
{
public:

	with_thread_mutex_set(MutexSetManager *manager_arg)
	{
		_manager = manager_arg;
		this->pair = _manager->getMutexPairs();
	}

	~with_thread_mutex_set()
	{
		_manager->release(this->pair);
	}

	MutexPairs *pair;


	with_thread_mutex_set(const with_thread_mutex_set&) = delete;
	with_thread_mutex_set& operator=(const with_thread_mutex_set&) = delete;


private:
	MutexSetManager *_manager;
};
