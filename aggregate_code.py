import os
import re


base_path = r'C:\Users\thoma\Documents\Settlers_Of_Catan'
file_paths = [
    r'generate_board_geometry.py',
    r'board_geometry.json',
    r'back_end\__init__.py',
    r'back_end\ai\__init__.py',
    r'back_end\ai\mcts_node.py',
    r'back_end\ai\neural_network.py',
    r'back_end\ai\self_play.py',
    r'back_end\ai\strategy.py',
    r'back_end\app.py',
    r'back_end\config.py',
    r'back_end\db\database.py',
    r'back_end\routes\next.py',
    r'back_end\routes\roads.py',
    r'back_end\routes\root.py',
    r'back_end\routes\settlements.py',
    r'back_end\routes\__init__.py',
    r'back_end\utilities\__init__.py',
    r'back_end\utilities\board.py',
    r'front_end\app\config.ts',
    r'front_end\app\globals.scss',
    r'front_end\app\page.tsx',
    r'front_end\app\components\HexTile.tsx',
    r'front_end\app\components\Ocean.tsx',
    r'front_end\app\components\Port.tsx',
    r'front_end\app\components\SettlementMarker.tsx',
    r'front_end\app\components\Vertex.tsx',
    r'front_end\app\hooks\useApi.ts',
    r'front_end\app\utilities\board.ts'
]


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
    aggregated_string = aggregate_files(base_path, file_paths)
    #aggregated_string = replace_import_groups(aggregated_string)
    output_path = os.path.join(base_path, 'aggregated_contents.txt')
    try:
        with open(output_path, 'w', encoding='utf-8') as output_file:
            output_file.write(aggregated_string)
        print(f"Aggregated content successfully saved to {output_path}")
    except Exception as e:
        raise Exception(f"Failed to write aggregated content to file: {e}")


if __name__ == "__main__":
    main()