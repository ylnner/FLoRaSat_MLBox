"""Figures 1 and 2 for the NTN competition paper.

Column width is 20 pica = 3.333 in. Type is Times to match the body font, set large
enough that the figure survives being blown up to A0 for the poster session.
Palette: categorical slots 1 and 2, validated (CVD dE 24.7 protan, 33.6 normal).

Usage:

    python3 make_figures.py

Reads frontier.csv from this directory (produced by make_frontier_data.py, and
committed here so the plots can be redrawn without touching the raw results) and
writes frontier.pdf and ladder.pdf beside it. Figure 2's numbers are inline in
MODELS below, from the ML baselines report.
"""
import csv, math, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

matplotlib.font_manager._load_fontmanager(try_read_cache=False)

HERE = os.path.dirname(os.path.abspath(__file__))
OUTDIR = HERE          # write the PDFs next to this script

BLUE, ORANGE = "#2a78d6", "#eb6834"
INK, INK2, MUTED = "#0b0b0b", "#52514e", "#8a8880"
COL_W = 3.333

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
    "pdf.fonttype": 42,
})


def dress(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.grid(True, which="major", axis="both", color="#e6e5e0", lw=0.5, zorder=0)
    ax.set_axisbelow(True)


# ============================================================ Figure 1
rows = list(csv.DictReader(open(os.path.join(HERE, "frontier.csv"))))
blind = [r for r in rows if r["series"] == "blind"]
gated = [r for r in rows if r["series"] == "gated_trf_no_buffer"]


def xy(rs):
    xs = [float(r["tx_mean"]) for r in rs]
    ys = [float(r["packets_mean"]) for r in rs]
    se = [float(r["packets_sd"]) / math.sqrt(int(r["runs"])) for r in rs]
    return xs, ys, se


bx, by, bse = xy(blind)
gx, gy, gse = xy(sorted(gated, key=lambda r: float(r["tx_mean"])))

fig, ax = plt.subplots(figsize=(COL_W, 2.25))
dress(ax)

ax.errorbar(bx, by, yerr=bse, color=ORANGE, lw=1.6, marker="s", ms=4.5,
            mec="white", mew=0.8, capsize=0, elinewidth=0.9, zorder=3, clip_on=False)
ax.errorbar(gx, gy, yerr=gse, color=BLUE, lw=1.6, marker="o", ms=4.5,
            mec="white", mew=0.8, capsize=0, elinewidth=0.9, zorder=4, clip_on=False)

# matched-budget comparison: the whole point of the figure
ax.plot([1225.9, 1260.0], [91.00, 38.94], color=INK2, lw=0.7, ls=(0, (2, 2)), zorder=2)
# Top-right is the only genuinely empty region; lead back to the comparison.
ax.annotate("2.4$\\times$ the packets\nfor the same energy", xy=(1243, 66),
            xytext=(2230, 96), fontsize=7.5, color=INK, ha="right", va="center",
            linespacing=1.3,
            arrowprops=dict(arrowstyle="-", lw=0.5, color=INK2,
                            shrinkA=4, shrinkB=2))

ax.text(1150, 85, "gated", color=BLUE, fontsize=8.5, ha="right", va="center")
ax.text(2090, 67, "blind", color=ORANGE, fontsize=8.5, ha="right", va="top")

ax.set_xlabel("transmissions emitted  (device energy)")
ax.set_ylabel("unique packets delivered")
ax.set_xlim(5, 2250)
ax.set_ylim(-2, 108)
ax.set_xticks([500, 1000, 1500, 2000])
ax.set_yticks([0, 25, 50, 75, 100])

legend = [Line2D([], [], color=BLUE, marker="o", ms=4.5, lw=1.6, mec="white",
                 mew=0.8, label="gated (threshold sweep)"),
          Line2D([], [], color=ORANGE, marker="s", ms=4.5, lw=1.6, mec="white",
                 mew=0.8, label="blind (1–5 repetitions)")]
ax.legend(handles=legend, loc="lower right", frameon=False, handlelength=1.8,
          borderpad=0.2, labelspacing=0.3, bbox_to_anchor=(1.02, -0.02))

fig.tight_layout(pad=0.25)
fig.savefig(os.path.join(OUTDIR, "frontier.pdf"))
fig.savefig(os.path.join(HERE, "frontier.png"), dpi=220)
plt.close(fig)

# ============================================================ Figure 2
# (label, size in bytes, AUC-PR, sees history?)
MODELS = [
    ("Logistic reg.", 920,          0.6448, False),
    ("Grad. boosting", 143 * 1024,  0.6893, False),
    ("Random forest", 110 * 1024**2, 0.6991, False),
    ("MLP",           97 * 1024,    0.7089, False),
    ("Bi-LSTM",       57 * 1024,    0.7137, True),
    ("Transformer",   64 * 1024,    0.7113, True),
]
ANALYTICAL = 0.5571

fig, ax = plt.subplots(figsize=(COL_W, 2.5))
dress(ax)
ax.set_xscale("log")

# the band every nonlinear model falls inside
lo = min(m[2] for m in MODELS if m[0] != "Logistic reg.")
hi = max(m[2] for m in MODELS)
ax.axhspan(lo, hi, color=BLUE, alpha=0.07, zorder=0)
ax.annotate("", xy=(1.35e3, lo), xytext=(1.35e3, hi),
            arrowprops=dict(arrowstyle="<->", lw=0.6, color=INK2, shrinkA=0, shrinkB=0))
ax.text(1.7e3, (lo + hi) / 2, "0.025", fontsize=7, color=INK2, ha="left", va="center")

ax.axhline(ANALYTICAL, color=MUTED, lw=0.8, ls=(0, (3, 2)), zorder=1)
ax.text(3.6e2, ANALYTICAL + 0.006, "no learning (analytical rule)",
        fontsize=7, color=INK2, va="bottom", ha="left")

for name, size, auc, seq in MODELS:
    ax.plot([size], [auc], marker="o" if seq else "D", ms=6 if seq else 5.5,
            color=BLUE if seq else ORANGE, mec="white", mew=0.9, zorder=4,
            clip_on=False, ls="none")

# Four models sit between 57 and 143 kB, so labels go in the whitespace on
# leader lines rather than beside the markers.
lab = {
    "Logistic reg.":  (1.1e3, 0.622, "left"),
    "Bi-LSTM":        (5.0e3, 0.727, "left"),
    "Transformer":    (1.1e6, 0.722, "left"),
    "MLP":            (1.3e6, 0.706, "left"),
    "Grad. boosting": (1.3e6, 0.678, "left"),
    "Random forest":  (2.0e6, 0.660, "left"),
}
for name, size, auc, seq in MODELS:
    tx_, ty_, ha = lab[name]
    ax.annotate(name, xy=(size, auc), xytext=(tx_, ty_), fontsize=7.5, color=INK,
                ha=ha, va="center", zorder=5,
                arrowprops=dict(arrowstyle="-", lw=0.5, color=MUTED,
                                shrinkA=1, shrinkB=4))

ax.set_xlabel("deployed size")
ax.set_ylabel("AUC-PR")
ax.set_xlim(3e2, 4e8)
ax.set_ylim(0.535, 0.745)
ax.set_xticks([1e3, 1e5, 1e7])
ax.set_xticklabels(["1 kB", "100 kB", "10 MB"])
ax.set_yticks([0.55, 0.60, 0.65, 0.70])

legend = [Line2D([], [], color=BLUE, marker="o", ms=6, ls="none", mec="white",
                 mew=0.9, label="sees the 8-step window"),
          Line2D([], [], color=ORANGE, marker="D", ms=5.5, ls="none", mec="white",
                 mew=0.9, label="sees one transmission")]
# Parked in the empty band between the analytical line and the lowest marker.
ax.legend(handles=legend, loc="lower right", frameon=False, handlelength=1.2,
          borderpad=0.2, labelspacing=0.3, bbox_to_anchor=(1.02, 0.10))

fig.tight_layout(pad=0.25)
fig.savefig(os.path.join(OUTDIR, "ladder.pdf"))
fig.savefig(os.path.join(HERE, "ladder.png"), dpi=220)
plt.close(fig)

print("wrote frontier.pdf and ladder.pdf to", OUTDIR)
