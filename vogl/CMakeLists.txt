PROJECT(vogl)

SET(voglsources
arcs.c attr.c buffer.c clip.c curves.c draw.c drivers.c
viewing.c viewp.c mapping.c matrix.c move.c objects.c patches.c
points.c polygons.c rect.c tensor.c text.c newtokens.c trans.c
scale.c getgp.c valloc.c pref.c vcalls.c
queue.c lines.c
)
# -DPOSTSCRIPT	../drivers/ps.o
# -DHPGL		../drivers/hpdxy.o
# -DDXY		../drivers/hpdxy.o
# -DTEK		../drivers/tek.o
# -DSUN		../drivers/sun.o	(Sunview that is)
# -DX11		../drivers/X11.o
# -DNeXT		../drivers/NeXT.o	(NeXTStep)
IF(WIN32)
 # Can't find strings.h on mingw:
SET_SOURCE_FILES_PROPERTIES("src/text.c"
 PROPERTIES COMPILE_FLAGS "-DSYS5"
)
SET(driversources
 ps
)
SET(driverflags
 "-DPOSTSCRIPT -DVGUI"
)
ELSE(WIN32)
SET(driversources
 ps hpdxy tek X11
)
SET(driverflags
 "-DPOSTSCRIPT -DHPGL -DTEK -DX11 -DVGUI"
)
ENDIF(WIN32)
SET(ALL_SOURCES
)
FOREACH(src ${voglsources})
 LIST(APPEND ALL_SOURCES "src/${src}")
ENDFOREACH(src)
FOREACH(src ${driversources})
 LIST(APPEND ALL_SOURCES "drivers/${src}")
ENDFOREACH(src)
SET_SOURCE_FILES_PROPERTIES("src/drivers.c"
 PROPERTIES COMPILE_FLAGS ${driverflags}
)
INCLUDE_DIRECTORIES(src drivers)
ADD_LIBRARY(vogl ${ALL_SOURCES})
SET_TARGET_PROPERTIES(vogl
 PROPERTIES VERSION 2.0 SOVERSION 2
)

IF(BUILD_SHARED_LIBS)
INSTALL(
 TARGETS vogl
 DESTINATION ${INSTALLDIR}
 RUNTIME PERMISSIONS ${EXECUTABLE_PERMISSIONS}
 LIBRARY PERMISSIONS ${FILE_PERMISSIONS}
)
ENDIF(BUILD_SHARED_LIBS)
