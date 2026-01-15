# benchmarks/visualize_denoise.py
import numpy as np
import matplotlib.pyplot as plt
import sys
import re

def parse_output_and_plot(sim_output_file, data_prefix):
    # Load ground truth
    try:
        clean = np.load(data_prefix + "_target.npy")
        noisy = np.load(data_prefix + "_noisy.npy")
        size = clean.shape[0]
    except:
        print("Error: Could not load .npy data files. Run generate_denoise.py first.")
        return

    # Parse simulation output
    recovered = None
    with open(sim_output_file, 'r') as f:
        content = f.read()
        match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", content)
        if match:
            vals = [int(x) for x in match.group(1).split(',')]
            if len(vals) == size*size:
                recovered = np.array(vals).reshape((size, size))
    
    if recovered is None:
        print("Error: Could not parse FINAL_STATE from simulation output.")
        return

    # Plot
    fig, ax = plt.subplots(1, 3, figsize=(12, 4))
    
    ax[0].imshow(clean, cmap='gray', vmin=-1, vmax=1)
    ax[0].set_title("Original")
    ax[0].axis('off')
    
    ax[1].imshow(noisy, cmap='gray', vmin=-1, vmax=1)
    ax[1].set_title("Noisy Input")
    ax[1].axis('off')
    
    ax[2].imshow(recovered, cmap='gray', vmin=-1, vmax=1)
    ax[2].set_title("Restored (ThermoLang)")
    ax[2].axis('off')
    
    plt.savefig("denoise_result.png")
    print("Result saved to denoise_result.png")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python visualize_denoise.py <sim_output.txt> <data_file_prefix>")
    else:
        parse_output_and_plot(sys.argv[1], sys.argv[2])