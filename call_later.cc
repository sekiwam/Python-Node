#include "node_ref.h"

#include <string>

#include <thread>
#include <chrono>
#include <mutex>

#include <stdint.h>
#include <ctime>
#include "call_later.h"
#include "pyconverters.h"
#include "CustomModules.h"

#include "uibox.h"
#include "PlynthUtils.h"

/*
 * Implements call_later in EventLoop
 */

using namespace std;
using namespace v8;
using namespace plynth;


void CallLaterReg::Init(v8::Local<v8::Object> rootJs)
{
	//uv_thread_t this_thread = uv_thread_self();
	//auto eq = uv_thread_equal(&main_thread, &this_thread);
}



class AsyncData {

public:
	AsyncData();
	uint64_t late_value = 5;
	uint64_t start_time = 0;
	long long later_index = 0;

	virtual ~AsyncData() = default;
private:

	AsyncData(AsyncData const&) = delete;
	AsyncData(AsyncData&&) = delete;
	AsyncData& operator =(AsyncData const&) = delete;
	AsyncData& operator =(AsyncData&&) = delete;
};

AsyncData::AsyncData() {

}


static void _on_callat_elapsed(uv_timer_t *timer)
{
	DCHECK(PyObject_HasAttrString(PyVars::JsUvEventLoop, "invokeNext"));

	if (auto *invoke_next_attr = PyObject_GetAttrString(PyVars::JsUvEventLoop, "invokeNext")) {
		auto *arg_tuple = PyTuple_New(1);

		AsyncData *asyncData = (AsyncData *)timer->data;
		PyTuple_SetItem(arg_tuple, 0, PyLong_FromLongLong(asyncData->later_index));
		if (const PyObject* future = PyObject_CallObject(invoke_next_attr, arg_tuple)) {
			Py_DECREF(future);
		}
		Py_DECREF(invoke_next_attr);
		Py_DECREF(arg_tuple);
		
		if (PyErr_Occurred()) {
			PyErr_Clear();
		}
	}

	uv_close(reinterpret_cast<uv_handle_t*>(timer), [](uv_handle_t* handle2) {
		AsyncData *asyncData = (AsyncData *)handle2->data;
		delete asyncData;
		delete handle2;
	});
}


static void _on_completed_3(uv_async_t* handle)
{
	AsyncData *asyncData = (AsyncData *)handle->data;

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

	if (late < 0) {
		late = 0;
	}

	uv_timer_start(timer1, _on_callat_elapsed, /*timeout*/late, /*repeat*/	0);

	uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
		delete handle2;
	});
}


 


void _start_call_later(PyObject *self, PyObject *args)
{
	auto arg_len = PyTuple_Size(args);
	if (arg_len > 1) {
		auto *py_later_index = PyTuple_GetItem(args, 0);
		auto *py_interval = PyTuple_GetItem(args, 1);

		long later_index = -1;
		if (PyLong_Check(py_later_index)) {
			later_index = PyLong_AsLong(py_later_index);
		}

		if (later_index > -1) {
			int64_t late = -1;
			if (PyLong_Check(py_interval)) {
				late = PyLong_AsLong(py_interval);
			}
			else if (PyFloat_Check(py_interval)) {
				late = static_cast<int64_t>(PyFloat_AsDouble(py_interval));
			}

			auto *async1 = new uv_async_t;
			std::unique_ptr<AsyncData> asyncData(new AsyncData());

			uv_loop_t *loop = uv_default_loop();
			uv_update_time(loop);
			const auto nowtime = uv_now(loop);


			asyncData->late_value = late;
			asyncData->start_time = nowtime;
			asyncData->later_index = later_index;

			async1->data = asyncData.release();

			uv_async_init(loop, async1, _on_completed_3);

			uv_async_send(async1);
		}
	}
}


PyObject *CallLaterReg::start_call_later(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		_start_call_later(self, args);
	}
	Py_INCREF(Py_None);
	return Py_None;
}


 