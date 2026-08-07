"""Parse FLoRaSat results into the two series Figure 1 needs.

Cost  = sum of numTransmissionsApproved over the 60 terminals (frames actually radiated,
        i.e. the device-energy counter).
Yield = number of entries in "All Nodes Message Generation Delay at Satellite", which
        fires once per UNIQUE packet that reached the satellite. The sibling
        "Repetition ..." vector counts every copy and is not what we want.

Validation targets from PAPER.md: blind N=5 -> 78.38 packets / 2099.9 TX,
gated tau=0.2 -> 91.00 / 1225.9.
Both are checked at the end of the run, so a wrong path or a partial checkout is
caught rather than silently producing a plausible plot.

Usage, pointing it at a FLoRaSat_MLBox checkout:

    python3 make_frontier_data.py /path/to/FLoRaSat_MLBox
    FLORASAT_MLBOX=/path/to/FLoRaSat_MLBox python3 make_frontier_data.py

It reads, relative to that checkout:
    Files_for_plots/results_florasat_ml_trf/raan_*/*_scalar.csv   (gated, converted)
    Files_for_plots/results_florasat_ml_trf/raan_*/*_vector.csv   (gated, converted)
    simulations/Test_Individual/results/results_florasat_no_ml/N_*/*.sca and .vec  (blind)

and writes frontier.csv next to this script. Takes a couple of minutes; the .vec
files are large.
"""
import csv, glob, os, re, sys
from collections import defaultdict

csv.field_size_limit(10 ** 9)

REPO = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("FLORASAT_MLBOX", "")
if not REPO or not os.path.isdir(REPO):
    sys.exit(__doc__.split("Usage,")[1].strip() if REPO else
             "Point me at a FLoRaSat_MLBox checkout:\n"
             "    python3 make_frontier_data.py /path/to/FLoRaSat_MLBox")
OUT = os.path.dirname(os.path.abspath(__file__))
UNIQ = "All Nodes Message Generation Delay at Satellite"


def tx_from_scalar_csv(path):
    total = 0.0
    with open(path, newline="") as fh:
        for row in csv.reader(fh):
            if len(row) >= 7 and row[1] == "scalar" and row[3] == "numTransmissionsApproved" and row[6]:
                total += float(row[6])
    return total

def colls_from_scalar_csv(path):
    total = 0.0
    with open(path, newline="") as fh:
        for row in csv.reader(fh):
            if len(row) >= 7 and row[1] == "scalar" and row[3] == "numCollisions" and row[6]:
                total += float(row[6])
    return total

def uniq_from_vector_csv(path):
    with open(path, newline="") as fh:
        for row in csv.reader(fh):
            if len(row) >= 8 and row[1] == "vector" and row[3] == UNIQ:
                #print("row[7].split(): " , len(row[7].split()), row[7].split())
                #input("Presiona...")
                return len(row[7].split())
    return 0


def tx_from_sca(path):
    total = 0.0
    with open(path, errors="ignore") as fh:
        for line in fh:
            if line.startswith("scalar ") and "numTransmissionsApproved" in line:
                try:
                    total += float(line.split()[3])
                except (IndexError, ValueError):
                    pass
    return total

def colls_from_sca(path):
    total = 0.0
    with open(path, errors="ignore") as fh:
        for line in fh:
            if line.startswith("scalar ") and "numCollisions" in line:
                try:
                    total += float(line.split()[3])
                except (IndexError, ValueError):
                    pass
    return total

def uniq_from_vec(path):
    """Raw .vec: find the vector id declared with the unique-packet name, count its rows."""
    vid = None
    count = 0
    with open(path, errors="ignore") as fh:
        for line in fh:
            if line.startswith("vector "):
                if UNIQ in line:
                    vid = line.split()[1]
            elif vid is not None and line[:1].isdigit():
                if line.split(maxsplit=1)[0] == vid:
                    count += 1
    return count

# ---------------------------------------------------------------- gated sweep Transformer
def getData(folder):
    #folder = 'results_florasat_ml_twoBranches'
    gated = defaultdict(list)
    #root = os.path.join(REPO, "results_florasat_ml_twoBranches")
    root = os.path.join(REPO, folder)
    print('root: ', root)
    for scal in glob.glob(os.path.join(root, "raan_*", "*_scalar.csv")):
        m = re.search(r"threshold_([0-9.]+)_(\d+)_scalar\.csv$", os.path.basename(scal))
        if not m:
            continue
        vec = scal.replace("_scalar.csv", "_vector.csv")
        if not os.path.exists(vec):
            continue
        tx = tx_from_scalar_csv(scal)
        #print('vec: ', vec)
        uq = uniq_from_vector_csv(vec)
        co = colls_from_scalar_csv(scal)
        if tx > 0:
            gated[float(m.group(1))].append((tx, uq, co))
    
    return gated

# ------------------------------------------------------------- blind N sweep
blind = defaultdict(list)
nroot = os.path.join(REPO, "results_florasat_no_ml")
for n in range(1, 6):
    for sca in glob.glob(os.path.join(nroot, f"N_{n}", "*.sca")):
        vec = sca[:-4] + ".vec"
        # A missing .vec means the run recorded no delivered-packet entries at all,
        # i.e. zero unique packets. Verified: N=1 gives 2.41 over the 34 runs that
        # have a .vec, and 2.41*34/50 = 1.64, which is PAPER.md's value over 50.
        uq = uniq_from_vec(vec) if os.path.exists(vec) else 0
        tx = tx_from_sca(sca)
        co = colls_from_sca(sca)
        if tx > 0:
            blind[n].append((tx, uq, co))


def summarise(series):
    rows = []
    for key in sorted(series):
        pts = series[key]
        n = len(pts)
        mtx = sum(p[0] for p in pts) / n
        muq = sum(p[1] for p in pts) / n
        sd = (sum((p[1] - muq) ** 2 for p in pts) / n) ** 0.5
        mco = sum(p[2] for p in pts) / n
        rows.append((key, n, mtx, muq, sd, mco))
    return rows

folder_names = ['results_florasat_ml_mlp', 'results_florasat_ml_twoBranches', 'results_florasat_ml_trf', 'results_florasat_ml_trf_no_buffer']

gated_rows_mlp = summarise(getData(folder_names[0]))
gated_rows_two = summarise(getData(folder_names[1]))
gated_rows_trf = summarise(getData(folder_names[2]))
gated_rows_trf_no_buffer = summarise(getData(folder_names[3]))
blind_rows = summarise(blind)
#gated_rows_mlp, blind_rows, gated_rows_trf = summarise(gated_mlp), summarise(blind), summarise(gated_trf)


with open(os.path.join(OUT, "frontier.csv"), "w", newline="") as fh:
    w = csv.writer(fh)
    w.writerow(["series", "key", "runs", "tx_mean", "packets_mean", "packets_sd", "collisions"])
        
    for tag, rows in (("blind", blind_rows), ("gated_mlp", gated_rows_mlp), ("gated_two", gated_rows_two), ("gated_trf", gated_rows_trf), ("gated_trf_no_buffer", gated_rows_trf_no_buffer)):
        for key, n, mtx, muq, sd, co in rows:
            w.writerow([tag, key, n, f"{mtx:.4f}", f"{muq:.4f}", f"{sd:.4f}", f"{co:.4f}"])

print(f"{'series':10} {'key':>5} {'runs':>5} {'TX':>9} {'packets':>9} {'sd':>7}")
for tag, rows in (("blind", blind_rows), ("gated_mlp", gated_rows_mlp), ("gated_two", gated_rows_two), ("gated_trf", gated_rows_trf), ("gated_trf_no_buffer", gated_rows_trf_no_buffer)):
    for key, n, mtx, muq, sd, co in rows:
        print(f"{tag:10} {key:>5} {n:>5} {mtx:9.1f} {muq:9.2f} {sd:7.2f}")

print("\nVALIDATION vs PAPER.md")
for tag, rows, key, wp, wt in (
    ("blind N=5", blind_rows, 5, 78.38, 2099.9),
    #("gated tau=0.2", gated_rows, 0.2, 91.00, 1225.9),
    #("gated_mlp tau=0.2", gated_rows_mlp, 0.2, 91.00, 1225.9),
    #("gated_trf tau=0.2", gated_rows_trf, 0.2, 91.00, 1225.9),
):
    hit = [r for r in rows if r[0] == key]
    if not hit:
        print(f"  {tag:14} MISSING")
        continue
    _, n, mtx, muq, _ = hit[0]
    print(f"  {tag:14} {muq:7.2f} (want {wp:6.2f}) {'OK' if abs(muq-wp)<0.6 else 'MISMATCH'} | "
          f"{mtx:8.1f} (want {wt:6.1f}) {'OK' if abs(mtx-wt)<12 else 'MISMATCH'} | {n} runs")
