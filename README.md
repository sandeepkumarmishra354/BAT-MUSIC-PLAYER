# BAT-MUSIC-PLAYER
Simple music player program.
features- repeat, shuffle, add queue, create playlist, delete/remove/search, display song info...

# compile and run
Qt_5.6 or newer version must be installed in your target system before compiling.
# run this command
open terminal and `cd projectdir` directory and execute-
        `qmake -project -o filename.pro`
        
# After this open filename.pro and insert this at the begining of file-
        QT       += core gui
        QT += multimedia multimediawidgets
        
# Last step
        qmake filename.pro
        make
        
# Now you can see executed file run this
        ./file
