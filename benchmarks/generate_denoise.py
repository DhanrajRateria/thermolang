# benchmarks/generate_denoise.py
import numpy as np
import os

def generate_denoise_thermo(filename="examples/denoise_10x10.thermo", size=10):
    print(f"Generating Image Denoising Problem ({size}x{size})...")
    
    # 1. Create a clean image (e.g., a simple cross shape)
    clean_img = -1 * np.ones((size, size))
    # Draw a plus sign
    mid = size // 2
    clean_img[mid, :] = 1
    clean_img[:, mid] = 1
    
    # 2. Add Noise (flip 20% of pixels)
    rng = np.random.default_rng(42)
    noise_mask = rng.random((size, size)) < 0.2
    noisy_img = clean_img.copy()
    noisy_img[noise_mask] *= -1
    
    # 3. Generate ThermoLang Code
    # Model: E = -J * sum(neighbors) - h * sum(input_pixel * current_spin)
    # J (smoothing) > h (fidelity) usually helps clean clusters
    J = 1.0  
    h = 2.5  
    
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    with open(filename, "w") as f:
        f.write(f"// Image Denoising {size}x{size}\n")
        f.write(f"// J={J} (Smoothing), h={h} (Data Fidelity)\n\n")
        
        # Function signature
        params = [f"s{i}: float" for i in range(size*size)]
        f.write("energy fn denoise_model(\n    ")
        f.write(",\n    ".join(params))
        f.write("\n) -> float {\n")
        f.write("    let E = 0.0;\n")
        
        # A. Smoothing Terms (Coupling between neighbors)
        count_J = 0
        for r in range(size):
            for c in range(size):
                idx = r * size + c
                # Right neighbor
                if c + 1 < size:
                    idx_right = r * size + (c + 1)
                    f.write(f"    E = E + {-J} * s{idx} * s{idx_right};\n")
                    count_J += 1
                # Bottom neighbor
                if r + 1 < size:
                    idx_down = (r + 1) * size + c
                    f.write(f"    E = E + {-J} * s{idx} * s{idx_down};\n")
                    count_J += 1
        
        f.write(f"\n    // {count_J} smoothing couplings added.\n\n")

        # B. Data Fidelity Terms (Local Fields)
        # We want spin s_i to align with noisy_pixel_i.
        # Energy is lower if s_i == noisy_img[i].
        # Term: -h * noisy_pixel * s_i
        for i in range(size*size):
            r, c = divmod(i, size)
            pixel_val = noisy_img[r, c]
            bias = -1.0 * h * pixel_val
            f.write(f"    E = E + {bias:.4f} * s{i};\n")
            
        f.write("    return E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        f.write("    // Run annealing\n")
        f.write("    let res = thermal_anneal(denoise_model, 5.0, 0.95, 2000);\n")
        f.write("}\n")

    print(f"Done. File written to {filename}")
    
    # Save the target image for comparison later
    np.save(filename.replace(".thermo", "_target.npy"), clean_img)
    np.save(filename.replace(".thermo", "_noisy.npy"), noisy_img)

if __name__ == "__main__":
    generate_denoise_thermo()