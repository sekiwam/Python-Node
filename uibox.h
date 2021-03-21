#pragma once

#include <Python.h>
#include <mutex>
#include <functional>
#include <memory>
#include <atomic>
#include "PyVars.h"
#include "PlynthUtils.h"
#include <unordered_map>

namespace plynth
{
	/*
	 * UiBox keeps holding a PyObject* and a bool value indicating whether it's a exception or not
	 * during run code in UI thread
	 * the PyObject has a specific format when it's a exception
	 *       {obj: jsErrorObject}
	 */
	class UiBox
	{

	public:
		UiBox(PyObject *pyobj, bool is_exception);

		bool is_exception() {
			return this->_is_exception;
		}

		PyObject *get_pyobject() {
			return this->_pyobj;
		}

		~UiBox() = default;

	private:
		PyObject * _pyobj;
		bool _is_exception;

		//UiBox() = delete;

		UiBox(const UiBox&) = delete;
		UiBox(UiBox&&) = delete;
		UiBox& operator =(const UiBox&) = delete;
		UiBox& operator =(UiBox&&) = delete;
	};



	class JsCallFromBackground
	{
	public:
		JsCallFromBackground();
		virtual ~JsCallFromBackground();

		virtual PyObject* getPyObject() = 0;

		bool ui_get_done{ false };


		virtual void ui_getjs() = 0;

		void set_box(std::unique_ptr<UiBox> box)
		{
			this->return_pyobj = std::move(box);
		}

		// release box
		std::unique_ptr<UiBox> release_box()
		{
			return std::move(this->return_pyobj);
		}

		static PyObject *call_js_from_background(JsCallFromBackground *item);
		static PyObject *call_js_from_background(JsCallFromBackground *item, bool deleteItem);

		static PyObject *treat_exception_holder(std::unique_ptr<UiBox> box);
		static std::unique_ptr<UiBox> treat_exception(v8::Local<v8::Value> exception, bool is_main_thread);


	private:
		std::unique_ptr<UiBox> return_pyobj;

		JsCallFromBackground(const JsCallFromBackground&) = delete;
		JsCallFromBackground(JsCallFromBackground&&) = delete;
		JsCallFromBackground& operator =(const JsCallFromBackground&) = delete;
		JsCallFromBackground& operator =(JsCallFromBackground&&) = delete;
	};
}

