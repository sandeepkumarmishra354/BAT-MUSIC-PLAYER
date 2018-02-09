# BAT-MUSIC-PLAYER
A simple music player program.
features- repeat, shuffle, add queue, create playlist, delete/remove songs

# compile and run
**Qt_5.6 or newer version must be installed in your target system before compiling.
# run this command
**open terminal in this directory and execute(without quote)-
        'qmake -project'
        it generates a default .pro file. You can create desired .pro file using this-
        'qmake -project -o filename.pro'
        after this open filename.pro and insert this at the begining of file(without quote)-
        'QT       += core gui'
        'QT += multimedia multimediawidgets'
