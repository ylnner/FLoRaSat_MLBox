"""
Reproduce el calculo de la orbita de cada escenario (los valores que usa make_inis.py) y su geometria.

Referencia: el lazo cerrado actual (670 km, raan 85-125, MA 105, epoch = base = 2020 dia 1.5) es un paso descendente
sobre Europa: el subsatelite va de 73 N a 29 N en los 749 s.

Europa 850 / 950 km:   misma raan; MA tal que a mitad de la simulacion (t = 374.5 s) el satelite esta en el mismo
                       argumento de latitud que a 670 km (la orbita es mas lenta a mayor altitud).
Norteamerica:          misma raan 85-125 (valores ya vistos por el modelo); se desplaza epochDay = baseDay para que la
                       Tierra gire y el paso caiga sobre Norteamerica, y se ajusta MA para que en el mismo instante que en
                       Europa-670 (raan 105) el subsatelite este en la misma posicion relativa al centro de los dispositivos.

Uso:  python3 design_orbits.py
"""
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from orbit_model import GM, RE, J2, sat_ecef, elevation, subpoint   # noqa: E402

TEST_KD = "/home/ylnner/Documents/ToGitHubTemporal/FLoRaSat_MLBox/simulations/Test_KD"
EU_DEVICES = os.path.join(TEST_KD, "devices", "WorldWide-1000-ML.ini")
NA_DEVICES = os.path.join(os.path.dirname(HERE), "devices", "NorthAmerica-1000-ML.ini")
T_MID = 374.5
SIDEREAL_DEG_PER_DAY = 360.98564736629


def load_devices(path):
    text = open(path).read()
    lat = [float(x) for x in re.findall(r"centerLatitude\s*=\s*([-\d.]+)", text)]
    lon = [float(x) for x in re.findall(r"centerLongitude\s*=\s*([-\d.]+)", text)]
    return np.array(lat), np.array(lon)


def latitude_argument_rate(alt, incl=98.0):
    """d(argumento de latitud)/dt en rad/s (Kepler + J2 secular), igual que orbit_model."""
    a = RE + alt
    n = np.sqrt(GM / a ** 3)
    i = np.radians(incl)
    k = 1.5 * J2 * (RE / a) ** 2 * n
    return n + k * (2.5 * np.cos(i) ** 2 - 0.5) + k * (1.5 * np.cos(i) ** 2 - 0.5)


def europe_mean_anomaly(alt):
    u_mid = 105 + np.degrees(latitude_argument_rate(670)) * T_MID
    return u_mid - np.degrees(latitude_argument_rate(alt)) * T_MID


def north_america_orbit(alt, eu_center, na_center):
    """(MA, dia) para que el paso sobre NA tenga la misma posicion relativa que Europa-670 con raan 105."""
    t = np.arange(0, 750, 0.5)
    slat, slon = subpoint(sat_ecef(t, 670, 105, 105))
    k = np.argmin(abs(slat - eu_center[0]))
    t_ref = t[k]
    target_lat = na_center[0]
    target_lon = na_center[1] + (slon[k] - eu_center[1])
    best_ma, best_err, best_lon = None, 1e9, None
    for ma in np.arange(100, 130, 0.01):
        la, lo = subpoint(sat_ecef(np.array([t_ref]), alt, 105, ma))
        if abs(la[0] - target_lat) < best_err:
            best_ma, best_err, best_lon = ma, abs(la[0] - target_lat), lo[0]
    rotation = (-(target_lon - best_lon)) % 360           # grados que debe girar la Tierra (GMST +)
    day = 1.5 + rotation / SIDEREAL_DEG_PER_DAY
    return best_ma, day


def geometry(lat, lon, alt, raan, ma, day):
    t = np.arange(0, 750, 2.0)
    el = elevation(sat_ecef(t[:, None], alt, raan, ma, epoch=(2020, day)), lat[None, :], lon[None, :])
    vis = el >= 10
    seen = vis.any(axis=0)
    return 100 * seen.mean(), 100 * vis.sum(axis=1).max() / len(lat), np.median(el.max(axis=0)[seen])


def main():
    eu = load_devices(EU_DEVICES)
    na = load_devices(NA_DEVICES)
    eu_center = (eu[0].mean(), eu[1].mean())
    na_center = (na[0].mean(), na[1].mean())
    print(f"centro dispositivos Europa {eu_center[0]:.2f}, {eu_center[1]:.2f}; Norteamerica {na_center[0]:.2f}, {na_center[1]:.2f}")
    rows = [("EU_670 (actual)", eu, 670, 105.0, 1.5)]
    for alt in (850, 950):
        ma = europe_mean_anomaly(alt)
        print(f"Europa {alt} km: MA exacta {ma:.3f} -> se usa {round(ma):.1f}")
        rows.append((f"EU_{alt}", eu, alt, float(round(ma)), 1.5))
    for alt in (850, 950):
        ma, day = north_america_orbit(alt, eu_center, na_center)
        print(f"Norteamerica {alt} km: MA {ma:.2f} -> {round(ma, 1)}, epochDay = baseDay {day:.6f} -> {round(day, 5)}")
        rows.append((f"NA_{alt}", na, alt, round(ma, 1), round(day, 5)))
    print("\nescenario         raan: % dispositivos que ven el satelite / % maximo simultaneo / elevacion maxima mediana")
    for name, (lat, lon), alt, ma, day in rows:
        cells = []
        for raan in (85, 95, 105, 115, 125):
            seen, peak, elev = geometry(lat, lon, alt, raan, ma, day)
            cells.append(f"{raan}: {seen:.0f}/{peak:.0f}/{elev:.0f}")
        print(f"{name:16s} " + " | ".join(cells))


if __name__ == "__main__":
    main()
