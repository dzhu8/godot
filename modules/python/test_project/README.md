# Python Module Test Project

This is a test project for the Godot Python scripting module.

## Running the Tests

1. Build Godot with the Python module enabled:
   ```bash
   scons platform=windows target=editor
   ```
2. Run the test project from the command line:
   ```bash
   ./bin/godot.windows.editor.x86_64.exe --path modules/python/test_project res://main.tscn
   ```
   Or open this project in the Godot editor and press F5.
3. Check the Output console for test results
4. Use the "Run Tests Again" button to re-run tests manually

## Test Scripts

### hello_world.py - Lifecycle Test
Tests that Python scripts receive Godot lifecycle callbacks:
- `_ready()` - Called when node enters scene tree
- `_process(delta)` - Called every frame
- `_notification(what)` - Called for engine notifications

**SUCCESS**: You should see "PYTHON LIFECYCLE TEST: SUCCESS!" in the console

### inspector_test.py - Inspector Test
Tests that Python class annotations are exported to the Inspector:
- Integer, float, string, boolean properties
- Default values

**SUCCESS**: Properties should appear in the Inspector panel when the node is selected

### api_access_test.py - API Access Test
Tests that Python can call Godot engine methods:
- Node property access
- Scene tree access
- Print methods
- Input event handling

**SUCCESS**: You should see "ALL API ACCESS TESTS PASSED!" in the console

## UI Controls

- **Run Tests Again**: Re-runs all Python test scripts' `_ready()` methods
- **Quit (Escape)**: Closes the application

## Expected Console Output

```
============================================================
PYTHON SCRIPTING TEST PROJECT LOADED
============================================================
If you see Python test output below, the lifecycle works!
============================================================

==================================================
PYTHON LIFECYCLE TEST: SUCCESS!
==================================================
  _ready() was called successfully
  Message: Hello from Python!
  Speed: 1.0
  ...

============================================================
INSPECTOR TEST - Property Values
============================================================
  integer_value: 42 (expected: 42)
  ...

============================================================
API ACCESS TEST - Testing Godot Engine API from Python
============================================================
Test 1: Node Property Access
...
RESULTS: 3/3 tests passed
ALL API ACCESS TESTS PASSED!
============================================================
```

## Troubleshooting

### No Python output appears
- Ensure Python is properly initialized (check for "Python X.X.X initialized for Godot" message)
- Verify the scripts have the correct class structure with `_ready()` method

### F5 doesn't work
- Make sure `project.godot` has `run/main_scene="res://main.tscn"` set
- Try running from command line as shown above
