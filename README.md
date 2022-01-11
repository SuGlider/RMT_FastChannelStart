# RMT_FastChannelStart
Modified version of RMT IDF Driver with new functions to deal with fast channel activation.

This sketch is intended to solve RMT issue https://github.com/espressif/arduino-esp32/issues/2885 \
It just creates two new functions, extending RMT driver in order to allow writing to RMT channels but not starting it right away.\
There is a separated function to start all ESP32 eight channels as fast as possible.

As a result all 8 channels start within 2 us, nearly simultaneous.


### Logic Analyzer Screenshots

![8 channels starting - milliseconds vision](https://github.com/SuGlider/RMT_FastChannelStart/blob/main/issue_2885_RMT_start_8_channels_at_the_same_time/LogicAnalyser-ScreenShots/Channels_Starting_milliseconds.png)

![8 channels starting - microseconds vision](https://github.com/SuGlider/RMT_FastChannelStart/blob/main/issue_2885_RMT_start_8_channels_at_the_same_time/LogicAnalyser-ScreenShots/Channels_Starting_microseconds.png)
