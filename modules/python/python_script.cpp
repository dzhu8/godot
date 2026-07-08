/**************************************************************************/
/*  python_script.cpp                                                     */
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

#include "python_script.h"

#include "python_instance.h"
#include "python_language.h"

#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"

void PythonScript::_bind_methods() {
	// No special bindings needed for now
}

PythonScript::PythonScript() :
		script_list(this) {
	{
		MutexLock lock(PythonLanguage::get_singleton()->mutex);
		PythonLanguage::get_singleton()->script_list.add(&script_list);
	}
}

PythonScript::~PythonScript() {
	{
		MutexLock lock(PythonLanguage::get_singleton()->mutex);
		PythonLanguage::get_singleton()->script_list.remove(&script_list);
	}

	_clear();
}

void PythonScript::_clear() {
	if (py_module || py_class) {
		PyGILState_STATE gstate = PyGILState_Ensure();
		Py_XDECREF(py_class);
		Py_XDECREF(py_module);
		py_class = nullptr;
		py_module = nullptr;
		PyGILState_Release(gstate);
	}

	methods.clear();
	properties.clear();
	property_defaults.clear();
	signals.clear();
	valid = false;
}

bool PythonScript::_parse_source() {
	if (!PythonLanguage::get_singleton()->is_python_initialized()) {
		return false;
	}

	_clear();

	if (source.is_empty()) {
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	// Create a unique module name based on the script path
	String module_name = path.get_file().get_basename();
	if (module_name.is_empty()) {
		module_name = "python_script_" + String::num_int64((int64_t)this);
	}
	module_name = module_name.replace(" ", "_").replace("-", "_").replace(".", "_");

	CharString module_name_utf8 = module_name.utf8();
	CharString source_utf8 = source.utf8();
	CharString path_utf8 = path.utf8();

	// Compile the source code
	PyObject *code = Py_CompileString(source_utf8.get_data(), path_utf8.get_data(), Py_file_input);
	if (code == nullptr) {
		String error = PythonTypeConversion::get_python_error();
		ERR_PRINT("Python compilation error in " + path + ": " + error);
		PyGILState_Release(gstate);
		return false;
	}

	// Create a new module
	py_module = PyImport_ExecCodeModule(module_name_utf8.get_data(), code);
	Py_DECREF(code);

	if (py_module == nullptr) {
		String error = PythonTypeConversion::get_python_error();
		ERR_PRINT("Python import error in " + path + ": " + error);
		PyGILState_Release(gstate);
		return false;
	}

	// Find the main class in the module
	// Look for a class that inherits from a Godot base class or is decorated
	PyObject *module_dict = PyModule_GetDict(py_module);
	PyObject *key, *value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(module_dict, &pos, &key, &value)) {
		if (PyType_Check(value)) {
			// Check if this is a class
			PyObject *class_name = PyObject_GetAttrString(value, "__name__");
			if (class_name) {
				String cls_name = PythonTypeConversion::pystring_to_godot_string(class_name);
				Py_DECREF(class_name);

				// Skip private classes
				if (!cls_name.begins_with("_")) {
					py_class = value;
					Py_INCREF(py_class);
					global_name = StringName(cls_name);
					break;
				}
			}
		}
	}

	if (py_class == nullptr) {
		// No suitable class found, but module is still valid
		// This might be a utility script
		valid = true;
		PyGILState_Release(gstate);
		return true;
	}

	_extract_class_info();

	valid = true;
	PyGILState_Release(gstate);
	return true;
}

void PythonScript::_extract_class_info() {
	if (!py_class) {
		return;
	}

	// Extract methods
	PyObject *class_dict = PyObject_GetAttrString(py_class, "__dict__");
	if (class_dict && PyDict_Check(class_dict)) {
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(class_dict, &pos, &key, &value)) {
			String method_name = PythonTypeConversion::pystring_to_godot_string(key);

			if (PyFunction_Check(value) || PyMethod_Check(value)) {
				MethodInfo mi;
				mi.name = StringName(method_name);

				// Try to get argument count from function's code object
				PyObject *func_code = PyObject_GetAttrString(value, "__code__");
				if (func_code) {
					PyObject *argcount = PyObject_GetAttrString(func_code, "co_argcount");
					if (argcount) {
						int argc = PyLong_AsLong(argcount);
						// Subtract 1 for 'self'
						for (int i = 1; i < argc; i++) {
							mi.arguments.push_back(PropertyInfo(Variant::NIL, "arg" + itos(i)));
						}
						Py_DECREF(argcount);
					}
					Py_DECREF(func_code);
				}

				methods[StringName(method_name)] = mi;

				// Check for tool script
				if (method_name == "_init") {
					// Check if class has @tool decorator or tool = True
				}
			}
		}
		Py_DECREF(class_dict);
	}

	// Extract exported properties from class annotations
	PyObject *annotations = PyObject_GetAttrString(py_class, "__annotations__");
	if (annotations && PyDict_Check(annotations)) {
		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(annotations, &pos, &key, &value)) {
			String prop_name = PythonTypeConversion::pystring_to_godot_string(key);
			PropertyInfo pi;
			pi.name = StringName(prop_name);

			// Try to determine type from annotation
			if (value == (PyObject *)&PyFloat_Type) {
				pi.type = Variant::FLOAT;
			} else if (value == (PyObject *)&PyLong_Type) {
				pi.type = Variant::INT;
			} else if (value == (PyObject *)&PyUnicode_Type) {
				pi.type = Variant::STRING;
			} else if (value == (PyObject *)&PyBool_Type) {
				pi.type = Variant::BOOL;
			} else {
				pi.type = Variant::NIL;
			}

			properties[StringName(prop_name)] = pi;

			// Get default value if it exists as a class attribute
			PyObject *default_val = PyObject_GetAttr(py_class, key);
			if (default_val && default_val != Py_None) {
				property_defaults[StringName(prop_name)] = PythonTypeConversion::pyobject_to_variant(default_val);
				Py_DECREF(default_val);
			} else {
				PythonTypeConversion::clear_python_error();
			}
		}
		Py_DECREF(annotations);
	} else {
		PythonTypeConversion::clear_python_error();
	}

	// Check for base class
	PyObject *bases = PyObject_GetAttrString(py_class, "__bases__");
	if (bases && PyTuple_Check(bases) && PyTuple_Size(bases) > 0) {
		PyObject *base = PyTuple_GetItem(bases, 0);
		PyObject *base_name = PyObject_GetAttrString(base, "__name__");
		if (base_name) {
			base_class_name = StringName(PythonTypeConversion::pystring_to_godot_string(base_name));
			Py_DECREF(base_name);
		}
		Py_DECREF(bases);
	} else {
		PythonTypeConversion::clear_python_error();
	}
}

bool PythonScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
	return valid && (tool || ScriptServer::is_scripting_enabled());
#else
	return valid;
#endif
}

Ref<Script> PythonScript::get_base_script() const {
	return Ref<Script>();
}

StringName PythonScript::get_global_name() const {
	return global_name;
}

bool PythonScript::inherits_script(const Ref<Script> &p_script) const {
	// Simple check - we don't support Python script inheritance chains yet
	return false;
}

StringName PythonScript::get_instance_base_type() const {
	if (!base_class_name.is_empty() && ClassDB::class_exists(base_class_name)) {
		return base_class_name;
	}
	return StringName("Object");
}

ScriptInstance *PythonScript::instance_create(Object *p_this) {
	if (!valid) {
		return nullptr;
	}

	PythonInstance *instance = memnew(PythonInstance(p_this, Ref<PythonScript>(this)));

	{
		MutexLock lock(PythonLanguage::get_singleton()->mutex);
		instances.insert(p_this);
	}

	return instance;
}

PlaceHolderScriptInstance *PythonScript::placeholder_instance_create(Object *p_this) {
#ifdef TOOLS_ENABLED
	PlaceHolderScriptInstance *si = memnew(PlaceHolderScriptInstance(PythonLanguage::get_singleton(), Ref<Script>(this), p_this));
	List<PropertyInfo> props;
	get_script_property_list(&props);
	si->update(props, property_defaults);
	return si;
#else
	return nullptr;
#endif
}

bool PythonScript::has_source_code() const {
	return !source.is_empty();
}

String PythonScript::get_source_code() const {
	return source;
}

void PythonScript::set_source_code(const String &p_code) {
	if (source == p_code) {
		return;
	}
	source = p_code;
}

Error PythonScript::reload(bool p_keep_state) {
	if (_parse_source()) {
		return OK;
	}
	return ERR_PARSE_ERROR;
}

#ifdef TOOLS_ENABLED
StringName PythonScript::get_doc_class_name() const {
	return global_name;
}

Vector<DocData::ClassDoc> PythonScript::get_documentation() const {
	return Vector<DocData::ClassDoc>();
}

String PythonScript::get_class_icon_path() const {
	return String();
}
#endif

bool PythonScript::has_method(const StringName &p_method) const {
	return methods.has(p_method);
}

MethodInfo PythonScript::get_method_info(const StringName &p_method) const {
	if (methods.has(p_method)) {
		return methods[p_method];
	}
	return MethodInfo();
}

bool PythonScript::is_tool() const {
	return tool;
}

bool PythonScript::is_valid() const {
	return valid;
}

bool PythonScript::is_abstract() const {
	return is_abstract_class;
}

ScriptLanguage *PythonScript::get_language() const {
	return PythonLanguage::get_singleton();
}

bool PythonScript::has_script_signal(const StringName &p_signal) const {
	return signals.has(p_signal);
}

void PythonScript::get_script_signal_list(List<MethodInfo> *r_signals) const {
	for (const KeyValue<StringName, MethodInfo> &E : signals) {
		r_signals->push_back(E.value);
	}
}

bool PythonScript::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	if (property_defaults.has(p_property)) {
		r_value = property_defaults[p_property];
		return true;
	}
	return false;
}

void PythonScript::get_script_method_list(List<MethodInfo> *p_list) const {
	for (const KeyValue<StringName, MethodInfo> &E : methods) {
		p_list->push_back(E.value);
	}
}

void PythonScript::get_script_property_list(List<PropertyInfo> *p_list) const {
	for (const KeyValue<StringName, PropertyInfo> &E : properties) {
		p_list->push_back(E.value);
	}
}

const Variant PythonScript::get_rpc_config() const {
	return rpc_config;
}

// Resource loader implementation

Ref<Resource> ResourceFormatLoaderPythonScript::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<PythonScript> script;
	script.instantiate();

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}
		return Ref<Resource>();
	}

	String source = f->get_as_utf8_string();
	script->set_source_code(source);
	script->set_path(p_path);

	err = script->reload();
	if (r_error) {
		*r_error = err;
	}

	return script;
}

void ResourceFormatLoaderPythonScript::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("py");
}

bool ResourceFormatLoaderPythonScript::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "PythonScript";
}

String ResourceFormatLoaderPythonScript::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "py") {
		return "PythonScript";
	}
	return "";
}

// Resource saver implementation

Error ResourceFormatSaverPythonScript::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<PythonScript> script = p_resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot save Python script to file: " + p_path);

	f->store_string(script->get_source_code());

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void ResourceFormatSaverPythonScript::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<PythonScript>(*p_resource)) {
		p_extensions->push_back("py");
	}
}

bool ResourceFormatSaverPythonScript::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<PythonScript>(*p_resource) != nullptr;
}
