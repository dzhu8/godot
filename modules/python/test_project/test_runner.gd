# GDScript test runner for the Python scripting tests
# This provides button handlers and allows re-running tests

extends Node

@onready var hello_world_test = $"../HelloWorldTest"
@onready var inspector_test = $"../InspectorTestNode"
@onready var api_test = $"../APIAccessTestNode"
@onready var run_button = $VBoxContainer/RunTestsButton
@onready var quit_button = $VBoxContainer/QuitButton

func _ready():
	print("")
	print("============================================================")
	print("PYTHON SCRIPTING TEST PROJECT LOADED")
	print("============================================================")
	print("If you see Python test output below, the lifecycle works!")
	print("============================================================")
	print("")

	# Connect button signals
	run_button.pressed.connect(_on_run_tests_pressed)
	quit_button.pressed.connect(_on_quit_pressed)

func _on_run_tests_pressed():
	print("")
	print("============================================================")
	print("MANUALLY RUNNING TESTS...")
	print("============================================================")

	# Try to call _ready on each test node to re-run tests
	# This tests that we can call Python methods from GDScript
	if hello_world_test and hello_world_test.has_method("_ready"):
		print("Calling HelloWorldTest._ready()...")
		hello_world_test._ready()

	if inspector_test and inspector_test.has_method("_ready"):
		print("Calling InspectorTest._ready()...")
		inspector_test._ready()

	if api_test and api_test.has_method("_ready"):
		print("Calling APIAccessTest._ready()...")
		api_test._ready()

	# Also test the custom greet method
	if hello_world_test and hello_world_test.has_method("greet"):
		print("")
		print("Testing cross-language call: greet('Godot')")
		var result = hello_world_test.greet("Godot")
		print("Result: ", result)

func _on_quit_pressed():
	get_tree().quit()

func _input(event):
	if event.is_action_pressed("ui_cancel"):
		get_tree().quit()
