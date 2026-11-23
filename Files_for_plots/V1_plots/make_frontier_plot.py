"""Figure 1 (all series) for the NTN competition paper, after Template_plot.jpeg.

Same column width, Times type and rcParams as make_figures.py, but drawing every
series that make_frontier_data.py writes to frontier.csv:

    gated_trf         Transformer, threshold sweep
    gated_bilstm      Bi-LSTM, threshold sweep
    gated_analytical  analytical rule, threshold sweep
    blind             no ML, N = 1..5 repetitions

The matched-budget callout is picked from the data: the Transformer's peak-yield
point, against the cheapest blind point that spends at least as many TX (so the
ratio never flatters the gated policy with a larger budget).

Usage:

    python3 make_frontier_plot.py

Reads frontier.csv from this directory and writes frontier_all.pdf and
frontier_all.png beside it.
"""
import csv, math, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
OUTDIR = HERE          # write the figures next to this script

BLUE, ORANGE, GREEN, GREY = "#2a78d6", "#eb6834", "#1baf7a", "#7f7d76"
INK, INK2, MUTED = "#0b0b0b", "#52514e", "#8a8880"
COL_W = 3.333
MATCH_TOL = 0.15       # gated may spend up to this much LESS than blind; the label states the saving
CALLOUT_SERIES = "gated_trf"

# The analytical score only takes a few levels (P_no_collision is constant per SF),
# so its sweep collapses into three regimes. One threshold from the middle of each
# plateau represents it; the rest of the sweep only duplicates these points.
ANALYTICAL_KEYS = {
    0.1,  # every SF, gated by link margin only
    0.5,  # SF12 excluded
    0.8,  # SF11 excluded too
}

plt.rcParams.update({
    "font.family": "serif",
    "font.serif": ["Times New Roman"],
    "font.size": 8.5,
    "axes.labelsize": 8.5,
    "axes.titlesize": 8.5,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
    "legend.fontsize": 8,
    "axes.linewidth": 0.6,
    "xtick.major.width": 0.6,
    "ytick.major.width": 0.6,
    "xtick.direction": "out",
    "ytick.direction": "out",
    "axes.edgecolor": MUTED,
    "text.color": INK,
    "axes.labelcolor": INK,
    "xtick.color": INK2,
    "ytick.color": INK2,
    "mathtext.fontset": "stix",
    "pdf.fonttype": 42,
})


def dress(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.grid(True, which="major", axis="both", color="#e6e5e0", lw=0.5, zorder=0)
    ax.set_axisbelow(True)


# (csv tag, legend label, colour, marker, marker size, line style, zorder)
SERIES = [
    ("gated_trf",        "Transformer",     BLUE,   "o", 4.0, "-",            5),
    ("gated_bilstm",     "Bi-LSTM",         ORANGE, "s", 3.8, "-",            4),
    ("gated_analytical", "Analytical",      GREEN,  "^", 4.0, "-",            3),
    ("blind",            "No-ML, $N$ swept", GREY,  "*", 7.0, (0, (3.5, 2)),  2),
]

rows = list(csv.DictReader(open(os.path.join(HERE, "frontier.csv"))))


def points(tag):
    """Rows of one series as dicts with floats, ordered along the TX axis."""
    pts = [dict(key=r["key"], tx=float(r["tx_mean"]), y=float(r["packets_mean"]),
                se=float(r["packets_sd"]) / math.sqrt(int(r["runs"])))
           for r in rows if r["series"] == tag]
    return sorted(pts, key=lambda p: p["tx"])


data = {tag: points(tag) for tag, *_ in SERIES}
style = {tag: (label, c, m, ms, ls, z) for tag, label, c, m, ms, ls, z in SERIES}

fig, ax = plt.subplots(figsize=(245.2 / 72, 143.8 / 72))
dress(ax)

for tag, (label, c, m, ms, ls, z) in style.items():
    pts = data[tag]
    ax.plot([p["tx"] for p in pts], [p["y"] for p in pts], color=c, lw=1.6,
            ls=ls, marker=m, ms=ms, mec="white", mew=0.6, zorder=z, clip_on=False)

# N = 1..5 beside each blind star
for p in data["blind"]:
    ax.annotate(f"$N$={int(float(p['key']))}", xy=(p["tx"], p["y"]),
                xytext=(3, -4), textcoords="offset points", fontsize=7,
                color=INK2, ha="left", va="top")

# ------------------------------------------------- matched-budget comparison
best = None
tag = CALLOUT_SERIES
g = max(data[tag], key=lambda p: p["y"])
match = [b for b in data["blind"]
         if b["tx"] >= g["tx"] and (b["tx"] - g["tx"]) / b["tx"] <= MATCH_TOL and b["y"] > 0]
if match:
    best = (None, tag, g, min(match, key=lambda b: b["tx"]))

if best is not None:
    _, tag, g, b = best
    ratio = g["y"] / b["y"]
    hc = style[tag][1]
    ax.plot([g["tx"]], [g["y"]], marker="o", ms=11, mfc="none", mec=hc,
            mew=1.4, zorder=6, clip_on=False)
    ax.annotate("", xy=(b["tx"], b["y"] + 2.0), xytext=(g["tx"], g["y"] - 2.5),
                arrowprops=dict(arrowstyle="-|>", lw=1.0, color=hc,
                                mutation_scale=7, shrinkA=0, shrinkB=0),
                zorder=6)
    # Below the blind line, right of the arrow, is empty once the legend sits
    # above the axes; lead back to the middle of the arrow.
    saving = 100 * (b["tx"] - g["tx"]) / b["tx"]
    label = (f"same TX budget:\n{ratio:.1f}$\\times$ packets" if saving < 5 else
             f"{saving:.0f}% fewer TX:\n{ratio:.1f}$\\times$ packets")
    # Right of the arrow, between the blind line and the analytical plateau
    head, tail = label.split("\n")
    # Left of the arrow, in the empty band under the learned curves (as in the paper)
    y_mid = b["y"] + 0.22 * (g["y"] - b["y"])
    x_arrow = b["tx"] + (g["tx"] - b["tx"]) * (y_mid - b["y"]) / (g["y"] - b["y"])
    ax.text(x_arrow - 30, y_mid + 1, head, color=hc, fontsize=7, fontweight="bold",
            ha="right", va="bottom")
    ax.text(x_arrow - 30, y_mid, tail, color=hc, fontsize=8.5, fontweight="bold",
            ha="right", va="top")
    print(f"matched budget: {style[tag][0]} key={g['key']} "
          f"({g['tx']:.1f} TX, {g['y']:.2f} pkts) vs blind N={b['key']} "
          f"({b['tx']:.1f} TX, {b['y']:.2f} pkts) -> {ratio:.2f}x")

all_tx = [p["tx"] for pts in data.values() for p in pts]
all_y = [p["y"] for pts in data.values() for p in pts]

ax.set_xlabel("Device transmissions (repetitions sent)")
ax.set_ylabel("Unique packets delivered")
ax.set_xlim(min(all_tx) - 60, max(all_tx) + 120)
Y_STEP = 20
ax.set_ylim(-3, 100)
ax.set_xticks([t for t in range(250, int(max(all_tx)) + 1, 250) if t >= min(all_tx) - 60])
ax.set_yticks(range(0, 101, Y_STEP))

legend = [Line2D([], [], color=c, marker=m, ms=ms, lw=1.6, ls=ls, mec="white",
                 mew=0.6, label=label)
          for label, c, m, ms, ls, z in style.values()]
# Inside, lower right, framed (as in figures/pareto_energy.pdf)
ax.legend(handles=legend, loc="lower right", ncol=1, frameon=True, fancybox=True,
          framealpha=0.95, edgecolor="#d9d8d3", handlelength=1.8, borderpad=0.35,
          labelspacing=0.2, fontsize=7, borderaxespad=0.2)

fig.tight_layout(pad=0.25)
for name in ("frontier_all", "pareto_energy"):   # pareto_energy.pdf = figures/ name in the paper
    fig.savefig(os.path.join(OUTDIR, name + ".pdf"))
    fig.savefig(os.path.join(OUTDIR, name + ".png"), dpi=220)
plt.close(fig)

print("wrote frontier_all.{pdf,png} and pareto_energy.{pdf,png} to", OUTDIR)
