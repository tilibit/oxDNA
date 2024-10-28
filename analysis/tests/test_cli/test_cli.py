import time
from pathlib import Path

from oxDNA_analysis_tools.align import align
from oxDNA_analysis_tools.UTILS.logger import log


def test_align(align_input):
    start_time = time.time()
    align(
        traj=align_input["traj"],
        outfile=align_input["outfile"],
        ncpus=align_input["ncpus"],
        indexes=align_input["indexes"],
        ref_conf=align_input["ref_conf"],
        center=align_input["center"],
    )
    log("--- %s seconds ---" % (time.time() - start_time))

    outfile = Path(align_input["outfile"])
    assert outfile.exists()
    assert outfile.is_file()
    assert outfile.stat().st_size > 0
    assert outfile.read_text() == align_input["test"].read_text()
