#pragma once

#include "node_ref.h"

#include <stdint.h>
#include <stdlib.h>

#include <Python.h>

#include "PyVars.h"


class WaitPromise {
public:
	static PyObject *await_promise_or_future(PyObject *self, PyObject *args);
 };
