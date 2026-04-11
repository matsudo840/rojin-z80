import sys

def analyze_vcd_for_z80(filepath):
    """
    Parses a VCD file from a logic analyzer and prints out the state
    of the Z80 bus whenever the /MREQ signal goes LOW (Memory Request).
    """
    var_map = {}
    state = {}
    current_time = -1
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            if line.startswith('$var'):
                parts = line.split()
                if len(parts) >= 5:
                    var_id = parts[3]
                    var_name = parts[4]
                    var_map[var_id] = var_name
                    state[var_id] = 'X' 
            elif line.startswith('#'):
                parts = line.split()
                time_str = parts[0][1:]
                current_time = int(time_str)
                
                # Check for state changes on the same line
                for p in parts[1:]:
                    if p[0] in '01':
                        val = p[0]
                        var_id = p[1:]
                        if var_id in var_map:
                            state[var_id] = val
                            
                            var_name = var_map[var_id]
                            # Print bus state when /MREQ goes LOW
                            if var_name == '/MREQ' and val == '0':
                                print_bus_state(state, var_map, current_time)
                            
            elif line[0] in '01':
                val = line[0]
                var_id = line[1:]
                if var_id in var_map:
                    state[var_id] = val
                    var_name = var_map[var_id]
                    
                    # Print bus state when /MREQ goes LOW
                    if var_name == '/MREQ' and val == '0':
                        print_bus_state(state, var_map, current_time)

def print_bus_state(state, var_map, current_time):
    mreq_id = next((k for k, v in var_map.items() if v == '/MREQ'), None)
    rd_id = next((k for k, v in var_map.items() if v == '/RD'), None)
    clk_id = next((k for k, v in var_map.items() if v == 'CLK'), None)
    a0_id = next((k for k, v in var_map.items() if v == 'A0'), None)
    d3_id = next((k for k, v in var_map.items() if v == 'D3'), None)
    d2_id = next((k for k, v in var_map.items() if v == 'D2'), None)
    d1_id = next((k for k, v in var_map.items() if v == 'D1'), None)
    d0_id = next((k for k, v in var_map.items() if v == 'D0'), None)
    
    rd_val = state.get(rd_id, 'X') if rd_id else 'X'
    clk_val = state.get(clk_id, 'X') if clk_id else 'X'
    a0_val = state.get(a0_id, 'X') if a0_id else 'X'
    d3_val = state.get(d3_id, 'X') if d3_id else 'X'
    d2_val = state.get(d2_id, 'X') if d2_id else 'X'
    d1_val = state.get(d1_id, 'X') if d1_id else 'X'
    d0_val = state.get(d0_id, 'X') if d0_id else 'X'

    print(f"Time: {current_time}us - /MREQ LOW | /RD: {rd_val} | CLK: {clk_val} | A0: {a0_val} | D[3:0]: {d3_val}{d2_val}{d1_val}{d0_val}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 parse_vcd_fetch.py <file.vcd>")
        sys.exit(1)
    analyze_vcd_for_z80(sys.argv[1])
