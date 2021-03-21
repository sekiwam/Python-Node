#include "node_ref.h"

#include <string>

#include <stdio.h>


#include <queue>
#include <mutex>
#include <cstdint>
#include <Python.h>

#include <stdio.h>
#include <string>
#include <vector>

#include <sstream>

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory>
#include <thread>

#include "JsCallObject.h"
#include "JsObject.h"
#include "JsVars.h"

#include "call_later.h"
#include "bg_task.h"
#include "pyconverters.h"
#include "PlynthUtils.h"
#include "CustomModules.h"


#include "uibox.h"



using namespace v8;
using namespace std;

namespace plynth {


	UiBox::UiBox(PyObject *pyobj, bool is_exception)
	{
		this->_pyobj = pyobj;
		this->_is_exception = is_exception;
	}


	JsCallFromBackground::JsCallFromBackground() {

	}

	JsCallFromBackground::~JsCallFromBackground() {
		this->return_pyobj.reset(NULL);
	}

	//this->return_pyobj.reset(NULL);
//}



	template <typename T>
	class BgAsyncData {
		static_assert(std::is_base_of<JsCallFromBackground, T>::value, "T must inherit from JsCallFromBackground");

	public:
		BgAsyncData() = default;

		T *item;

		MutexPairs *mutex_pair;

		~BgAsyncData() = default;

	private:

		BgAsyncData(BgAsyncData const&) = delete;
		BgAsyncData(BgAsyncData&&) = delete;
		BgAsyncData& operator =(BgAsyncData const&) = delete;
		BgAsyncData& operator =(BgAsyncData&&) = delete;
	};


	PyObject * JsCallFromBackground::treat_exception_holder(std::unique_ptr<UiBox> box) 
	{
		if (box->is_exception()) {
			if (PyObject * obj = box->get_pyobject()) {
				PyErr_SetObject(PyVars::JsException, obj);
				Py_DECREF(obj);
			}

			return NULL;
		}

		return box->get_pyobject();
	}


	std::unique_ptr<UiBox> JsCallFromBackground::treat_exception(v8::Local<v8::Value> exception, bool is_main_thread)
	{
		PyObject *py_dict = PyDict_New();
		PyObject *arg = JsValue_to_PyObject(exception);
		PyDict_SetItemString(py_dict, "obj", arg);
		Py_DECREF(arg);

		if (is_main_thread) {
			PyErr_SetObject(PyVars::JsException, py_dict);
			Py_DECREF(py_dict);
			return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, true));
		}
		else {
			return std::unique_ptr<UiBox>(new UiBox(py_dict, true));
		}
	}


	PyObject *JsCallFromBackground::call_js_from_background(JsCallFromBackground *item)
	{
		return JsCallFromBackground::call_js_from_background(item, true);
	}


	static std::vector<BgAsyncData<JsCallFromBackground>*> _jscall_items;
	static std::mutex _organize_mutex;

	void _on_completed_bg_thread2(uv_async_t* handle)
	{

		std::vector<BgAsyncData<JsCallFromBackground>*> do_items;

		BackgroundTask::restoreSleeperReady();

		{
			std::lock_guard<std::mutex> lock{ _organize_mutex };

			for (auto *asyncData : _jscall_items) {
				do_items.push_back(asyncData);
			}

			_jscall_items.clear();

		}

		for (auto *asyncData : do_items) {
			{
				std::lock_guard<std::mutex> lk{ asyncData->mutex_pair->async_mutex };

				if (asyncData->item->ui_get_done == false) {
					asyncData->item->ui_get_done = true;
					asyncData->item->ui_getjs();
				}

				asyncData->mutex_pair->ready = true;
			}

			asyncData->mutex_pair->cond.notify_all();

		}



		do_items.clear();

		uv_close(reinterpret_cast<uv_handle_t*>(handle), [](uv_handle_t* handle2) {
			delete handle2;
		});
	}

	static bool add_next_async_item(BgAsyncData<JsCallFromBackground> *asyncData)
	{
		std::lock_guard<std::mutex> lock{ _organize_mutex };

		{
			bool should_start_async = _jscall_items.size() == 0;

			_jscall_items.push_back(asyncData);

			if (should_start_async) {
				auto *async1 = new uv_async_t;

				uv_async_init(uv_default_loop(), async1, _on_completed_bg_thread2);
				uv_async_send(async1);

				BackgroundTask::requestSleepEnd();

				return true;
			}


			return false;

		}
	}


	PyObject *JsCallFromBackground::call_js_from_background(JsCallFromBackground *item, bool deleteItem)
	{
		static MutexSetManager jsCallThreadMutexSetManager;


		PyObject *pyobj = BackgroundTask::canDoUiCode(item);
		if (pyobj != PyVars::JsExceptionHolder) {
			if (deleteItem) {
				delete item;
			}
			return pyobj;
		}


		with_thread_mutex_set pair_scope(&jsCallThreadMutexSetManager);

		auto *pair = pair_scope.pair;
		DCHECK(pair);

		{
			// lock 1
			std::lock_guard<std::mutex> lock0{ pair->method_entry_mutex };
			{
				auto *asyncData = new BgAsyncData<JsCallFromBackground>();
				asyncData->item = item;
				asyncData->mutex_pair = pair;

				pair->ready = false;

				add_next_async_item(asyncData);

				PyThreadState *saved_state = PyEval_SaveThread();



				// lock 2 
				std::unique_lock<std::mutex> lock{ pair->async_mutex };
				{
					pair->cond.wait(lock, [&pair] { return pair->ready == true; });
					


					PyEval_RestoreThread(saved_state);

					PyObject *ret_py = item->getPyObject();

					if (deleteItem) {
						delete asyncData->item;
					}

					delete asyncData;

					return ret_py;
				}
			}

		}
	}
}
