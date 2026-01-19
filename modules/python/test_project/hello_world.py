# Python script - Hello World Lifecycle Test
# This script tests the basic lifecycle methods: _ready, _process, _notification

class HelloWorld(object):
    """
    A basic Python script to test the lifecycle integration with Godot.
    Attach this script to a Node3D to test Python scripting.
    """

    # Exported properties (visible in Inspector) - Inspector Test
    speed: float = 1.0
    rotation_speed: float = 2.0
    message: str = "Hello from Python!"
    enabled: bool = True

    # Internal state
    _time_elapsed: float = 0.0
    _ready_called: bool = False

    def _ready(self):
        """
        Called when the node enters the scene tree for the first time.
        This is the Hello World Lifecycle Test - SUCCESS if you see this message.
        """
        self._ready_called = True
        print("=" * 50)
        print("PYTHON LIFECYCLE TEST: SUCCESS!")
        print("=" * 50)
        print("  _ready() was called successfully")
        print("  Message:", self.message)
        print("  Speed:", self.speed)
        print("  Rotation Speed:", self.rotation_speed)
        print("  Enabled:", self.enabled)
        print("=" * 50)

    def _process(self, delta):
        """
        Called every frame. 'delta' is the elapsed time since the previous frame.
        This proves the engine loop is correctly calling Python methods.
        """
        if not self.enabled:
            return

        self._time_elapsed += delta

        # Rotate the node if this is a Node3D (visual confirmation)
        # Access Godot methods through self.owner
        try:
            # Try to rotate (will work if attached to Node3D)
            self.owner.rotate_y(delta * self.rotation_speed)
        except:
            pass  # Not a Node3D or owner not available, that's OK

    def _notification(self, what):
        """
        Called for various engine notifications.
        """
        # Common notification constants
        NOTIFICATION_READY = 13
        NOTIFICATION_PROCESS = 17
        NOTIFICATION_EXIT_TREE = 32

        if what == NOTIFICATION_READY:
            print("PYTHON: Received NOTIFICATION_READY")
        elif what == NOTIFICATION_EXIT_TREE:
            print("PYTHON: Received NOTIFICATION_EXIT_TREE - Goodbye!")

    def _input(self, event):
        """
        Called when an input event is received.
        API Access Test - demonstrates receiving Godot input events.
        """
        # Check if it's a key press
        try:
            if event.is_action_pressed("ui_accept"):
                print("PYTHON API ACCESS TEST: ui_accept pressed!")
                print("  This proves Python can receive and process Godot input events.")
        except:
            pass

    # Custom method that can be called from GDScript
    def greet(self, name):
        """
        A custom method demonstrating cross-language calls.
        Call this from GDScript: python_node.greet("World")
        """
        greeting = f"Hello, {name}! Greetings from Python!"
        print("PYTHON:", greeting)
        return greeting

    def get_status(self):
        """
        Returns the current status of the script.
        Useful for cross-language testing.
        """
        return {
            "ready_called": self._ready_called,
            "time_elapsed": self._time_elapsed,
            "message": self.message,
            "enabled": self.enabled
        }
