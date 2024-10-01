import re
import sys

# Regular expression pattern for a valid line
# The expected CSV format is: CATEGORY,ID,VALUE1,START,END
pattern = re.compile(r"^(COMMUNICATION|COMPUTATION|ORCHESTRATION|SETUP),r2i\d+n\d+,\d+,\d+\.\d+,\d+\.\d+$")

def fix_csv(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        for line in infile:
            # Split line by all known categories to handle multiple entries in one line
            possible_entries = re.split(r"(COMMUNICATION|COMPUTATION|ORCHESTRATION|SETUP)", line.strip())
            
            # If the first item is empty, remove it (it happens when splitting at the beginning)
            if possible_entries[0] == '':
                possible_entries = possible_entries[1:]
            
            # Combine categories with their respective entries
            for i in range(0, len(possible_entries) - 1, 2):
                category = possible_entries[i]
                entry = possible_entries[i + 1]
                
                # Reconstruct the entry
                full_entry = category + entry
                
                # If the reconstructed entry matches the expected pattern, write it to the output
                if pattern.match(full_entry.strip()):
                    outfile.write(full_entry.strip() + '\n')
                else:
                    # Attempt to handle broken records
                    parts = re.split(r"(COMMUNICATION|COMPUTATION|ORCHESTRATION|SETUP)", full_entry)
                    if len(parts) > 1:
                        for j in range(1, len(parts), 2):
                            rebuilt_entry = parts[j] + parts[j + 1]
                            if pattern.match(rebuilt_entry.strip()):
                                outfile.write(rebuilt_entry.strip() + '\n')

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python fix_csv.py <input_file> <output_file>")
        sys.exit(1)
    
    input_csv = sys.argv[1]
    output_csv = sys.argv[2]
    
    fix_csv(input_csv, output_csv)
