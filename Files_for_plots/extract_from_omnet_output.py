import os
import re
import subprocess

def extract_threshold_and_run(filename):
    """
    Extract threshold and run index from filenames with the pattern:
    <anything>-th-<threshold>-General-<run>.sca

    Example:
        transformer-th-0.10-General-0.sca
        lstm-best-th-0.00293165740758182-General-15.sca
    """
    #pattern = r".*-th-([0-9]*\.?[0-9]+)-General-(\d+)\.sca$"
    pattern = r".*-th-([0-9]*\.?[0-9]+)-General-(\d+)\.[^.]+$"

    match = re.match(pattern, filename)
    if not match:
        raise ValueError(f"Filename does not match expected pattern: {filename}")

    threshold = float(match.group(1))
    run = int(match.group(2))

    return threshold, run

def extract_raan_threshold_and_run(filename):
    """
    Extract raan, threshold, and run index from filenames with the pattern:
    <anything>-raan-<raan>-th-<threshold>-General-<run>.<extension>

    Example:
        transformer-raan-165-th-0.90-General-2.vec
    """
    # El nuevo patrón busca '-raan-', captura sus dígitos, luego sigue con '-th-', etc.
    pattern = r".*-raan-(\d+)-th-([0-9]*\.?[0-9]+)-General-(\d+)\.[^.]+$"

    match = re.match(pattern, filename)
    if not match:
        raise ValueError(f"Filename does not match expected pattern: {filename}")

    # Capturamos los 3 grupos en orden
    raan = int(match.group(1))
    threshold = float(match.group(2))
    run = int(match.group(3))

    return raan, threshold, run


# Function to extract the run index from the filename
def extract_run_index(filename):    
    # Search for a '-' followed by one or more digits (\d+)
    match = re.search(r'-(\d+)', filename)
    return match.group(1) if match else None


def extract_raan_and_run_index(filename):    
    match = re.search(r'-raan-(\d+)-General-(\d+)', filename)
    
    if match:
        raan = int(match.group(1))
        run = int(match.group(2))
        return raan, run
    
    return None, None


dirs = ['results_florasat_ml_trf', 'results_florasat_ml_lstm', 'results_florasat_analytical', 'results_florasat_no_ml']
names = ['wML_Transformer', 'wML_BiLSTM', 'wAnalytical', 'wNoML']

#dirs = ['results_florasat_ml_trf']
#names = ['wML_Transformer']

subdirs = ['raan_165', 'raan_175', 'raan_185', 'raan_195', 'raan_205']

#dirs = ['results_florasat_no_ml']
#names = ['wNoML']

for dir, name in zip(dirs, names):
    print(f"Processing directory: {dir}")

    # Exportar cada archivo .sca y .vec individualmente
    for filename in os.listdir(dir):
        if filename.endswith('.sca') or filename.endswith('.vec'):        
            #run_idx = extract_run_index(filename)
            if dir == 'results_florasat_no_ml':
                threshold = -1  # No ML no tiene thresholds
                #run_idx = extract_run_index(filename)
                raan, run_idx = extract_raan_and_run_index(filename) 
                print(f"Processing {filename} with run index: {run_idx}")
            else:
                #threshold, run_idx = extract_threshold_and_run(filename)
                #print(f"Processing {filename} with threshold: {threshold} and run index: {run_idx}")
                raan, threshold, run_idx = extract_raan_threshold_and_run(filename)                
                print(f"Processing {filename} with threshold: {threshold} and run index: {run_idx} and raan: {raan}")
            #input("Presiona Enter para continuar...")
            '''
            if not run_idx or not threshold:
                print(f"Not valid index or threshold: {filename}")
                continue 
            '''
            #print(f"Processing {filename} with threshold: {threshold} and run index: {run_idx}")
            subdir_name = 'raan_' + str(raan)
            subdir_path = os.path.join(dir, subdir_name)
            
            if not os.path.exists(subdir_path):
                print(f"Creating folder {subdir_path}...")
                os.makedirs(subdir_path, exist_ok=True)


            src_file = os.path.join(dir,filename)
            print(f"Source file: {src_file}")
            #input("Presiona Enter para continuar...")

            #''' 
            if filename.endswith('.sca'):
                if threshold >0:
                    out_csv = os.path.join(dir,subdir_name, f'{name}_raan_{raan}_threshold_{threshold}_{run_idx}_scalar.csv')
                else:
                    out_csv = os.path.join(dir,subdir_name, f'{name}_raan_{raan}_run_{run_idx}_scalar.csv')
                    
                cmd = [
                    'opp_scavetool', 'export',
                    '-F', 'CSV-R',
                    '-o', out_csv,
                    src_file
                ]
                subprocess.run(cmd, check=True)
                print(f"Exported {filename} to {out_csv}")
                #input("Presiona Enter para continuar...")
                
            elif filename.endswith('.vec'):
                if threshold >0:
                    out_csv = os.path.join(dir, subdir_name, f'{name}_raan_{raan}_threshold_{threshold}_{run_idx}_vector.csv')
                else:
                    out_csv = os.path.join(dir, subdir_name, f'{name}_raan_{raan}_run_{run_idx}_vector.csv')
                cmd = [
                    'opp_scavetool', 'export',
                    '-F', 'CSV-R',
                    '-o', out_csv,
                    src_file
                ]
                subprocess.run(cmd, check=True)
                print(f"Exported {filename} to {out_csv}")
                #input("Presiona Enter para continuar...")
            #'''