"""Figures 1 and 2 for the NTN competition paper.

Updated with colorblind-friendly palettes, standard column layout sizing,
and high-contrast marker/linestyle pairings suitable for publication and print.
"""
import csv
import math
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

HERE = os.path.dirname(os.path.abspath(__file__))
OUTDIR = HERE          # write the PDFs next to this script

# --- Colorblind-Friendly Palette (Okabe-Ito & Paul Tol variants) ---
COLOR_BLUE   = "#0072B2"  # gated MLP
COLOR_TEAL   = "#009E73"  # gated TRF
COLOR_VERM   = "#D55E00"  # gated TWO
COLOR_PURP   = "#CC79A7"  # gated TRF no buffer
COLOR_AMBER  = "#E69F00"  # blind baseline

INK   = "#111111"
INK2  = "#444444"
MUTED = "#999999"
COL_W = 3.333  # Standard column width in inches (20 pica)

plt.rcParams.update({
    "font.family": "serif",
    "font.serif": ["Times New Roman", "DejaVu Serif"],
    "font.size": 8.0,
    "axes.labelsize": 8.5,
    "axes.titlesize": 8.5,
    "xtick.labelsize": 8.0,
    "ytick.labelsize": 8.0,
    "legend.fontsize": 7.5,
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
    """Apply minimal, clean publication styling to spines and grid."""
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.grid(True, which="major", axis="both", color="#E0E0E0", lw=0.5, zorder=0)
    ax.set_axisbelow(True)


# ============================================================ Figure 1
rows = list(csv.DictReader(open(os.path.join(HERE, "frontier.csv"))))
blind = [r for r in rows if r["series"] == "blind"]
gated_mlp = [r for r in rows if r["series"] == "gated_mlp"]
gated_trf = [r for r in rows if r["series"] == "gated_trf"]
gated_two = [r for r in rows if r["series"] == "gated_two"]
gated_trf_no_buffer = [r for r in rows if r["series"] == "gated_trf_no_buffer"]


def xy(rs):
    xs = [float(r["tx_mean"]) for r in rs]
    ys = [float(r["packets_mean"]) for r in rs]
    se = [float(r["packets_sd"]) / math.sqrt(int(r["runs"])) for r in rs]
    return xs, ys, se


bx, by, bse = xy(blind)
gmx, gmy, gmse = xy(sorted(gated_mlp, key=lambda r: float(r["tx_mean"])))
gtx, gty, gtse = xy(sorted(gated_trf, key=lambda r: float(r["tx_mean"])))
gwx, gwy, gwse = xy(sorted(gated_two, key=lambda r: float(r["tx_mean"])))
gnbx, gnby, gnbse = xy(sorted(gated_trf_no_buffer, key=lambda r: float(r["tx_mean"])))

# Reverted back to proper paper column height to match font sizes
fig, ax = plt.subplots(figsize=(COL_W, 2.5))
dress(ax)

# Plot curves with visual encodings (distinct shapes + distinct line styles)
ax.errorbar(bx, by, yerr=bse, color=COLOR_AMBER, lw=1.4, ls="--", marker="s", ms=4,
            mec="white", mew=0.6, capsize=0, elinewidth=0.8, zorder=3, label="blind (1–5 reps)")

ax.errorbar(gmx, gmy, yerr=gmse, color=COLOR_BLUE, lw=1.4, ls="-", marker="o", ms=4,
            mec="white", mew=0.6, capsize=0, elinewidth=0.8, zorder=4, label="MLP")

ax.errorbar(gtx, gty, yerr=gtse, color=COLOR_TEAL, lw=1.4, ls="-", marker="^", ms=4.5,
            mec="white", mew=0.6, capsize=0, elinewidth=0.8, zorder=5, label="Transformer")

ax.errorbar(gwx, gwy, yerr=gwse, color=COLOR_VERM, lw=1.4, ls="-.", marker="D", ms=3.5,
            mec="white", mew=0.6, capsize=0, elinewidth=0.8, zorder=5, label="TwoBranches-no buffer")

ax.errorbar(gnbx, gnby, yerr=gnbse, color=COLOR_PURP, lw=1.4, ls=":", marker="X", ms=4.5,
            mec="white", mew=0.6, capsize=0, elinewidth=0.8, zorder=5, label="Transformer-No Buffer")

# Matched-budget comparison dash line
ax.plot([1225.9, 1260.0], [91.00, 38.94], color=INK2, lw=0.8, ls=(0, (2, 2)), zorder=2)

# Trade-off Annotation
ax.annotate("2.3$\\times$ packets\nsame energy", xy=(1243, 66),
            xytext=(1850, 85), fontsize=7, color=INK, ha="right", va="center",
            linespacing=1.2,
            arrowprops=dict(arrowstyle="->", lw=0.6, color=INK2,
                            shrinkA=2, shrinkB=4, patchA=None))

ax.set_xlabel("Transmissions Emitted (Device Energy)")
ax.set_ylabel("Unique Packets Delivered")
ax.set_xlim(0, 2250)
ax.set_ylim(-2, 108)
ax.set_xticks([0, 500, 1000, 1500, 2000])
ax.set_yticks([0, 25, 50, 75, 100])

# Clean, legible legend inside plot area
ax.legend(loc="lower right", frameon=True, facecolor="white", edgecolor="none",
          framealpha=0.85, handlelength=2.0, borderpad=0.3, labelspacing=0.25)

fig.tight_layout(pad=0.2)
fig.savefig(os.path.join(OUTDIR, "frontier.pdf"))
fig.savefig(os.path.join(HERE, "frontier.png"), dpi=300)
plt.close(fig)

# ============================================================ Figure 2
MODELS = [
    ("Logistic reg.", 920,            0.6448, False),
    ("Grad. boosting", 143 * 1024,   0.6893, False),
    ("Random forest",  110 * 1024**2, 0.6991, False),
    ("MLP",            97 * 1024,    0.7089, False),
    ("Bi-LSTM",        57 * 1024,    0.7137, True),
    ("Transformer",    64 * 1024,    0.7113, True),
]
ANALYTICAL = 0.5571

fig, ax = plt.subplots(figsize=(COL_W, 2.5))
dress(ax)
ax.set_xscale("log")

# Highlight band for non-linear performance range
lo = min(m[2] for m in MODELS if m[0] != "Logistic reg.")
hi = max(m[2] for m in MODELS)
ax.axhspan(lo, hi, color=COLOR_BLUE, alpha=0.08, zorder=0)

ax.annotate("", xy=(1.35e3, lo), xytext=(1.35e3, hi),
            arrowprops=dict(arrowstyle="<->", lw=0.6, color=INK2, shrinkA=0, shrinkB=0))
ax.text(1.7e3, (lo + hi) / 2, "0.025", fontsize=7, color=INK2, ha="left", va="center")

# Analytical rule baseline
ax.axhline(ANALYTICAL, color=MUTED, lw=0.8, ls=(0, (3, 2)), zorder=1)
ax.text(3.6e2, ANALYTICAL + 0.006, "no learning (analytical rule)",
        fontsize=7, color=INK2, va="bottom", ha="left")

# Plot points
for name, size, auc, seq in MODELS:
    ax.plot(size, auc, marker="o" if seq else "D", ms=5.5 if seq else 5.0,
            color=COLOR_BLUE if seq else COLOR_VERM, mec="white", mew=0.8,
            zorder=4, clip_on=False, ls="none")

# Optimized annotation positions to prevent line crossings
lab = {
    "Logistic reg.":  (1.1e3, 0.622, "left"),
    "Bi-LSTM":        (3.5e3, 0.730, "left"),
    "Transformer":    (2.5e5, 0.732, "center"),
    "MLP":            (1.2e6, 0.712, "left"),
    "Grad. boosting": (1.2e6, 0.682, "left"),
    "Random forest":  (1.5e6, 0.655, "left"),
}

for name, size, auc, seq in MODELS:
    tx_, ty_, ha = lab[name]
    ax.annotate(name, xy=(size, auc), xytext=(tx_, ty_), fontsize=7.0, color=INK,
                ha=ha, va="center", zorder=5,
                arrowprops=dict(arrowstyle="-", lw=0.5, color=INK2,
                                shrinkA=1, shrinkB=3))

ax.set_xlabel("Deployed Size")
ax.set_ylabel("AUC-PR")
ax.set_xlim(3e2, 4e8)
ax.set_ylim(0.535, 0.748)
ax.set_xticks([1e3, 1e5, 1e7])
ax.set_xticklabels(["1 kB", "100 kB", "10 MB"])
ax.set_yticks([0.55, 0.60, 0.65, 0.70])

# Category Legend
legend = [
    Line2D([], [], color=COLOR_BLUE, marker="o", ms=5.5, ls="none", mec="white",
           mew=0.8, label="sees 8-step window"),
    Line2D([], [], color=COLOR_VERM, marker="D", ms=5.0, ls="none", mec="white",
           mew=0.8, label="sees single transmission")
]
ax.legend(handles=legend, loc="lower right", frameon=False, handlelength=1.2,
          borderpad=0.2, labelspacing=0.3, bbox_to_anchor=(1.0, 0.08))

fig.tight_layout(pad=0.2)
fig.savefig(os.path.join(OUTDIR, "ladder.pdf"))
fig.savefig(os.path.join(HERE, "ladder.png"), dpi=300)
plt.close(fig)

print("Successfully wrote updated frontier.pdf and ladder.pdf to", OUTDIR)
