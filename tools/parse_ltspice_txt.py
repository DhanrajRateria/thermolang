#!/usr/bin/env python3
"""
LTSPICE Text Log Parser for ThermoLang
"""
import sys
import re

def parse_ltspice_log(filename):
    """Parse LTSPICE log file to extract node voltages"""
    try:
        with open(filename, 'r') as f:
            content = f.read()
        
        # Find all node voltages in the final timestep
        voltages = {}
        
        # Look for the last "Timestep" or "Time point" line
        time_matches = list(re.finditer(r'(Timestep|Time point).*?:\s*([\d\.e\-+]+)', content))
        if not time_matches:
            print("No timestep information found in log")
            return None
            
        last_time = time_matches[-1]
        last_time_pos = last_time.end()
        
        # Find all node voltage lines after this position
        node_pattern = r'V\(([Nn]\d+)\):\s*([\d\.\-+e]+)V'
        voltage_matches = re.finditer(node_pattern, content[last_time_pos:])
        
        for match in voltage_matches:
            node = match.group(1)
            voltage = float(match.group(2))
            voltages[node] = voltage
        
        # Convert voltages to spins (+1 for positive voltage, -1 for negative)
        spins = []
        for i in range(len(voltages)):
            node = f"N{i}"
            if node in voltages:
                spins.append(1 if voltages[node] > 0 else -1)
            else:
                node = f"n{i}"  # Try lowercase
                if node in voltages:
                    spins.append(1 if voltages[node] > 0 else -1)
                else:
                    print(f"Warning: No voltage found for node {i}")
                    spins.append(0)  # Unknown state
        
        print(f"Found {len(spins)} spin states:")
        print(f"[FINAL_STATE]: {spins}")
        return spins
    
    except Exception as e:
        print(f"Error parsing LTSPICE log: {e}")
        return None

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_ltspice_txt.py <ltspice_log_file>")
        sys.exit(1)
    
    parse_ltspice_log(sys.argv[1])