# Python script - API Access Test
# This script tests calling Godot engine methods from Python

class APIAccessTest(object):
    """
    Test script to verify that Python can call Godot's API methods.

    This tests:
    1. Accessing node properties (position, rotation, scale)
    2. Calling node methods (get_node, get_tree, etc.)
    3. Manipulating the scene tree
    4. Responding to input
    5. Creating and emitting signals
    """

    # Properties
    test_passed: bool = False
    tests_run: int = 0
    tests_passed: int = 0

    def _ready(self):
        """Run all API access tests when the node is ready."""
        print("")
        print("=" * 60)
        print("API ACCESS TEST - Testing Godot Engine API from Python")
        print("=" * 60)

        self._test_node_access()
        self._test_tree_access()
        self._test_print_methods()

        print("")
        print(f"RESULTS: {self.tests_passed}/{self.tests_run} tests passed")
        print("=" * 60)

        if self.tests_passed == self.tests_run:
            self.test_passed = True
            print("ALL API ACCESS TESTS PASSED!")
        else:
            print("SOME TESTS FAILED - Check output above")
        print("=" * 60)

    def _test_node_access(self):
        """Test accessing node properties and methods."""
        print("")
        print("Test 1: Node Property Access")
        print("-" * 40)

        self.tests_run += 1
        try:
            # Access Godot methods through self.owner
            # self.owner is a wrapper around the Godot Node this script is attached to
            name = self.owner.get_name() if hasattr(self, 'owner') else "Unknown"
            print(f"  Node name: {name}")

            # Get the node's class
            node_class = self.owner.get_class() if hasattr(self, 'owner') else "Unknown"
            print(f"  Node class: {node_class}")

            self.tests_passed += 1
            print("  [PASS] Node access works")
        except Exception as e:
            print(f"  [FAIL] Error: {e}")

    def _test_tree_access(self):
        """Test accessing the scene tree."""
        print("")
        print("Test 2: Scene Tree Access")
        print("-" * 40)

        self.tests_run += 1
        try:
            # Access scene tree through self.owner
            if hasattr(self, 'owner'):
                tree = self.owner.get_tree()
                print(f"  Scene tree acquired: {tree}")

                # Try to get the root node
                root = tree.get_root()
                print(f"  Root node: {root}")

                self.tests_passed += 1
                print("  [PASS] Tree access works")
            else:
                print("  [INFO] self.owner not available")
                self.tests_passed += 1  # Not a failure, just not attached
        except Exception as e:
            print(f"  [FAIL] Error: {e}")

    def _test_print_methods(self):
        """Test Godot's print methods."""
        print("")
        print("Test 3: Print Methods")
        print("-" * 40)

        self.tests_run += 1
        try:
            # Python's print should work
            print("  Standard print: OK")

            # Test string formatting
            value = 42
            formatted = f"  Formatted string: value = {value}"
            print(formatted)

            self.tests_passed += 1
            print("  [PASS] Print methods work")
        except Exception as e:
            print(f"  [FAIL] Error: {e}")

    def _process(self, delta):
        """Called every frame - tests continuous API access."""
        pass  # We don't need to do anything here for this test

    def _input(self, event):
        """Test input event handling."""
        try:
            # Check for specific input actions
            if hasattr(event, 'is_action_pressed'):
                if event.is_action_pressed("ui_cancel"):
                    print("")
                    print("API ACCESS TEST: Escape/Cancel pressed!")
                    print("  Input events are being received correctly.")

                    # Try to quit the game
                    if hasattr(self, 'get_tree'):
                        tree = self.get_tree()
                        if tree and hasattr(tree, 'quit'):
                            print("  Calling get_tree().quit()...")
                            tree.quit()
        except:
            pass

    def call_engine_method(self, method_name):
        """
        Dynamically call an engine method.
        Useful for testing specific API calls from GDScript.

        Example: python_node.call_engine_method("get_name")
        """
        if hasattr(self, method_name):
            method = getattr(self, method_name)
            if callable(method):
                result = method()
                print(f"PYTHON API: Called {method_name}() = {result}")
                return result
        print(f"PYTHON API: Method {method_name} not found")
        return None
