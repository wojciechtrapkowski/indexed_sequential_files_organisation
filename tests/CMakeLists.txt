# Find Python for test script
find_package(Python3 REQUIRED)

# Create test script
# Create test script
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/run_test.py "" "
import sys
import subprocess
import filecmp
import os
import difflib

def run_test(test_number):
    # Paths
    input_file = f'test_{test_number}.txt'
    output_file = f'test_{test_number}_output.txt'
    expected_file = f'test_{test_number}_expected.txt'
    
    # Clean database files before each test
    subprocess.run(['${CMAKE_BINARY_DIR}/SBD_2', '--clean'])
    
    # Run the program
    with open(output_file, 'w') as f:
        subprocess.run(['${CMAKE_BINARY_DIR}/SBD_2', input_file], stdout=f)
    
    # Compare files
    if filecmp.cmp(output_file, expected_file):
        print(f'Test {test_number} passed!')
        return 0
    else:
        print(f'Test {test_number} failed!')
        print('Diff:')
        
        # Read both files
        with open(output_file, 'r') as f1, open(expected_file, 'r') as f2:
            output_lines = f1.readlines()
            expected_lines = f2.readlines()
        
        # Generate diff
        diff = difflib.unified_diff(
            expected_lines, 
            output_lines,
            fromfile='Expected',
            tofile='Got',
            lineterm=''
        )
        
        # Print diff
        for line in diff:
            print(line)
        return 1

if __name__ == '__main__':
    sys.exit(run_test(int(sys.argv[1])))
" "")

# Function to add test
function(add_parser_test test_number)
    add_test(
        NAME ParserTest_${test_number}
        COMMAND Python3::Interpreter ${CMAKE_CURRENT_BINARY_DIR}/run_test.py ${test_number}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests
    )
endfunction()

add_parser_test(0)
add_parser_test(1)
add_parser_test(2)
add_parser_test(3)
add_parser_test(4)
add_parser_test(5)
add_parser_test(6)
add_parser_test(7)
add_parser_test(8)