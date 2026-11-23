import os
import subprocess

# Source and destination directories
src_dir = os.path.join(os.path.dirname(__file__), '../FlorasatExtensionPaper/results')
dst_dir = os.path.dirname(__file__)

# Helper to extract run index from filename (assumes format: ...R{i}...)
def extract_run_index(filename):
    import re
    match = re.search(r'General(\d+)', filename)
    return match.group(1) if match else None

# Export each .sca and .vec file individually
for filename in os.listdir(src_dir):
    if filename.endswith('.sca') or filename.endswith('.vec'):
        run_idx = extract_run_index(filename)
        if not run_idx:
            continue  # skip files without run index
        src_file = os.path.join(src_dir, filename)
        if filename.endswith('.sca'):
            out_csv = os.path.join(dst_dir, f'wMLImplementation_{run_idx}_scalar.csv')
            cmd = [
                'opp_scavetool', 'export',
                '-F', 'CSV-R',
                '-o', out_csv,
                src_file
            ]
            subprocess.run(cmd, check=True)
            print(f"Exported {filename} to {out_csv}")
        elif filename.endswith('.vec'):
            out_csv = os.path.join(dst_dir, f'wMLImplementation_{run_idx}_vector.csv')
            cmd = [
                'opp_scavetool', 'export',
                '-F', 'CSV-R',
                '-o', out_csv,
                src_file
            ]
            subprocess.run(cmd, check=True)
            print(f"Exported {filename} to {out_csv}")
