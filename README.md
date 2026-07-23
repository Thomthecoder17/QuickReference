# QuickReference
This project is designed for an ESP32 based board with an st7735 based TFT display.

## Configuration
- Configuration is hangled through menuconfig. To access this, run `idf.py menuconfig` prior to flashing code and navigate to Component Config > QuickReference Configuration. Make sure to source IDF's export script for your operating system prior to running this. In VSCode with the IDF extension, this entire step can be done by clicking on the gear icon on the status bar, then navigating.
    - Defaults:  
        - Wi-Fi configuration does not have defaults
        - LCD Configuration:
            -  LCD CS Pin = 34  
            -  LCD DC Pin = 37  
            -  LCD SCLK Pin = 36  
            -  LCD MOSI Pin = 35  
            -  LCD MISO Pin = -1  
            -  LCD RST Pin = 38 
            -  LCD Horizontal Resolution = 128  
            -  LCD Vertical Resolution = 160
            -  LCD Color Depth = 16  
            -  LCD Pixel Clock Frequency = 30000000 (30 MHz) 
        - Weather API Configuration:
            - Weather API URL = "https://api.weather.gov/"
            - Weather Station ID = "BHBM3" (Boston, MA)
            - Require Quality Control = true
        - Transit API Configuration
            - Transit API URL = "https://api-v3.mbta.com/" (MBTA, Boston MA)
            - Transit Stop ID = "place-pktrm" (Park St. Station)
            - Transit Route ID = "Red" (Red Line)
    - Modify this to work for your application
### After this, you are good to build and flash the firmware!

## Additional Notes:
- For boarda with limited DIRAM, lower LVGL's memory allocation to 24 instead of 48kB in menuconfig.
- If low on flash storage, set the partition table to the custom csv at partitions.csv. This is designed for 2MB flash boards.
- ### Currently, the transit side is not implemented. You can ignore configuration for now. The weather side does work.
- Bill of materials also substitutes many components. The components I used were gathered from old projects and from a local makerspace, so many were discontinued.
    - Notable changes:
        - ESP32-S2-Saola-1M was replaced by ESP32-S2-DevKitC-1-N8R2. I believe this was a similar price to my board, and it seems to actually have more flash storage and RAM.
        - SainSmart 1.8" TFT SPI LCD Screen was replaced with a HiLetgo 1.8" TFT LCD. They both use the exact same contorller driver, so it should be a direct replacement.

## Known Bugs:
- Weather.gov will sometimes report null data. In testing, this was not handled properly, resulting in an inaccurate prediction of 32F (0C).
