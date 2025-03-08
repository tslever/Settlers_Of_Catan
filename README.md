# Settlers_Of_Catan

This repository includes extensive code for an application related to the game "Settlers of Catan: Cities & Knights", involving both frontend and backend components. The backend part includes various components such as database interactions, game state management, artificial intelligence based gameplay with Monte Carlo Tree Search, and training data generation. The frontend is focused on displaying the game layout with interactive elements for players to play the game online.

To start the back end,

1. Download Latest Microsoft Visual C++ Redistributable Version from https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version .

2. Install Microsoft Visual C++ 2015-2022 Redistributable (x64) - 14.42.34438.

3. Download the Windows (x86, 64-bit), MSI Installer for MySQL Community Server 9.2.0 Innovation from https://dev.mysql.com/downloads/mysql/ .

4. Install MySQL with setup type Typical.

5. Run MySQL 9.2 Configurator.

6. Specify a MySQL Root Password.

7. Execute configuration.

8. Change directory to `back_end`.

9. Change MySQL password in `settings.py`.

10. Install Python 3.13.2.

11. Create a virtual environment using `python -m venv env`.

12. Activate the environment using `source env/Scripts/activate`.

10. Run `python -m pip install --upgrade pip`.

13. Run `pip install -r requirements.txt`.

14. Run `python -m back_end.db.set_up_or_tear_down_game_database --set_up`.

15. Run `cd ..`.

16. Run `python -m back_end.app`.