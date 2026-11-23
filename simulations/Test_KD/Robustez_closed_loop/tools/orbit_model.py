"""Propagador circular (Kepler + J2 secular) equivalente a cOrbitA/SGP4 de FLoRaSat durante ~15 min desde el epoch.
Validado contra elevSat registrada por FLoRaSat (600, 670, 700, 740 y 850 km): error medio +0.2 grados, p95 <= 0.7.
t = segundos desde el epoch (epochDay = baseDay -> gap 0). Constantes WGS72 como libnorad."""
import numpy as np

GM = 398600.8                 # km^3/s^2 (WGS72)
RE = 6378.135                 # km (XKMPER_WGS72)
F = 1.0 / 298.26              # achatamiento WGS72 (libnorad)
J2 = 1.082616e-3
OMEGA_E = 7.29211510e-5       # rad/s (libnorad OMEGA_E * 2pi/86400 ~)


def julian(year, day):
    """cJulian(year, day): dia 1.0 = 1 de enero 00:00 UT."""
    a = (year - 1) // 100
    jd_jan0 = int(365.25 * (year - 1)) + int(30.6001 * 14) + 1720994.5 + (2 - a + a // 4)
    return jd_jan0 + day


def gmst_rad(jd):
    """GMST (IAU 1982), como cJulian::toGMST."""
    ut = (jd + 0.5) % 1.0
    jd0 = jd - ut
    tu = (jd0 - 2451545.0) / 36525.0
    gmst = 24110.54841 + tu * (8640184.812866 + tu * (0.093104 - tu * 6.2e-6))
    gmst = (gmst + 86400.0 * 1.00273790934 * ut) % 86400.0
    return 2 * np.pi * gmst / 86400.0


def sat_ecef(t, alt, raan_deg, ma_deg, incl_deg=98.0, epoch=(2020, 1.5)):
    a = RE + alt
    n = np.sqrt(GM / a ** 3)
    i = np.radians(incl_deg)
    # J2 secular (pequenos en 15 min, pero gratis)
    k = 1.5 * J2 * (RE / a) ** 2 * n
    raan = np.radians(raan_deg) - k * np.cos(i) * t
    u = np.radians(ma_deg) + (n + k * (2.5 * np.cos(i) ** 2 - 0.5) + k * (1.5 * np.cos(i) ** 2 - 0.5)) * t
    x = a * (np.cos(raan) * np.cos(u) - np.sin(raan) * np.sin(u) * np.cos(i))
    y = a * (np.sin(raan) * np.cos(u) + np.cos(raan) * np.sin(u) * np.cos(i))
    z = a * np.sin(u) * np.sin(i)
    theta = gmst_rad(julian(*epoch)) + OMEGA_E * t
    xe = np.cos(theta) * x + np.sin(theta) * y
    ye = -np.sin(theta) * x + np.cos(theta) * y
    return np.stack([xe, ye, z], axis=-1)


def site_ecef(lat_deg, lon_deg, h=0.0):
    lat, lon = np.radians(lat_deg), np.radians(lon_deg)
    e2 = F * (2 - F)
    N = RE / np.sqrt(1 - e2 * np.sin(lat) ** 2)
    return np.stack([(N + h) * np.cos(lat) * np.cos(lon), (N + h) * np.cos(lat) * np.sin(lon),
                     (N * (1 - e2) + h) * np.sin(lat)], axis=-1)


def elevation(sat, lat_deg, lon_deg):
    """sat [..., 3] ECEF; devuelve elevacion en grados (geodetica)."""
    s = site_ecef(lat_deg, lon_deg)
    lat, lon = np.radians(lat_deg), np.radians(lon_deg)
    up = np.stack([np.cos(lat) * np.cos(lon), np.cos(lat) * np.sin(lon), np.sin(lat)], axis=-1)
    d = sat - s
    return np.degrees(np.arcsin((d * up).sum(-1) / np.linalg.norm(d, axis=-1)))


def subpoint(sat):
    x, y, z = sat[..., 0], sat[..., 1], sat[..., 2]
    lon = np.degrees(np.arctan2(y, x))
    lat = np.degrees(np.arctan2(z, np.hypot(x, y)))   # geocentrica, suficiente para orientarse
    return lat, lon
