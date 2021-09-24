#****************************************************************************
#* Copyright (c) 2021, Fougue Ltd. <http://www.fougue.pro>
#* All rights reserved.
#* See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
#****************************************************************************

TEMPLATE = app
TARGET = mayo
include(version.pri)
CONFIG(debug, debug|release) {
    message(Mayo version $$MAYO_VERSION debug)
} else {
    message(Mayo version $$MAYO_VERSION release)
}

QT += core gui widgets
message(Qt version $$QT_VERSION)

CONFIG += c++17
CONFIG += file_copies
CONFIG(debug, debug|release) {
    CONFIG += console
} else {
    CONFIG -= console
    CONFIG += release_with_debuginfo
}

release_with_debuginfo:*msvc* {
    # https://docs.microsoft.com/en-us/cpp/build/reference/how-to-debug-a-release-build
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF
}

*msvc* {
    QMAKE_CXXFLAGS += /we4150 # Deletion of pointer to incomplete type 'XXXX'; no destructor called
    QMAKE_CXXFLAGS += /std:c++17
}
*g++*|*clang* {
    QMAKE_CXXFLAGS += -std=c++17
}
*clang* {
    # Silent Clang warnings about instantiation of variable 'Mayo::GenericProperty<T>::TypeName'
    QMAKE_CXXFLAGS += -Wno-undefined-var-template
}
*clang-libc++* {
    # See https://libcxx.llvm.org/docs/UsingLibcxx.html
    LIBS += -lc++fs
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
}

*win* {
    LIBS += -lUser32
}

INCLUDEPATH += \
    src/app \
    src/3rdparty

HEADERS += \
    $$files(src/base/*.h) \
    $$files(src/io_occ/*.h) \
    $$files(src/io_dxf/*.h) \
    $$files(src/graphics/*.h) \
    $$files(src/gui/*.h) \
    $$files(src/app/*.h) \

SOURCES += \
    $$files(src/base/*.cpp) \
    $$files(src/io_occ/*.cpp) \
    $$files(src/io_dxf/*.cpp) \
    $$files(src/graphics/*.cpp) \
    $$files(src/gui/*.cpp) \
    $$files(src/app/*.cpp) \
    \
    src/3rdparty/fmt/src/format.cc \

win* {
    QT += winextras
    HEADERS += $$files(src/app/windows/*.h)
    SOURCES += $$files(src/app/windows/*.cpp)

    COPIES += WinInstallerFiles
    WinInstallerFiles.files  = $$files($$PWD/installer/*.iss)
    WinInstallerFiles.files += $$files($$PWD/installer/*.conf)
    WinInstallerFiles.path = $$OUT_PWD/installer

    QMAKE_SUBSTITUTES += $$PWD/installer/setupvars.iss.in
}

FORMS += $$files(src/app/*.ui)

RESOURCES += mayo.qrc
RC_ICONS = images/appicon.ico

OTHER_FILES += \
    README.md \
    appveyor.yml \
    .travis.yml \
    images/credits.txt

# OpenCascade
include(opencascade.pri)
message(OpenCascade version $$OCC_VERSION_STR)
LIBS += \
    -lTKBin \
    -lTKBinL \
    -lTKBinXCAF \
    -lTKBO \
    -lTKBool \
    -lTKBRep \
    -lTKCAF \
    -lTKCDF \
    -lTKernel \
    -lTKG2d \
    -lTKG3d \
    -lTKGeomAlgo \
    -lTKGeomBase \
    -lTKHLR \
    -lTKLCAF \
    -lTKMath \
    -lTKMesh \
    -lTKMeshVS \
    -lTKOpenGl \
    -lTKPrim \
    -lTKService \
    -lTKShHealing \
    -lTKTopAlgo  \
    -lTKXSDRAW \  # Seems to be required on macOS(see https://github.com/fougue/mayo/issues/41#issuecomment-742732322)
    -lTKV3d  \
    -lTKVCAF \
    -lTKXCAF \
    -lTKXml \
    -lTKXmlL \
    -lTKXmlXCAF \
    -lTKXSBase \

# -- IGES support
LIBS += -lTKIGES -lTKXDEIGES
# -- STEP support
LIBS += -lTKSTEP -lTKSTEP209 -lTKSTEPAttr -lTKSTEPBase -lTKXDESTEP
# -- STL support
LIBS += -lTKSTL
# -- OBJ/glTF support
minOpenCascadeVersion(7, 4, 0) {
    LIBS += -lTKRWMesh
} else {
    SOURCES -= \
        src/io_occ/io_occ_base_mesh.cpp \
        src/io_occ/io_occ_gltf_reader.cpp \
        src/io_occ/io_occ_obj.cpp
}

!minOpenCascadeVersion(7, 5, 0) {
    SOURCES -= src/io_occ/io_occ_gltf_writer.cpp
}

# -- VRML support
LIBS += -lTKVRML

CASCADE_LIST_OPTBIN_DIR = $$split(CASCADE_OPTBIN_DIRS, ;)
for(binPath, CASCADE_LIST_OPTBIN_DIR) {
    lowerBinPath = $$lower($${binPath})

    findLib = $$find(lowerBinPath, "ffmpeg")
    !isEmpty(findLib):FFMPEG_BIN_DIR = $${binPath}

    findLib = $$find(lowerBinPath, "freeimage")
    !isEmpty(findLib):FREEIMAGE_BIN_DIR = $${binPath}

    findLib = $$find(lowerBinPath, "freetype")
    !isEmpty(findLib):FREETYPE_BIN_DIR = $${binPath}

    findLib = $$find(lowerBinPath, "tbb")
    !isEmpty(findLib):TBB_BIN_DIR = $${binPath}

    findLib = $$find(lowerBinPath, "openvr")
    !isEmpty(findLib):OPENVR_BIN_DIR = $${binPath}
}

# -- Create file "opencascade_dlls.iss" that will contain the required OpenCascade DLL files to be
# -- added in the InnoSetup [Files] section
# -- The list of OpenCascade libraries is retrieved from the LIBS QMake variable
win* {
    for(lib, LIBS) {
        findTK = $$find(lib, "-lTK")
        !isEmpty(findTK) {
            lib = $$replace(lib, "-l", "")
            CASCADE_INNOSETUP_DLLS += "Source: \"$$CASCADE_BIN_DIR\\$${lib}.dll\"; DestDir: \"{app}\"; Flags: ignoreversion"
        }
    }

    write_file($$OUT_PWD/installer/opencascade_dlls.iss, CASCADE_INNOSETUP_DLLS)
}

# gmio
isEmpty(GMIO_ROOT) {
    message(gmio OFF)
} else {
    message(gmio ON)
    CONFIG(debug, debug|release) {
        #GMIO_BIN_SUFFIX = d
        GMIO_BIN_SUFFIX =
    } else {
        GMIO_BIN_SUFFIX =
    }

    HEADERS += $$files(src/io_gmio/*.h)
    SOURCES += $$files(src/io_gmio/*.cpp)

    INCLUDEPATH += $$GMIO_ROOT/include
    LIBS += -L$$GMIO_ROOT/lib -lgmio_static -lzlibstatic
    SOURCES += \
#        $$GMIO_ROOT/src/gmio_support/stl_occ_brep.cpp \
#        $$GMIO_ROOT/src/gmio_support/stl_occ_polytri.cpp \
        $$GMIO_ROOT/src/gmio_support/stream_qt.cpp
    DEFINES += HAVE_GMIO
}
