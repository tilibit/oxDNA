import argparse
from json import dump
from json import load
from os import path
from os import remove
from typing import List

import matplotlib.pyplot as plt
import numpy as np
from matplotlib import animation
from matplotlib.colors import ListedColormap
from oxDNA_analysis_tools.config import check
from oxDNA_analysis_tools.UTILS.data_structures import TopInfo
from oxDNA_analysis_tools.UTILS.data_structures import TrajInfo
from oxDNA_analysis_tools.UTILS.logger import log
from oxDNA_analysis_tools.UTILS.logger import logger_settings
from oxDNA_analysis_tools.UTILS.RyeReader import conf_to_str
from oxDNA_analysis_tools.UTILS.RyeReader import describe
from oxDNA_analysis_tools.UTILS.RyeReader import get_confs
from oxDNA_analysis_tools.UTILS.RyeReader import linear_read
from oxDNA_analysis_tools.UTILS.RyeReader import write_conf
from sklearn.cluster import DBSCAN


def split_trajectory(traj_info, top_info, labs) -> None:
    """
    Splits the trajectory into the clustered trajectories.  Names each trajectory cluster_<n>.dat

    Parameters:
        traj_info (TrajInfo): Metadata on the trajectory file
        top_info (TopInfo): Metadata on the topology file
        labs (numpy.array): The cluster each configuration belongs to.
    """

    slabs = set(labs)

    for cluster in slabs:
        # Clear old trajectory files
        try:
            remove("cluster_" + str(cluster) + ".dat")
        except:
            pass

    log("splitting trajectory...")

    fnames = ["cluster_" + str(cluster) + ".dat" for cluster in slabs]
    files = [open(f, "w+") for f in fnames]
    i = 0

    for chunk in linear_read(traj_info, top_info):
        for conf in chunk:
            files[labs[i]].write(conf_to_str(conf, include_vel=traj_info.incl_v))
            i += 1

    for f in files:
        f.close()

    log(f"Wrote trajectory files: {fnames}")

    return


def find_element(n, x, array):
    """
    Finds the id of the nth time element x appears in an array.
    """
    c = 0
    for i, j in enumerate(array):
        if j == x:
            if c == n:
                return i
            c += 1
    return -1


def get_centroid(
    points: np.ndarray, metric_name: str, labs: np.ndarray, traj_info: TrajInfo, top_info: TopInfo
) -> List[int]:
    """
    Takes the output from DBSCAN and finds the centroid of each cluster.

    Parameters:
        points (numpy.array): The points fed to the clustering algorithm.
        metric_name (str): The type of data the points represent ('euclidean' or 'precomputed').
        labs (numpy.array): The cluster each point belongs to.
        traj_info (TrajInfo): Trajectory metadata.
        top_info (TopInfo): Topology metadata.

    Returns:
        List[int]: The configuration ID for the centroid of each cluster
    """

    log("Finding cluster centroids...")
    if metric_name == "euclidean":
        points = points[np.newaxis, :, :] - points[:, np.newaxis, :]
        points = np.sum(points**2, axis=2)  # squared distance is still correct distance

    cids = []
    for cluster in set(labs):
        to_extract = labs == cluster
        masked = points[np.ix_(to_extract, to_extract)]
        in_cluster_id = np.sum(masked, axis=1).argmin()

        centroid_id = find_element(in_cluster_id, cluster, labs)
        cids.append(centroid_id)

        centroid = get_confs(top_info, traj_info, centroid_id, 1)[0]
        fname = "centroid_" + str(cluster) + ".dat"
        write_conf(fname, centroid, include_vel=traj_info.incl_v)
        log(f"Wrote centroid file {fname}")

    return cids


def make_plot(op, labels, centroid_ids, interactive_plot, op_names):
    # Prepping a plot of the first 3 dimensions of the provided op
    dimensions = []
    x = []
    y = []
    dimensions.append(x)
    dimensions.append(y)
    if len(op_names) == 0:
        op_names = ["OP0", "OP1", "OP2"]

    # if the op is 1-dimensional add a time dimension
    add_time = False
    if op.shape[1] == 1:
        add_time = True
        op = np.hstack((op, np.arange(op.shape[0]).reshape(op.shape[0], 1)))

    if op.shape[1] > 2:
        z = []
        dimensions.append(z)

    for i in op:
        for j, dim in enumerate(dimensions):
            dim.append(i[j])

    dimensions = np.array(dimensions)

    n_clusters = len(set(labels)) - (1 if -1 in labels else 0)

    log("Making cluster plot using first three OPs...")
    if len(dimensions) == 3:
        fig = plt.figure()
        ax = fig.add_subplot(111, projection="3d")
    else:
        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)

    ax.set_xlabel(op_names[0])
    ax.set_ylabel(op_names[1])

    if len(dimensions) == 3:
        ax.set_zlabel(op_names[2])

        # to make a video showing a rotating plot
        plot_file = "animated.mp4"

        def init():
            nonlocal labels, dimensions, n_clusters, centroid_ids, ax
            a = ax.scatter(x, y, z, s=2, alpha=0.4, c=labels, cmap=plt.get_cmap("tab10", n_clusters + 1))
            cen = ax.scatter(
                dimensions[0][centroid_ids],
                dimensions[1][centroid_ids],
                dimensions[2][centroid_ids],
                s=1.5,
                c=[0 for _ in centroid_ids],
                cmap=ListedColormap(["black"]),
            )
            fig.colorbar(a, ax=ax, ticks=list(set(labels)))
            handles, _ = cen.legend_elements(prop="colors", num=1)
            l = ax.legend(handles, ["Centroids"])
            return [fig]

        def animate(i):
            ax.view_init(elev=10.0, azim=i)
            return [fig]

        if not interactive_plot:
            try:
                anim = animation.FuncAnimation(fig, animate, init_func=init, frames=range(360), interval=20, blit=True)
                anim.save(plot_file, fps=30, extra_args=["-vcodec", "libx264"])
                log(f"Saved cluster plot to {plot_file}")
            except:
                print("WARNING: ffmpeg not found, cannot make animated plot, opening interactivley instead")
                f = init()
                plt.show()
        else:
            f = init()
            plt.show()

    else:
        plot_file = "cluster_plot.png"
        if add_time:
            a = ax.scatter(
                dimensions[1],
                dimensions[0],
                s=2,
                alpha=0.4,
                c=labels,
                cmap=plt.get_cmap("tab10", n_clusters + 1),
                vmin=min(labels) - 0.5,
                vmax=max(labels) + 0.5,
            )
            cen = ax.scatter(
                dimensions[1][centroid_ids],
                dimensions[0][centroid_ids],
                s=1.5,
                c=[0 for _ in centroid_ids],
                cmap=ListedColormap(["black"]),
            )
            ax.set_xlabel("conf id")
            ax.set_ylabel("OP0")
        else:
            a = ax.scatter(
                x,
                y,
                s=2,
                alpha=0.4,
                c=labels,
                cmap=plt.get_cmap("tab10", n_clusters + 1),
                vmin=min(labels) - 0.5,
                vmax=max(labels) + 0.5,
            )
            cen = ax.scatter(
                dimensions[0][centroid_ids],
                dimensions[1][centroid_ids],
                s=1.5,
                c=[0 for _ in centroid_ids],
                cmap=ListedColormap(["black"]),
            )

        b = fig.colorbar(a, ax=ax, ticks=list(set(labels)))
        handles, _ = cen.legend_elements(prop="colors", num=1)
        l = ax.legend(handles, ["Centroids"])
        ax.add_artist(l)
        if not interactive_plot:
            plt.tight_layout()
            plt.savefig(plot_file)
            log(f"Saved cluster plot to {plot_file}")
        else:
            plt.show()

    return


def perform_DBSCAN(
    traj_info: TrajInfo,
    top_info: TopInfo,
    op: np.ndarray,
    metric: str,
    eps: float,
    min_samples: int,
    op_names: List[str] = [],
    no_traj: bool = False,
    interactive_plot: bool = False,
    min_clusters: int = -1,
) -> np.ndarray:
    """
    Use the DBSCAN algorithm to identify clusters of configurations based on a given order parameter.

    Parameters:
        traj_info (TrajInfo): Information about the trajectory
        top_info (TopInfo): Information about the topology
        op (np.ndarray): The order parameter(s) to use (shape = n_confs x n_op for metric=euclidean, n_confs x n_confs for metric=precomputed)
        metric (str): Either 'euclidean' or 'precomputed' for whether the distance needs to be calculated
        eps (float): The maximum distance between two points to be considered in the same neighborhood
        min_samples (int): The minimum number of points to be considered a neighborhood
        no_traj (bool): If True, skip splitting the trajectory (these are slow)
        interactive_plot (bool): If True, show plot interactivley instead of saving as an animation
        min_clusters (int): If less than min_clusters are found, return and don't do further calculations

    Returns:
        np.ndarray: The label for each configuration
    """

    check(["python", "sklearn", "matplotlib"])
    if traj_info.nconfs != len(op):
        raise RuntimeError(
            f"Length of trajectory ({traj_info.nconfs}) is not equal to length of order parameter array {len(op)}"
        )

    # dump the input as a json file so you can iterate on eps and min_samples
    dump_file = "cluster_data.json"
    log(f"Serializing input data to {dump_file}")
    log(f"Run  `oat clustering {dump_file} -e<eps> -m<min_samples>`  to adjust clustering parameters")
    out = {"data": op.tolist(), "traj": traj_info.path, "metric": metric}
    dump(out, open(dump_file, "w+"))

    log("Running DBSCAN...")

    # DBSCAN parameters:
    # eps: the pairwise distance that configurations below are considered neighbors
    # min_samples: The smallest number of neighboring configurations required to start a cluster
    # metric: If the matrix fed in are points in n-dimensional space, then the metric needs to be "euclidean".
    #        If the matrix is already a square distance matrix, the metrix needs to be "precomputed".
    # the eps and min_samples need to be determined for each input based on the values of the input data
    # If you're making your own multidimensional data, you probably want to normalize your data first.
    log(f"Current values: eps={eps}, min_samples={min_samples}")
    db = DBSCAN(eps=eps, min_samples=min_samples, metric=metric).fit(op)
    labels = db.labels_

    n_clusters_ = len(set(labels)) - (1 if -1 in labels else 0)
    print("Number of clusters:", n_clusters_)

    # How many are in each cluster?
    print("cluster\tmembers")
    slabs = set(labels)
    for cluster in slabs:
        in_cluster = list(labels).count(cluster)
        print(f"{cluster}\t{in_cluster}")

    # If the hyperparameters don't split the data well, end the run before the long stuff.
    if n_clusters_ < min_clusters:
        log("Did not find the minimum number of clusters requested, returning early")
        return labels

    # Split the trajectory into cluster trajectories
    if not no_traj:
        split_trajectory(traj_info, top_info, labels)

    # Get the centroid id from each cluster
    centroid_ids = get_centroid(op, metric, labels, traj_info, top_info)

    # Make a plot showing the clusters
    make_plot(op, labels, centroid_ids, interactive_plot, op_names)

    log(f"Run  `oat clustering {dump_file} -e<eps> -m<min_samples>`  to adjust clustering parameters")

    return labels


def cli_parser(prog="clustering.py"):
    parser = argparse.ArgumentParser(prog=prog, description="Calculates clusters based on provided order parameters.")
    parser.add_argument("serialized_data", type=str, nargs=1, help="The json-formatted input file")
    parser.add_argument(
        "-e",
        "--eps",
        type=float,
        nargs=1,
        help="The epsilon parameter for DBSCAN (maximum distance to be considered a 'neighbor')",
    )
    parser.add_argument(
        "-m",
        "--min_samples",
        type=int,
        nargs=1,
        help="The min_samples parameter for DBSCAN (number of neighbors which define a point as a central point in a cluster)",
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


def main():
    parser = cli_parser(path.basename(__file__))
    args = parser.parse_args()

    logger_settings.set_quiet(args.quiet)
    data_file = args.serialized_data[0]
    if args.eps:
        eps = args.eps[0]
    else:
        eps = 12
    if args.min_samples:
        min_samples = args.min_samples[0]
    else:
        min_samples = 8

    # load a previously serialized dataset
    with open(data_file) as f:
        data = load(f)
    points = np.array(data["data"])
    top_info, traj_info = describe(None, data["traj"])
    metric = data["metric"]
    labels = perform_DBSCAN(traj_info, top_info, points, metric, eps, min_samples)


if __name__ == "__main__":
    main()
