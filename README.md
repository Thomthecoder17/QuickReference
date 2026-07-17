# QuickReference
- Run `idf.py menuconfig` prior to flashing code, and in Component Config > QuickReference Configuration set Wi-Fi information and LCD pins. Make sure to source IDF's export script for your operating system prior to running this. In VSCode with the IDF extension, this can be done by clicking on the gear icon on the status bar. 
- Build and flash the project with ESP-IDF (fire icon in VSCode status bar)
## Additional Notes:
- For boarda with limited DIRAM, lower LVGL's memory allocation to 24 instead of 48kB in menuconfig.
- If low on flash storage, set the partition table to the custom csv at partitions.csv. This is designed for 2MB flash boards.