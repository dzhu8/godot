/**************************************************************************/
/*  python_instance.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "python_instance.h"
#include "python_language.h"
#include "scene/main/node.h"

PythonInstance::PythonInstance(Object *p_owner, const Ref<PythonScript> &p_script) {
	owner = p_owner;
	script = p_script;

	if (!PythonLanguage::get_singleton()->is_python_initialized()) {
		return;
	}

	PyObject *py_class = script->get_py_class();
	if (py_class == nullptr) {
		return;
	}

	// Create an instance of the Python class
	PyGILState_STATE gstate = PyGILState_Ensure();

	py_instance = PyObject_CallObject(py_class, nullptr);
	if (py_instance == nullptr) {
		String error = PythonTypeConversion::get_python_error();
		ERR_PRINT("Failed to create Python instance: " + error);
	} else {
		// Store reference to the Godot owner object as a wrapper
		// This allows Python scripts to call methods like self.owner.get_tree()
		PyObject *owner_wrapper = PythonTypeConversion::wrap_godot_object(owner);
		if (owner_wrapper) {
			PyObject_SetAttrString(py_instance, "owner", owner_wrapper);
			Py_DECREF(owner_wrapper);
		}

		// Also keep the capsule for backwards compatibility
		PyObject *owner_capsule = PyCapsule_New(owner, "godot_owner", nullptr);
		if (owner_capsule) {
			PyObject_SetAttrString(py_instance, "_godot_owner", owner_capsule);
			Py_DECREF(owner_capsule);
		}
	}

	PyGILState_Release(gstate);
}

PythonInstance::~PythonInstance() {
	if (py_instance) {
		PyGILState_STATE gstate = PyGILState_Ensure();
		Py_DECREF(py_instance);
		py_instance = nullptr;
		PyGILState_Release(gstate);
	}

	if (script.is_valid()) {
		MutexLock lock(PythonLanguage::get_singleton()->mutex);
		script->instances.erase(owner);
	}
}

bool PythonInstance::set(const StringName &p_name, const Variant &p_value) {
	if (!py_instance) {
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	CharString name_utf8 = String(p_name).utf8();
	PyObject *py_value = PythonTypeConversion::variant_to_pyobject(p_value);

	int result = PyObject_SetAttrString(py_instance, name_utf8.get_data(), py_value);
	Py_DECREF(py_value);

	if (result == -1) {
		PythonTypeConversion::clear_python_error();
		PyGILState_Release(gstate);
		return false;
	}

	PyGILState_Release(gstate);
	return true;
}

bool PythonInstance::get(const StringName &p_name, Variant &r_ret) const {
	if (!py_instance) {
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	CharString name_utf8 = String(p_name).utf8();
	PyObject *py_value = PyObject_GetAttrString(py_instance, name_utf8.get_data());

	if (py_value == nullptr) {
		PythonTypeConversion::clear_python_error();
		PyGILState_Release(gstate);
		return false;
	}

	r_ret = PythonTypeConversion::pyobject_to_variant(py_value);
	Py_DECREF(py_value);

	PyGILState_Release(gstate);
	return true;
}

void PythonInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (script.is_valid()) {
		script->get_script_property_list(p_properties);
	}
}

Variant::Type PythonInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (script.is_valid() && script->properties.has(p_name)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return script->properties[p_name].type;
	}
	if (r_is_valid) {
		*r_is_valid = false;
	}
	return Variant::NIL;
}

void PythonInstance::validate_property(PropertyInfo &p_property) const {
	// No special validation needed
}

bool PythonInstance::property_can_revert(const StringName &p_name) const {
	return script.is_valid() && script->property_defaults.has(p_name);
}

bool PythonInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
	if (script.is_valid() && script->property_defaults.has(p_name)) {
		r_ret = script->property_defaults[p_name];
		return true;
	}
	return false;
}

void PythonInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool PythonInstance::has_method(const StringName &p_method) const {
	if (!py_instance) {
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	CharString method_utf8 = String(p_method).utf8();
	bool has = PyObject_HasAttrString(py_instance, method_utf8.get_data());

	if (has) {
		PyObject *attr = PyObject_GetAttrString(py_instance, method_utf8.get_data());
		has = attr && PyCallable_Check(attr);
		Py_XDECREF(attr);
	}

	PyGILState_Release(gstate);
	return has;
}

int PythonInstance::get_method_argument_count(const StringName &p_method, bool *r_is_valid) const {
	if (script.is_valid() && script->methods.has(p_method)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return script->methods[p_method].arguments.size();
	}
	if (r_is_valid) {
		*r_is_valid = false;
	}
	return 0;
}

bool PythonInstance::_call_method(const StringName &p_method, const Variant **p_args, int p_argcount, Variant &r_return, Callable::CallError &r_error) {
	if (!py_instance) {
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	CharString method_utf8 = String(p_method).utf8();
	PyObject *py_method = PyObject_GetAttrString(py_instance, method_utf8.get_data());

	if (!py_method || !PyCallable_Check(py_method)) {
		Py_XDECREF(py_method);
		PythonTypeConversion::clear_python_error();
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		PyGILState_Release(gstate);
		return false;
	}

	// Build arguments tuple
	PyObject *args_tuple = PyTuple_New(p_argcount);
	for (int i = 0; i < p_argcount; i++) {
		PyObject *py_arg = PythonTypeConversion::variant_to_pyobject(*p_args[i]);
		PyTuple_SET_ITEM(args_tuple, i, py_arg); // Steals reference
	}

	// Call the method
	PyObject *result = PyObject_CallObject(py_method, args_tuple);

	Py_DECREF(args_tuple);
	Py_DECREF(py_method);

	if (result == nullptr) {
		String error = PythonTypeConversion::get_python_error();
		ERR_PRINT("Python error in " + String(p_method) + ": " + error);
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		PyGILState_Release(gstate);
		return false;
	}

	r_return = PythonTypeConversion::pyobject_to_variant(result);
	Py_DECREF(result);

	r_error.error = Callable::CallError::CALL_OK;
	PyGILState_Release(gstate);
	return true;
}

Variant PythonInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	Variant ret;
	_call_method(p_method, p_args, p_argcount, ret, r_error);
	return ret;
}

void PythonInstance::notification(int p_notification, bool p_reversed) {
	if (!py_instance) {
		return;
	}

	// Handle special notifications that need to call lifecycle methods
	// These mirror how GDScript handles notifications in Node
	switch (p_notification) {
		case Node::NOTIFICATION_READY: {
			// Enable processing if the script has _process or _physics_process methods
			Node *node = Object::cast_to<Node>(owner);
			if (node) {
				if (has_method("_process")) {
					node->set_process(true);
				}
				if (has_method("_physics_process")) {
					node->set_physics_process(true);
				}
				if (has_method("_input")) {
					node->set_process_input(true);
				}
				if (has_method("_unhandled_input")) {
					node->set_process_unhandled_input(true);
				}
			}

			// Call _ready() method
			if (has_method("_ready")) {
				Callable::CallError err;
				Variant ret;
				_call_method("_ready", nullptr, 0, ret, err);
			}
		} break;

		case Node::NOTIFICATION_PROCESS: {
			// Call _process(delta) method
			if (has_method("_process")) {
				Node *node = Object::cast_to<Node>(owner);
				if (node) {
					double delta = node->get_process_delta_time();
					Variant delta_var = delta;
					const Variant *args[1] = { &delta_var };
					Callable::CallError err;
					Variant ret;
					_call_method("_process", args, 1, ret, err);
				}
			}
		} break;

		case Node::NOTIFICATION_PHYSICS_PROCESS: {
			// Call _physics_process(delta) method
			if (has_method("_physics_process")) {
				Node *node = Object::cast_to<Node>(owner);
				if (node) {
					double delta = node->get_physics_process_delta_time();
					Variant delta_var = delta;
					const Variant *args[1] = { &delta_var };
					Callable::CallError err;
					Variant ret;
					_call_method("_physics_process", args, 1, ret, err);
				}
			}
		} break;

		default:
			break;
	}

	// Always call _notification if it exists
	if (has_method("_notification")) {
		Variant notification_val = p_notification;
		const Variant *args[1] = { &notification_val };
		Callable::CallError err;
		Variant ret;
		_call_method("_notification", args, 1, ret, err);
	}
}

Ref<Script> PythonInstance::get_script() const {
	return script;
}

ScriptLanguage *PythonInstance::get_language() {
	return PythonLanguage::get_singleton();
}

const Variant PythonInstance::get_rpc_config() const {
	if (script.is_valid()) {
		return script->get_rpc_config();
	}
	return Variant();
}
