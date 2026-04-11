import sys

def analyze_vcd(filepath):
    var_map = {}
    values = {}
    time = 0
    
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            if line.startswith("$var"):
                parts = line.split()
                var_map[parts[3]] = parts[4]
                values[parts[4]] = "X"
            elif line.startswith("#"):
                # Handle #0 1! 1" etc.
                parts = line.split()
                time = int(parts[0][1:])
                for p in parts[1:]:
                    if p[0] in "01":
                        v_val = p[0]
                        v_id = p[1:]
                        if v_id in var_map:
                            update_val(var_map[v_id], v_val, values, time)
            elif line[0] in "01":
                v_val = line[0]
                v_id = line[1:]
                if v_id in var_map:
                    update_val(var_map[v_id], v_val, values, time)

def update_val(name, val, values, time):
    old = values.get(name, "X")
    values[name] = val
    
    # Logic for logging
    if name == "RD" and old == "1" and val == "0":
        a1 = values.get("A1", "X")
        a0 = values.get("A0", "X")
        d1 = values.get("D1", "X")
        d0 = values.get("D0", "X")
        wait = values.get("WAIT", "X")
        print(f"Time {time:8}: [RD Fall] A1:A0={a1}{a0} D1:D0={d1}{d0} WAIT={wait}")
    
    if name == "WAIT" and old == "0" and val == "1":
        d1 = values.get("D1", "X")
        d0 = values.get("D0", "X")
        print(f"Time {time:8}: [WAIT Rise] Data stable? D1:D0={d1}{d0}")

if __name__ == "__main__":
    analyze_vcd(sys.argv[1])
