#include "node_ref.h"
//#include <uv.h>
#include <string>
#include <unordered_map>

#include <stdio.h>


#include <unordered_map>
#include <cstdint>
#include <Python.h>
#include <structmember.h>

#include <stdio.h>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>


#include <thread>

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h> //  uint8_t, uint32_t


#include "WeakValueMap.h"
#include "JsCallObject.h"
#include "JsObject.h"
#include "JsVars.h"

#include "call_later.h"
#include "bg_task.h"
#include "CustomModules.h"

#include "pyconverters.h"


using namespace v8;
using namespace std;
using namespace plynth;




class Counter {
public:
	static long long _current_func_index;
	static long long _obj_index;
};
long long Counter::_current_func_index = 1;
long long Counter::_obj_index = 1;


static constexpr char FUNC_KEY_INDEX_NAME[] = "__js_session_index__";
static constexpr char OBJ_BIND_KEY_INDEX_NAME[] = "__sidx__";

static WeakValueMap *closureMap = nullptr;
static WeakValueMap *jsobjMap = nullptr;

static Isolate *_isolate = nullptr;

static PyTypeObject *_jsObjectType = nullptr;
static PyTypeObject *_jsCallObjectType = nullptr;


static inline PyObject *_v8_eq_(v8::Local<v8::Object> a, v8::Local<v8::Object> b, bool reverse)
{
	bool same = false;
	if (a == b) {
		same = true;
	}

	if (reverse) {
		same = !same;
	}

	auto *boo = same ? Py_True : Py_False;
	Py_INCREF(boo);
	return boo;
}


// compare func
PyObject *richcmpfunc3(v8::Local<v8::Object> a, v8::Local<v8::Object> b, int op)
{
	if (op == Py_LT) { // <

	}
	else if (op == Py_LE) {  // <=

	}
	else if (op == Py_EQ) { // ==
		return _v8_eq_(a, b, false);
	}
	else if (op == Py_NE) { // !=
		return _v8_eq_(a, b, true);
	}
	else if (op == Py_GT) { // >

	}
	else if (op == Py_GE) { // >=

	}


	Py_INCREF(Py_False);
	return Py_False;
}




// converts v8::Array to PyList
// New Reference
PyObject* jsarray_to_pylist(const v8::Local<v8::Array> &jsArray)
{
	uint32_t len = jsArray->IsArray() ? jsArray->Length() : 0;

	auto *pylist = PyList_New(len);
	auto context = _isolate->GetCurrentContext();

	for (uint32_t i = 0; i < len; i++) {
		auto result = jsArray->Get(context, i);
		if (result.IsEmpty() == false) {
			auto jsItem = result.ToLocalChecked();
			PyList_SetItem(pylist, i, JsValue_to_PyObject(jsItem));
		}
		else {
			Py_INCREF(Py_None);
			PyList_SetItem(pylist, i, Py_None);
		}
	}

	return pylist;
}

// converts v8::Value to PyObject
// Return New Reference
PyObject* JsValue_to_PyObject(v8::Local<v8::Value> local_value)
{
	return JsValue_to_PyObject(v8::Undefined(JsVars::getIsolate()), "", local_value);
}

static v8::Global<v8::String> global_then_holder;


PyObject* JsValue_to_PyObject(v8::Local<v8::Value> ownerValue, const char *access_name, v8::Local<v8::Value> local_value)
{
	if (local_value.IsEmpty()) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	// Isolate::Scope s;
	// HandleScope scope(_isolate);

	auto context = _isolate->GetCurrentContext();


    printf("{1}");
	// Can we support for keeping reference on primitive values?
	if (local_value->IsString()) {// || local_value->IsStringObject()) {
		v8::String::Utf8Value utf_str(_isolate, local_value);
		return PyUnicode_FromString(*utf_str);
	}

	if (local_value->IsBoolean()) {// || local_value->IsBooleanObject()) {
		auto boo = local_value->BooleanValue(_isolate);// //.ToChecked();
		if (boo) {
			auto *tr = boo ? Py_True : Py_False;
			Py_INCREF(tr);
			return  tr;
		}
		else {
			Py_INCREF(Py_False);
			return Py_False;
		}
	}

	if (local_value->IsUint32()) {
		auto *py_long = PyLong_FromLong(local_value->Uint32Value(context).ToChecked());
		return py_long;
	}

	if (local_value->IsInt32()) {
		auto *py_long = PyLong_FromLong(local_value->Int32Value(context).ToChecked());
		return py_long;
	}

    printf("{2}");

	if (local_value->IsNumber()) {// || local_value->IsNumberObject()) {
		Maybe<double> numv = local_value->NumberValue(context);
		auto *pyfloat = PyFloat_FromDouble(numv.ToChecked());
		if (pyfloat != NULL) {
			return pyfloat;
		}
	}

	if (local_value->IsNullOrUndefined()) {
		Py_INCREF(Py_None);
		return Py_None;
	}


	if (local_value->IsProxy()) {
		auto *jsCallObject = (JsCallObject*)JsCallObjectReg::JsCallObject_New(NULL, NULL);
		jsCallObject->_owner.Reset(_isolate, ownerValue.As<Object>());
		jsCallObject->_function.Reset(_isolate, local_value.As<Function>());
		jsCallObject->access_name = std::string(access_name);

		return (PyObject*)jsCallObject;
	}

	if (local_value->IsFunction()) {
		auto *jsCallObject = (JsCallObject*)JsCallObjectReg::JsCallObject_New(NULL, NULL);
		jsCallObject->_owner.Reset(_isolate, ownerValue.As<Object>());
		jsCallObject->_function.Reset(_isolate, local_value.As<Function>());
		jsCallObject->access_name = std::string(access_name);

		return (PyObject*)jsCallObject;
	}

    printf("{3}");

	//* converts JsPromise to py.future which has __wait__*/
	bool promiseLike = local_value->IsPromise();
	if (promiseLike == false) {
		bool isObject = local_value->IsObject();

		if (isObject) {
			auto maybe_bool = local_value.As<Object>()->Get(context, global_then_holder.Get(_isolate));
			if (maybe_bool.IsEmpty() == false) {
				promiseLike = maybe_bool.ToLocalChecked()->IsFunction();
			}

				//promiseLike = local_value.As<Object>()->Get(String::NewFromUtf8(_isolate, "then"))->IsFunction()
				//&& local_value.As<Object>()->Get(String::NewFromUtf8(_isolate, "catch"))->IsFunction()
				//&& local_value.As<Object>()->Has(String::NewFromUtf8(_isolate, "notifyWith")) == false
		}
	}

	if (promiseLike) {
		if (PyVars::JsPromiseWrapper) {

			auto obj = local_value.As<Object>();
			//auto ownerObj = ownerValue.As<Object>();

			// We should check more than one call for memory leak?
			auto jsfunc = [](const FunctionCallbackInfo<Value>& info) {
			};

			auto context = _isolate->GetCurrentContext();
			auto passData = String::NewFromUtf8(_isolate, "abc", v8::NewStringType::kNormal).ToLocalChecked();
			auto func = v8::Function::New(context, jsfunc, passData).ToLocalChecked();


			auto func2 = obj->Get(context, String::NewFromUtf8(_isolate, "then", v8::NewStringType::kNormal).ToLocalChecked());
			if (!func2.IsEmpty()) {
				auto ff2 = func2.ToLocalChecked();
				if (ff2->IsFunction()) {
					Local<Value> create_args[] = {
						func, func
					};
					ff2.As<Function>()->Call(context, obj, 2, create_args).IsEmpty();
				}
			}
			
			/*
			auto *jsObject = (JsObject*)JsObjectReg::JsObject_new(NULL, NULL);
			jsObject->_object.Reset(_isolate, obj);

			auto *arg_tuple = PyTuple_New(1);
			PyTuple_SetItem(arg_tuple, 0, (PyObject*)jsObject);
			auto *future = PyObject_CallObject(PyVars::JsPromiseWrapper, arg_tuple);

			if (PyErr_Occurred()) {
				PyErr_Clear();
			}

			Py_DECREF(arg_tuple);
			return future;
			*/

		}

	}

    printf("{4}");


	auto *jsObject = (JsObject*)JsObjectReg::getInstance()->JsObject_new(NULL, NULL);
	jsObject->_object.Reset(_isolate, local_value.As<Object>());
	//jsObject->_object.Reset(_isolate, local_value->ToObject());
    printf("{5}");

	return (PyObject*)jsObject;

	Py_INCREF(Py_None);
	return (Py_None);

}

// this should be called in the main thread
void pyErrorLogConsole2()
{
	if (PyErr_Occurred()) {
		PyObject *type, *value, *traceback;
		PyErr_Fetch(&type, &value, &traceback);

		if (value && PyVars::error_trace) {
			auto *args = PyTuple_New(2);

			PyObject *arg0 = value == NULL ? Py_None : value;
			PyObject *arg1 = traceback == NULL ? Py_None : traceback;
			Py_INCREF(arg0);
			Py_INCREF(arg1);
			PyTuple_SetItem(args, 0, arg0);
			PyTuple_SetItem(args, 1, arg1);

			if (auto *ret = PyObject_CallObject(PyVars::error_trace, args)) {
				Py_DECREF(ret);
			}
			Py_DECREF(args);
		}

		PyErr_Clear();

	}

	if (PyErr_Occurred()) {
		PyObject *type = NULL, *value = NULL, *traceback = NULL;
		PyErr_Fetch(&type, &value, &traceback);

		if (value) {
			if (auto *str = PyObject_Str(value)) {
				auto *chs = PyUnicode_AsUTF8(str);

				auto *isolate = v8::Isolate::GetCurrent();
				auto context = _isolate->GetCurrentContext();
				auto global = isolate->GetCurrentContext()->Global();

				auto prop_name = v8::String::NewFromUtf8(_isolate, "console", v8::NewStringType::kNormal).ToLocalChecked();
				auto maybe_console = global->Get(context, prop_name);
				if (maybe_console.IsEmpty() == false) {

					auto console = maybe_console.ToLocalChecked();// String::NewFromUtf8(_isolate, "console", ));
					if (console.IsEmpty() == false) {

						auto errorname = v8::String::NewFromUtf8(_isolate, "error", v8::NewStringType::kNormal).ToLocalChecked();

						auto maybe_consoleError = console.As<Object>()->Get(context, errorname);
						if (maybe_consoleError.IsEmpty() == false)
						{
							auto consoleError = maybe_consoleError.ToLocalChecked().As<Function>();
							if (consoleError.IsEmpty() == false && consoleError->IsFunction()) {

								auto context = _isolate->GetCurrentContext();

								Local<Value> args[] = {
									String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked()
								};
								consoleError->Call(context, console, 1, args).IsEmpty();
							}
						}
						
						
					}
				}

				Py_DECREF(str);
			}
		}

		PyErr_Restore(type, value, traceback);
		PyErr_Clear();
	}

}



static void _call_pyfunc_from_jsfunc_info(const FunctionCallbackInfo<Value>& info)
{
	auto data = info.Data();
	if (data->IsString() || data->IsStringObject()) {
		auto real_arg_len = info.Length();
		
		auto context = _isolate->GetCurrentContext();
		auto __strData = data->ToString(context);
		if (__strData.IsEmpty() == false) {

			auto strData = __strData.ToLocalChecked();

			v8::String::Utf8Value utf_str(JsVars::getIsolate(), strData);


			const std::string key_str{ *utf_str };

			DCHECK(PyGILState_GetThisThreadState() != NULL);
			DCHECK(PyGILState_Check() == 1);
			DCHECK(PyThreadState_Get() != NULL);

			auto *pyKey = PyUnicode_FromString(key_str.c_str());
			auto *func = PyDict_GetItem(PyVars::pyfunction_dict, pyKey); // Return value: Borrowed reference
			Py_DECREF(pyKey);

			if (func != NULL && (PyCallable_Check(func) > 0)) {
				PyObject *func_info = func;
				//PyObject *self = nullptr;

				int self_count = 0;
				if (PyMethod_Check(func)) {
					func_info = PyMethod_GET_FUNCTION(func);
					//self = PyMethod_Self(func);
					self_count = 1;
				}

				DCHECK(PyFunction_Check(func_info));
				if (PyFunction_Check(func_info)) {
					auto *func_obj = reinterpret_cast<PyFunctionObject*>(func_info);
					DCHECK(PyCode_Check(func_obj->func_code));
					auto *code_obj = reinterpret_cast<PyCodeObject*>(func_obj->func_code);


					const bool has_varargs_flag = code_obj->co_flags & CO_VARARGS;
					auto py_func_arg_len = code_obj->co_argcount - self_count;
					// @plynth.background leads -1:  0(*args has 0) - 1(self) = -1
					//DCHECK(py_func_arg_len >= 0);
					if (py_func_arg_len < 0) {
						py_func_arg_len = 0;
					}


					// args should be filled at least by this count
					/*
					auto _defaults = func_obj->func_defaults;
					unsigned long long def_arg_count = 0;
					if (_defaults != NULL) {
					def_arg_count = PyTuple_Size(_defaults);
					}
					*/

					//int to_call_arg_len = has_varargs_flag ? real_arg_len : py_func_arg_len;
					int to_call_arg_len = py_func_arg_len;
					if (has_varargs_flag) {
						if (real_arg_len > py_func_arg_len) {
							to_call_arg_len = real_arg_len;
						}
					}

					auto *argTuple = PyTuple_New(to_call_arg_len);
					//auto undef = v8::Undefined(_isolate);

					/*
					const auto js_arguments = v8::Array::New(_isolate, static_cast<int>(real_arg_len));
					for (int i = 0; i < real_arg_len; i++) {
						js_arguments->Set(i, info[i]);
					}
					*/


					for (int js_index = 0, k = 0; k < to_call_arg_len; k++) {
						if (js_index < real_arg_len) {
							PyTuple_SetItem(argTuple, k, JsValue_to_PyObject(info[js_index]));
							js_index++;
						}
						else {
							Py_INCREF(Py_None);
							PyTuple_SetItem(argTuple, k, Py_None);
						}
					}


					// store js this
					const auto jsThis = info.This();
					if (jsThis.IsEmpty() == false) {
						JsVars::getInstance()->_current_this_context.Reset(_isolate, jsThis);
					}

					//FunctionCallbackInfo<Value> *info2 = const_cast<FunctionCallbackInfo<Value> *>(&info); // (info);
					//JsVars::getInstance()->info = info2;// &info;// .Reset(_isolate, js_arguments);
					//JsVars::getInstance()->_current_args.Reset(_isolate, js_arguments);



					/*
					PyThreadState *saveState2 = nullptr;
					//if (BackgroundTask::doing_sync_js_run) {
						if ((rand() % 53) == 1) {
							CustomModuleManager::consoleInfo("new thread ok? A");
						}

						//should
						saveState2 = PyEval_SaveThread();
						std::this_thread::sleep_for(std::chrono::microseconds((int)((rand() % 3) * 1000)));

						if ((rand() % 53) == 1) {
							CustomModuleManager::consoleInfo("new thread ok? B");
						}

						if (saveState2) {
							PyEval_RestoreThread(saveState2);
						}

						if ((rand() % 53) == 1) {
							CustomModuleManager::consoleInfo("new thread ok? C");
						}
					//}
					*/


					// We can't determine why this is necessary
					{
						if (auto *ensure_future = PyObject_GetAttrString(PyVars::JsUvEventLoop, "invokeNext2")) {
							if (auto *fromCoroutine = PyObject_CallObject(ensure_future, NULL)) {
								Py_DECREF(fromCoroutine);
							}
							Py_DECREF(ensure_future);
						}
					}

					if (PyErr_Occurred()) {
						PyErr_Clear();
					}

					PyObject *retObj = PyObject_CallObject(func, argTuple);

					//PyObject *retObj = PyObject_CallObject(func_info, argTuple);
					if (retObj) {
						info.GetReturnValue().Set(PyObject_to_JsValue(retObj));// String::NewFromUtf8(isolate, str.c_str()));
					}

					Py_DECREF(argTuple);

					pyErrorLogConsole2();

					if (retObj != NULL) {

						// future
						if (PyObject_HasAttrString(retObj, "_is_plynth")) {
							if (PyVars::print_line_number) {
								auto *args = PyTuple_New(1);
								Py_INCREF(retObj);
								PyTuple_SetItem(args, 0, retObj);
								if (auto *ret = PyObject_CallObject(PyVars::print_line_number, args)) {
									Py_DECREF(ret);
								}
								Py_DECREF(args);

							}
						}

						if (Py_TYPE(retObj) == &PyCoro_Type) {
							if (auto *ensure_future = PyObject_GetAttrString(PyVars::JsUvEventLoop, "ensure_future")) {

								// wrap with wrap_coro
								auto *wrap_coro_args = PyTuple_New(1);
								Py_INCREF(retObj);
								PyTuple_SetItem(wrap_coro_args, 0, retObj);
								auto *wrapped_coro = PyObject_CallObject(PyVars::wrap_coro, wrap_coro_args);
								Py_DECREF(wrap_coro_args);

								if (wrapped_coro) {
									auto *empty_args = PyTuple_New(1);
									PyTuple_SetItem(empty_args, 0, wrapped_coro);
									auto *fromCoroutine = PyObject_CallObject(ensure_future, empty_args);
									//PyErr_Clear();
									if (fromCoroutine) {
										Py_DECREF(fromCoroutine);
									}

									Py_DECREF(empty_args);
								}

								Py_DECREF(ensure_future);
							}
						}


						Py_DECREF(retObj);
					}


					pyErrorLogConsole2();



					// should release js this
					JsVars::getInstance()->_current_this_context.Reset();
					//				JsVars::getInstance()->_current_args.Reset();
									//JsVars::getInstance()->info = nullptr;// info2;// &info;// .Reset(_isolate, js_arguments);


				}
			}
		}
	}

	if (PyErr_Occurred()) {
		//PyErr_Clear();
	}
}


/**
*	PyFunction to v8::Function
*/
static v8::Local<v8::Function> pyfunction_to_Jsfunction2(PyObject *py_func)
{
	// return same instance of v8::function for the same PyFunction as long as possible

	auto *target_pyfunction = PyMethod_Check(py_func) ? PyMethod_GET_FUNCTION(py_func) : py_func;

	// check if jsfunc is already resistered
	if (PyObject_HasAttrString(target_pyfunction, FUNC_KEY_INDEX_NAME) > 0) {
		if (auto *att = PyObject_GetAttrString(target_pyfunction, FUNC_KEY_INDEX_NAME)) {
			auto *uni = PyUnicode_FromObject(att);
			auto *func_hidden_index_chs = PyUnicode_AsUTF8(uni);

			if (func_hidden_index_chs != NULL) {
				const auto addedIndex = (std::string)func_hidden_index_chs;
				Py_DECREF(uni); // gc after internal char* used 
				Py_DECREF(att);

				const auto func = closureMap->Get(addedIndex);
				if (func->IsFunction()) {
					return func.As<v8::Function>();
				}
			}
			else {
				Py_DECREF(uni);
				Py_DECREF(att);
			}
		}

		if (PyErr_Occurred()) {
			PyErr_Clear();
		}
	}


	Counter::_current_func_index++;

	const std::string std_str_key = std::to_string(Counter::_current_func_index);
	const char *str_key = std_str_key.c_str();

	if (auto *py_key = PyUnicode_FromString(str_key)) {
		int result = PyObject_SetAttrString(target_pyfunction, FUNC_KEY_INDEX_NAME, py_key);
		if (result < 0) {

		}
		Py_DECREF(py_key);
	}


	auto prop_name = v8::String::NewFromUtf8(_isolate, std_str_key.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
	const auto context = _isolate->GetCurrentContext();
	const auto js_func = v8::Function::New(context, _call_pyfunc_from_jsfunc_info, prop_name).ToLocalChecked();

	const auto set_result = PyDict_SetItemString(PyVars::pyfunction_dict, str_key, py_func);
	if (set_result > 0) {

	}

	closureMap->Set(std_str_key, js_func);

	// type object is callable but cannot set attr, so catch here
	if (PyErr_Occurred()) {
		PyErr_Clear();
	}

	return js_func;
}




PyObject *resolve_pyholder(v8::Local<v8::Value> jsobj)
{
	if (!jsobj.IsEmpty() && jsobj->IsObject()) {
		auto jsob = jsobj.As<v8::Object>();

		auto prop_name = v8::String::NewFromUtf8(_isolate, OBJ_BIND_KEY_INDEX_NAME, v8::NewStringType::kNormal).ToLocalChecked();
		const auto context = _isolate->GetCurrentContext();

		auto key = jsob->Get(context, prop_name);
		if (key.IsEmpty() == false) {
			v8::String::Utf8Value utf_str(_isolate, key.ToLocalChecked().As<v8::String>());

			auto *set_result = PyDict_GetItemString(PyVars::pyobj_dict, *utf_str);//PyDict_GetItemString: Return value : Borrowed reference.
			if (set_result) {
				Py_INCREF(set_result);
				return set_result;
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;

}

v8::Local<v8::Value> PyObject_to_JsValue_with_bind(PyObject *handle)
{
	auto newobj = v8::Object::New(_isolate);
	const auto context = _isolate->GetCurrentContext();

	Counter::_obj_index++;
	const std::string std_str_key = std::to_string(Counter::_obj_index);

	auto prop_name = v8::String::NewFromUtf8(_isolate, OBJ_BIND_KEY_INDEX_NAME, v8::NewStringType::kNormal).ToLocalChecked();
	auto propvalue = v8::String::NewFromUtf8(_isolate, std_str_key.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

	auto result = newobj->Set(context, prop_name, propvalue);
	if (result.IsNothing() == false) {

	}

	const char *str_key = std_str_key.c_str();

	/*
	if (auto *py_key = PyUnicode_FromString(str_key)) {
		int result = PyObject_SetAttrString(handle, OBJ_BIND_KEY_INDEX_NAME, py_key);
		if (result < 0) {

		}
		Py_DECREF(py_key);
	}
	*/

	const auto set_result = PyDict_SetItemString(PyVars::pyobj_dict, str_key, handle);
	if (set_result > 0) {

	}

	jsobjMap->Set(std_str_key, newobj);

	return newobj;
}

v8::Local<v8::Value> PyObject_to_JsValue(PyObject *pyobj)
{

	// this should be before PyFunction_Check because __call__ makes it a function (not sure though)
	if (Py_TYPE(pyobj) == _jsObjectType) {
		return ((JsObject*)pyobj)->_object.Get(_isolate);
	}

	if (Py_TYPE(pyobj) == _jsCallObjectType) {
		return ((JsCallObject*)pyobj)->_function.Get(_isolate);
	}

	if (PyBool_Check(pyobj)) {
		if (pyobj == Py_True) {
			return v8::Boolean::New(_isolate, true);
		}
		else {
			return v8::Boolean::New(_isolate, false);
		}
	}

	if (Py_None == pyobj) {
		return v8::Null(_isolate);
	}

	if (PyUnicode_CheckExact(pyobj)) {
		auto *chs = PyUnicode_AsUTF8(pyobj);
		//v8::String::NewExternalTwoByte

		//auto context = _isolate->GetCurrentContext();
		return String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();

		//return v8::String::NewFromUtf8(_isolate, chs);
	}


	if (PyType_CheckExact(pyobj)) {
		// type object is actually a function which has __call__
		if (auto *py_str = PyObject_Str(pyobj)) {
			if (auto *chs = PyUnicode_AsUTF8(py_str)) {

				const auto v8_str = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
				Py_DECREF(py_str);
				return v8_str;
			}
		}

		return v8::Undefined(_isolate);
	}

	if (PyInstanceMethod_Check(pyobj)) {
		auto *func = PyInstanceMethod_GET_FUNCTION(pyobj);
		return pyfunction_to_Jsfunction2(func);
	}

	if (PyMethod_Check(pyobj)) {
		return pyfunction_to_Jsfunction2(pyobj);
	}

	if (PyFunction_Check(pyobj)) {
		return pyfunction_to_Jsfunction2(pyobj);
	}

	if (pyobj == PyVars::undefined) {
		return v8::Undefined(_isolate);
	}

	if (PyFloat_Check(pyobj)) {
		auto double_value = PyFloat_AsDouble(pyobj);
		if (double_value == -1.0 && PyErr_Occurred()) {
			PyErr_Clear();

			if (auto *py_str = PyObject_Str(pyobj)) {
				auto *chs = PyUnicode_AsUTF8(py_str);
				Py_DECREF(py_str);
				if (chs != NULL) {
					auto context = _isolate->GetCurrentContext();
					auto maybe_num = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked()->ToNumber(context);
					if (!maybe_num.IsEmpty()) {
						return maybe_num.ToLocalChecked();
					}
				}
			}

			return v8::Number::New(_isolate, 0.0f);
		}
		else {
			return v8::Number::New(_isolate, double_value);
		}
	}

	if (PyByteArray_CheckExact(pyobj)) {
		//const auto len = PyByteArray_Size(pyobj);
		//const auto array_buffer = v8::ArrayBuffer::New(_isolate, len);
		//const auto array_buffer = v8::ArrayBuffer::New()

		//const auto u8 = v8::Uint8Array::New(array_buffer, 0, len);

		// static Local<Uint8Array> New(Local<ArrayBuffer> array_buffer,
		// size_t byte_offset, size_t length);

	}


	if (PyLong_Check(pyobj)) {
		auto int_val = PyLong_AsLongLong(pyobj);
		if (PyErr_Occurred()) {
			PyErr_Clear();
		}
		else {
			if (0 <= int_val) {
				if (int_val <= 2147483647) {
					return v8::Int32::New(_isolate, static_cast<int32_t>(int_val));
				}
				else if (int_val <= 4294967295/*ui32*/) {
					return v8::Uint32::NewFromUnsigned(_isolate, static_cast<uint32_t>(int_val));
				}
			}
			else {
				if (INT32_MIN <= int_val) {
					return v8::Int32::New(_isolate, static_cast<int32_t>(int_val));
				}
			}

			return v8::Number::New(_isolate, static_cast<double>(int_val));
		}

		// use js's string converter
		if (auto *py_str = PyObject_Str(pyobj)) {
			auto *chs = PyUnicode_AsUTF8(py_str);
			if (chs != NULL) {
				auto str = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
				auto context = _isolate->GetCurrentContext();
				auto numval = str->ToNumber(context);
				if (numval.IsEmpty() == false) {
					return numval.ToLocalChecked();
				}
			}
			Py_DECREF(py_str);

		}

		return v8::Number::New(_isolate, 0);
	}


	if (PyDict_CheckExact(pyobj)) {
		const auto jsObj = v8::Object::New(_isolate);
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(pyobj, &pos, &key, &value)) {
			// key and value are borrowed
			if (PyUnicode_Check(key)) {
				auto *chs = PyUnicode_AsUTF8(key);

				auto str = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();

				auto context = _isolate->GetCurrentContext();
				const auto result = jsObj->Set(context, str, PyObject_to_JsValue(value));
				if (result.IsNothing()) {

				}
			}
			else {
				const auto result = jsObj->Set(_isolate->GetCurrentContext(), PyObject_to_JsValue(key), PyObject_to_JsValue(value));
				if (result.IsNothing()) {

				}
			}
		}

		return jsObj;
	}

	if (PyTuple_CheckExact(pyobj)) {
		const auto len = PyTuple_Size(pyobj);
		const auto jsObj = v8::Array::New(_isolate, static_cast<int>(len));

		const auto context = _isolate->GetCurrentContext();

		for (int i = 0; i < len; i++) {
			if (auto *item = PyTuple_GetItem(pyobj, i)) { // Return value: Borrowed reference
				const auto hdn = PyObject_to_JsValue(item);
				if (!hdn.IsEmpty()) {
					auto b = jsObj->Set(context, i, hdn);
					if (b.IsNothing()) {

					}
					continue;
				}
			}

			auto res = jsObj->Set(context, i, v8::Undefined(_isolate));
			if (res.IsNothing()) {

			}
		}

		return jsObj;
	}

	if (PyList_CheckExact(pyobj)) {
		const auto len = PyList_Size(pyobj);
		const auto jsObj = v8::Array::New(_isolate, static_cast<int>(len));
		const auto context = _isolate->GetCurrentContext();

		//auto has_error = false;
		for (int i = 0; i < len; i++) {
			auto *item = PyList_GetItem(pyobj, i); // Return value: Borrowed reference.
			const auto hdn = PyObject_to_JsValue(item);
			if (hdn.IsEmpty()) {
				auto res = jsObj->Set(context, i, v8::Undefined(_isolate));
				if (res.IsNothing()) {

				}
			}
			else {
				auto res = jsObj->Set(context, i, hdn);
				if (res.IsNothing()) {

				}
			}
		}
		return jsObj;
	}


	// use str() if unknown python object is coming
	if (auto *py_str = PyObject_Str(pyobj)) {
		if (auto *chs = PyUnicode_AsUTF8(py_str)) {
			auto v8_str = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
			return v8_str;
		}
		Py_DECREF(py_str);

	}

	return v8::Undefined(_isolate);
}


static int clean(const std::string &key)
{
	PyDict_DelItemString(PyVars::pyfunction_dict, key.c_str());
	return 32;
}


static int clean_jsobj(const std::string &key)
{
	PyDict_DelItemString(PyVars::pyobj_dict, key.c_str());
	//throw new Exception();
	return 99;
}

void PyConverters::pre_register(v8::Local<v8::Object> rootJs)
{

}


void PyConverters::Clean()
{
	global_then_holder.Reset();
}

void PyConverters::Init(v8::Local<v8::Object> rootJs)
{

	closureMap = new WeakValueMap(&(clean));
	jsobjMap = new WeakValueMap(&(clean_jsobj));

	_jsObjectType = JsObjectReg::getInstance()->getJsObjectType();
	_jsCallObjectType = JsCallObjectReg::getInstance()->getJsCallObjectType();

	_isolate = v8::Isolate::GetCurrent();


	auto str = String::NewFromUtf8(_isolate, "then", v8::NewStringType::kNormal).ToLocalChecked();
	global_then_holder.Reset(_isolate, str);
}


