1: This tool depends on nodejs, please install Nodejs before use, the local test environment is node V20.5.1, it is best to use the same node version
2: You need to add the compile mo tool msgfmt to the system environment PATH variable
3: If you have any problems, please contact samuel directly, thank you


How to use:
node  .\importPoFiles.js   sourcePoFilePATH   resourcePoFilePATH
sourcePoFilePATH: The folder where the po file exported from the volcano engine is located
resourcePoFilePATH: The directory where resource files exist in the project
EG:
node .\importPoFiles.js "D:\\WorkLog\\Multi-lang\\11\\AnkerMake_PC(Prusa)_k4vx5sto_20230825_100143" "D:\\WorkCode\\AnkerSlicer_P_Working\\AnkerStudio\\resources\\localization" 

After executing the statement, you need to select the import mode: as follows
*********************************************
*1: Append import po and Build Mo *
*2: Append import Po file *
*3: Override import po and Build Mo *
*4: Override import Po file *
*5: Build Mo file *
*Please enter the specific working mode: just enter the specific number 1,2,3... etc*
***************************************************** *****

Input 1: Import the new po file at the end of the existing resource po file (the newly imported part and the original file have a split comment: # import PO file from volcengine \n"), and batch build to mo file
Input 2: Same as above, just not build the Mo file
Input 3: Import the po file in an override mode, and the override just covers the part imported from the volcano Translate text
Input 4: Import the po file in an override mode, override only covers the part of the translation text imported from the volcano, and build it into mo files in batches
Input 5: Just build to mo in the project



