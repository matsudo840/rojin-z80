import sys
def dump_vcd_events(filepath):
    var_map = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('$var'):
                parts = line.split()
                if len(parts) >= 5:
                    var_map[parts[3]] = parts[4]
            elif line.startswith('#'):
                parts = line.split()
                time_str = parts[0][1:]
                print(f"\n--- Time {time_str} ---")
                for p in parts[1:]:
                    if p[0] in '01':
                        val = p[0]
                        var_id = p[1:]
                        if var_id in var_map:
                            print(f"{var_map[var_id]} = {val}")
            elif line and line[0] in '01':
                val = line[0]
                var_id = line[1:]
                if var_id in var_map:
                    print(f"{var_map[var_id]} = {val}")

if __name__ == "__main__":
    dump_vcd_events(sys.argv[1])
