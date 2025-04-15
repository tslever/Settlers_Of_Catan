import os
import re
from paths_of_important_files import paths_of_back_end_files
from paths_of_important_files import paths_of_configuration_files
from paths_of_important_files import paths_of_files_to_alter_database
from paths_of_important_files import paths_of_files_to_generate_board_geometry
from paths_of_important_files import paths_of_front_end_files


base_path = r'C:\Users\thoma\Documents\Settlers_Of_Catan'


def aggregate_files(base_path, relative_paths):
    aggregated_content = ""
    for relative_path in relative_paths:
        full_path = os.path.join(base_path, relative_path)
        aggregated_content += f"=== {full_path} ===\n"
        try:
            with open(full_path, 'r', encoding='utf-8') as file:
                content = file.read()
                aggregated_content += content + "\n\n"
        except FileNotFoundError:
            raise Exception(f"Error: File not found: {full_path}\n\n")
        except Exception as e:
            raise Exception(f"Error reading {full_path}: {e}\n\n")
    return aggregated_content


def replace_import_groups(content):
    pattern = r'(?m)^(import\s+.*\n)+'
    modified_content = re.sub(pattern, '...\n', content)
    return modified_content


def main():
    aggregated_string = aggregate_files(
        base_path,
        (
            paths_of_back_end_files +
            #paths_of_Python_back_end_files +
            paths_of_configuration_files +
            paths_of_files_to_alter_database +
            paths_of_files_to_generate_board_geometry +
            paths_of_front_end_files
        )
    )
    #aggregated_string = replace_import_groups(aggregated_string)
    output_path = os.path.join(base_path, 'aggregate_code', 'aggregated_contents.txt')
    try:
        with open(output_path, 'w', encoding='utf-8') as output_file:
            output_file.write(aggregated_string)
        print(f"Aggregated content successfully saved to {output_path}")
    except Exception as e:
        raise Exception(f"Failed to write aggregated content to file: {e}")


if __name__ == "__main__":
    main()