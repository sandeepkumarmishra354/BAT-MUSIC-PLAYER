# BAT-MUSIC-PLAYER
Simple music player program.
features- repeat, shuffle, add queue, create playlist, delete/remove/search, display song info, set timer...

# compile and run
Qt_5.6 or newer version must be installed in your target system before compiling.
# run this command
open terminal and `cd projectdir` directory and execute (run this if you want to create new .pro file)
        `qmake -project -o filename.pro`
feel free to use any "filename"
        
# After this open filename.pro and insert this at begining of file(only when you created a new .pro file)-
        QT += core gui
        QT += multimedia multimediawidgets
        
# Last step
        qmake filename.pro
        make
        
# WOW !!! you got an executable file
run this file
        `./file`
