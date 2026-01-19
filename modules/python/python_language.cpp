/**************************************************************************/
/*  python_language.cpp                                                   */
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

#include "python_language.h"
#include "python_config.gen.h"

#include <cstdlib> // For getenv

PythonLanguage *PythonLanguage::singleton = nullptr;

// Python stdout/stderr redirector to Godot's print functions
static PyObject *godot_stdout_write(PyObject *self, PyObject *args) {
	const char *str;
	if (!PyArg_ParseTuple(args, "s", &str)) {
		return nullptr;
	}
	String text = String::utf8(str);
	// Remove trailing newline since print_line adds one
	if (text.ends_with("\n")) {
		text = text.substr(0, text.length() - 1);
	}
	if (!text.is_empty()) {
		print_line(text);
	}
	Py_RETURN_NONE;
}

static PyObject *godot_stdout_flush(PyObject *self, PyObject *args) {
	Py_RETURN_NONE;
}

static PyObject *godot_stderr_write(PyObject *self, PyObject *args) {
	const char *str;
	if (!PyArg_ParseTuple(args, "s", &str)) {
		return nullptr;
	}
	String text = String::utf8(str);
	if (text.ends_with("\n")) {
		text = text.substr(0, text.length() - 1);
	}
	if (!text.is_empty()) {
		ERR_PRINT(text);
	}
	Py_RETURN_NONE;
}

static PyMethodDef godot_stdout_methods[] = {
	{ "write", godot_stdout_write, METH_VARARGS, "Write to Godot output" },
	{ "flush", godot_stdout_flush, METH_VARARGS, "Flush output" },
	{ nullptr, nullptr, 0, nullptr }
};

static PyMethodDef godot_stderr_methods[] = {
	{ "write", godot_stderr_write, METH_VARARGS, "Write to Godot error output" },
	{ "flush", godot_stdout_flush, METH_VARARGS, "Flush output" },
	{ nullptr, nullptr, 0, nullptr }
};

static struct PyModuleDef godot_stdout_module = {
	PyModuleDef_HEAD_INIT,
	"godot_stdout",
	nullptr,
	-1,
	godot_stdout_methods
};

static struct PyModuleDef godot_stderr_module = {
	PyModuleDef_HEAD_INIT,
	"godot_stderr",
	nullptr,
	-1,
	godot_stderr_methods
};

void PythonLanguage::_setup_python_output_redirect() {
	// Create stdout redirector module
	PyObject *stdout_module = PyModule_Create(&godot_stdout_module);
	if (stdout_module) {
		PySys_SetObject("stdout", stdout_module);
		Py_DECREF(stdout_module);
	}

	// Create stderr redirector module
	PyObject *stderr_module = PyModule_Create(&godot_stderr_module);
	if (stderr_module) {
		PySys_SetObject("stderr", stderr_module);
		Py_DECREF(stderr_module);
	}
}

PythonLanguage::PythonLanguage() {
	ERR_FAIL_COND(singleton);
	singleton = this;
}

PythonLanguage::~PythonLanguage() {
	singleton = nullptr;
}

String PythonLanguage::get_name() const {
	return "Python";
}

void PythonLanguage::init() {
	if (python_initialized) {
		return;
	}

	// Initialize Python interpreter
	if (!Py_IsInitialized()) {
		// Configure Python for embedding
		PyConfig config;
		PyConfig_InitPythonConfig(&config);
		config.isolated = 0; // Allow Python to use environment variables
		config.site_import = 0; // Don't import site module initially

		// Try to set Python home if not already set via environment
		const char *python_home = getenv("PYTHONHOME");
		if (!python_home && strlen(PYTHON_HOME_PATH) > 0) {
			// Use the embedded path from build time
			PyStatus status = PyConfig_SetBytesString(&config, &config.home, PYTHON_HOME_PATH);
			if (PyStatus_Exception(status)) {
				WARN_PRINT("Failed to set embedded PYTHONHOME path");
				PyConfig_Clear(&config);
			} else {
				print_line("Using embedded Python home: " + String(PYTHON_HOME_PATH));
			}
		} else if (python_home) {
			print_line("Using PYTHONHOME from environment: " + String(python_home));
		}

		PyStatus status = Py_InitializeFromConfig(&config);
		PyConfig_Clear(&config);

		if (PyStatus_Exception(status)) {
			ERR_PRINT("Failed to initialize Python interpreter. "
					  "Ensure Python is properly installed or set PYTHONHOME environment variable.");
			return;
		}
	}

	// Redirect Python stdout/stderr to Godot's print functions
	_setup_python_output_redirect();

	// Initialize the Godot object wrapper type for Python
	PythonTypeConversion::initialize_godot_object_type();

	// Release the GIL so other threads can use Python
	// The main thread will acquire it when needed
	PyEval_SaveThread();

	python_initialized = true;
	print_line("Python " + String(Py_GetVersion()) + " initialized for Godot");
}

String PythonLanguage::get_type() const {
	return "PythonScript";
}

String PythonLanguage::get_extension() const {
	return "py";
}

void PythonLanguage::finish() {
	if (!python_initialized) {
		return;
	}

	// Acquire GIL before finalizing
	PyGILState_STATE gstate = PyGILState_Ensure();

	// Clean up all scripts
	{
		MutexLock lock(mutex);
		SelfList<PythonScript> *elem = script_list.first();
		while (elem) {
			elem->self()->_clear();
			elem = elem->next();
		}
	}

	// Note: We don't call Py_Finalize() here because it can cause issues
	// if Python is used after finalization in the same process
	// Py_Finalize();

	PyGILState_Release(gstate);
	python_initialized = false;
}

Vector<String> PythonLanguage::get_reserved_words() const {
	Vector<String> words;
	// Python keywords
	words.push_back("False");
	words.push_back("None");
	words.push_back("True");
	words.push_back("and");
	words.push_back("as");
	words.push_back("assert");
	words.push_back("async");
	words.push_back("await");
	words.push_back("break");
	words.push_back("class");
	words.push_back("continue");
	words.push_back("def");
	words.push_back("del");
	words.push_back("elif");
	words.push_back("else");
	words.push_back("except");
	words.push_back("finally");
	words.push_back("for");
	words.push_back("from");
	words.push_back("global");
	words.push_back("if");
	words.push_back("import");
	words.push_back("in");
	words.push_back("is");
	words.push_back("lambda");
	words.push_back("nonlocal");
	words.push_back("not");
	words.push_back("or");
	words.push_back("pass");
	words.push_back("raise");
	words.push_back("return");
	words.push_back("try");
	words.push_back("while");
	words.push_back("with");
	words.push_back("yield");
	return words;
}

bool PythonLanguage::is_control_flow_keyword(const String &p_keyword) const {
	return p_keyword == "if" ||
			p_keyword == "elif" ||
			p_keyword == "else" ||
			p_keyword == "for" ||
			p_keyword == "while" ||
			p_keyword == "break" ||
			p_keyword == "continue" ||
			p_keyword == "return" ||
			p_keyword == "pass" ||
			p_keyword == "try" ||
			p_keyword == "except" ||
			p_keyword == "finally" ||
			p_keyword == "raise" ||
			p_keyword == "with" ||
			p_keyword == "yield";
}

Vector<String> PythonLanguage::get_comment_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("#");
	return delimiters;
}

Vector<String> PythonLanguage::get_doc_comment_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("\"\"\" \"\"\"");
	delimiters.push_back("''' '''");
	return delimiters;
}

Vector<String> PythonLanguage::get_string_delimiters() const {
	Vector<String> delimiters;
	delimiters.push_back("\" \"");
	delimiters.push_back("' '");
	delimiters.push_back("\"\"\" \"\"\"");
	delimiters.push_back("''' '''");
	return delimiters;
}

Ref<Script> PythonLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<PythonScript> script;
	script.instantiate();

	String template_code = p_template;
	if (template_code.is_empty()) {
		template_code = R"(# Python script for Godot
# Attach this script to a node

class _CLASS_(object):
    """
    A Python script that extends _BASE_.
    """

    # Exported properties (visible in Inspector)
    speed: float = 1.0

    def _ready(self):
        """Called when the node enters the scene tree."""
        print("PYTHON: _ready() called")

    def _process(self, delta):
        """Called every frame."""
        pass
)";
	}

	template_code = template_code.replace("_CLASS_", p_class_name);
	template_code = template_code.replace("_BASE_", p_base_class_name);

	script->set_source_code(template_code);
	return script;
}

Vector<ScriptLanguage::ScriptTemplate> PythonLanguage::get_built_in_templates(const StringName &p_object) {
	Vector<ScriptTemplate> templates;

	ScriptTemplate default_template;
	default_template.inherit = String(p_object);
	default_template.name = "Default";
	default_template.description = "Basic Python script template";
	default_template.content = R"(# Python script for Godot

class _CLASS_(object):
    """A script that extends _BASE_."""

    def _ready(self):
        """Called when the node enters the scene tree."""
        print("PYTHON: Hello from _CLASS_!")

    def _process(self, delta):
        """Called every frame."""
        pass
)";
	default_template.origin = TEMPLATE_BUILT_IN;
	templates.push_back(default_template);

	return templates;
}

bool PythonLanguage::is_using_templates() {
	return true;
}

bool PythonLanguage::validate(const String &p_script, const String &p_path, List<String> *r_functions, List<ScriptLanguage::ScriptError> *r_errors, List<ScriptLanguage::Warning> *r_warnings, HashSet<int> *r_safe_lines) const {
	if (!python_initialized) {
		if (r_errors) {
			ScriptError err;
			err.line = 1;
			err.column = 1;
			err.message = "Python interpreter not initialized";
			r_errors->push_back(err);
		}
		return false;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	CharString source_utf8 = p_script.utf8();
	CharString path_utf8 = p_path.is_empty() ? CharString("<string>") : p_path.utf8();

	// Try to compile the source
	PyObject *code = Py_CompileString(source_utf8.get_data(), path_utf8.get_data(), Py_file_input);

	if (code == nullptr) {
		if (r_errors) {
			PyObject *type, *value, *traceback;
			PyErr_Fetch(&type, &value, &traceback);
			PyErr_NormalizeException(&type, &value, &traceback);

			ScriptError err;
			err.line = 1;
			err.column = 1;

			if (value) {
				// Try to get line number from syntax error
				PyObject *lineno = PyObject_GetAttrString(value, "lineno");
				if (lineno && PyLong_Check(lineno)) {
					err.line = PyLong_AsLong(lineno);
				}
				Py_XDECREF(lineno);

				PyObject *offset = PyObject_GetAttrString(value, "offset");
				if (offset && PyLong_Check(offset)) {
					err.column = PyLong_AsLong(offset);
				}
				Py_XDECREF(offset);

				PyObject *msg = PyObject_GetAttrString(value, "msg");
				if (msg) {
					err.message = PythonTypeConversion::pystring_to_godot_string(msg);
					Py_DECREF(msg);
				}
			}

			if (err.message.is_empty()) {
				err.message = "Syntax error";
			}

			r_errors->push_back(err);

			Py_XDECREF(type);
			Py_XDECREF(value);
			Py_XDECREF(traceback);
		} else {
			PythonTypeConversion::clear_python_error();
		}

		PyGILState_Release(gstate);
		return false;
	}

	Py_DECREF(code);
	PyGILState_Release(gstate);
	return true;
}

Script *PythonLanguage::create_script() const {
	return memnew(PythonScript);
}

bool PythonLanguage::supports_builtin_mode() const {
	return false; // Python scripts should be in separate files
}

bool PythonLanguage::supports_documentation() const {
	return true;
}

int PythonLanguage::find_function(const String &p_function, const String &p_code) const {
	// Simple search for "def function_name("
	String search = "def " + p_function + "(";
	int pos = p_code.find(search);
	if (pos == -1) {
		return -1;
	}

	// Count lines up to this position
	int line = 1;
	for (int i = 0; i < pos; i++) {
		if (p_code[i] == '\n') {
			line++;
		}
	}
	return line;
}

String PythonLanguage::make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const {
	String func = "def " + p_name + "(self";
	for (int i = 0; i < p_args.size(); i++) {
		func += ", " + p_args[i];
	}
	func += "):\n    pass\n";
	return func;
}

void PythonLanguage::auto_indent_code(String &p_code, int p_from_line, int p_to_line) const {
	// Basic auto-indent using 4 spaces (Python standard)
	Vector<String> lines = p_code.split("\n");

	int indent_level = 0;

	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();

		// Decrease indent for certain keywords at start
		if (line.begins_with("elif ") || line.begins_with("else:") ||
				line.begins_with("except") || line.begins_with("finally:")) {
			if (indent_level > 0) {
				indent_level--;
			}
		}

		// Apply indent if within range
		if (i >= p_from_line && i <= p_to_line && !line.is_empty()) {
			String indent;
			for (int j = 0; j < indent_level; j++) {
				indent += "    ";
			}
			lines.write[i] = indent + line;
		}

		// Increase indent after colons (simplified)
		if (line.ends_with(":")) {
			indent_level++;
		}

		// Decrease indent after return, pass, break, continue, raise
		if (line.begins_with("return") || line == "pass" ||
				line == "break" || line == "continue" ||
				line.begins_with("raise")) {
			if (indent_level > 0) {
				indent_level--;
			}
		}
	}

	p_code = String("\n").join(lines);
}

void PythonLanguage::add_global_constant(const StringName &p_variable, const Variant &p_value) {
	if (!python_initialized) {
		return;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();

	PyObject *main_module = PyImport_AddModule("__main__");
	if (main_module) {
		PyObject *main_dict = PyModule_GetDict(main_module);
		CharString name_utf8 = String(p_variable).utf8();
		PyObject *py_value = PythonTypeConversion::variant_to_pyobject(p_value);
		PyDict_SetItemString(main_dict, name_utf8.get_data(), py_value);
		Py_DECREF(py_value);
	}

	PyGILState_Release(gstate);
}

/* DEBUGGER FUNCTIONS */

String PythonLanguage::debug_get_error() const {
	return _debug_error;
}

int PythonLanguage::debug_get_stack_level_count() const {
	return 0; // TODO: Implement Python stack inspection
}

int PythonLanguage::debug_get_stack_level_line(int p_level) const {
	return 0;
}

String PythonLanguage::debug_get_stack_level_function(int p_level) const {
	return String();
}

String PythonLanguage::debug_get_stack_level_source(int p_level) const {
	return String();
}

void PythonLanguage::debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
}

void PythonLanguage::debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
}

void PythonLanguage::debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems, int p_max_depth) {
}

String PythonLanguage::debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems, int p_max_depth) {
	return String();
}

void PythonLanguage::reload_all_scripts() {
	MutexLock lock(mutex);
	SelfList<PythonScript> *elem = script_list.first();
	while (elem) {
		elem->self()->reload();
		elem = elem->next();
	}
}

void PythonLanguage::reload_scripts(const Array &p_scripts, bool p_soft_reload) {
	for (int i = 0; i < p_scripts.size(); i++) {
		Ref<PythonScript> script = p_scripts[i];
		if (script.is_valid()) {
			script->reload();
		}
	}
}

void PythonLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) {
	Ref<PythonScript> script = p_script;
	if (script.is_valid()) {
		script->reload();
	}
}

void PythonLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("py");
}

void PythonLanguage::get_public_functions(List<MethodInfo> *p_functions) const {
	// Add any built-in Python utility functions exposed to Godot
}

void PythonLanguage::get_public_constants(List<Pair<String, Variant>> *p_constants) const {
	// Add any Python-specific constants
}

void PythonLanguage::get_public_annotations(List<MethodInfo> *p_annotations) const {
	// Add Python decorator annotations
	MethodInfo tool;
	tool.name = "@tool";
	p_annotations->push_back(tool);

	MethodInfo export_ann;
	export_ann.name = "@export";
	p_annotations->push_back(export_ann);
}

void PythonLanguage::profiling_start() {
}

void PythonLanguage::profiling_stop() {
}

void PythonLanguage::profiling_set_save_native_calls(bool p_enable) {
}

int PythonLanguage::profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) {
	return 0;
}

int PythonLanguage::profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) {
	return 0;
}

bool PythonLanguage::handles_global_class_type(const String &p_type) const {
	return p_type == "PythonScript";
}

String PythonLanguage::get_global_class_name(const String &p_path, String *r_base_type, String *r_icon_path, bool *r_is_abstract, bool *r_is_tool) const {
	if (!p_path.ends_with(".py")) {
		return String();
	}

	// TODO: Parse the Python file to extract class name and metadata
	return String();
}

void PythonLanguage::acquire_gil() {
	if (python_initialized) {
		gil_state = PyGILState_Ensure();
	}
}

void PythonLanguage::release_gil() {
	if (python_initialized) {
		PyGILState_Release(gil_state);
	}
}
