TEMPLATE = subdirs

SUBDIRS += miniupnp.pro \
           breakpad.pro

!win32:SUBDIRS += qtsystems.pro

OTHER_FILES += miniupnpc.pri \
               breakpad.pri \
               winpcap.pri \
               qtsystems.pri
