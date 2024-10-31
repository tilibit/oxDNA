import argparse
import logging
from os import path
from typing import Any
from typing import Dict
from typing import List

from oxDNA_analysis_tools.external_force_utils.force_reader import read_force_file
from oxDNA_analysis_tools.pairs2db import pairs2db
from oxDNA_analysis_tools.UTILS.RyeReader import strand_describe

logger = logging.getLogger(__name__)


def cli_parser(prog="forces2db"):
    parser = argparse.ArgumentParser(prog=prog, description="Convert a force file to dot-bracket notation")
    parser.add_argument("topology", type=str, help="Topology file for the structure")
    parser.add_argument("force_file", type=str, help="The force file to generate the dot-bracket from")
    parser.add_argument(
        "-o", "--output", type=str, help="If set, print the dot-bracket to a file, otherwise to the screen"
    )
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


def forces2db(n_bases: int, forces: List[Dict[str, Any]]) -> str:
    """
    Convert a force list to dot-bracket notation.

    Parameters:
        n_bases (int): The total number of bases in the structure
        forces (List[Dict[str, Any]]): A list of mutual trap dictionaries (from read_force_file)

    returns:
        str: The forces as a dot-bracket string
    """
    pairs = {f["particle"]: f["ref_particle"] for f in forces}
    db = pairs2db(n_bases, pairs)
    return db


def main():
    parser = cli_parser(path.basename(__file__))
    args = parser.parse_args()

    # run system checks
    if args.quiet:
        logger.setLevel(logging.CRITICAL)
    from oxDNA_analysis_tools.config import check

    check(["python"])

    # Parse CLI input
    top_file = args.topology
    _, elems = strand_describe(top_file)
    seq = "".join([e.btype for e in elems])
    force_file = args.force_file

    forces = read_force_file(force_file)
    db = forces2db(len(elems), forces)

    if args.output:
        out = args.output
    else:
        logger.info("No outfile provided, printing to screen")
        print(seq)
        print(db)
        exit(0)

    with open(out, "w+") as f:
        f.write(seq + "\n")
        f.write(db + "\n")
        logger.info(f"Wrote dot-bracket to file {out}.")


if __name__ == "__main__":
    main()
