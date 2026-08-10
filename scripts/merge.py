import os

directories = [
    "src",
    "include",
    "tests"
]
output_file = "merged.txt"

with open(output_file, 'w', encoding='utf-8') as out:
    for dir_path in directories:
        if os.path.exists(dir_path):
            for root, _, files in os.walk(dir_path):
                for file in files:
                    file_path = os.path.join(root, file)
                    try:
                        out.write(f"\n=== {file_path} ===\n")
                        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                            out.write(f.read())
                    except:
                        pass
