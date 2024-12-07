#!/usr/bin/env python3
import sys
import os


def create_test_files(test_number):
    # Create test directory if it doesn't exist
    test_dir = "tests"
    os.makedirs(test_dir, exist_ok=True)

    # Create input test file
    with open(f"{test_dir}/test_{test_number}.txt", "w") as f:
        f.write("insert 1 100\n")
        f.write("search 1\n")
        f.write("print\n")

    # Create expected output file
    with open(f"{test_dir}/test_{test_number}_expected.txt", "w") as f:
        f.write("Inserted: 1 -> 100\n")
        f.write("Found: 1 -> 100\n")
        f.write("Page 0: [1 -> 100]\n")

    # Create empty output file
    with open(f"{test_dir}/test_{test_number}_output.txt", "w") as f:
        pass

    print(f"Created test files for test {test_number}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_test.py <test_number>")
        sys.exit(1)

    try:
        test_number = int(sys.argv[1])
        create_test_files(test_number)
    except ValueError:
        print("Error: test_number must be an integer")
        sys.exit(1)
