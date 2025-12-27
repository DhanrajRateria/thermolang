import numpy as np

def generate_rag_thermo(filename="examples/rag_optimization.thermo", n_docs=10):
    print(f"Generating RAG Optimization (MMR) Problem for {n_docs} docs...")
    
    # 1. Simulate Document Embeddings (Relevance scores)
    # Higher score = Better match for the user query
    rng = np.random.default_rng(42)
    relevance = rng.uniform(0.1, 1.0, n_docs)
    
    # 2. Simulate Similarity Matrix (Redundancy)
    # High similarity between doc i and j means we shouldn't pick both.
    similarity = np.zeros((n_docs, n_docs))
    for i in range(n_docs):
        for j in range(i+1, n_docs):
            sim = rng.uniform(0.0, 0.5)
            # Create a "duplicate" cluster to force hard choices
            if i < 3 and j < 3: sim = 0.9 
            similarity[i][j] = sim

    with open(filename, "w") as f:
        f.write("// RAG Context Selector (Max Relevance, Min Redundancy)\n")
        f.write("energy fn rag_selector(")
        f.write(", ".join([f"doc{i}: float" for i in range(n_docs)]))
        f.write(") -> float {\n")
        f.write("    let E = 0.0;\n")
        
        # Term 1: Maximize Relevance (Minimizing -Relevance)
        # H_field = - h_i * s_i
        for i in range(n_docs):
            # We map boolean selection (0,1) to spin (-1, 1).
            # The math simplifies to standard field terms.
            f.write(f"    E = E + {-relevance[i]:.4f} * doc{i}; // Relevance reward\n")
            
        # Term 2: Minimize Redundancy (Penalty for selecting similar docs)
        # H_coupling = + J_ij * s_i * s_j (Antiferromagnetic penalty)
        lambda_redundancy = 2.0
        for i in range(n_docs):
            for j in range(i+1, n_docs):
                penalty = similarity[i][j] * lambda_redundancy
                if penalty > 0.01:
                    f.write(f"    E = E + {penalty:.4f} * doc{i} * doc{j}; // Redundancy penalty\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(rag_selector, 10.0, 0.95, 2000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_rag_thermo()