from dataclasses import dataclass
import os
from pathlib import Path
import time
import pandas as pd
from typing import Any, List, Tuple
from plotly.subplots import make_subplots
import plotly.graph_objects as go
import plotly.express as px
import tomli

config_name = ".florasat_config.toml"

pd.options.plotting.backend = "plotly"


@dataclass
class Config:
    algorithms: List[str]
    cstl: List[str]
    sim_name: List[str]
    runs: int
    florasat_results_path: Path
    routes_path: Path
    satellites_path: Path
    results_path: Path


def load_simulation_paths(
    config: Config, cstl: str, sim_name: str, alg: str, run: int
) -> Tuple[Path, Path, Path]:
    path_directory = (
        config.florasat_results_path.joinpath(alg).joinpath(cstl).joinpath(sim_name)
    )
    print("\t", "Load run", run, "from", path_directory)
    content_dir: List[str] = os.listdir(path_directory)

    stats_file_name = f"{run}.stats.csv"
    stats_file_path = path_directory.joinpath(stats_file_name)

    routes_file_name = f"{run}.routes.csv"
    routes_file_path = path_directory.joinpath(routes_file_name)

    sats_file_name = f"{run}.sats.csv"
    sats_file_path = path_directory.joinpath(sats_file_name)

    if (
        not stats_file_name in content_dir
        or not routes_file_name in content_dir
        or not sats_file_name in content_dir
    ):
        raise FileNotFoundError(
            f"Could not find {stats_file_name} or {routes_file_name} or {sats_file_name} in {path_directory}"
        )

    return (stats_file_path, routes_file_path, sats_file_path)


def get_route_dump_file(
    config: Config, cstl: str, sim_name: str, alg: str, run: int
) -> Tuple[Path, Path]:
    path = config.routes_path.joinpath(alg).joinpath(cstl).joinpath(sim_name)
    file_path = path.joinpath(f"{run}.routes.msgpack")
    return (path, file_path)


def get_sats_dump_file(
    config: Config, cstl: str, sim_name: str, alg: str, run: int
) -> Tuple[Path, Path]:
    path = config.satellites_path.joinpath(alg).joinpath(cstl).joinpath(sim_name)
    file_path = path.joinpath(f"{run}.sats.msgpack")
    return (path, file_path)


def load_stats(
    config: Config, cstl: str, sim_name: str, alg: str
) -> List[pd.DataFrame]:
    dfs: List[pd.DataFrame] = []
    for run in range(0, config.runs):
        (stats_fp, _, _) = load_simulation_paths(config, cstl, sim_name, alg, run)
        print("\t\t", "Read:", stats_fp)
        dfs.append(pd.read_csv(stats_fp))
    return dfs


def fix_loading_mathjax():
    # garbage graph
    fig = px.scatter(x=[0, 1, 2, 3, 4], y=[0, 1, 4, 9, 16])
    fig.write_image("/tmp/fix-mathjax.pdf", engine="kaleido")
    time.sleep(1)


def apply_default(fig, size=22, width=600, height=400, mt=10):
    fix_loading_mathjax()
    fig.update_layout(
        margin=dict(l=10, r=10, b=10, t=mt), font=dict(size=size), width=width, height=height
    )


def plot_cdf(
    dfs: List[Tuple[str, pd.DataFrame]],
    col: str,
    file_path: Path,
    x_name: str = "",
    mean: bool = False,
    mean_unit: str = "",
    percent_1_low: bool = False,
    percent_01_low: bool = False,
):
    print("\t", "Create plot...")
    fig = make_subplots()
    colors = ["#636efa", "#ef553b", "#2ca02c", "#00cc96"]
    positions = ["top right", "top left", "bottom left", "bottom right"]
    for name, df in dfs:
        color = colors.pop(0)
        position = positions.pop(0)

        mean_val = df[col].mean().round(2)  # type: ignore

        percent_1 = df[col].quantile(q=0.99)
        if df[df[col] > percent_1][col] is not None:
            percent_1_mean = df[df[col] > percent_1][col].mean()
        percent_01 = df[col].quantile(q=0.999, interpolation="linear")
        if df[df[col] > percent_01][col] is not None:
            percent_01_mean = df[df[col] > percent_01][col].mean()

        print(name)
        print("mean:", mean_val)  # type: ignore

        print("1%:", percent_1, "mean:", percent_1_mean)  # type: ignore
        
        print("0.1%:", percent_01, "mean:", percent_01_mean)  # type: ignore

        stats_df = (
            df.groupby(col)[col]
            .agg("count")
            .pipe(pd.DataFrame)
            .rename(columns={col: "frequency"})
        )
        # if not continuous:
        #     stats_df = stats_df.reindex(list(range(0, max_val + 1)), fill_value=0)

        stats_df["pdf"] = stats_df["frequency"] / sum(stats_df["frequency"])

        stats_df["cdf"] = stats_df["pdf"].cumsum()
        stats_df = stats_df.reset_index()

        fig.add_trace(
            go.Scatter(
                name=name,
                x=stats_df[col].values,
                y=stats_df["cdf"],
                mode="markers+lines",
                line=dict(color=color),
            )
        )
        if mean:
            fig.add_vline(
                mean_val,
                line_dash="dot",
                line_width=1,
                line_color=color,
                annotation_text=f"{mean_val}{mean_unit}",
                annotation_font_color=color,
                annotation_font_size=20,
                annotation_position=position,
            )
    fig.update_traces(line=dict(width=2), marker=dict(size=2))
    fig.update_layout(
        legend=dict(yanchor="bottom", y=0.05, xanchor="right", x=0.95),
    )
    apply_default(fig)
    fig.update_xaxes(title_text=x_name, tickmode="auto", nticks=10)
    fig.update_yaxes(title_text="CDF", dtick=0.1)
    print("\t", "Write plot to file...")
    fig.write_image(file_path, engine="kaleido")


def load_config(path_raw: str | None = None) -> dict[str, Any]:
    xdg_config_home = os.environ.get("XDG_CONFIG_HOME")
    home = os.environ.get("HOME")
    if path_raw is not None:
        print(f"Try to load config from {path_raw}...")
        path = Path(path_raw)
        if not (path.exists() and path.is_file()):
            raise RuntimeError(
                f"Could not load config: {path} does not exists or is no file."
            )
        with open(path, "rb") as file:
            config = tomli.load(file)
            return config
    elif xdg_config_home is not None:
        print("Try to load config from $XDG_CONFIG_HOME...")
        path = Path(xdg_config_home).joinpath(config_name)
        if not (path.exists() and path.is_file()):
            raise RuntimeError(
                f"Could not load config: {path} does not exists or is no file."
            )
        with open(path, "rb") as file:
            config = tomli.load(file)
            return config
    elif home is not None:
        print("Try to load config from $HOME...")
        path = Path(home).joinpath(config_name)
        if not (path.exists() and path.is_file()):
            raise RuntimeError(
                f"Could not load config: {path} does not exists or is no file."
            )
        with open(path, "rb") as file:
            config = tomli.load(file)
            return config
    else:
        raise RuntimeError("Neither $XDG_CONFIG_HOME or $HOME are set")
