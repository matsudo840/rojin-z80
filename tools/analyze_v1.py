import sys

def analyze_vcd(filepath):
    var_map = {}
    values = {}
    time = 0
    
    # Target signals
    target_names = ["CLK", "WAIT", "MREQ", "RD", "A0", "A1", "D0", "D1"]
    
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            if line.startswith("$var"):
                parts = line.split()
                var_id = parts[3]
                var_name = parts[4]
                var_map[var_id] = var_name
                values[var_name] = "X"
            elif line.startswith("#"):
                time = int(line[1:])
            elif line[0] in "01":
                val = line[0]
                var_id = line[1:]
                if var_id in var_map:
                    name = var_map[var_id]
                    old = values[name]
                    values[name] = val
                    
                    # Log when RD or MREQ changes
                    if name == "RD" and old == "1" and val == "0":
                        a0 = values.get("A0", "X")
                        a1 = values.get("A1", "X")
                        d0 = values.get("D0", "X")
                        d1 = values.get("D1", "X")
                        wait = values.get("WAIT", "X")
                        print(f"Time {time:8}: [RD Fall] A1:A0={a1}{a0} D1:D0={d1}{d0} WAIT={wait}")
                    
                    if name == "WAIT" and old == "0" and val == "1":
                        # When WAIT is released, Z80 will soon sample the data
                        d0 = values.get("D0", "X")
                        d1 = values.get("D1", "X")
                        print(f"Time {time:8}: [WAIT Rise] Data stable? D1:D0={d1}{d0}")

if __name__ == "__main__":
    analyze_vcd(sys.argv[1])
