#include "node_ref.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string>

#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>

#include <stdint.h>
#include <ctime>
#include "bg_task.h"
#include "pyconverters.h"
#include "CustomModules.h"

#include "JsObject.h"
#include "uibox.h"
#include "PlynthUtils.h"
#include "pyconverters.h"

/*
 * Implements background tasks
 */

using namespace std;
using namespace v8;
using namespace plynth;


void BackgroundTask::Init(v8::Local<v8::Object> rootJs)
{

}


class with_gil
{
public:
	with_gil() { state_ = PyGILState_Ensure(); }
	~with_gil() { PyGILState_Release(state_); }

	with_gil(const with_gil&) = delete;
	with_gil& operator=(const with_gil&) = delete;
private:
	PyGILState_STATE state_;
};



//static std::mutex background_keeper_mutex;
static std::unordered_map<std::string, PyObject*> background_task_runners;
static double current_fps = 60;
//static double current_load = 1;

static uint_fast32_t current_skip_ms;
static int64_t current_sleep_nanoseconds;


void gen_random(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}


// @return: is empty
bool remove_background_task()
{
	//std::lock_guard<std::mutex> guard(background_keeper_mutex);
	{
		std::vector<std::string> needs_removing;

		for (auto&& entry : background_task_runners) {
			const std::string &key = entry.first;
			auto *background_task_runner = entry.second;
			if (auto *finished = PyObject_GetAttrString(background_task_runner, "_finished")) {
				if (Py_True == finished) {
					if (auto *exc = PyObject_GetAttrString(background_task_runner, "invoke_on_finished")) {
						if (const PyObject* future = PyObject_CallObject(exc, NULL)) {
							Py_DECREF(future);
						}
						Py_DECREF(exc);
					}

					Py_DECREF(background_task_runner);
					needs_removing.push_back(key);
				}
				Py_DECREF(finished);
			}
		}

		for (auto&& key : needs_removing) {
			background_task_runners.erase(key);
		}

		return background_task_runners.size() == 0;
	}
}


/*
 def invoke_each_skip_step(self, skip_seconds = 0.005, sleep_seconds = 0.012) :
*/

void start_keep_background(PyObject *backgroundTaskRunner, double fps, double load_factor)
{
	//std::lock_guard<std::mutex> guard(background_keeper_mutex);

	{
		// create key
		static constexpr int KEY_LENGTH = 6;
		char key[KEY_LENGTH + 1];
		std::string key_str;
		do {
			gen_random(key, KEY_LENGTH);
			key_str.assign(key);
		} while (background_task_runners.find(key_str) != background_task_runners.end());


		bool should_start = background_task_runners.size() == 0;
		if (should_start) {
			current_fps = fps;
		}

		background_task_runners.insert(std::make_pair(key_str, backgroundTaskRunner));


		load_factor = 1;

		if (current_fps < fps) {
			current_fps = fps;
		}

		// validation 1
		if (load_factor < 0.000001) {
			load_factor = 0.000001;
		}
		else if (load_factor > 0.999999) {
			load_factor = 0.999999;
		}

		// set limitation for fps
		if (current_fps < 0.000001) {
			current_fps = 0.000001;
		}
		else if (current_fps > 1000000) {
			current_fps = 1000000;
		}

		double skip_factor = 1 - load_factor;
		double time_of_frame = 1 / current_fps; // 0.0166667

		// validation 2
		if (time_of_frame * skip_factor < 0.001) {
			skip_factor = 0.001 / time_of_frame;

			if (skip_factor < 0.000001) {
				skip_factor = 0.000001;
			}
			else if (skip_factor > 0.999999) {
				skip_factor = 0.999999;
			}
			load_factor = 1 - skip_factor;
		}

		current_skip_ms = static_cast<uint_fast32_t>(1000 * time_of_frame * skip_factor);

		current_sleep_nanoseconds = static_cast<int64_t>(1000 * 1000 * 1000 * time_of_frame * load_factor);

		if (should_start) {
			BackgroundTask::start_request_next_skip();
		}
	}
}



PyObject *_run_in_background(PyObject *self, PyObject *args, PyObject *keywds)
{
	auto arg_len = PyTuple_Size(args);
	if (PyVars::BackgroundTaskRunner && arg_len > 0) {
		auto *func = PyTuple_GetItem(args, 0); // Borrowed reference
		if (func && PyCallable_Check(func)) {
			Py_INCREF(func);

			// runb(func, {}, 1, 2)
			// runb(func, 1, 2)
			// runb(func)
			size_t args_size = PyTuple_GET_SIZE(args);

			int next_start_index = 1;
			if (args_size > 1) {
				auto *first = PyTuple_GetItem(args, 1);
				next_start_index = PyDict_Check(first) ? 2 : 1;
			}


			size_t func_arg_count = 0;
			bool has_varargs_flag = false;

			{
				PyObject *func_info = func;
				int self_count = 0;
				if (PyMethod_Check(func)) {
					func_info = PyMethod_GET_FUNCTION(func);
					self_count = 1;
				}


				DCHECK(PyFunction_Check(func_info));
				if (PyFunction_Check(func_info)) {
					auto *func_obj = reinterpret_cast<PyFunctionObject*>(func_info);
					DCHECK(PyCode_Check(func_obj->func_code));
					auto *code_obj = reinterpret_cast<PyCodeObject*>(func_obj->func_code);


					has_varargs_flag = code_obj->co_flags & CO_VARARGS;
					func_arg_count = code_obj->co_argcount - self_count;

					auto *_defaults = func_obj->func_defaults;
					size_t def_arg_count = 0;
					if (_defaults  && PyTuple_Check(_defaults)) {
						def_arg_count = PyTuple_Size(_defaults);

						if (def_arg_count > 0) {
							func_arg_count -= def_arg_count;
						}
					}

					if (func_arg_count < 0) {
						DCHECK(false);
						func_arg_count = 0;
					}
				}
			}

			size_t set_arg_count = args_size - next_start_index;
			if (func_arg_count > set_arg_count) {
				set_arg_count = func_arg_count;
			}
			else {
				if (has_varargs_flag == false) {
					if (func_arg_count < set_arg_count) {
						set_arg_count = func_arg_count;
					}
				}
			}


			PyObject *args_tuple = PyTuple_New(1 + set_arg_count);
			PyTuple_SetItem(args_tuple, 0, func);
			for (size_t insert_index = 1, i = next_start_index; insert_index < set_arg_count + 1; i++, insert_index++) {
				if (i < args_size) {
					auto *item = PyTuple_GetItem(args, i);
					Py_INCREF(item);
					PyTuple_SetItem(args_tuple, insert_index, item);
				}
				else {
					Py_INCREF(Py_None);
					PyTuple_SetItem(args_tuple, insert_index, Py_None);
				}
			}

			//     def __init__(self, function, *args, **kwargs):
			PyObject *backgroundTaskRunner = PyObject_Call(PyVars::BackgroundTaskRunner, args_tuple, keywds);
			Py_DECREF(args_tuple);


			std::thread thread([](PyObject *backgroundTaskRunner) {

				// obtain gil to run function in background thread
				{
					with_gil gil;

					if (auto *invoker = PyObject_GetAttrString(backgroundTaskRunner, "invoke")) {
						if (auto *ret = PyObject_CallObject(invoker, NULL)) {
							Py_DECREF(ret);
						}
						Py_DECREF(invoker);
					}
				}

				/*
				if (background_task_runners.size() == 1) {
					BackgroundTask::requestSleepEnd();
				}
				*/

				// maybe should check keeping-background ends if no threads

			}, backgroundTaskRunner);

			thread.detach();

			/*
			PyThreadState *_save = PyEval_SaveThread();
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
			PyEval_RestoreThread(_save);
			*/

			// call start_skip_on_ui to keep calling minimal 
			{
				double fps = 60.0f;
				double load_factor = 0.9127;

				if (arg_len > 1) {
					PyObject *option_dict = PyTuple_GetItem(args, 1);

					if (PyDict_Check(option_dict)) {
						PyObject *fpsvalue = PyDict_GetItemString(option_dict, "fps");
						PyObject *loadvalue = PyDict_GetItemString(option_dict, "load");

						if (fpsvalue) {
							if (PyFloat_Check(fpsvalue)) {
								fps = PyFloat_AsDouble(fpsvalue);
							}
							else if (PyLong_Check(fpsvalue)) {
								fps = PyLong_AsDouble(fpsvalue);
							}
						}

						if (loadvalue) {
							if (PyFloat_Check(loadvalue)) {
								load_factor = PyFloat_AsDouble(loadvalue);
							}
							else if (PyLong_Check(loadvalue)) {
								load_factor = PyLong_AsDouble(loadvalue);

							}
						}

					}
				}

				start_keep_background(backgroundTaskRunner, fps, load_factor);
			}

			PyErr_Clear();

			if (auto *get_future = PyObject_GetAttrString(backgroundTaskRunner, "get_future")) {
				PyObject* future = PyObject_CallObject(get_future, NULL);
				Py_DECREF(get_future);

				if (future) {
					return future;
				}
			}

			PyErr_Clear();
		}
	}


	// what is this? what for?
	if (auto *ensure_future = PyObject_GetAttrString(PyVars::JsUvEventLoop, "invokeNext2")) {
		auto *fromCoroutine = PyObject_CallObject(ensure_future, NULL);
		if (fromCoroutine != NULL) {
			Py_DECREF(fromCoroutine);
		}
		Py_DECREF(ensure_future);
	}

	Py_INCREF(Py_None);
	return Py_None;
}


class run_in_newthread_Item : public JsCallFromBackground {
public:
	~run_in_newthread_Item() override = default;

	PyObject *self;
	PyObject *args;
	PyObject *keywds;

	void ui_getjs() override {
		this->set_box(std::unique_ptr<UiBox>(new UiBox(_run_in_background(this->self, this->args, this->keywds), false)));
	}

	PyObject* getPyObject() override {
		return JsCallFromBackground::treat_exception_holder(this->release_box());
	}
};



PyObject *BackgroundTask::run_in_newthread(PyObject *self, PyObject *args, PyObject *keywds)
{
	if (CustomModuleManager::inMainThread()) {
		return _run_in_background(self, args, keywds);
	}

	auto *item = new run_in_newthread_Item();
	item->self = self;
	item->args = args;
	item->keywds = keywds;

	auto *pyobj = JsCallFromBackground::call_js_from_background(item);

	if (pyobj) {
		return pyobj;
	}

	Py_INCREF(Py_False);
	return Py_False;
}






/*
 *	skip time for BackgroundTaskRunner
*/



class SkipAsyncData {

public:
	SkipAsyncData();

	int64_t late_value = 5;
	uint64_t start_time = 0;

	virtual ~SkipAsyncData() = default;
private:

	SkipAsyncData(SkipAsyncData const&) = delete;
	SkipAsyncData(SkipAsyncData&&) = delete;
	SkipAsyncData& operator =(SkipAsyncData const&) = delete;
	SkipAsyncData& operator =(SkipAsyncData&&) = delete;
};

SkipAsyncData::SkipAsyncData() {

}


static void _on_background_task_runner_call_back(uv_timer_t *timer)
{
	bool empty = remove_background_task();

	if (empty == false) {
		BackgroundTask::request_next_skip();
	}

	//PyErr_Clear();

	uv_close(reinterpret_cast<uv_handle_t*>(timer), [](uv_handle_t* handle2) {
		SkipAsyncData *asyncData = (SkipAsyncData *)handle2->data;
		delete asyncData;
		delete handle2;
	});
}


static void _on_background_task_runner(uv_async_t* handle)
{
	auto *asyncData = reinterpret_cast<SkipAsyncData *>(handle->data);

	int64_t late = asyncData->late_value;

	auto *timer1 = new uv_timer_t;
	uv_loop_t *loop = uv_default_loop();
	uv_timer_init(loop, timer1);
	timer1->data = handle->data;


	uv_update_time(loop);
	const auto nowtime = uv_now(loop);
	if (nowtime > asyncData->start_time) {
		late -= nowtime - asyncData->start_time;
	}

	if (late <= 0) {
		late = 0;
	}

	uv_timer_start(timer1, _on_background_task_runner_call_back, /*timeout*/late, /*repeat*/ 0);

	uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
		delete handle2;
	});
}


static std::mutex main_sleeper_mutex;
static std::atomic<bool> main_sleeper_ready{ false };
//static bool main_sleeper_ready{ false };
static std::condition_variable main_sleeper_cond;
static std::atomic<bool> is_waiting{ false };



static std::mutex wait_back_mutex;
static std::condition_variable wait_back_cond;
static bool wait_back_ready;


static std::atomic<uint64_t> wait_start_time;
static JsCallFromBackground* current_wait_item = nullptr;

static std::mutex entry_lock_mutex;

static std::atomic<bool> is_waitback_request{ false };

static std::atomic<bool> is_serving{ false };


static std::atomic<bool> is_boost_waiting{ false };
static std::atomic<uint64_t> client_boost_index{ 0 };
static std::atomic<uint64_t> boost_accept_index{ 1 };
static std::atomic<uint64_t> server_boost_index{ 1 };
static std::atomic<bool> boost_request_accepted{ false };
static std::atomic<bool> boost_server_executed{ false };
static std::atomic<bool> boost_client_complete{ false };
static std::atomic<bool> boost_client_accepted{ false };

static std::atomic<JsCallFromBackground*> boost_request_item;


#if defined(OS_MACOSX)
constexpr bool BOOST_ENABLED{ true };
constexpr int BOOST_WAIT_TIME{ 7777 };
#else
constexpr bool BOOST_ENABLED{ true };
constexpr int BOOST_WAIT_TIME{ 3880 };

#endif



PyObject* BackgroundTask::canDoUiCode(JsCallFromBackground *item)
{
	std::unique_lock<std::mutex> lock(entry_lock_mutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		//return PyVars::JsExceptionHolder;
		PyThreadState *saveState5 = PyEval_SaveThread();
		lock.lock();
		PyEval_RestoreThread(saveState5);
	}


	if (BOOST_ENABLED) {
		for (int a = 0; a < 400; a++) {
			if (is_boost_waiting && client_boost_index.load() < server_boost_index.load()) {
				//if (is_serving) {
				if (uv_hrtime() - wait_start_time + 10 * 1000 > static_cast<uint64_t>(current_sleep_nanoseconds)) {
					return PyVars::JsExceptionHolder;
				}
				//}

				boost_request_item = nullptr;
				boost_request_accepted = false;
				boost_server_executed = false;

				boost_client_complete = false;
				boost_client_accepted = false;

				boost_request_item = item;
				boost_accept_index++;
				auto this_time_accept_index = boost_accept_index.load();
				client_boost_index = server_boost_index.load();


				for (int j = 0; j < 2800; j++) {
					if (server_boost_index.load() != client_boost_index) {
						break;
					}
					if (boost_request_accepted) {
						auto *saveState2 = PyThreadState_Get();

						boost_client_accepted = true;

						auto complete_start_time = uv_hrtime();
						uint64_t wait_count = 0;
						bool using_sleep_mode = false;

						while (true) {
							if (using_sleep_mode) {
								std::this_thread::sleep_for(std::chrono::microseconds(1000));
							}
							else {
								if (wait_count++ % 10000 == 7777) {
									if (uv_hrtime() - complete_start_time > 3 * 1000 * 1000) {
										// js process takes a few milli seconds,
										// now it's worth using sleep with 1 milli seconds
										using_sleep_mode = true;
									}
								}
							}

							if (this_time_accept_index != boost_accept_index) {
								//if (server_boost_index.load() != client_boost_index.load()) {
									//DCHECK(CustomModuleManager::consoleInfo("break"));
								break;
							}

							if (boost_server_executed) {
								PyThreadState_Swap(saveState2);
								boost_client_complete = true;

								PyObject * pyobj = item->getPyObject();
								return pyobj;
							}
						}
						break;
					}
				}
				break;
			}

			if (is_waiting) {
				break;
			}
		}
	}


	if (is_waiting == false) {
		//return PyVars::JsExceptionHolder;
	}

	wait_back_ready = false;
	is_waitback_request = false;

	std::unique_lock<std::mutex> lock2{ wait_back_mutex };

	//if (is_serving) {
	if (uv_hrtime() - wait_start_time + 10 * 1000 > static_cast<uint64_t>(current_sleep_nanoseconds)) {
		return PyVars::JsExceptionHolder;
	}
	//}

	PyThreadState *saveState = nullptr;

	{
		std::lock_guard<std::mutex> lock5{ main_sleeper_mutex };

		if (is_waiting == false) { // || main_sleeper_ready == true) {
			return PyVars::JsExceptionHolder;
		}

		current_wait_item = item;

		is_waitback_request = true;
		main_sleeper_ready = true;

		saveState = PyThreadState_Get();

		main_sleeper_cond.notify_all();
	}

	wait_back_cond.wait(lock2, [] { return wait_back_ready == true; });
	is_waitback_request = false;

	PyThreadState_Swap(saveState);

	auto *pyobj = item->getPyObject();
	return pyobj;
}



void BackgroundTask::requestSleepEnd()
{
	std::lock_guard<std::mutex> l(main_sleeper_mutex);
	main_sleeper_ready = true;
	if (is_waiting) {
		main_sleeper_cond.notify_all();
	}
}

void BackgroundTask::restoreSleeperReady()
{
	std::lock_guard<std::mutex> l(main_sleeper_mutex);

	if (main_sleeper_ready) {
		main_sleeper_ready = false;
	}
	/*
	*/
}


static int64_t wait_remain;

void BackgroundTask::start_request_next_skip()
{
	wait_remain = current_sleep_nanoseconds;
	main_sleeper_ready = false;
	BackgroundTask::request_next_skip();
}



void BackgroundTask::request_next_skip()
{
	is_serving = true;

	uint_fast32_t late = current_skip_ms;

	// uv_update_time(uv_default_loop()); // no need

	PyThreadState *saveState = PyEval_SaveThread();
	wait_start_time = uv_hrtime();

	//wait_remain = current_sleep_nanoseconds;
	if (wait_remain < 1000 * 1000) {
		if (wait_remain < 0) {
			wait_remain = 0;
		}
		wait_remain = current_sleep_nanoseconds -wait_remain;
	}
	else {
		//wait_remain = wait_remain;
		//DCHECK(wait_remain > 0);
		//if (wait_remain < 0) {
			//wait_remain = current_sleep_nanoseconds;
		//}
	}


	int64_t start_wait_remain = wait_remain;
	{


		int count = 0;
		int gcount = 0;
		bool might_have_js_access = false;
		bool first = true;
		while (true) {
			if (BOOST_ENABLED) { // boost with no mutexes
				bool is_accepted = false;
				server_boost_index++;
				is_boost_waiting = true;

				for (int i = 0; i < BOOST_WAIT_TIME/*6666*//*3880*/; i++) {
					if (client_boost_index == server_boost_index) {
						is_boost_waiting = false;
						auto *item = boost_request_item.load();
						DCHECK(item);
						if (item) {
							boost_request_accepted = true;

							for (int j = 0; j < 5500; j++) {
								if (boost_client_accepted) {
									//server_boost_index++;

									is_accepted = true;

									if (item->ui_get_done == false) {
										PyThreadState_Swap(saveState);
										item->ui_get_done = true;
										item->ui_getjs();
										PyThreadState_Swap(NULL);
									}

									boost_server_executed = true;
									for (int k = 0; k < 10; k++) {
										//while (true) {
										if (boost_client_complete) {
											break;
										}
									}
									break;

								}
							}
							if (is_accepted == false) {
								boost_accept_index++;
							}
						}
						break;
					}

					if (might_have_js_access == false) {
						might_have_js_access = true;
						//break;
					}
				}


				server_boost_index++;

				is_boost_waiting = false;

				wait_remain = start_wait_remain - (uv_hrtime() - wait_start_time);
				if (wait_remain < 10 * 1000) {
					if (is_accepted) {
						late = 0;
					}
					break;
				}

				if (is_accepted) {
					if (first) {
						first = false;
					}
					gcount++;
					continue;
				}
			}

			std::unique_lock<std::mutex> lock{ main_sleeper_mutex };

			{
				if (main_sleeper_ready == true) {
					main_sleeper_ready = false;
					late = 0;
					break;
				}

				is_waiting = true;
				auto notified = main_sleeper_cond.wait_for(lock, std::chrono::nanoseconds(wait_remain),
					[] { return main_sleeper_ready == true; }
				);
				is_waiting = false;
				main_sleeper_ready = false;

				if (notified) {
					if (is_waitback_request == true) {
						is_waitback_request = false;

						{
							std::lock_guard<std::mutex> lock4{ wait_back_mutex };

							if (!current_wait_item->ui_get_done) {
								DCHECK(PyGILState_GetThisThreadState() == saveState);
								DCHECK(PyThreadState_Get() != saveState);
								PyThreadState_Swap(saveState);
								DCHECK(PyThreadState_Get() == saveState);

								current_wait_item->ui_get_done = true;
								current_wait_item->ui_getjs();
								PyThreadState_Swap(NULL);
							}

							wait_back_ready = true;
							wait_back_cond.notify_all();
						}

						wait_remain = start_wait_remain - (uv_hrtime() - wait_start_time);

						if (wait_remain > 500 * 1000) {
							count++;
							continue;
						}
					}

					late = 0;
				}
				else {
					late = current_skip_ms;
				}
			}

			break;
		}

		/*

		printf("c=%d,", count);
		printf("g=%d,", gcount);
		printf("late=%d,", late);
		printf("wait_remain=%lld\n", wait_remain);
		//fflush(stdout);
						*/

	}


	PyEval_RestoreThread(saveState);
	wait_remain = start_wait_remain - (uv_hrtime() - wait_start_time);
	
	/*if (late == 0) {
		wait_remain = start_wait_remain - (uv_hrtime() - wait_start_time);
	}
	else {
		wait_remain = current_sleep_nanoseconds;
	}
	*/


	{
		uv_loop_t *loop = uv_default_loop();
		uv_update_time(loop);
		const auto nowtime = uv_now(loop);


		auto *async1 = new uv_async_t;
		SkipAsyncData *asyncData = new SkipAsyncData();

		asyncData->late_value = late;
		asyncData->start_time = nowtime;

		async1->data = asyncData;

		uv_async_init(loop, async1, _on_background_task_runner);
		uv_async_send(async1);

	}

	is_serving = false;

}