# Contributing to ThermoLang

Thank you for your interest in contributing to ThermoLang! We're excited to build a community around this project and advance the field of thermodynamic computing together. Whether you're interested in language design, compiler optimization, new hardware backends, or documentation, **your contributions are welcome**.

---

## 🎯 Ways to Contribute

We welcome contributions in many forms:

- 🐛 **Bug Reports** - Help us identify and fix issues
- ✨ **Feature Requests** - Suggest new capabilities
- 💻 **Code Contributions** - Implement features or fix bugs
- 📝 **Documentation** - Improve guides, tutorials, and examples
- 🧪 **Testing** - Add test cases and improve coverage
- 🎨 **Examples** - Create new `.thermo` example programs
- 🔧 **Backend Targets** - Add support for new hardware platforms
- ⚡ **Optimization Passes** - Improve compiler performance
- 🎓 **Educational Content** - Write tutorials or teaching materials

---

## 🚀 Getting Started

### Quick Contribution Workflow

1. **Find or Create an Issue**
   - Browse [open issues](https://github.com/DhanrajRateria/thermolang/issues)
   - Look for issues tagged `good-first-issue` or `help-wanted`
   - Have a new idea? [Open an issue](https://github.com/DhanrajRateria/thermolang/issues/new) first to discuss it

2. **Fork the Repository**
   ```bash
   # Click the "Fork" button on GitHub, then clone your fork
   git clone https://github.com/YOUR-USERNAME/thermolang.git
   cd thermolang
   ```

3. **Create a Feature Branch**
   ```bash
   # Use a descriptive branch name
   git checkout -b feature/new-optimizer
   # or
   git checkout -b fix/parser-bug
   # or
   git checkout -b docs/improve-readme
   ```

4. **Make Your Changes**
   - Write clean, well-documented code
   - Follow our [coding standards](#coding-style)
   - Test your changes thoroughly

5. **Commit Your Work**
   ```bash
   git add .
   git commit -m "Add feature: description of your changes"
   ```

6. **Push and Submit a Pull Request**
   ```bash
   git push origin feature/new-optimizer
   ```
   Then open a Pull Request on GitHub with:
   - Clear description of changes
   - Reference to related issues (e.g., "Fixes #123")
   - Screenshots or examples if applicable

---

## 🛠️ Development Setup

### Prerequisites

Ensure you have the following installed:

- **C++17 Compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
- **CMake** 3.14 or higher
- **Python** 3.8 or higher
- **Git**

### Building from Source

```bash
# 1. Clone the repository
git clone https://github.com/DhanrajRateria/thermolang.git
cd thermolang

# 2. Configure for development (includes tests and debug symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# 3. Build the project
cmake --build build --parallel

# 4. Run the test suite to verify your setup
cd build
ctest --verbose
```


---

## ✅ Contribution Guidelines

### Before You Submit

- [ ] **Code compiles** without warnings
- [ ] **All tests pass** (`ctest` in build directory)
- [ ] **New features have tests** in `/tests` directory
- [ ] **Documentation is updated** (README, docs/, inline comments)
- [ ] **Code follows style guide** (see below)
- [ ] **Commit messages are clear** and descriptive
- [ ] **Branch is up to date** with `main`

### Pull Request Process

1. **Ensure CI Passes**: All automated checks must pass
2. **Request Review**: Tag relevant maintainers for review
3. **Address Feedback**: Be responsive to review comments
4. **Squash Commits**: Clean up commit history if requested
5. **Celebrate**: Once merged, you're officially a contributor! 🎉

### Writing Tests

All new features **must** include tests. Place test files in the `/tests` directory.

Example test structure:

```cpp
// tests/test_new_feature.cpp
#include <gtest/gtest.h>
#include "your_feature.h"

TEST(FeatureTest, BasicFunctionality) {
    // Arrange
    auto feature = YourFeature();
    
    // Act
    auto result = feature.process(input);
    
    // Assert
    EXPECT_EQ(result, expected_output);
}
```

Run tests with:

```bash
cd build
ctest --verbose
# or run specific tests
./tests/test_new_feature
```

---

## 📝 Coding Style

Consistent code style helps maintain readability and makes collaboration easier.

### C++ Style Guidelines

We follow a style similar to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html):

#### Naming Conventions

```cpp
// Variables and functions: snake_case
int variable_name;
void function_name() { }

// Classes and structs: PascalCase
class MyClass { };
struct DataStructure { };

// Constants: UPPER_SNAKE_CASE
const int MAX_ITERATIONS = 1000;

// Private members: trailing underscore
class Example {
private:
    int private_member_;
};
```

#### Code Organization

```cpp
// Use header guards
#ifndef THERMOLANG_MODULE_NAME_H
#define THERMOLANG_MODULE_NAME_H

// Include order: C system, C++ standard, other libraries, project headers
#include <cstdint>
#include <string>
#include <vector>

#include "thermolang/common.h"

// Namespace usage
namespace thermolang {

// Document complex logic with comments
class Parser {
public:
    // Brief description of what this does
    void parse_expression() {
        // Explain non-obvious implementation details
        // ...
    }
};

} // namespace thermolang

#endif // THERMOLANG_MODULE_NAME_H
```

#### Best Practices

- **Use RAII**: Leverage constructors/destructors for resource management
- **Prefer `const`**: Make variables `const` by default
- **Smart Pointers**: Use `unique_ptr` and `shared_ptr` over raw pointers
- **Modern C++**: Take advantage of C++17 features when appropriate
- **Error Handling**: Use exceptions for exceptional cases

### Python Style Guidelines

Follow [PEP 8](https://peps.python.org/pep-0008/) standards:

```python
# Variables and functions: snake_case
variable_name = 42
def function_name():
    pass

# Classes: PascalCase
class MySimulator:
    pass

# Constants: UPPER_SNAKE_CASE
MAX_TEMPERATURE = 10.0

# Use docstrings
def simulate_annealing(schedule):
    """
    Performs simulated annealing with the given schedule.
    
    Args:
        schedule: AnnealingSchedule object defining temperature progression
        
    Returns:
        Final system state after annealing
    """
    pass

# Type hints when appropriate
def process_energy(value: float) -> float:
    return value * 2.0
```

### Documentation Style

- **Inline Comments**: Explain *why*, not *what*
- **Function Comments**: Describe purpose, parameters, return values
- **Complex Algorithms**: Add high-level explanation before implementation
- **TODO Comments**: Use `// TODO(username): Description` format

---

## 🏷️ Issue Labels

We use labels to organize issues and PRs:

| Label | Description |
|-------|-------------|
| `good-first-issue` | Great for newcomers |
| `help-wanted` | Seeking contributors |
| `bug` | Something isn't working |
| `enhancement` | New feature or request |
| `documentation` | Improvements to docs |
| `backend` | Related to code generation |
| `optimizer` | Compiler optimization |
| `language` | Language design |
| `performance` | Speed or efficiency |
| `breaking-change` | Requires version bump |

---

## 💡 Contribution Ideas

Not sure where to start? Here are some areas that need attention:

### Beginner-Friendly

- Add more example programs in `/examples`
- Improve error messages in the compiler
- Write unit tests for existing features
- Fix typos or improve documentation clarity
- Add comments to complex code sections

### Intermediate

- Implement new optimization passes
- Add support for additional physical models
- Create benchmark problems
- Improve compiler diagnostics
- Extend the standard library

### Advanced

- Design new backend targets (e.g., GPU, custom ASICs)
- Implement advanced type checking
- Add parallelization support
- Create IDE integration (LSP server)
- Develop circuit layout optimization

---

## 🤝 Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inspiring community for all. Please be respectful and considerate in all interactions.

### Expected Behavior

- Be respectful of differing viewpoints
- Accept constructive criticism gracefully
- Focus on what's best for the community
- Show empathy toward other community members

### Unacceptable Behavior

- Harassment, discriminatory language, or personal attacks
- Trolling, insulting comments, or disruptive behavior
- Publishing others' private information
- Any conduct inappropriate for a professional setting

---

## 📞 Getting Help

Stuck? Need clarification? We're here to help!

- **Questions**: Open a [Discussion](https://github.com/DhanrajRateria/thermolang/discussions)
- **Bugs**: File an [Issue](https://github.com/DhanrajRateria/thermolang/issues)
- **Security**: Email security concerns privately to [maintainer email]
- **Chat**: Join our community [Discord/Slack] (if available)

---

## 🎓 Learning Resources

New to compiler development or thermodynamic computing? Check out these resources:

- **Compiler Design**: *Crafting Interpreters* by Robert Nystrom
- **Thermodynamic Computing**: Extropic's website
- **C++ Best Practices**: *Effective Modern C++* by Scott Meyers
- **CMake**: [Modern CMake Guide](https://cliutils.gitlab.io/modern-cmake/)

---

## 🙏 Recognition

All contributors will be recognized in our:

- `CONTRIBUTORS.md` file
- Release notes for their contributions
- Project documentation (for significant contributions)

---

## 📜 License

By contributing to ThermoLang, you agree that your contributions will be licensed under the [MIT License](LICENSE).

---

<div align="center">
  <p><strong>Thank you for helping make ThermoLang better!</strong></p>
  <p>We look forward to your contributions! 🚀</p>
</div>