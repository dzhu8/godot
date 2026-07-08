/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "python_language.h"
#include "python_script.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"

PythonLanguage *script_language_python = nullptr;
Ref<ResourceFormatLoaderPythonScript> resource_loader_python;
Ref<ResourceFormatSaverPythonScript> resource_saver_python;

void initialize_python_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		GDREGISTER_CLASS(PythonScript);

		script_language_python = memnew(PythonLanguage);
		ScriptServer::register_language(script_language_python);

		resource_loader_python.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_python);

		resource_saver_python.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_python);
	}
}

void uninitialize_python_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		ScriptServer::unregister_language(script_language_python);

		if (script_language_python) {
			memdelete(script_language_python);
			script_language_python = nullptr;
		}

		ResourceLoader::remove_resource_format_loader(resource_loader_python);
		resource_loader_python.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_python);
		resource_saver_python.unref();
	}
}
