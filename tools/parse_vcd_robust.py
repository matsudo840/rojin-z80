import sys

filepath = sys.argv[1]

var_map = {}
values = {}
time = 0

with open(filepath, "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        if line.startswith("$var"):
            parts = line.split()
            var_map[parts[3]] = parts[4]
            values[parts[4]] = "X"
        elif line.startswith("#"):
            parts = line.split()
            time = int(parts[0][1:])
            # Handle if there are variable assignments on the same line
            for p in parts[1:]:
                if len(p) >= 2 and p[0] in "01":
                     var_id = p[1:]
                     if var_id in var_map:
                         values[var_map[var_id]] = p[0]
        elif len(line) >= 2 and line[0] in "01":
            var_id = line[1:]
            if var_id in var_map:
                name = var_map[var_id]
                old = values[name]
                new = line[0]
                values[name] = new
                if name == "/MREQ" and old == "1" and new == "0":
                    print(f"\n--- /MREQ fell at {time} ---")
                if name == "/RD" and old == "1" and new == "0":
                    d0 = values.get("D0", "X")
                    d2 = values.get("D2", "X")
                    d4 = values.get("D4", "X")
                    d6 = values.get("D6", "X")
                    clk = values.get("CLK", "X")
                    a0 = values.get("A0", "X")
                    print(f"Time {time}: /RD fell. D={d6}{d4}{d2}{d0} CLK={clk} A0={a0}")
                if name == "CLK" and old == "0" and new == "1":
                    if values.get("/MREQ") == "0":
                        d0 = values.get("D0", "X")
                        d2 = values.get("D2", "X")
                        d4 = values.get("D4", "X")
                        d6 = values.get("D6", "X")
                        rd = values.get("/RD", "X")
                        print(f"Time {time}: CLK rose while /MREQ=0. /RD={rd} D={d6}{d4}{d2}{d0}")
