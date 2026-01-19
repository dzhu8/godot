# Python script - Inspector Test
# This script specifically tests that exported properties appear in the Godot Inspector

class InspectorTest(object):
    """
    Test script to verify that Python class annotations are properly
    exported to the Godot Inspector panel.

    INSTRUCTIONS:
    1. Attach this script to any Node
    2. Select the node in the Scene tree
    3. Look at the Inspector panel on the right
    4. You should see all the properties below with their default values
    5. Try modifying them - changes should be reflected in the script
    """

    # Basic types - these should all appear in Inspector
    integer_value: int = 42
    float_value: float = 3.14159
    string_value: str = "Hello Inspector!"
    boolean_value: bool = True

    # Grouped properties (by naming convention)
    movement_speed: float = 5.0
    movement_acceleration: float = 10.0
    movement_friction: float = 0.9

    # More properties for testing
    health: int = 100
    max_health: int = 100
    damage: float = 25.0

    # String properties
    player_name: str = "Player1"
    team_name: str = "Blue Team"

    def _ready(self):
        """Log all exported properties to verify they match Inspector values."""
        print("")
        print("=" * 60)
        print("INSPECTOR TEST - Property Values")
        print("=" * 60)
        print(f"  integer_value: {self.integer_value} (expected: 42)")
        print(f"  float_value: {self.float_value} (expected: 3.14159)")
        print(f"  string_value: {self.string_value} (expected: 'Hello Inspector!')")
        print(f"  boolean_value: {self.boolean_value} (expected: True)")
        print("")
        print("  Movement Properties:")
        print(f"    movement_speed: {self.movement_speed}")
        print(f"    movement_acceleration: {self.movement_acceleration}")
        print(f"    movement_friction: {self.movement_friction}")
        print("")
        print("  Combat Properties:")
        print(f"    health: {self.health}")
        print(f"    max_health: {self.max_health}")
        print(f"    damage: {self.damage}")
        print("")
        print("  Identity Properties:")
        print(f"    player_name: {self.player_name}")
        print(f"    team_name: {self.team_name}")
        print("=" * 60)
        print("")
        print("SUCCESS: If you modified any values in the Inspector,")
        print("they should be different from the defaults shown above.")
        print("=" * 60)

    def reset_to_defaults(self):
        """Reset all properties to their default values."""
        self.integer_value = 42
        self.float_value = 3.14159
        self.string_value = "Hello Inspector!"
        self.boolean_value = True
        self.movement_speed = 5.0
        self.movement_acceleration = 10.0
        self.movement_friction = 0.9
        self.health = 100
        self.max_health = 100
        self.damage = 25.0
        self.player_name = "Player1"
        self.team_name = "Blue Team"
        print("PYTHON: All properties reset to defaults")
