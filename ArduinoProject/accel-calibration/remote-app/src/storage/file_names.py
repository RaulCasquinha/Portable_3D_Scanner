"""
Names typed by the user, made safe to use as file and folder names.

Layer: storage.
"""

import re

__all__ = ["clean_name"]


def clean_name(name):
    """A name safe for a file: letters, digits, - _ and spaces only, trimmed."""
    return re.sub(r"[^\w\- ]", "", name).strip()
