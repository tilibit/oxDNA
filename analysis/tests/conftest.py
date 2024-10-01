import logging
from pathlib import Path
from shutil import copy
from typing import Dict, Any

import pytest


MINI_TRAJ = "minitraj.dat"
ALIGN_TRAJ = "aligntraj.dat"
INDEX = "index.txt"
PAIRS = "pairs.txt"

RNA_TILE_TOP = "rna_tile.top"
INPUT_RNA = "input_rna"
SEQUENCE_DEPS_RNA = "rna_sequence_dependent_parameters.txt"


def get_test_resource(file_name: str) -> Path:
    base_dir = Path.cwd()
    return base_dir / "tests" / file_name


@pytest.fixture(scope="session")
def align_input(tmp_path_factory) -> Dict[str, Any]:

    dest_dir = tmp_path_factory.mktemp("test_cli")
    data = {}

    mini_traj = get_test_resource(f"{MINI_TRAJ}")
    data["traj"] = copy(mini_traj, dest_dir)
    data["outfile"] = dest_dir / ALIGN_TRAJ
    data["indexes"] = []
    data["ref_conf"] = None
    data["center"] = True
    data["ncpus"] = 1

    test_result = get_test_resource(f"{ALIGN_TRAJ}")
    data["test"] = test_result

    return data
