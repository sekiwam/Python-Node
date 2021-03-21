#include "node_ref.h"

#include <string>

#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>

#include <stdint.h>
#include <ctime>
#include "wait_promise.h"
#include "bg_task.h"
#include "pyconverters.h"
#include "CustomModules.h"

#include "JsObject.h"
#include "JsVars.h"
#include "uibox.h"
#include "PlynthUtils.h"

/*
 * Implements background task related task
 */

using namespace std;
using namespace v8;
using namespace plynth;



class await_Item : public JsCallFromBackground {
public:
	~await_Item() override = default;

	PyObject *self;
	PyObject *args;
	std::string thread_id;

	MutexPairs *mutexPairs;

	void ui_getjs() override;

	PyObject* getPyObject() override {
		return NULL;
	}
};



MutexSetManager wait_mutexManager;

std::unordered_map<std::string, await_Item*> await_item_map;
// static uint64_t await_index = 0;

/// <summary>
/// call await anywhere even in background to make it true thread-safe
/// 
/// result = plynth.wait(future)
/// result = plynth.wait(jsPromise)
/// </summary>
static PyObject *_await_promise_or_future(await_Item *item)
{
	PyObject *args = item->args;
	bool set_then = false;

	auto arg_len = PyTuple_Size(args);
	if (arg_len > 0) {
		auto *arg = PyTuple_GetItem(args, 0); // Borrowed reference

		// check whether it' PromiseLike
		if (PyObject_HasAttrString(arg, "then")) {

			PyObject* pyobj = arg;
			auto jsobj = JsObjectReg::getTargetV8ObjectFromPyObject(pyobj);
			if (jsobj.IsEmpty() == false && jsobj->IsUndefined() == false) {

				//await_Item
				auto context = JsVars::getInstance()->getIsolate()->GetCurrentContext();

				std::ostringstream oss;
				oss << std::dec << reinterpret_cast<uintptr_t>((void*)item);
				std::string s(oss.str());

				{
					// then
					auto jsfunc = [](const FunctionCallbackInfo<Value>& info) {
						v8::Local<v8::Value> str = info.Data();

						v8::String::Utf8Value utf_str(JsVars::getIsolate(), str);
						std::string stdstr(*utf_str);

						auto t = await_item_map.find(stdstr);
						auto *item = (t == await_item_map.end()) ? nullptr : t->second;

						DCHECK(item);

						if (item) {
							await_item_map.erase(stdstr);

							if (info.Length() > 0) {
								auto result = info[0];
								if (result.IsEmpty() == false) {
									PyObject* pyobj = JsValue_to_PyObject(result);
									item->set_box(std::unique_ptr<UiBox>(new UiBox(pyobj, false)));
								}
							}

							{
								std::lock_guard<std::mutex> l(item->mutexPairs->async_mutex);
								item->mutexPairs->ready = true;
							}

							item->mutexPairs->cond.notify_all();
						}

					};

					// catch
					auto jsfunc2 = [](const FunctionCallbackInfo<Value>& info) {
						v8::Local<v8::Value> str = info.Data();
						//auto *ex_chs = *v8::String::Utf8Value(str);
						v8::String::Utf8Value utf_str(JsVars::getIsolate(), str);
						std::string stdstr(*utf_str);

						auto t = await_item_map.find(stdstr);
						auto *item = (t == await_item_map.end()) ? nullptr : t->second;

						DCHECK(item);
						if (item) {
							await_item_map.erase(stdstr);

							if (info.Length() > 0) {
								auto result = info[0];

								if (result.IsEmpty() == false) {
									auto box = JsCallFromBackground::treat_exception(result, false);
									item->set_box(std::move(box));
								}
							}

							{
								std::lock_guard<std::mutex> l(item->mutexPairs->async_mutex);
								item->mutexPairs->ready = true;
							}

							item->mutexPairs->cond.notify_all();

						}
					};

					// std::string a = std::string("a") + std::to_string(await_index++);


					await_item_map[item->thread_id] = item;

					//auto passData = String::NewFromUtf8(JsVars::getInstance()->getIsolate(), item->thread_id.c_str());

					auto passData  = String::NewFromUtf8(JsVars::getInstance()->getIsolate(), item->thread_id.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

					auto func = v8::Function::New(context, jsfunc, passData).ToLocalChecked();
					auto func2 = v8::Function::New(context, jsfunc2, passData).ToLocalChecked();

					auto v8name = String::NewFromUtf8(JsVars::getInstance()->getIsolate(), "then",
						v8::NewStringType::kNormal).ToLocalChecked();
					auto catcher_caller = jsobj->Get(context, v8name);
					if (catcher_caller.IsEmpty() == false) {
						Local<Value> create_args[] = {
							func,func2
						};

						catcher_caller.ToLocalChecked().As<Function>()->Call(context, jsobj, 2, create_args).IsEmpty();
						set_then = true;
					}
				}

				/*
				if (PyErr_Occurred()) {
					PyErr_Clear();
				}
				*/
			}

			//Py_DECREF(pyobj);
		}
	}

	if (set_then == false) {
		DCHECK(false);
		{
			std::lock_guard<std::mutex> l(item->mutexPairs->async_mutex);
			item->mutexPairs->ready = true;
		}

		//wait_condition5.notify_all();
		item->mutexPairs->cond.notify_all();
	}


	Py_INCREF(Py_False);
	return Py_False;
}


void await_Item::ui_getjs() {
	this->set_box(std::unique_ptr<UiBox>(new UiBox(_await_promise_or_future(this), false)));
}



class wait_loop_Item : public JsCallFromBackground {
public:
	~wait_loop_Item() override;

	PyObject *self;
	void ui_getjs() override {
	}

	PyObject* getPyObject() override {
		return NULL;
	}
};


wait_loop_Item::~wait_loop_Item() {

}

/*
static PyObject *plynth_sleep(PyObject *self, PyObject *args)
{
	PyThreadState *saveState = PyEval_SaveThread();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	PyEval_RestoreThread(saveState);

	Py_INCREF(Py_None);
	return Py_None;
}
*/

// plynth.wait() in @plynth.background
PyObject *WaitPromise::await_promise_or_future(PyObject *self, PyObject *args)
{
	//if (CustomModuleManager::inMainThread()) {
	//	return _await_promise_or_future(self, args);
	//}
	PyThreadState *_save0 = PyEval_SaveThread();
	std::stringstream ss; ss << std::this_thread::get_id();
	const std::string &th_id = ss.str();


	with_thread_mutex_set pair_scope(&wait_mutexManager);
	auto *mutexPair = pair_scope.pair;// wait_mutexManager.getMutexPairs(th_id);

	std::lock_guard<std::mutex> l(mutexPair->method_entry_mutex);
	{
		auto *item = new await_Item();

		item->self = self;
		item->args = args;
		item->mutexPairs = mutexPair;

		item->thread_id = th_id;

		mutexPair->ready = false;

		PyEval_RestoreThread(_save0);

		if (PyObject * nonepy = JsCallFromBackground::call_js_from_background(item, false)) {
			// we will use uibox->get_pyobject() for processing later
			Py_DECREF(nonepy);
		}

		PyThreadState *_save = PyEval_SaveThread();

		BackgroundTask::requestSleepEnd();

		unique_lock<mutex> lk(mutexPair->async_mutex);

		mutexPair->cond.wait(lk, [&]() { return mutexPair->ready == true; });

		PyEval_RestoreThread(_save);

		PyObject *ret_pyobj = NULL;

		auto uibox = item->release_box();
		if (uibox) {
			if (uibox->is_exception()) {
				auto *pyo = uibox->get_pyobject();
				PyErr_SetObject(PyVars::JsException, pyo);
				Py_DECREF(pyo);
			}
			else {
				ret_pyobj = uibox->get_pyobject();
				if (ret_pyobj == NULL) {
					Py_INCREF(Py_False);
					ret_pyobj = Py_False;
				}

				if (PyErr_Occurred()) {
					//PyErr_Clear();
				}
			}
		}

		delete item;

		return ret_pyobj;

	}


	Py_INCREF(Py_False);
	return Py_False;
}