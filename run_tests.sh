#!/bin/bash
# run_tests.sh - Quick test runner for Patika library
# Usage: ./run_tests.sh [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default options
BUILD_DIR="build"
VERBOSE=0
CLEAN=0
SPECIFIC_TEST=""
COVERAGE=0

# Help message
show_help() {
    cat << EOF
Patika Test Runner

Usage: ./run_tests.sh [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -v, --verbose       Verbose test output
    -c, --clean         Clean build directory before building
    -t, --test NAME     Run specific test (e.g., test_agent_pool)
    -a, --all           Run all tests (default)
    -q, --quick         Run only fast unit tests
    -s, --stress        Run only stress tests
    -i, --integration   Run only integration tests
    --coverage          Enable code coverage (requires gcov)
    --valgrind          Run tests with valgrind (memory leak detection)
    --sanitize          Build with address sanitizer

EXAMPLES:
    ./run_tests.sh                      # Run all tests
    ./run_tests.sh -v                   # Verbose output
    ./run_tests.sh -t test_agent_pool   # Run specific test
    ./run_tests.sh -c                   # Clean rebuild and test
    ./run_tests.sh -q                   # Quick unit tests only
    ./run_tests.sh --valgrind           # Check for memory leaks

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -t|--test)
            SPECIFIC_TEST="$2"
            shift 2
            ;;
        -q|--quick)
            TEST_FILTER="unit"
            shift
            ;;
        -s|--stress)
            TEST_FILTER="stress"
            shift
            ;;
        -i|--integration)
            TEST_FILTER="integration"
            shift
            ;;
        --coverage)
            COVERAGE=1
            shift
            ;;
        --valgrind)
            USE_VALGRIND=1
            shift
            ;;
        --sanitize)
            USE_SANITIZER=1
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake with options
CMAKE_OPTS=""
if [ $COVERAGE -eq 1 ]; then
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_C_FLAGS='--coverage' -DCMAKE_EXE_LINKER_FLAGS='--coverage'"
fi

if [ -n "$USE_SANITIZER" ]; then
    CMAKE_OPTS+=(
        -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
    )
fi

echo -e "${YELLOW}Configuring build...${NC}"
cmake .. $CMAKE_OPTS

# Build
echo -e "${YELLOW}Building...${NC}"
cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}\n"

# Run tests
if [ ! -z "$SPECIFIC_TEST" ]; then
    # Run specific test
    echo -e "${YELLOW}Running $SPECIFIC_TEST...${NC}"
    if [ ! -z "$USE_VALGRIND" ]; then
        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./"$SPECIFIC_TEST"
    else
        ./"$SPECIFIC_TEST"
    fi
elif [ ! -z "$TEST_FILTER" ]; then
    # Run filtered tests
    echo -e "${YELLOW}Running $TEST_FILTER tests...${NC}"
    if [ $VERBOSE -eq 1 ]; then
        ctest -V -R "$TEST_FILTER"
    else
        ctest --output-on-failure -R "$TEST_FILTER"
    fi
else
    # Run all tests
    echo -e "${YELLOW}Running all tests...${NC}"
    if [ $VERBOSE -eq 1 ]; then
        ctest -V
    else
        ctest --output-on-failure
    fi
fi

# Check result
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}✓ All tests passed!${NC}"
    
    # Generate coverage report if enabled
    if [ $COVERAGE -eq 1 ]; then
        echo -e "\n${YELLOW}Generating coverage report...${NC}"
        gcov ../*.c
        lcov --capture --directory . --output-file coverage.info
        genhtml coverage.info --output-directory coverage_html
        echo -e "${GREEN}Coverage report generated in build/coverage_html/index.html${NC}"
    fi
    
    exit 0
else
    echo -e "\n${RED}✗ Some tests failed!${NC}"
    exit 1
fi
