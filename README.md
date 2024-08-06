# CEN3031 Library Manager Project
## Installation Instructions
1. Download and run the OpenSSL installer from https://slproweb.com/products/Win32OpenSSL.html.
Use the “Win64 OpenSSL v3.3.1” version (not the “Light” version).
2. Download MySQL and MySQL Workbench (https://dev.mysql.com/downloads/installer/). MySQL is for the database and MySQL Workbench provides an interface to debug the database. I followed this tutorial.
3. Using MySQL Workbench, import the database.sql file from GitHub. You do this by going to Server->Data Import, then selecting “Import from Self-Contained file” and selecting the database.sql file.
4. Open up the “CEN3031 Project.sln” file. In the solution explorer, right-click on the project and go to Properties->Configuration Properties->C/C++->General. In the field that says “Additional Include Directories”, paste this string: C:\Program Files\OpenSSL-Win64\include;..\mysql-connector-c++-9.0.0-winx64\include\jdbc;../imgui;../imgui/backends;%(AdditionalIncludeDirectories)
5. Go to Properties->Configuration Properties->C/C++->Preprocessor. Go the the field that says “Preprocessor Definitions” and paste this string: STATIC_CONCPP;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)
6. Go to Properties->Configuration Properties->Linker->General. Go to the field that says “Additional Library Directories” and paste this string: ..\mysql-connector-c++-9.0.0-winx64\lib64\vs14;$(DXSDK_DIR)/Lib/x64;%(AdditionalLibraryDirectories) without the quotes.
7. Go to Properties->Configuration Properties->Linker->Input. Go the the field that says “Additional Dependencies” and paste this string: urlmon.lib;mysqlcppconn-static.lib;d3d11.lib;d3dcompiler.lib;dxgi.lib;kernel32.lib;user32.lib;gdi32.lib;winspool.lib;comdlg32.lib;advapi32.lib;shell32.lib;ole32.lib;oleaut32.lib;uuid.lib;odbc32.lib;odbccp32.lib;%(AdditionalDependencies)
8. Make sure you are on Release Mode x64 on the Configuration Manager.
