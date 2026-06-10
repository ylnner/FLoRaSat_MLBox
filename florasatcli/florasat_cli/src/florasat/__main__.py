import os
import pathlib
import argparse
import sys
import tomli


from florasat.cli import generate_parser, run_command


def run():
    # create CLI
    parser = generate_parser()
    # parse arguments
    args = parser.parse_args()
    # run command
    try:
        run_command(args)
    except Exception as e:
        print("X Failed:", e)
        raise e


def main():
    run()


if __name__ == "__main__":
    main()
