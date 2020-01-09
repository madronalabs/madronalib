ml-vst
======

Example of a VST3 / Audio Units plugin made with cmake, madronalib and the VST3 SDK. A GUI-free project, about as simple as I could make it.


to build:
---------

to create XCode project using cmake:

- mkdir build
- cd build
- cmake -DVST3_SDK_ROOT=(your VST3 SDK location) -DCMAKE_BUILD_TYPE=Debug -GXcode ..

then open the project and build all. Links to VST3 plugins will be made in ~/Library/Audio/Plug-Ins/VST3. The au component will be copied to ~/Library/Audio/Plug-Ins/Components.

