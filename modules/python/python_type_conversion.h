/**************************************************************************/
/*  python_type_conversion.h                                              */
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

#pragma once

// We need to ensure Python.h is included before any standard headers
// to avoid conflicts with various macros
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include "core/variant/variant.h"
#include "core/object/object.h"
#include "core/string/ustring.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/color.h"
#include "core/templates/hash_map.h"

class PythonTypeConversion {
public:
	// Convert Godot Variant to Python object
	static PyObject *variant_to_pyobject(const Variant &p_variant);

	// Convert Python object to Godot Variant
	static Variant pyobject_to_variant(PyObject *p_obj);

	// Convert Godot String to Python string
	static PyObject *godot_string_to_pystring(const String &p_string);

	// Convert Python string to Godot String
	static String pystring_to_godot_string(PyObject *p_str);

	// Convert Godot Array to Python list
	static PyObject *godot_array_to_pylist(const Array &p_array);

	// Convert Python list to Godot Array
	static Array pylist_to_godot_array(PyObject *p_list);

	// Convert Godot Dictionary to Python dict
	static PyObject *godot_dict_to_pydict(const Dictionary &p_dict);

	// Convert Python dict to Godot Dictionary
	static Dictionary pydict_to_godot_dict(PyObject *p_dict);

	// Convert Godot PackedStringArray to Python list
	static PyObject *packed_string_array_to_pylist(const PackedStringArray &p_array);

	// Convert Python list to PackedStringArray
	static PackedStringArray pylist_to_packed_string_array(PyObject *p_list);

	// Handle Python exceptions and convert to Godot error messages
	static String get_python_error();

	// Clear Python error state
	static void clear_python_error();

	// Check if Python has an error set
	static bool has_python_error();

	// Godot Object wrapper for Python
	static PyTypeObject *get_godot_object_type();
	static PyObject *wrap_godot_object(Object *p_object);
	static Object *unwrap_godot_object(PyObject *p_wrapper);
	static bool is_godot_object_wrapper(PyObject *p_obj);
	static void initialize_godot_object_type();
};
