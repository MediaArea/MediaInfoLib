In order to build with qmake:
qmake
make

This will create Library/libmediainfo.a

You may consider to add some configuration options:
qmake CONFIG="c++11" QMAKE_CXXFLAGS_WARN_ON="-Wno-unused-private-field -Wno-unused-const-variable -Wno-pointer-sign -Wno-invalid-source-encoding -Wno-pointer-sign"