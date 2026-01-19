/**************************************************************************/
/*  python_type_conversion.cpp                                            */
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

#include "python_type_conversion.h"

PyObject *PythonTypeConversion::variant_to_pyobject(const Variant &p_variant) {
	switch (p_variant.get_type()) {
		case Variant::NIL:
			Py_RETURN_NONE;

		case Variant::BOOL:
			if ((bool)p_variant) {
				Py_RETURN_TRUE;
			} else {
				Py_RETURN_FALSE;
			}

		case Variant::INT:
			return PyLong_FromLongLong((int64_t)p_variant);

		case Variant::FLOAT:
			return PyFloat_FromDouble((double)p_variant);

		case Variant::STRING:
			return godot_string_to_pystring((String)p_variant);

		case Variant::VECTOR2: {
			Vector2 v = p_variant;
			return Py_BuildValue("(dd)", v.x, v.y);
		}

		case Variant::VECTOR3: {
			Vector3 v = p_variant;
			return Py_BuildValue("(ddd)", v.x, v.y, v.z);
		}

		case Variant::COLOR: {
			Color c = p_variant;
			return Py_BuildValue("(dddd)", c.r, c.g, c.b, c.a);
		}

		case Variant::ARRAY:
			return godot_array_to_pylist((Array)p_variant);

		case Variant::DICTIONARY:
			return godot_dict_to_pydict((Dictionary)p_variant);

		case Variant::PACKED_STRING_ARRAY:
			return packed_string_array_to_pylist((PackedStringArray)p_variant);

		default:
			// For unsupported types, return a string representation
			return godot_string_to_pystring(p_variant.stringify());
	}
}

Variant PythonTypeConversion::pyobject_to_variant(PyObject *p_obj) {
	if (p_obj == nullptr || p_obj == Py_None) {
		return Variant();
	}

	if (PyBool_Check(p_obj)) {
		return p_obj == Py_True;
	}

	if (PyLong_Check(p_obj)) {
		return (int64_t)PyLong_AsLongLong(p_obj);
	}

	if (PyFloat_Check(p_obj)) {
		return PyFloat_AsDouble(p_obj);
	}

	if (PyUnicode_Check(p_obj)) {
		return pystring_to_godot_string(p_obj);
	}

	if (PyList_Check(p_obj)) {
		return pylist_to_godot_array(p_obj);
	}

	if (PyTuple_Check(p_obj)) {
		// Convert tuple to Array as well
		Py_ssize_t size = PyTuple_Size(p_obj);
		Array arr;
		arr.resize(size);
		for (Py_ssize_t i = 0; i < size; i++) {
			arr[i] = pyobject_to_variant(PyTuple_GetItem(p_obj, i));
		}
		return arr;
	}

	if (PyDict_Check(p_obj)) {
		return pydict_to_godot_dict(p_obj);
	}

	// For unknown types, try to convert to string
	PyObject *str_obj = PyObject_Str(p_obj);
	if (str_obj) {
		String result = pystring_to_godot_string(str_obj);
		Py_DECREF(str_obj);
		return result;
	}

	return Variant();
}

PyObject *PythonTypeConversion::godot_string_to_pystring(const String &p_string) {
	CharString utf8 = p_string.utf8();
	return PyUnicode_FromStringAndSize(utf8.get_data(), utf8.length());
}

String PythonTypeConversion::pystring_to_godot_string(PyObject *p_str) {
	if (!PyUnicode_Check(p_str)) {
		return String();
	}

	Py_ssize_t size;
	const char *utf8 = PyUnicode_AsUTF8AndSize(p_str, &size);
	if (utf8 == nullptr) {
		clear_python_error();
		return String();
	}

	return String::utf8(utf8, size);
}

PyObject *PythonTypeConversion::godot_array_to_pylist(const Array &p_array) {
	Py_ssize_t size = p_array.size();
	PyObject *list = PyList_New(size);

	for (Py_ssize_t i = 0; i < size; i++) {
		PyObject *item = variant_to_pyobject(p_array[i]);
		PyList_SET_ITEM(list, i, item); // Steals reference
	}

	return list;
}

Array PythonTypeConversion::pylist_to_godot_array(PyObject *p_list) {
	Array arr;

	if (!PyList_Check(p_list)) {
		return arr;
	}

	Py_ssize_t size = PyList_Size(p_list);
	arr.resize(size);

	for (Py_ssize_t i = 0; i < size; i++) {
		arr[i] = pyobject_to_variant(PyList_GetItem(p_list, i));
	}

	return arr;
}

PyObject *PythonTypeConversion::godot_dict_to_pydict(const Dictionary &p_dict) {
	PyObject *dict = PyDict_New();

	Array keys = p_dict.keys();
	for (int i = 0; i < keys.size(); i++) {
		PyObject *key = variant_to_pyobject(keys[i]);
		PyObject *value = variant_to_pyobject(p_dict[keys[i]]);
		PyDict_SetItem(dict, key, value);
		Py_DECREF(key);
		Py_DECREF(value);
	}

	return dict;
}

Dictionary PythonTypeConversion::pydict_to_godot_dict(PyObject *p_dict) {
	Dictionary dict;

	if (!PyDict_Check(p_dict)) {
		return dict;
	}

	PyObject *key, *value;
	Py_ssize_t pos = 0;

	while (PyDict_Next(p_dict, &pos, &key, &value)) {
		dict[pyobject_to_variant(key)] = pyobject_to_variant(value);
	}

	return dict;
}

PyObject *PythonTypeConversion::packed_string_array_to_pylist(const PackedStringArray &p_array) {
	Py_ssize_t size = p_array.size();
	PyObject *list = PyList_New(size);

	for (Py_ssize_t i = 0; i < size; i++) {
		PyObject *item = godot_string_to_pystring(p_array[i]);
		PyList_SET_ITEM(list, i, item);
	}

	return list;
}

PackedStringArray PythonTypeConversion::pylist_to_packed_string_array(PyObject *p_list) {
	PackedStringArray arr;

	if (!PyList_Check(p_list)) {
		return arr;
	}

	Py_ssize_t size = PyList_Size(p_list);
	arr.resize(size);

	for (Py_ssize_t i = 0; i < size; i++) {
		PyObject *item = PyList_GetItem(p_list, i);
		if (PyUnicode_Check(item)) {
			arr.set(i, pystring_to_godot_string(item));
		}
	}

	return arr;
}

String PythonTypeConversion::get_python_error() {
	if (!PyErr_Occurred()) {
		return String();
	}

	PyObject *type, *value, *traceback;
	PyErr_Fetch(&type, &value, &traceback);
	PyErr_NormalizeException(&type, &value, &traceback);

	String error_msg;

	if (value) {
		PyObject *str_value = PyObject_Str(value);
		if (str_value) {
			error_msg = pystring_to_godot_string(str_value);
			Py_DECREF(str_value);
		}
	}

	if (type) {
		PyObject *type_name = PyObject_GetAttrString(type, "__name__");
		if (type_name) {
			String type_str = pystring_to_godot_string(type_name);
			error_msg = type_str + ": " + error_msg;
			Py_DECREF(type_name);
		}
	}

	// Try to get traceback information
	if (traceback) {
		PyObject *traceback_module = PyImport_ImportModule("traceback");
		if (traceback_module) {
			PyObject *format_tb = PyObject_GetAttrString(traceback_module, "format_tb");
			if (format_tb) {
				PyObject *tb_list = PyObject_CallFunctionObjArgs(format_tb, traceback, nullptr);
				if (tb_list && PyList_Check(tb_list)) {
					error_msg += "\nTraceback:\n";
					for (Py_ssize_t i = 0; i < PyList_Size(tb_list); i++) {
						error_msg += pystring_to_godot_string(PyList_GetItem(tb_list, i));
					}
					Py_DECREF(tb_list);
				}
				Py_DECREF(format_tb);
			}
			Py_DECREF(traceback_module);
		}
	}

	Py_XDECREF(type);
	Py_XDECREF(value);
	Py_XDECREF(traceback);

	return error_msg;
}

void PythonTypeConversion::clear_python_error() {
	PyErr_Clear();
}

bool PythonTypeConversion::has_python_error() {
	return PyErr_Occurred() != nullptr;
}

// ============================================================================
// Godot Object Wrapper for Python
// ============================================================================

// Structure to hold a Godot Object pointer in Python
typedef struct {
	PyObject_HEAD
	Object *godot_object;
	ObjectID object_id; // For safety checking if object is still valid
} PyGodotObject;

// Called when the Python wrapper is deallocated
static void PyGodotObject_dealloc(PyGodotObject *self) {
	// We don't own the Godot object, so don't delete it
	self->godot_object = nullptr;
	Py_TYPE(self)->tp_free((PyObject *)self);
}

// Get attribute - this is where the magic happens
// It allows Python to call any method on the Godot object
static PyObject *PyGodotObject_getattro(PyGodotObject *self, PyObject *name) {
	// First check if the object is still valid
	if (!self->godot_object || !ObjectDB::get_instance(self->object_id)) {
		PyErr_SetString(PyExc_RuntimeError, "Godot object has been freed");
		return nullptr;
	}

	String attr_name = PythonTypeConversion::pystring_to_godot_string(name);

	// Check if it's a method FIRST - return a callable wrapper
	// This must come before property check because get() might return Callables for methods
	if (self->godot_object->has_method(StringName(attr_name))) {
		String method_name = attr_name;
		// Create a bound method wrapper using a lambda-like approach
		// We'll use PyCFunction with closure data

		// Store method name and object reference for later call
		PyObject *capsule = PyCapsule_New(self->godot_object, "godot_object", nullptr);
		if (!capsule) {
			return nullptr;
		}

		// Create a tuple with (object_capsule, method_name, object_id)
		PyObject *closure = PyTuple_New(3);
		PyTuple_SET_ITEM(closure, 0, capsule); // Steals reference
		Py_INCREF(name);
		PyTuple_SET_ITEM(closure, 1, name);
		PyTuple_SET_ITEM(closure, 2, PyLong_FromUnsignedLongLong(self->object_id));

		// Create a method that calls the Godot method
		static PyMethodDef method_def = { "godot_method_caller", [](PyObject *closure_arg, PyObject *args) -> PyObject * {
			PyObject *capsule = PyTuple_GetItem(closure_arg, 0);
			PyObject *method_name_obj = PyTuple_GetItem(closure_arg, 1);
			uint64_t obj_id = PyLong_AsUnsignedLongLong(PyTuple_GetItem(closure_arg, 2));

			// Check if object is still valid
			Object *obj = ObjectDB::get_instance(ObjectID(obj_id));
			if (!obj) {
				PyErr_SetString(PyExc_RuntimeError, "Godot object has been freed");
				return nullptr;
			}

			String method_name = PythonTypeConversion::pystring_to_godot_string(method_name_obj);

			// Convert Python args to Godot Variants
			Py_ssize_t arg_count = args ? PyTuple_Size(args) : 0;
			Vector<Variant> variants;
			variants.resize(arg_count);
			const Variant **variant_ptrs = nullptr;

			if (arg_count > 0) {
				variant_ptrs = (const Variant **)alloca(sizeof(Variant *) * arg_count);
				for (Py_ssize_t i = 0; i < arg_count; i++) {
					variants.write[i] = PythonTypeConversion::pyobject_to_variant(PyTuple_GetItem(args, i));
					variant_ptrs[i] = &variants[i];
				}
			}

			// Call the Godot method
			Callable::CallError error;
			Variant result = obj->callp(StringName(method_name), variant_ptrs, arg_count, error);

			if (error.error != Callable::CallError::CALL_OK) {
				String err_msg = "Error calling method '" + method_name + "': ";
				switch (error.error) {
					case Callable::CallError::CALL_ERROR_INVALID_ARGUMENT:
						err_msg += "Invalid argument at position " + String::num_int64(error.argument);
						break;
					case Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS:
						err_msg += "Too many arguments";
						break;
					case Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS:
						err_msg += "Too few arguments";
						break;
					case Callable::CallError::CALL_ERROR_INVALID_METHOD:
						err_msg += "Invalid method";
						break;
					default:
						err_msg += "Unknown error";
				}
				PyErr_SetString(PyExc_RuntimeError, err_msg.utf8().get_data());
				return nullptr;
			}

			// If result is an Object, wrap it
			if (result.get_type() == Variant::OBJECT) {
				Object *result_obj = result;
				if (result_obj) {
					return PythonTypeConversion::wrap_godot_object(result_obj);
				}
			}

			return PythonTypeConversion::variant_to_pyobject(result);
		}, METH_VARARGS, "Call a Godot method" };

		PyObject *method = PyCFunction_New(&method_def, closure);
		Py_DECREF(closure); // PyCFunction_New increfs it
		return method;
	}

	// Check if it's a property (after method check to avoid Callable conversion issues)
	bool valid = false;
	Variant prop_value = self->godot_object->get(StringName(attr_name), &valid);
	if (valid) {
		// If the property is an Object, wrap it
		if (prop_value.get_type() == Variant::OBJECT) {
			Object *prop_obj = prop_value;
			if (prop_obj) {
				return PythonTypeConversion::wrap_godot_object(prop_obj);
			}
		}
		return PythonTypeConversion::variant_to_pyobject(prop_value);
	}

	// Fall back to default attribute lookup
	return PyObject_GenericGetAttr((PyObject *)self, name);
}

// Set attribute
static int PyGodotObject_setattro(PyGodotObject *self, PyObject *name, PyObject *value) {
	if (!self->godot_object || !ObjectDB::get_instance(self->object_id)) {
		PyErr_SetString(PyExc_RuntimeError, "Godot object has been freed");
		return -1;
	}

	String prop_name = PythonTypeConversion::pystring_to_godot_string(name);
	Variant godot_value = PythonTypeConversion::pyobject_to_variant(value);

	bool valid = false;
	self->godot_object->set(StringName(prop_name), godot_value, &valid);

	if (!valid) {
		PyErr_Format(PyExc_AttributeError, "Cannot set attribute '%s'", prop_name.utf8().get_data());
		return -1;
	}

	return 0;
}

// String representation
static PyObject *PyGodotObject_str(PyGodotObject *self) {
	if (!self->godot_object || !ObjectDB::get_instance(self->object_id)) {
		return PyUnicode_FromString("<freed Godot object>");
	}

	String str = "[" + self->godot_object->get_class() + "]";
	return PythonTypeConversion::godot_string_to_pystring(str);
}

// Type definition - initialized here
static PyTypeObject PyGodotObjectType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"godot.Object",                        /* tp_name */
	sizeof(PyGodotObject),                 /* tp_basicsize */
	0,                                     /* tp_itemsize */
	(destructor)PyGodotObject_dealloc,     /* tp_dealloc */
	0,                                     /* tp_vectorcall_offset */
	nullptr,                               /* tp_getattr */
	nullptr,                               /* tp_setattr */
	nullptr,                               /* tp_as_async */
	nullptr,                               /* tp_repr */
	nullptr,                               /* tp_as_number */
	nullptr,                               /* tp_as_sequence */
	nullptr,                               /* tp_as_mapping */
	nullptr,                               /* tp_hash */
	nullptr,                               /* tp_call */
	(reprfunc)PyGodotObject_str,           /* tp_str */
	(getattrofunc)PyGodotObject_getattro,  /* tp_getattro */
	(setattrofunc)PyGodotObject_setattro,  /* tp_setattro */
	nullptr,                               /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                    /* tp_flags */
	"Wrapper for Godot Object",            /* tp_doc */
	nullptr,                               /* tp_traverse */
	nullptr,                               /* tp_clear */
	nullptr,                               /* tp_richcompare */
	0,                                     /* tp_weaklistoffset */
	nullptr,                               /* tp_iter */
	nullptr,                               /* tp_iternext */
	nullptr,                               /* tp_methods */
	nullptr,                               /* tp_members */
	nullptr,                               /* tp_getset */
	nullptr,                               /* tp_base */
	nullptr,                               /* tp_dict */
	nullptr,                               /* tp_descr_get */
	nullptr,                               /* tp_descr_set */
	0,                                     /* tp_dictoffset */
	nullptr,                               /* tp_init */
	nullptr,                               /* tp_alloc */
	PyType_GenericNew,                     /* tp_new */
};

static bool godot_object_type_initialized = false;

void PythonTypeConversion::initialize_godot_object_type() {
	if (godot_object_type_initialized) {
		return;
	}

	if (PyType_Ready(&PyGodotObjectType) < 0) {
		ERR_PRINT("Failed to initialize PyGodotObjectType");
		return;
	}

	godot_object_type_initialized = true;
}

PyTypeObject *PythonTypeConversion::get_godot_object_type() {
	if (!godot_object_type_initialized) {
		initialize_godot_object_type();
	}
	return &PyGodotObjectType;
}

PyObject *PythonTypeConversion::wrap_godot_object(Object *p_object) {
	if (!p_object) {
		Py_RETURN_NONE;
	}

	if (!godot_object_type_initialized) {
		initialize_godot_object_type();
	}

	PyGodotObject *wrapper = PyObject_New(PyGodotObject, &PyGodotObjectType);
	if (!wrapper) {
		return nullptr;
	}

	wrapper->godot_object = p_object;
	wrapper->object_id = p_object->get_instance_id();

	return (PyObject *)wrapper;
}

Object *PythonTypeConversion::unwrap_godot_object(PyObject *p_wrapper) {
	if (!p_wrapper || !PyObject_TypeCheck(p_wrapper, &PyGodotObjectType)) {
		return nullptr;
	}

	PyGodotObject *wrapper = (PyGodotObject *)p_wrapper;

	// Check if object is still valid
	if (!ObjectDB::get_instance(wrapper->object_id)) {
		return nullptr;
	}

	return wrapper->godot_object;
}

bool PythonTypeConversion::is_godot_object_wrapper(PyObject *p_obj) {
	return p_obj && PyObject_TypeCheck(p_obj, &PyGodotObjectType);
}
