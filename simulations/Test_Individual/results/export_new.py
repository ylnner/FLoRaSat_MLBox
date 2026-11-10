import os
import re
import subprocess

# Nueva ruta absoluta de origen para los resultados
src_dir = "/home/ylnner/Documents/omnet++/omnetpp-6.3.0/projects/florasat_v4_ult_routing/simulations/Test_Individual/results"

# El destino se mantiene en la misma carpeta donde ejecutes este script
dst_dir = os.path.dirname(__file__)

# Nueva función para extraer el índice después del símbolo '#' (ej. "General-15937-#2")
def extract_run_index(filename):
    # Busca un '#' seguido de uno o más dígitos (\d+)
    match = re.search(r'#(\d+)', filename)
    return match.group(1) if match else None

def extract_raan_and_run(filename):
    # Explicación del regex:
    # raan(\d+) -> Captura el número después de "raan" (Grupo 1)
    # -(\d+)    -> Captura el número del run antes del punto (Grupo 2)
    match = re.search(r'raan(\d+)-(\d+)\.\w+$', filename)
    if match:
        return match.group(1), match.group(2)  # Retorna (raan, run_idx)
    return None, None

# Exportar cada archivo .sca y .vec individualmente
for filename in os.listdir(src_dir):
    if filename.endswith('.sca') or filename.endswith('.vec'):        
        run_idx = extract_run_index(filename)
        if not run_idx:
            print(f"Saltado (sin índice válido): {filename}")
            continue  # Salta archivos que no tengan el formato esperado
        
        #raan, run_idx = extract_raan_and_run(filename)
        #if not raan or not run_idx:
        #    print(f"Saltado (sin formato válido): {filename}")
        #    continue  
        
        src_file = os.path.join(src_dir, filename)
        
        if filename.endswith('.sca'):
            out_csv = os.path.join(dst_dir, f'wMLImplementation_{run_idx}_scalar.csv')
            #out_csv = os.path.join(dst_dir, f'wMLImplementation_raan_{raan}_{run_idx}_scalar.csv')
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
            #out_csv = os.path.join(dst_dir, f'wMLImplementation_raan_{raan}_{run_idx}_vector.csv')
            cmd = [
                'opp_scavetool', 'export',
                '-F', 'CSV-R',
                '-o', out_csv,
                src_file
            ]
            subprocess.run(cmd, check=True)
            print(f"Exported {filename} to {out_csv}")
