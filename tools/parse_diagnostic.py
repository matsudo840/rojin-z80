import sys
def parse_vcd_for_signals(filepath, signals):
    var_map = {}
    current_time = 0
    events = []
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            if line.startswith('$var'):
                parts = line.split()
                if len(parts) >= 5:
                    var_id = parts[3]
                    var_name = parts[4]
                    if var_name in signals:
                        var_map[var_id] = var_name
            elif line.startswith('#'):
                parts = line.split()
                current_time = int(parts[0][1:])
                for p in parts[1:]:
                    if p[0] in '01':
                        val = int(p[0])
                        var_id = p[1:]
                        if var_id in var_map:
                            events.append((current_time, var_map[var_id], val))
            elif line[0] in '01':
                val = int(line[0])
                var_id = line[1:]
                if var_id in var_map:
                    events.append((current_time, var_map[var_id], val))
                    
    # Summarize events per signal
    summary = {s: 0 for s in signals}
    for e in events:
        summary[e[1]] += 1
        
    print("Toggle count per signal in VCD:")
    for s, count in summary.items():
        print(f"{s}: {count} toggles")
        if count == 0:
            print(f"  WARNING: {s} never changed state!")

if __name__ == "__main__":
    parse_vcd_for_signals(sys.argv[1], ["CLK", "WAIT", "RESET", "MREQ", "RD", "WR"])
