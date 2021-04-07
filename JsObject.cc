#include <unordered_map>

#include <functional>

#include <vector>

#include <chrono>
#include <mutex>
#include <system_error>

#include <stdlib.h>
#include <assert.h>

#include <atomic>

#include <stdio.h>
#include <string>
#include <sstream>

#include <stdint.h>

#include "node_ref.h"
#include "JsObject.h"
#include "PyVars.h"
#include "JsVars.h"
#include "JsCallObject.h"
#include "pyconverters.h"
#include "CustomModules.h"
#include "call_later.h"
#include "uibox.h"



using namespace std;
using namespace v8;
using namespace plynth;

static v8::Isolate *_isolate = nullptr;


static PyTypeObject* _pytype_JsObject = nullptr;


static void JsObject_dealloc(JsObject *self)
{
	self->_object.Reset();
	
	//while (!_isolate->IdleNotificationDeadline(1)) {};
	//_isolate->IdleNotification(0.1);
	//while (!v8::kGCCallbackScheduleIdleGarbageCollection()) {};

	Py_TYPE(self)->tp_free((PyObject *)self);
}




static std::unique_ptr<UiBox> _JsObject_getattr(JsObject *obj, const char *name, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);


	const auto targetLocalObject = obj->_object.Get(_isolate);
	DCHECK(targetLocalObject.IsEmpty() == false);


	auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();
	auto context = _isolate->GetCurrentContext();

    printf("[0]");


	auto localValue = targetLocalObject->Get(context, v8name);

    printf("[1]");

    printf("1;");


	if (localValue.IsEmpty() || localValue.ToLocalChecked()->IsUndefined()) {
    printf("11;");

		if (targetLocalObject == JsVars::getInstance()->getJsGlobal()) {
    printf("44;");

			const auto top_level_var = JsVars::getInstance()->_js_toplevel_var_getter.Get(_isolate);
    printf("55;");

			if (top_level_var.IsEmpty() == false) {
				auto func = top_level_var.As<Function>();
				if (func.IsEmpty() == false) {
					v8::Local<v8::Value> stack_js_arg_array[1];

    printf("2;");

					stack_js_arg_array[0] = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();

					auto context = _isolate->GetCurrentContext();

					auto result = func->Call(context, v8::Undefined(_isolate), 1, stack_js_arg_array);
					if (result.IsEmpty() == false) {
						const auto jsvalue = result.ToLocalChecked().As<v8::Object>();

    printf("3;");

						/*
						if (trycatch.HasCaught()) {
							v8::Local<v8::Value> exception = trycatch.Exception();
							DCHECK(!exception.IsEmpty());
							auto ex_chs = *v8::String::Utf8Value(exception->ToString());
							return CustomModuleManager::treat_exception(ex_chs, is_main_thread);
						}
						*/

						//const auto jsvalue = func->Get(String::NewFromUtf8(_isolate, name));
						if (jsvalue.IsEmpty() == false) {
							const auto targetLocalObject2 = obj->_object.Get(_isolate);

							return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(targetLocalObject2, name, jsvalue), false));
						}
    printf("4;");

					}

				}
			}
		}
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	if (!localValue.IsEmpty()) {
		return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(targetLocalObject, name, localValue.ToLocalChecked()), false));
	}

	return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, false));
}



class JsObject_getattr_Item : public JsCallFromBackground {
public:
	JsObject_getattr_Item(char *chars) : name{chars} {


	}
	~JsObject_getattr_Item() override = default;

	JsObject *jsobj;

	void ui_getjs() override {
		const char *name_char = this->name;//.c_str();
		this->set_box(_JsObject_getattr(this->jsobj, name_char, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}

private:
	//std::string name;
	char *name;
};




static PyObject *JsObject_getattr(JsObject *obj, char *name)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _JsObject_getattr(obj, reinterpret_cast<const char*>(name), true);
		return box->get_pyobject();
	}

	auto *item = new JsObject_getattr_Item(name);
	item->jsobj = obj;

	return JsCallFromBackground::call_js_from_background(item);
}




static std::unique_ptr<UiBox> _JsObject_setattr(JsObject *obj, char *name, PyObject *v, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);

	if (v == NULL) {
		auto context = _isolate->GetCurrentContext();
		auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();

		// delete property
		auto result = obj->_object.Get(_isolate)->Delete(context, v8name);
		if (result.IsNothing()) {
		}
	}
	else
	{
		// set property
		auto jsValue = PyObject_to_JsValue(v);

		auto v8name = String::NewFromUtf8(_isolate, name, v8::NewStringType::kNormal).ToLocalChecked();
		auto context = _isolate->GetCurrentContext();



		auto result = obj->_object.Get(_isolate)->Set(context, v8name, jsValue);
		if (result.IsNothing()) {
		}
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, false));

	//	PyErr_Format(PyExc_RuntimeError, "Read-only attribute: %s", name);
	//return -1;
}


class JsObject_setattr_Item : public JsCallFromBackground {
public:
	JsObject_setattr_Item(char *chars) : name{chars} {


	}

	~JsObject_setattr_Item() override = default;

	JsObject *jsobj;
	std::string name;
	PyObject *py_setter_value;

	void ui_getjs() override {
		char *name_char = (char*)this->name.c_str();
		this->set_box(_JsObject_setattr(this->jsobj, name_char, this->py_setter_value, true));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};



static int JsObject_setattr(JsObject *obj, char *name, PyObject *v)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _JsObject_setattr(obj, name, v, true);
		if (box->is_exception()) {
			return -1;
		}
		return 0;
	}

	auto *item = new JsObject_setattr_Item(name);
	item->py_setter_value = v;
	item->jsobj = obj;

	if (auto *pyobj = JsCallFromBackground::call_js_from_background(item)) {
		Py_DECREF(pyobj);
	}
	if (PyErr_Occurred()) {
		return -1;
	}
	return 0;
}


static Py_ssize_t lenfunc2(PyObject *arg)
{
	auto jsobj = JsObjectReg::getTargetV8ObjectFromPyObject(arg);
	if (jsobj.IsEmpty()) {

	}

	return 1;
}


bool JsObjectReg::isJsObject(PyObject *pyobj)
{
	if (Py_TYPE(pyobj) == _pytype_JsObject) {
		return true;
	}
	else if (Py_TYPE(pyobj) == JsCallObjectReg::getInstance()->getJsCallObjectType()) {
		return true;
	}

	return false;
}

v8::Local<v8::Object> JsObjectReg::getTargetV8ObjectFromPyObject(PyObject *pyobj)
{
	if (Py_TYPE(pyobj) == _pytype_JsObject) {
		return ((JsObject*)pyobj)->_object.Get(_isolate);
	}
	else if (Py_TYPE(pyobj) == JsCallObjectReg::getInstance()->getJsCallObjectType()) {
		return JsCallObjectReg::getTargetV8Object((JsCallObject*)pyobj);
	}

	//DCHECK(false);
	return v8::Undefined(_isolate).As<Object>();
}


// impl of __getitem__ for like `obj[2]`
static std::unique_ptr<UiBox> _JsObject_subscript(JsObject *self, PyObject *key, bool is_main_thread)
{
	// supports a key of integer/string only. slices in the future
	if (self->_object.IsEmpty()) {
		return NULL;
	}

	v8::TryCatch trycatch(_isolate);

	auto targetLocalObject = self->_object.Get(_isolate);
	auto context = _isolate->GetCurrentContext();


	//if (targetLocalObject->IsSymbol()) {
	//
	//}

	if (PyLong_Check(key)) {
		const auto long_key = PyLong_AsLong(key);
		auto localValue = targetLocalObject->Get(context, long_key);

		if (!localValue.IsEmpty()) {
			return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(localValue.ToLocalChecked()), false));
		}
	}

	if (PySlice_Check(key)) {

	}


	bool used_symbol = false;
	auto key_js_obj = JsObjectReg::getTargetV8ObjectFromPyObject(key);
	if (key_js_obj.IsEmpty() == false) {
		if (key_js_obj->IsSymbol()) {
			auto symbol_key = key_js_obj.As<v8::Symbol>();

			auto val =  targetLocalObject->Get(context, symbol_key);

			if (!val.IsEmpty()) {
				used_symbol = true;
				return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(val.ToLocalChecked()), false));
			}
		}
	}

	if (used_symbol == false) {
		if (auto *str_key = PyObject_Str(key)) {

			auto *chs = PyUnicode_AsUTF8(str_key);

			auto v8name = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
			auto jsvalue = targetLocalObject->Get(context, v8name);

			Py_DECREF(str_key);

			if (!jsvalue.IsEmpty()) {
				return std::unique_ptr<UiBox>(new UiBox(JsValue_to_PyObject(jsvalue.ToLocalChecked()), false));
			}
		}
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	Py_INCREF(Py_None);
	return std::unique_ptr<UiBox>(new UiBox(Py_None, false));

}


class JsObject_subscript_Item : public JsCallFromBackground {
public:
	~JsObject_subscript_Item() override = default;

	JsObject *self;
	PyObject *key;

	void ui_getjs() override {
		this->set_box(_JsObject_subscript(this->self, this->key, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};



static PyObject* JsObject_subscript(JsObject *self, PyObject *key)
{
	if (CustomModuleManager::inMainThread()) {
		auto val = _JsObject_subscript(self, key, true);
		return val->get_pyobject();
	}

	auto *item = new JsObject_subscript_Item();
	item->self = self;
	item->key = key;

	return JsCallFromBackground::call_js_from_background(item);
}

static std::unique_ptr<UiBox> _JsObject_subscript_set(JsObject *self, PyObject *key, PyObject *arg, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);


	if (arg == NULL) { // delete mode
		if (PyLong_Check(key)) {
			auto long_key = (PyLong_AsLong(key));

			auto runInContext = self->_object.Get(_isolate)->Delete(_isolate->GetCurrentContext(), (uint32_t)long_key);
			if (runInContext.IsNothing()) {

			}
		}
		else {
			auto *str_key = PyObject_Str(key);
			auto *chs = PyUnicode_AsUTF8(str_key);

			auto v8name = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
			auto context = _isolate->GetCurrentContext();

			auto runInContext = self->_object.Get(_isolate)->Delete(context, v8name);
			if (runInContext.IsNothing()) {

			}
			Py_DECREF(str_key);

		}
	}
	else {
		const auto jsValue = PyObject_to_JsValue(arg);

		if (PyLong_Check(key)) {
			const auto long_key = PyLong_AsLong(key);
			const auto runInContext = self->_object.Get(_isolate)->Set(_isolate->GetCurrentContext(), (uint32_t)long_key, jsValue);
			if (runInContext.IsNothing()) {

			}
		}
		else {

			bool used_symbol = false;
			auto key_js_obj = JsObjectReg::getTargetV8ObjectFromPyObject(key);
			if (key_js_obj.IsEmpty() == false) {
				if (key_js_obj->IsSymbol()) {
					auto symbol_key = key_js_obj.As<v8::Symbol>();

					used_symbol = true;
					auto runInContext = self->_object.Get(_isolate)->Set(_isolate->GetCurrentContext(), symbol_key, jsValue);
					if (runInContext.IsNothing()) {
					}
				}
			}

			if (used_symbol == false) {
				if (auto *str_key = PyObject_Str(key)) {
					auto *chs = PyUnicode_AsUTF8(str_key);

					auto v8name = String::NewFromUtf8(_isolate, chs, v8::NewStringType::kNormal).ToLocalChecked();
					auto context = _isolate->GetCurrentContext();

					auto runInContext = self->_object.Get(_isolate)->Set(context, v8name, jsValue);
					if (runInContext.IsNothing()) {

					}
					Py_DECREF(str_key);
				}
			}
		}
	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox((PyObject*)NULL, false));
}


class JsObject_subscript_set_Item : public JsCallFromBackground {
public:
	~JsObject_subscript_set_Item() override = default;

	JsObject *self;
	PyObject *key;
	PyObject *arg;

	void ui_getjs() override {
		this->set_box(_JsObject_subscript_set(this->self, this->key, this->arg, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};


static int JsObject_subscript_set(JsObject *self, PyObject *key, PyObject *arg)
{
	if (CustomModuleManager::inMainThread()) {
		_JsObject_subscript_set(self, key, arg, true);
		return 0;
	}

	auto *item = new JsObject_subscript_set_Item();
	item->self = self;
	item->key = key;
	item->arg = arg;

	if (auto *pyobj = JsCallFromBackground::call_js_from_background(item)) {
		Py_DECREF(pyobj);
		//DCHECK(pyobj == nullptr);
	}
	return 0;
}


// Py_to_Js_RETURN(return__dir__(self, args));
#define	Py_to_Js_RETURN(str) do { \
	return str; \
} while(0)


PyObject *static_get_dir(v8::Local<v8::Object> targetLocalObject)
{
	auto context = _isolate->GetCurrentContext();

	auto result = targetLocalObject->GetPropertyNames(context);
	if (result.IsEmpty() == false) {

		auto names = result.ToLocalChecked();
		auto len = names->Length();

		auto *py_list = PyList_New(len);
		for (unsigned int i = 0; i < len; i++) {
			auto name = names->Get(context, i);
			if (name.IsEmpty() == false) {
				auto nn = name.ToLocalChecked()->ToString(context);
				if (nn.IsEmpty() == false) {
					v8::String::Utf8Value utf_str(JsVars::getIsolate(), nn.ToLocalChecked());// jsobj->ToString());

					std::string st{ *utf_str };// v8::String::Utf8Value(name));
					PyList_SetItem(py_list, i, PyUnicode_FromString(st.c_str()));//PyList_SetItem: This function "steals" a reference to item
				}
			}
			else {
				PyList_SetItem(py_list, i, PyUnicode_FromString("null"));//PyList_SetItem: This function "steals" a reference to item
			}
		}
		/*
		auto *py_list = PyList_New(len + 3);
		for (unsigned int i = 0; i < len; i++) {
			auto name = names->Get(i)->ToString();
			std::string st = std::string(*v8::String::Utf8Value(name));
			PyList_SetItem(py_list, i, PyUnicode_FromString(st.c_str()));// *v8::String::Utf8Value(name)));
		}
		PyList_SetItem(py_list, len, PyUnicode_FromString("__iter__"));
		PyList_SetItem(py_list, len + 1, PyUnicode_FromString("__next__"));
		PyList_SetItem(py_list, len + 2, PyUnicode_FromString("__await__"));
		*/

		return py_list;
	}

	return PyList_New(0);

}


static std::unique_ptr<UiBox> _return__dir__(PyObject *self, PyObject *args, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);

	const auto v8val = JsObjectReg::getTargetV8ObjectFromPyObject(self);
	auto *v8list = static_get_dir(v8val);

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}

	return std::unique_ptr<UiBox>(new UiBox(v8list, false));
}


class JsObject_dir_Item : public JsCallFromBackground {
public:
	~JsObject_dir_Item() override = default;

	PyObject *self;
	PyObject *args;

	void ui_getjs() override {
		this->set_box(_return__dir__(this->self, this->args, false));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};


static PyObject *return__dir__(PyObject *self, PyObject *args)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _return__dir__(self, args, true);
		return box->get_pyobject();
	}

	auto *item = new JsObject_dir_Item();
	item->self = self;
	item->args = args;

	auto *pyobj = JsCallFromBackground::call_js_from_background(item);
	return pyobj;
}

static PyObject *__dir__(PyObject *self, PyObject *args)
{
	return return__dir__(self, args);
}


/*
static PyObject *__getstate__(PyObject *self, PyObject *args)
{
	return PyDict_New();// Long_FromLong(222);//
}

static PyObject *__setstate__(PyObject *self, PyObject *args)
{
	return PyLong_FromLong(222);//

//	return return__dir__(self, args);
}
*/


PyMethodDef JsObjectMethods[] = {
	{ "__dir__", reinterpret_cast<PyCFunction>(__dir__), METH_VARARGS, "Return the member list." },
	//	{ "__getstate__", reinterpret_cast<PyCFunction>(__getstate__), METH_VARARGS, "Return the member list." },
	//	{ "__setstate__", reinterpret_cast<PyCFunction>(__setstate__), METH_VARARGS, "Return the member list." },


	{ NULL, NULL, 0, NULL }
};


v8::Local<v8::Object> JsObjectReg::getTargetV8Object(JsObject *jsObject)
{
	return jsObject->_object.Get(_isolate);
}







static std::unique_ptr<UiBox> _str_func(PyObject *arg, bool is_main_thread)
{
	v8::TryCatch trycatch(_isolate);

	const auto jsobj = JsObjectReg::getTargetV8ObjectFromPyObject(arg);

	if (jsobj->IsSymbol()) {
		return std::unique_ptr<UiBox>(new UiBox(PyUnicode_FromString("null"), false));
	}

	auto context = _isolate->GetCurrentContext();
	auto str = jsobj->ToString(context);// .ToLocalChecked();
	if (!str.IsEmpty()) {

		v8::String::Utf8Value utf_str(JsVars::getIsolate(), str.ToLocalChecked());

		PyObject *ex_str = PyUnicode_FromString(*utf_str);

		if (ex_str == NULL) {
			ex_str = PyUnicode_FromString("null");
		}
		return std::unique_ptr<UiBox>(new UiBox(ex_str, false));

	}

	if (trycatch.HasCaught()) {
		v8::Local<v8::Value> exception = trycatch.Exception();
		DCHECK(!exception.IsEmpty());
		return JsCallFromBackground::treat_exception(exception, is_main_thread);
	}


	auto *ex_str = PyUnicode_FromString("null");
	return std::unique_ptr<UiBox>(new UiBox(ex_str, false));

}


class JsObject_str_Item : public JsCallFromBackground {
public:

	JsObject_str_Item(PyObject *arg) : arg{ arg } { }

	~JsObject_str_Item() override = default;
	
	PyObject *arg;

	void ui_getjs() override {
		this->set_box(_str_func(this->arg, false));
	}

	PyObject* getPyObject() override {
		auto *str = this->treat_exception_holder(this->release_box());
		if (str == NULL) {

		}
		return str;
	}
};



PyObject *JsObjectReg::JsObject_strfunc(PyObject *arg)
{
	if (CustomModuleManager::inMainThread()) {
		auto box = _str_func(arg, true);
		return box->get_pyobject();
	}

	auto *item = new JsObject_str_Item(arg);

	return JsCallFromBackground::call_js_from_background(item);
}












static PyObject *_richcmpfunc2(PyObject *a, PyObject *b, int op)
{
	auto jsobj_a = JsObjectReg::getTargetV8ObjectFromPyObject(a);
	auto jsobj_b = JsObjectReg::getTargetV8ObjectFromPyObject(b);

	return richcmpfunc3(jsobj_a, jsobj_b, op);
}

class JsObject_richcmp_Item : public JsCallFromBackground {
public:
	~JsObject_richcmp_Item() override = default;

	PyObject *a;
	PyObject *b;
	int op;

	void ui_getjs() override {
		this->set_box(std::unique_ptr<UiBox>(new UiBox(_richcmpfunc2(this->a, this->b, this->op), false)));
	}

	PyObject* getPyObject() override {
		return this->treat_exception_holder(this->release_box());
	}
};



static Py_hash_t hash_func(PyObject *obj)
{
	auto jsobj_a = JsObjectReg::getTargetV8ObjectFromPyObject(obj);
	if (jsobj_a.IsEmpty() == false) {
		if (jsobj_a->IsObject()) {
			return jsobj_a->GetIdentityHash();
		}

		if (jsobj_a->IsSymbol()) {
			auto gk = jsobj_a.As<v8::Symbol>();
			return gk->GetIdentityHash();
		}
	}

	return 5;
}

static PyObject *richcmpfunc2(PyObject *a, PyObject *b, int op)
{
	if (CustomModuleManager::inMainThread()) {
		return _richcmpfunc2(a, b, op);
	}

	auto *item = new JsObject_richcmp_Item();
	item->a = a;
	item->b = b;
	item->op = op;

	return JsCallFromBackground::call_js_from_background(item);
}









static PyObject *am_await(PyObject *self)
{
	PyObject* iter = nullptr;

	v8::Local<v8::Object> jsValue = JsObjectReg::getTargetV8ObjectFromPyObject(self);
	if (jsValue.IsEmpty() == false && jsValue->IsUndefined() == false) {

		auto *jsObject = (JsObject*)JsObjectReg::JsObject_new(NULL, NULL);
		jsObject->_object.Reset(_isolate, jsValue);

		auto *arg_tuple = PyTuple_New(1);
		PyTuple_SetItem(arg_tuple, 0, (PyObject*)jsObject); // This function "steals" a reference to o.

		auto *promiseWrapper = PyObject_CallObject(PyVars::JsPromiseWrapper, arg_tuple);

		/*
		if (PyErr_Occurred()) {
			PyErr_Clear();
		}
		*/

		Py_DECREF(arg_tuple);

		if (promiseWrapper) {
			if (auto *__await__ = PyObject_GetAttrString(promiseWrapper, "__await__")) {
				iter = PyObject_CallObject(__await__, NULL);
				Py_DECREF(__await__);
			}
			Py_DECREF(promiseWrapper);
		}
	}

	if (iter) {
		return iter;
	}
	else {
		return PyLong_FromLong(2299);
	}
}



void setSubscriptFuncs(PyMappingMethods * mappingMethods)
{
	mappingMethods->mp_length = lenfunc2;
	mappingMethods->mp_subscript = reinterpret_cast<binaryfunc>(JsObject_subscript);
	mappingMethods->mp_ass_subscript = reinterpret_cast<objobjargproc>(JsObject_subscript_set);
}



struct JsObjectReg::pimpl
{
public:
	pimpl() : JsObject_typeObject(new PyTypeObject()), mappingMethods(new PyMappingMethods())
	{
		_pytype_JsObject = JsObject_typeObject.get();

	}

	static PyObject * binaryfunc2(PyObject *arg1, PyObject *arg2)
	{
		auto *custom = JsObjectReg::JsObject_new(NULL, NULL);
		return custom;
	}


	std::unique_ptr<PyTypeObject> JsObject_typeObject;
	std::unique_ptr<PyMappingMethods> mappingMethods;


	void on_append_inittab()
	{
		auto *CustomType = this->JsObject_typeObject.get();
		CustomType->ob_base.ob_base.ob_type = NULL;
		CustomType->ob_base.ob_base.ob_refcnt = 1;
		CustomType->ob_base.ob_size = 0;

		CustomType->tp_name = "plynth.native";
		CustomType->tp_doc = "plynth.native";
		CustomType->tp_basicsize = sizeof(JsObject);
		CustomType->tp_itemsize = 0;
		CustomType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;// | Py_TPFLAGS_HEAPTYPE;

		// CustomType->tp_init = (initproc)Custom_init;
		CustomType->tp_dealloc = (destructor)JsObject_dealloc;

		CustomType->tp_dict = PyDict_New();
		CustomType->tp_str = JsObject_strfunc;
		/*
		[](PyObject * arg) -> PyObject* {
			auto jsobj = JsObjectReg::getTargetV8ObjectFromPyObject(arg);
			const auto ex_chs = *v8::String::Utf8Value(jsobj->ToString());
			PyObject *ex_str = PyUnicode_FromString(ex_chs);
			return ex_str;
			//return PyUnicode_FromString("[#JsCallObject]");
		};
		*/
		CustomType->tp_repr = JsObject_strfunc;

		CustomType->tp_methods = JsObjectMethods;
		CustomType->tp_getattr = reinterpret_cast<getattrfunc>(JsObject_getattr);
		CustomType->tp_setattr = reinterpret_cast<setattrfunc>(JsObject_setattr);


//		typedef Py_hash_t(*hashfunc)(PyObject *);

		CustomType->tp_hash = hash_func;


		CustomType->tp_richcompare = reinterpret_cast<richcmpfunc>(richcmpfunc2);

		CustomType->tp_as_async = new PyAsyncMethods();
		CustomType->tp_as_async->am_await = am_await;

		CustomType->tp_as_mapping = this->mappingMethods.get();
		setSubscriptFuncs(CustomType->tp_as_mapping);
	}
};


JsObjectReg::JsObjectReg()
	: _pimpl(new pimpl())
{
}


PyTypeObject *JsObjectReg::getJsObjectType()
{
	return this->_pimpl->JsObject_typeObject.get();
}

void JsObjectReg::on_AppendInittab()
{
	this->_pimpl->on_append_inittab();
}

void JsObjectReg::Init()
{
	_isolate = v8::Isolate::GetCurrent();
}



PyObject *JsObjectReg::JsObject_new(PyObject *args, PyObject *kwds)
{
	JsObject *self;

	auto *type = JsObjectReg::instance->getJsObjectType();

	self = (JsObject*)type->tp_alloc(type, 0);
	if (self != NULL) {
	}

	return (PyObject *)self;
}




JsObject::JsObject() {

}
JsObject::~JsObject() = default;



JsObjectReg::~JsObjectReg() = default;

JsObjectReg* JsObjectReg::instance = 0;

JsObjectReg* JsObjectReg::getInstance()
{
	if (instance == 0) {

		instance = new JsObjectReg();
	}

	return instance;
}

