#!/usr/bin/env python
# Created by: Erik Poppleton
# Date: 6/29/18
# Python2
# Converts the forces file printed out by tiamat2oxdna to a pairs file containing all designed H-bonds
import argparse
import logging
import os
from typing import List
from typing import Tuple

logger = logging.getLogger(__name__)


def cli_parser(prog="forces2pairs"):
    parser = argparse.ArgumentParser(
        prog=prog, description="Convert an external force file to a list of particle pairs"
    )
    parser.add_argument("force_file", type=str, nargs=1, help="The force file to generate pairs from")
    parser.add_argument("-o", "--output", type=str, nargs=1, help="name of the file to write the pair list to")
    parser.add_argument(
        "-q",
        "--quiet",
        metavar="quiet",
        dest="quiet",
        action="store_const",
        const=True,
        default=False,
        help="Don't print 'INFO' messages to stderr",
    )
    return parser


def forces2pairs(force_file: str) -> List[Tuple]:
    """
    Returns a list of tuples containig the pairing information for a structure

    Parameters:
        force_file (str): path to the force file

    Returns:
        List[Tuple]: A list of tuples where each tuple corresponds to a pair found in the force file.
    """
    pairs = []
    a = b = -1
    with open(force_file) as f:
        for line in f.readlines():
            line = line.strip()
            if line.startswith("particle"):
                a = int(float(line.split("=")[1].strip()))
            if "ref_particle" in line:
                b = int(float(line.split("=")[1].strip()))
            if "}" in line:
                if a < b:
                    pairs.append((a, b))
                a = b = -1

    return pairs


def main():
    parser = cli_parser(os.path.basename(__file__))
    args = parser.parse_args()

    if args.quiet:
        logger.setLevel(logging.CRITICAL)
    infile = args.force_file[0]

    try:
        out = args.output[0]
    except:
        logger.info("No outfile provided, defaulting to pairs.txt")
        out = "pairs.txt"

    pairs = forces2pairs(infile)

    with open(out, "w+") as f:
        for p in pairs:
            f.write(f"{p[0]} {p[1]}\n")

    logger.info(f"pairing information written to {out}")


if __name__ == "__main__":
    main()
