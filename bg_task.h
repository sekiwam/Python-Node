#pragma once

#include "node_ref.h"

#include <stdint.h>
#include <stdlib.h>

#include <Python.h>

#include "PyVars.h"
#include "uibox.h"

class BackgroundTask
{
public:

	static void Init(v8::Local<v8::Object> rootJs);

	static PyObject *canDoUiCode(plynth::JsCallFromBackground *item);

	static void request_next_skip();
	static void start_request_next_skip();


	static void requestSleepEnd();
	static void restoreSleeperReady();




	static PyObject *run_in_newthread(PyObject *self, PyObject *args, PyObject *keywds);



private:
	BackgroundTask();

	BackgroundTask(BackgroundTask const&) = delete;
	BackgroundTask(BackgroundTask&&) = delete;

	BackgroundTask& operator =(BackgroundTask const&) = delete;
	BackgroundTask& operator =(BackgroundTask&&) = delete;
};
