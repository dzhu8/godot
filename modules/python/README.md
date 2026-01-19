# Python Module for Godot Engine

This module adds Python scripting support to the Godot Engine, allowing you to write game logic using Python instead of (or alongside) GDScript.

## Features

- **Python 3 Support**: Uses the system's Python 3 interpreter
- **Lifecycle Integration**: Full support for `_ready()`, `_process()`, `_notification()`, etc.
- **Inspector Export**: Python class annotations are exposed in the Godot Inspector
- **Type Conversion**: Automatic conversion between Godot Variant types and Python objects
- **Syntax Highlighting**: Python keywords and syntax are highlighted in the editor

## Building

The module requires Python development headers to be available:

### Windows
Make sure Python is installed and the `include` and `libs` directories are accessible.

### Linux/macOS
Install Python development packages:
```bash
# Ubuntu/Debian
sudo apt install python3-dev

# macOS (with Homebrew)
brew install python3
```

### Build Command
```bash
scons platform=windows target=editor module_python_enabled=yes
```

## Usage

### Creating a Python Script

1. Create a new file with `.py` extension
2. Define a class with the methods you want to implement:

```python
class MyPlayer(object):
    # Exported to Inspector
    speed: float = 5.0
    health: int = 100

    def _ready(self):
        print("Player ready!")

    def _process(self, delta):
        # Game logic here
        pass

    def _input(self, event):
        # Handle input
        pass
```

3. Attach the script to a node in your scene

### Supported Lifecycle Methods

- `_ready()` - Called when node enters scene tree
- `_process(delta)` - Called every frame
- `_physics_process(delta)` - Called every physics frame
- `_input(event)` - Called for input events
- `_notification(what)` - Called for engine notifications

### Type Mapping

| Godot Type | Python Type |
|------------|-------------|
| nil | None |
| bool | bool |
| int | int |
| float | float |
| String | str |
| Array | list |
| Dictionary | dict |
| Vector2 | tuple(x, y) |
| Vector3 | tuple(x, y, z) |
| Color | tuple(r, g, b, a) |

## Testing

A test project is included in `test_project/`. Open it in the Godot editor to verify the Python integration is working correctly.

## Limitations

- Python scripts must be in separate `.py` files (no built-in scripts)
- GIL (Global Interpreter Lock) considerations for multi-threading
- Some advanced Godot features may not be fully supported yet

## License

This module is part of the Godot Engine and is released under the MIT license.
