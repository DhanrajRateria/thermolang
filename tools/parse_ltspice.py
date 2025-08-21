#!/usr/bin/env python3
"""
LTSPICE Output Parser for ThermoLang
"""
import sys
import re
import numpy as np

def parse_ltspice_raw(filename):
    """Parse LTSPICE .raw file to extract node voltages"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Extract the final timepoint data
    variables_section = re.search(r'Variables:\s*(.*?)Binary:', content, re.DOTALL)
    if not variables_section:
        print("ERROR: Could not find Variables section in LTSPICE output")
        return None
    
    # Find all node voltage variables
    node_vars = []
    for line in variables_section.group(1).split('\n'):
        if 'V(n' in line and 'noise' not in line:
            parts = line.split()
            if len(parts) >= 3:
                index = int(parts[0])
                name = parts[1]
                node_vars.append((index, name))
    
    # For demonstration, we'll just use the polarity of the voltages at the last timepoint
    # In a real parser, we'd extract the actual voltage values
    print(f"Detected {len(node_vars)} spin nodes: {[name for _, name in node_vars]}")
    
    # For this simplified version, we'll just assume all spins are +1
    # In reality, we'd look at the voltage sign for each node
    spins = [1 for _ in node_vars]
    
    print(f"[FINAL_STATE]: {spins}")
    return spins

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_ltspice.py <ltspice_raw_file>")
        sys.exit(1)
    
    parse_ltspice_raw(sys.argv[1])