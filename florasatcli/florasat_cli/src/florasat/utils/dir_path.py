import argparse
import pathlib


def dir_path(string):
    path = pathlib.Path(string)
    if path.is_dir():
        return path
    else:
        raise argparse.ArgumentTypeError(f"'{path}' is not a valid directory path!")

