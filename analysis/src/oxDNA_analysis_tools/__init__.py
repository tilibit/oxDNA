from pathlib import Path

__version__ = "2.4.2"


def get_resource(resources: str) -> Path:
    """Return the path to the resource file.

    Args:
        resources: Name of the resource file.

    Returns:
        Path to the resource file.

    Raises:
        FileNotFoundError: If the resource file is not found.
    """
    file = Path(__file__).parent / "resources" / resources
    if not file.exists():
        raise FileNotFoundError(f"Resource file {file} not found.")
    return file
