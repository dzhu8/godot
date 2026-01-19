def can_build(env, platform):
    # Check if Python development headers are available
    # For now, we'll assume they are available
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "PythonScript",
    ]


def get_doc_path():
    return "doc_classes"
