# July 5, 2026
- Downloaded and set up ESP-IDF to develop a new project.
- Came across an LCD at the makerspace which I wanted to use for the project.

# July 6, 2026
- Created a project in ESP-IDF
- Found a library for the st7735 TFT screen
    - Actually found 2, but the first one had very weak documentation
- Began to learn how to set up the TFT LCD to display stuff. This took a while, as I had never used ESP-IDF before, and was not prepared for the complexity of setting up the LCD.
    - As part of this process, I also learned how the SPI bus works, so I could better debug this in the future.

# July 7, 2026
- https://lapse.hackclub.com/timelapse/b8yl2glTMUOq 
- Searched for ways of displaying text on the screen. Ended up landing on LVGL, a lightweight graphics library, to display content. 
- Wrote a file to initialize the LCD for future use by LVGL
- Also created a new file to hold the future weather API
- Tries to get LVGL's demos to work
    - This, unfortunately, was a huge pain, as I kept running out of RAM or was unable to run the demos for other reasons.
    - Was able to get a small bit of text displaying "Hi :3" on the screen, although the colors were very messed up.

# July 8, 2026
- Created a file to handle LVGL initialization and diaplaying content
- Worked on migrating LVGL initialization out of lcd_init
    - I did still have to leave some of the lvgl_port stuff in lcd_init, which took some time to figure out.
- Finally got the demo widget to "work"
    - Colors were still messed up.

# July 9, 2026
- https://lapse.hackclub.com/timelapse/mLdYgdqiPT_5 
- Finally got text to display once again on the screen, with the newer modular system (not everything is in main anymore!)
- Also tried to get Figma working to design stuff, but unfortunately could not. 
- Spent a while trying to figure out how to even use LVGL, plus a lot of debugging errors.

# July 14, 2026
- https://lapse.hackclub.com/timelapse/MWtmRektYdO9 
- Created a new file to initialize and connect to Wi-Fi. This took forever with tons of bugs.
- Ran out of RAM and flash storage (again)
    - Re-partitioned the flash storage to allocate more space to the code
- Tried to fugure out the weird colors on the screen. 
    - I tried using a logic analyzer to figure out what was being sent to the screen to debug it. Ended up being that I forgot to tell LVGL to swap bytes, causing the inverted colors. This took me a painfully long time to find.
- Wrote a new configuration menu for WiFi SSID and password

# July 16, 2026
- https://lapse.hackclub.com/timelapse/Cy6x8SXXK3PI 
- Finally got Wi-Fi. working by decreasing the amount of RAM allocated to LVGL. Also cut a lot of LVGL components to further decrease RAM.
- Wrote some README documentation on how to configure the board in menuconfig.

# July 17, 2026
- https://lapse.hackclub.com/timelapse/BGnuhuecmgzE (Private, as it contains WiFi information)
- Began to learn how to set up an HTTP client for the ESP32, and began to implement it.
- Added menuconfig entries to specify LCD details and weather API details.
- Began to interact with the weather.gov api, and wrote much of the weather_api file. 
    - Unfortunately, this refused to connect to weather.gov

# July 19, 2026
- Found what caused the API to not connect. Appaerently I needed an event handler to process when the network is connected. I think it just was not waiting to connect before trying to request information.
- Weather display now shows weather.

# July 22, 2026
-https://lapse.hackclub.com/timelapse/UuDt-n6QwesS 
- Began to write the file to handle the transit API. Wrote a menuconfig entry for this. 
    - This file is intended to ask for realtime data for the specified line at the specified station and return the number of minutes until the next train in each direction. If realtime data does not exist, it should use schedule data in its place. 
    - Unable to fully test due to the error listed below, along with the confusion of weather.gov reporting bad data (I thought my program broke).
- Constantly ran out of RAM, so I had to restrict how many predictions are sent back. I might consider getting a devboard with more RAM.
- Ended up commenting this section out to ship, as I was unable to test this. 

# July 23, 2026
- Modfied README to be more clear, so this can be reproduced by others.
- Created a Bill of Materials and attached it to the Github repository
- Created this journal, as I had forgot to do so before
- Committed the actual transit API handler that I had written on the 22nd
- Created a demo video for the weather side of the project
- Updated the Horizons page with new project information
- Touched up gitignore and removed an old LVGL submodule.
- Submtted project to Horizons.


