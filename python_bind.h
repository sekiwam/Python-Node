#pragma once

#include <string>
#include "node_ref.h"
 

void start();
std::string startPythonBinding(v8::Local<v8::Object> global_obj, const char* pathon_module, const char* func_name);
void set_execPath(const char* exec_path, const char* dirname);
void python_node_register();

std::string set_var_resolver(v8::Local<v8::Object> func);

std::string get_plynth_version();

