rows = ["split,alt,raan,seed,mix_id,p_sf10,p_sf11,p_sf12,p_tp12,p_tp13,p_tp14"]; k = 0
for ntx in range(1, 9):
    for raan in (85, 95, 105, 115, 125):
        for seed in range(10):
            rows.append(f"no_ml_603010_{ntx},670,{raan},{seed},{k},0.6,0.3,0.1,0,0,1"); k += 1
open("mixes_603010.csv", "w").write("\n".join(rows) + "\n")
