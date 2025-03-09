import os
import re


base_path = r'C:\Users\thoma\Documents\Settlers_Of_Catan'
file_paths = [
    r'.gitignore',

    r'back_end\back_end.sln',
    r'back_end\back_end\back_end.vcxproj',
    r'back_end\back_end\back_end.vcxproj.filters',
    r'back_end\back_end\back_end.vcxproj.user',

    r'back_end\back_end\back_end.cpp',

    r'back_end\back_end\mcts_node.hpp',
    r'back_end\back_end\neural_network.hpp',

    r'back_end\back_end\database.hpp',
    r'back_end\back_end\models.hpp',

    r'back_end\back_end\game_state.hpp'
]
file_paths = [
    r'Python_back_end\ai\mcts\__init__.py',
    r'Python_back_end\ai\mcts\backpropagation.py',
    r'Python_back_end\ai\mcts\expansion.py',
    r'Python_back_end\ai\mcts\node.py',
    r'Python_back_end\ai\mcts\selection.py',
    r'Python_back_end\ai\mcts\simulation.py',

    r'Python_back_end\ai\__init__.py',
    r'Python_back_end\ai\continuous_training.py',
    r'Python_back_end\ai\io_helper.py',
    r'Python_back_end\ai\neural_network.py',
    r'Python_back_end\ai\self_play.py',
    r'Python_back_end\ai\strategy.py',
    r'Python_back_end\ai\train.py',

    r'Python_back_end\db\__init__.py',
    r'Python_back_end\db\change_password.py',
    r'Python_back_end\db\database.py',
    r'Python_back_end\db\set_up_or_tear_down_game_database.py',

    r'Python_back_end\game\__init__.py',
    r'Python_back_end\game\game_actions.py',
    r'Python_back_end\game\game_state.py',

    r'Python_back_end\routes\__init__.py',
    r'Python_back_end\routes\cities.py',
    r'Python_back_end\routes\next.py',
    r'Python_back_end\routes\reset.py',
    r'Python_back_end\routes\roads.py',
    r'Python_back_end\routes\root.py',
    r'Python_back_end\routes\settlements.py',

    r'Python_back_end\__init__.py',
    r'Python_back_end\app.py',
    r'Python_back_end\board.py',
    r'Python_back_end\logger.py',
    r'Python_back_end\phase_state_machine.py',
    r'Python_back_end\phase.py',
    r'Python_back_end\requirements.txt',
    r'Python_back_end\settings.py',

    r'front_end\app\components\HexTile.tsx',
    r'front_end\app\components\Marker.tsx',
    r'front_end\app\components\MarkerComponents.tsx',
    r'front_end\app\components\Ocean.tsx',
    r'front_end\app\components\Port.tsx',
    r'front_end\app\components\QueryBoundary.tsx',
    r'front_end\app\components\Vertex.tsx',

    r'front_end\app\hooks\useCentralQuery.ts',

    r'front_end\app\api.ts',
    r'front_end\app\board.ts',
    r'front_end\app\BoardLayout.tsx',
    r'front_end\app\CanvasLayer.tsx',
    r'front_end\app\globals.scss',
    r'front_end\app\layout.tsx',
    r'front_end\app\page.tsx',
    r'front_end\app\Providers.tsx',
    r'front_end\app\types.ts',

    r'board_geometry.json',
    r'generate_board_geometry.py'
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