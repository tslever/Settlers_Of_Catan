# Settlers_Of_Catan

This repository includes extensive code for an application related to the game "Settlers of Catan: Cities & Knights", involving both front end and back end components. The back end part includes various components such as database interactions, game state management, artificial intelligence based gameplay with Monte Carlo Tree Search, and training data generation. The front end is focused on displaying the game layout with interactive elements for players to play the game online.

To start the back end,

1. Download Latest Microsoft Visual C++ Redistributable Version from https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version .

2. Install Microsoft Visual C++ 2015-2022 Redistributable (x64) - 14.42.34438.

3. Download the Windows (x86, 64-bit), MSI Installer for MySQL Community Server 9.2.0 Innovation from https://dev.mysql.com/downloads/mysql/ .

4. Install MySQL with setup type Typical.

5. Run MySQL 9.2 Configurator.

6. Specify a MySQL Root Password.

7. Execute configuration.

8. Change MySQL password in `cud_database/settings.py`.

9. Install Python 3.13.2.

10. Change directory to root.

11. Create a virtual environment using `python -m venv env`.

12. Activate the environment using `source env/Scripts/activate`.

13. Run `python -m pip install --upgrade pip`.

14. TODO: Create `requirements.txt`.

15. Run `python -m set_up_or_tear_down_game_database --set_up`.

16. Install [asio 1.30.2](https://think-async.com/Asio/Download.html).
    
18. Install [Crow 1.2.1.2](https://github.com/CrowCpp/Crow).

19. Install [Windows (x86, 64-bit), ZIP Archive Debug Binaries for MySQL Connector 9.2.0](https://dev.mysql.com/downloads/connector/cpp/).

20. Install [libtorch 2.6.0 for Windows, C++/Java, and CUDA 12.6](https://download.pytorch.org/libtorch/cu126/libtorch-win-shared-with-deps-debug-2.6.0%2Bcu126.zip).

21. Install [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h).

22. Clean, build, and debug back end in Visual Studio.

To start the front end,

1. Download Node.js v22.14.0 from https://nodejs.org/en .

2. Install Node.js.

3. Run `cd front_end`.

4. Run `npm install`.

5. Run `npm run dev`.
