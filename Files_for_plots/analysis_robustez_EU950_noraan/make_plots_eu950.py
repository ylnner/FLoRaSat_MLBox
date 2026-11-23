"""Figures of the EU_950 closed loop with the no-raan model (v3_sequential_noraan, 7 inputs, 10 thresholds, coin q = 0.5 / 0.7).
Reads ONLY data/per_run_eu950.csv (extract_eu950.py). Same figures and style as ../analysis_robustez_EU670/make_plots_eu670.py,
with one change: N = 60, 100, 300, 500, 1000 (panels in a grid of up to 3 columns). Only the MLP WITHOUT coin is drawn (the coin
results are in data/vs_baselines.csv). Every threshold is drawn even when its point coincides with others or with FV_1;
coinciding thresholds share one label (e.g. "0.1–0.8").

Methods: REP_1 ... REP_5, FV_1 / FV_2 (first k visible slots), RV_1 / RV_2 (visible slot sent with p = 0.5 until k copies),
KD / NoKD (MLP, stopTarget 0.01, thresholds 0.1 ... 0.95) without coin.
Every point is the mean over the 50 runs of its configuration (5 RAAN x 10 seeds), with ± 1 standard error:
    SE = s / sqrt(n), s = sample standard deviation over the n = 50 runs (ddof = 1)

Writes in figures/ (PNG at 300 dpi and vector PDF):
    pareto_unique.png              x = mean total tx, y = mean unique messages, per N: REP_1..5 curve, FV_k / RV_k markers,
                                   KD and NoKD curves (one point per threshold, ± 1 SE)
    pareto_unique_zoom.png         zoom on the 1-copy zone (REP_1, FV_1, RV_1 and the ML curves), threshold written next to
                                   every ML point; no error bars (paired SE in delta_unique_paired)
    tx_destination_fractions.png   destination of every transmission, horizontal stacked bars per N:
                                       first copy received        = unique
                                       extra copies received      = recv - unique
                                       below sensitivity          = rcvBelowSensitivity of the satellite receiver
                                       collision / other failure  = tx - recv - below sensitivity
                                   rows: REP_1..5, FV_1, FV_2, RV_1, RV_2, and KD / NoKD at the threshold with the most
                                   unique messages; ± 1 SE of unique and of tx
    delta_unique_paired.png        paired difference in unique messages (%), x = threshold, per N:
                                       row 1: ML - FV_1      row 2: ML - RV_1      row 3: KD - NoKD (same threshold)
                                   ± 1 paired SE: runs paired by (RAAN, run), d_i = a_i - b_i, SE = s_d / sqrt(n)

Usage:
    python3 make_plots_eu950.py
"""
import math
import os

import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
RUNS_CSV = os.path.join(HERE, "data", "per_run_eu950.csv")
OUT = os.path.join(HERE, "figures")
SCENARIO = "EU_950"
SCENARIO_NOTE = ''

NS = [60, 100, 300, 500, 1000]
ARMS = ["KD", "NoKD"]
COINS = [1.0]                           # only the MLP without coin
DESTINATION_COINS = [1.0]
ZOOM_COINS = [1.0]
REPS = ["REP_1", "REP_2", "REP_3", "REP_4", "REP_5"]
VISIBLE_BASELINES = ["FV_1", "FV_2", "RV_1", "RV_2"]
CONFIG_KEY = ["N", "method", "threshold", "coin_prob"]      # -1 = not applicable for that method
RUN_KEY = ["raan", "run"]
THRESHOLD_TICKS = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
MAX_COLS = 3
DPI = 300

# Colours (same palette as ../analysis_robustez_EU670/make_plots_eu670.py)
KD = "#2a78d6"
NOKD = "#eb6834"
INK = "#1f1f1e"
MUTED = "#6b6a64"
GRID = "#e4e3dd"
SURF = "#fcfcfb"
BAR_FIRST = "#2f6f4f"
BAR_EXTRA = "#8fbf9f"
BAR_FAIL = "#d9a441"
BAR_BELOW = "#b5523b"
FV_COLOR = "#7b4fb8"
RV_COLOR = "#a0652a"
ARM_COLOR = {"KD": KD, "NoKD": NOKD}
ARM_LABEL = {"KD": "KD", "NoKD": "noKD"}
COIN_STYLE = {1.0: dict(ls="-", lw=1.8, alpha=1.0), 0.7: dict(ls="--", lw=1.3, alpha=0.85),
              0.5: dict(ls=":", lw=1.3, alpha=0.85)}
COIN_LABEL = {1.0: "no coin", 0.7: "coin q = 0.7", 0.5: "coin q = 0.5"}
VISIBLE_COLOR = {"FV_1": FV_COLOR, "FV_2": FV_COLOR, "RV_1": RV_COLOR, "RV_2": RV_COLOR}
VISIBLE_MARKER = {"FV_1": "D", "FV_2": "s", "RV_1": "D", "RV_2": "s"}      # diamond = 1 copy, square = 2 copies
VISIBLE_LABEL = {"FV_1": "FV_1 (first visible slot)", "FV_2": "FV_2 (first 2 visible slots)",
                 "RV_1": "RV_1 (random visible slot, 1 copy)", "RV_2": "RV_2 (random visible slot, 2 copies)"}

plt.rcParams.update({
    "font.size": 10, "axes.edgecolor": MUTED, "axes.labelcolor": INK, "xtick.color": MUTED, "ytick.color": MUTED,
    "axes.spines.top": False, "axes.spines.right": False, "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
    "figure.facecolor": SURF, "axes.facecolor": SURF, "savefig.facecolor": SURF, "legend.frameon": False,
})


def standard_error(values):
    """SE = s / sqrt(n), with the sample standard deviation (ddof = 1)."""
    values = np.asarray(values, dtype=float)
    return values.std(ddof=1) / np.sqrt(len(values))


def ml_method(arm, coin):
    if coin == 1.0:
        return f"{arm}_v3"
    return f"{arm}_coin"


def load_runs():
    runs = pd.read_csv(RUNS_CSV)
    runs["threshold"] = runs["threshold"].round(2)
    runs = runs.rename(columns={"ml_coin_prob": "coin_prob"})
    return runs


def load_means(runs):
    """Mean of tx, unique, recv and below sensitivity, and SE of tx and unique, per configuration."""
    rows = []
    for key, group in runs.groupby(CONFIG_KEY):
        row = dict(zip(CONFIG_KEY, key))
        row["n_runs"] = len(group)
        for metric in ["tx", "unique", "recv"]:
            row[metric] = group[metric].mean()
        row["below"] = group["sat_below_sensitivity"].mean()
        row["tx_se"] = standard_error(group["tx"])
        row["unique_se"] = standard_error(group["unique"])
        rows.append(row)
    means = pd.DataFrame(rows)
    assert (means["n_runs"] == 50).all()
    means["extra"] = means["recv"] - means["unique"]
    return means


def panel_grid(n_panels, width, height):
    """Grid of up to MAX_COLS columns; the unused axes are hidden."""
    ncols = min(MAX_COLS, n_panels)
    nrows = math.ceil(n_panels / ncols)
    fig, axs = plt.subplots(nrows, ncols, figsize=(width * ncols, height * nrows), squeeze=False)
    axs = axs.ravel()
    for ax in axs[n_panels:]:
        ax.set_visible(False)
    return fig, axs[:n_panels], nrows


def ml_curve(sn, arm, coin):
    sel = (sn["method"] == ml_method(arm, coin)) & np.isclose(sn["coin_prob"], coin)
    return sn[sel].sort_values("threshold")


LABEL_MERGE_REL = 0.0015     # consecutive thresholds closer than 0.15 % in tx and in unique share one label


def threshold_labels(curve):
    """One label per group of consecutive thresholds whose points (almost) coincide, e.g. '0.1–0.8'.
    The label goes on the first point of the group."""
    groups = []
    for _, row in curve.sort_values("threshold").iterrows():
        if groups:
            first = groups[-1][0]
            close_tx = abs(row["tx"] - first["tx"]) <= LABEL_MERGE_REL * first["tx"]
            close_unique = abs(row["unique"] - first["unique"]) <= LABEL_MERGE_REL * first["unique"]
            if close_tx and close_unique:
                groups[-1].append(row)
                continue
        groups.append([row])
    labels = []
    for group in groups:
        point = (group[0]["tx"], group[0]["unique"])
        if len(group) == 1:
            labels.append((point, f"{group[0]['threshold']:g}"))
        else:
            labels.append((point, f"{group[0]['threshold']:g}–{group[-1]['threshold']:g}"))
    return labels


def best_row(rows):
    return rows.loc[rows["unique"].idxmax()]


def draw_se(ax, x, y, x_se, y_se, color):
    ax.errorbar(x, y, xerr=x_se, yerr=y_se, fmt="none", ecolor=color, elinewidth=0.9, capsize=2, alpha=0.8)


def fmt(value, decimals=0):
    return f"{value:,.{decimals}f}"


def save(fig, name):
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, name + ".png"), dpi=DPI, bbox_inches="tight")
    fig.savefig(os.path.join(OUT, name + ".pdf"), bbox_inches="tight")
    plt.close(fig)


def y_axis_to_ticks(ax):
    """Extend the y axis so that it starts and ends on a tick with its number."""
    low, high = ax.get_ylim()
    ticks = ax.yaxis.get_major_locator().tick_values(low, high)
    ax.set_ylim(max(t for t in ticks if t <= low), min(t for t in ticks if t >= high))


def draw_baselines(ax, sn, with_reps, with_se=True):
    if with_reps:
        rep = sn[sn["method"].isin(REPS)].sort_values("tx")
        ax.plot(rep["tx"], rep["unique"], color=INK, marker="s", ms=4, mec=SURF, lw=1.6, label="REP_1…5")
        draw_se(ax, rep["tx"], rep["unique"], rep["tx_se"], rep["unique_se"], INK)
        for _, row in rep.iterrows():
            ax.annotate(row["method"][-1] + "×", (row["tx"], row["unique"]), xytext=(3, -10),
                        textcoords="offset points", fontsize=7, color=INK)
        visible = VISIBLE_BASELINES
    else:
        rep = sn[sn["method"] == "REP_1"]
        ax.plot(rep["tx"], rep["unique"], color=INK, marker="s", ms=7, ls="none", label="REP_1")
        visible = ["FV_1", "RV_1"]
    for method in visible:
        row = sn[sn["method"] == method]
        ax.plot(row["tx"], row["unique"], color=VISIBLE_COLOR[method], marker=VISIBLE_MARKER[method], ms=8, ls="none",
                mec=SURF, label=VISIBLE_LABEL[method], zorder=5)
        if with_se:
            draw_se(ax, row["tx"], row["unique"], row["tx_se"], row["unique_se"], VISIBLE_COLOR[method])


def figure_footer(fig, nrows, text):
    fig.text(0.01, -0.035 - 0.06 / nrows, text, fontsize=8.5, color=MUTED, ha="left", va="top")


# ---------------------------------------------------------------- 1. Pareto frontier
def fig_pareto_unique(means):
    fig, axs, nrows = panel_grid(len(NS), 5.7, 5.4)
    for ax, n in zip(axs, NS):
        sn = means[means["N"] == n]
        draw_baselines(ax, sn, with_reps=True)
        for arm in ARMS:
            for coin in COINS:
                curve = ml_curve(sn, arm, coin)
                ax.plot(curve["tx"], curve["unique"], color=ARM_COLOR[arm], marker="o", ms=3.5, mec=ARM_COLOR[arm],
                        label=ARM_LABEL[arm], zorder=6, **COIN_STYLE[coin])     # above the FV_1 marker
                if coin == 1.0:
                    draw_se(ax, curve["tx"], curve["unique"], curve["tx_se"], curve["unique_se"], ARM_COLOR[arm])
        ax.set_title(f"N = {n}", loc="left", color=INK)
        ax.set_xlim(left=0)
        ax.set_ylim(bottom=0)
        y_axis_to_ticks(ax)
        ax.set_xlabel("Total transmissions")
        ax.set_ylabel("Unique messages received")
    handles, labels = axs[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=6, fontsize=8.5, bbox_to_anchor=(0.5, 0.0))
    fig.suptitle(f"{SCENARIO}, no-raan model: Pareto frontier transmissions – unique messages vs REP, FV and RV baselines",
                 x=0.01, ha="left", color=INK)
    figure_footer(fig, nrows,
                  "Mean ± 1 standard error (SE = s/√n, n = 50 runs: 5 RAAN × 10 seeds). "
                  "ML: MLP v3 without raan (N = 300 model), stopTarget 0.01;\neach curve goes from threshold 0.1 (right) "
                  "to 0.95 (left). Higher and further left = more messages with fewer transmissions." + SCENARIO_NOTE)
    save(fig, "pareto_unique")


def fig_pareto_unique_zoom(means):
    fig, axs, nrows = panel_grid(len(NS), 5.7, 5.6)
    for ax, n in zip(axs, NS):
        sn = means[means["N"] == n]
        draw_baselines(ax, sn, with_reps=False, with_se=False)
        for arm in ARMS:
            for coin in ZOOM_COINS:
                curve = ml_curve(sn, arm, coin)
                ax.plot(curve["tx"], curve["unique"], color=ARM_COLOR[arm], marker="o", ms=4, mec=ARM_COLOR[arm],
                        label=ARM_LABEL[arm], zorder=6, **COIN_STYLE[coin])     # above the FV_1 marker
                if arm == "KD":
                    offset = (4, 4)
                else:
                    offset = (4, -11)
                for (x, y), label in threshold_labels(curve):
                    ax.annotate(label, (x, y), xytext=offset, textcoords="offset points", fontsize=7,
                                color=ARM_COLOR[arm])
        # zoom: the 1-copy baselines and the ML curves drawn here, with a small margin
        one_copy = sn[sn["method"].isin(["REP_1", "FV_1", "RV_1"])]
        ml_shown = pd.concat([ml_curve(sn, arm, coin) for arm in ARMS for coin in ZOOM_COINS])
        shown = pd.concat([one_copy, ml_shown])
        y_low, y_high = shown["unique"].min(), shown["unique"].max()
        x_low, x_high = shown["tx"].min(), shown["tx"].max()
        ax.set_ylim(y_low - 0.05 * (y_high - y_low), y_high + 0.05 * (y_high - y_low))
        ax.set_xlim(x_low - 0.05 * (x_high - x_low), x_high + 0.08 * (x_high - x_low))
        ax.set_title(f"N = {n}", loc="left", color=INK)
        ax.set_xlabel("Total transmissions")
        ax.set_ylabel("Unique messages received")
    handles, labels = axs[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=5, fontsize=8.5, bbox_to_anchor=(0.5, 0.0))
    fig.suptitle(f"{SCENARIO}, no-raan model: zoom on the 1-copy zone (thresholds 0.1 … 0.95) vs 1-copy baselines",
                 x=0.01, ha="left", color=INK)
    figure_footer(fig, nrows,
                  "Means over n = 50 runs, without error bars: the run-to-run spread hides differences that are "
                  "significant when paired by run (see delta_unique_paired).\nNumbers next to the ML curves = decision "
                  "threshold θ (consecutive thresholds within 0.15 % in tx and unique share one label)." + SCENARIO_NOTE)
    save(fig, "pareto_unique_zoom")


# ---------------------------------------------------------------- 2. Destination of every transmission
def destination_rows(sn):
    labels = []
    rows = []
    for method in REPS + VISIBLE_BASELINES:
        labels.append(method)
        rows.append(sn[sn["method"] == method].iloc[0])
    for coin in DESTINATION_COINS:
        for arm in ARMS:
            row = best_row(ml_curve(sn, arm, coin))
            if coin == 1.0:
                labels.append(f"{ARM_LABEL[arm]} (θ={row['threshold']:g})")  # best threshold
            else:
                labels.append(f"{ARM_LABEL[arm]} q={coin:g} (θ={row['threshold']:g})")
            rows.append(row)
    return labels, rows


def label_color(label):
    if label.startswith("KD"):
        return KD
    elif label.startswith("noKD"):
        return NOKD
    elif label.startswith("FV"):
        return FV_COLOR
    elif label.startswith("RV"):
        return RV_COLOR
    else:
        return INK


def fig_tx_destination_fractions(means):
    fig, axs = plt.subplots(len(NS), 1, figsize=(13, 6.2 * len(NS)), squeeze=False)
    axs = axs.ravel()
    for ax, n in zip(axs, NS):
        labels, rows = destination_rows(means[means["N"] == n])
        y = np.arange(len(rows))[::-1]
        first = np.array([row["unique"] for row in rows])
        extra = np.array([row["extra"] for row in rows])
        below = np.array([row["below"] for row in rows])
        total = np.array([row["tx"] for row in rows])
        collision = total - first - extra - below
        assert (collision >= 0).all()
        unique_se = np.array([row["unique_se"] for row in rows])
        tx_se = np.array([row["tx_se"] for row in rows])

        ax.barh(y, first, color=BAR_FIRST, edgecolor=SURF, lw=2, label="First copy received (= unique message)")
        ax.barh(y, extra, left=first, color=BAR_EXTRA, edgecolor=SURF, lw=2, label="Extra copies received")
        ax.barh(y, below, left=first + extra, color=BAR_BELOW, edgecolor=SURF, lw=2,
                label="Lost: below receiver sensitivity (link)")
        ax.barh(y, collision, left=first + extra + below, color=BAR_FAIL, edgecolor=SURF, lw=2,
                label="Lost: collision / other failure")
        ax.errorbar(first, y, xerr=unique_se, fmt="none", ecolor=INK, elinewidth=1, capsize=3)
        ax.errorbar(total, y, xerr=tx_se, fmt="none", ecolor=INK, elinewidth=1, capsize=3,
                    label="± 1 standard error (unique, tx)")
        for yi, tx, se_tx, unique, se_unique, n_below in zip(y, total, tx_se, first, unique_se, below):
            ax.text(tx + se_tx, yi,
                    f"  {fmt(tx)} ± {fmt(se_tx)} tx · {fmt(unique)} ± {fmt(se_unique, 1)} unique · "
                    f"eff. {unique / tx:.2f} · {100 * n_below / tx:.1f} % below sens.",
                    va="center", fontsize=7.5, color=MUTED)
        ax.set_yticks(y, labels)
        for tick, label in zip(ax.get_yticklabels(), labels):
            tick.set_color(label_color(label))
            if label.startswith(("KD", "noKD")):
                tick.set_fontweight("bold")
        ax.set_xlim(0, 1.75 * total.max())
        ax.grid(axis="y", visible=False)
        ax.set_xlabel("Total transmissions")
        ax.set_title(f"N = {n}", loc="left", color=INK)
    handles, labels = axs[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=3, fontsize=9, bbox_to_anchor=(0.5, 0.0))
    fig.suptitle(f"{SCENARIO}, no-raan model: destination of every transmission (baselines, and KD / noKD at their best "
                 "threshold)", x=0.01, y=1.0 + 0.01 * 3 / len(NS), ha="left", color=INK)
    fig.text(0.01, -0.025 * 3 / len(NS),
             "Mean ± 1 SE (n = 50 runs) of unique messages and of total transmissions. ML rows: threshold with the most unique "
             "messages. eff. = unique / tx.\nBelow sensitivity = rcvBelowSensitivity of the satellite receiver; "
             "collision / other = tx − successful receptions − below sensitivity." + SCENARIO_NOTE,
             fontsize=8.5, color=MUTED, ha="left", va="top")
    save(fig, "tx_destination_fractions")


# ---------------------------------------------------------------- 3. Paired differences
def paired_pct(runs_a, runs_b):
    """Mean and paired SE of (a - b) in unique messages, in % of the mean of b, pairing by (RAAN, run)."""
    pairs = runs_a.merge(runs_b, on=RUN_KEY, suffixes=("_a", "_b"))
    assert len(pairs) == 50
    diff = pairs["unique_a"].to_numpy(dtype=float) - pairs["unique_b"].to_numpy(dtype=float)
    base = pairs["unique_b"].mean()
    return 100 * diff.mean() / base, 100 * standard_error(diff) / base


def select(runs, n, method, threshold=None, coin=None):
    sel = (runs["N"] == n) & (runs["method"] == method)
    if threshold is not None:
        sel &= np.isclose(runs["threshold"], threshold)
    if coin is not None:
        sel &= np.isclose(runs["coin_prob"], coin)
    return runs[sel]


def plot_paired(ax, thresholds, values, color, label, style):
    mean = np.array([v[0] for v in values])
    se = np.array([v[1] for v in values])
    ax.errorbar(thresholds, mean, yerr=se, color=color, marker="o", ms=3.5, elinewidth=0.9, capsize=2, label=label,
                **style)


def fig_delta_unique_paired(runs):
    fig, axs = plt.subplots(3, len(NS), figsize=(5.7 * len(NS), 12.5), squeeze=False)
    thresholds = sorted(runs.loc[runs["method"] == "KD_v3", "threshold"].unique())
    for col, n in enumerate(NS):
        for row, ref in enumerate(["FV_1", "RV_1"]):
            ax = axs[row, col]
            ref_runs = select(runs, n, ref)
            for arm in ARMS:
                for coin in COINS:
                    values = [paired_pct(select(runs, n, ml_method(arm, coin), th, coin), ref_runs) for th in thresholds]
                    plot_paired(ax, thresholds, values, ARM_COLOR[arm], ARM_LABEL[arm], COIN_STYLE[coin])
            ax.axhline(0, color=INK, lw=1)
            if row == 0:
                ax.set_title(f"N = {n}\nML − FV_1 (first visible slot)", loc="left", color=INK)
            else:
                ax.set_title("ML − RV_1", loc="left", color=INK)

        ax = axs[2, col]
        for coin in COINS:
            values = [paired_pct(select(runs, n, ml_method("KD", coin), th, coin),
                                 select(runs, n, ml_method("NoKD", coin), th, coin)) for th in thresholds]
            plot_paired(ax, thresholds, values, INK, "KD − noKD", COIN_STYLE[coin])
        ax.axhline(0, color=MUTED, lw=1, ls="--")
        ax.set_title("KD − noKD (same threshold)", loc="left", color=INK)

    for ax in axs.ravel():
        ax.set_xlim(0.05, 1.0)
        ax.set_xticks(THRESHOLD_TICKS)
        ax.set_xlabel("Decision threshold θ")
        y_axis_to_ticks(ax)
    axs[0, 0].set_ylabel("Δ unique messages (%)\n(ML − FV_1)")
    axs[1, 0].set_ylabel("Δ unique messages (%)\n(ML − RV_1)")
    axs[2, 0].set_ylabel("Δ unique messages (%)\n(KD − noKD)")
    handles, labels = axs[0, 0].get_legend_handles_labels()
    handles_kd, labels_kd = axs[2, 0].get_legend_handles_labels()
    fig.legend(handles + handles_kd, labels + labels_kd, loc="upper center", ncol=3, fontsize=9,
               bbox_to_anchor=(0.5, 0.0))
    fig.suptitle(f"{SCENARIO}, no-raan model: paired difference in unique messages (%) vs FV_1 and RV_1, and KD vs noKD",
                 x=0.01, y=1.01, ha="left", color=INK)
    fig.text(0.01, -0.09,
             "Mean ± 1 paired SE over 50 runs paired by (RAAN, seed), in % of the reference mean. Above 0 = more unique "
             "messages than the reference. Only unique messages: the transmissions of every point are in pareto_unique."
             + SCENARIO_NOTE, fontsize=8.5, color=MUTED, ha="left", va="top")
    save(fig, "delta_unique_paired")


def main():
    os.makedirs(OUT, exist_ok=True)
    runs = load_runs()
    means = load_means(runs)
    print(f"configurations: {len(means)}")
    fig_pareto_unique(means)
    fig_pareto_unique_zoom(means)
    fig_tx_destination_fractions(means)
    fig_delta_unique_paired(runs)
    print(f"written in {OUT}: pareto_unique, pareto_unique_zoom, tx_destination_fractions, delta_unique_paired")


if __name__ == "__main__":
    main()
