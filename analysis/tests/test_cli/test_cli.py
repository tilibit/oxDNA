import logging
from pathlib import Path

from oxDNA_analysis_tools.align import align


def test_align(align_input, caplog):
    caplog.set_level(logging.DEBUG, logger="oxDNA_analysis_tools")

    align(
        traj=align_input["traj"],
        outfile=align_input["outfile"],
        ncpus=align_input["ncpus"],
        indexes=align_input["indexes"],
        ref_conf=align_input["ref_conf"],
        center=align_input["center"],
    )

    n_criticals = 0
    for record in caplog.records:
        if record.levelname == "CRITICAL":
            n_criticals += 1

    outfile = Path(align_input["outfile"])
    assert n_criticals == 0
    assert outfile.exists()
    assert outfile.is_file()
    assert outfile.stat().st_size > 0
    assert outfile.read_text() == align_input["test"].read_text()
