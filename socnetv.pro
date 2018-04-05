lessThan(QT_VERSION, 5.0) {
    error("SocNetV requires at least Qt 5.0!")
}

# START added for ArchLinux / openSUSE compatibility
INSTALLPATH = /
target.path = $$[INSTALLPATH]usr/bin
TARGET = socnetv

pixmap.path = $$[INSTALLPATH]usr/share/pixmaps
pixmap.files = src/images/socnetv.png

documentation.path = $$[INSTALLPATH]usr/share/doc/socnetv
documentation.files = manual

manpage.path = $$[INSTALLPATH]usr/share/man/man1
manpage.files = man/socnetv.1.gz

translations.path = $$[INSTALLPATH]usr/share/socnetv
translations.files = translations

doc.path = $$[INSTALLPATH]usr/share/doc/socnetv
doc.files = license changelog.gz NEWS README.md TODO COPYING AUTHORS INSTALL

INSTALLS += target pixmap documentation manpage translations doc

# END


TEMPLATE = app
CONFIG  += qt thread warn_on release
#CONFIG  += qt thread warn_on debug

LANGUAGE = C++

# support
QT += xml 
QT += network
QT += widgets
QT += printsupport 

INCLUDEPATH  += ./src
FORMS += src/forms/dialogfilteredgesbyweight.ui \
         src/forms/dialogwebcrawler.ui \
         src/forms/dialognodeedit.ui \
         src/forms/dialogdatasetselect.ui \
         src/forms/dialograndsmallworld.ui \
         src/forms/dialograndscalefree.ui \
         src/forms/dialogranderdosrenyi.ui \
         src/forms/dialograndregular.ui \
         src/forms/dialogsettings.ui \
         src/forms/dialogsimilaritypearson.ui \
         src/forms/dialogsimilaritymatches.ui \
         src/forms/dialogdissimilarities.ui \
         src/forms/dialogclusteringhierarchical.ui \  
    src/forms/dialograndlattice.ui \
    src/forms/dialognodefind.ui

HEADERS += src/graphicswidget.h \
           src/graphicsedge.h \
           src/graphicsedgeweight.h \
           src/graphicsedgelabel.h \
           src/graphicsguide.h \
           src/graphicsnode.h \
           src/graphicsnodelabel.h \
           src/graphicsnodenumber.h \
           src/graph.h \
           src/graphvertex.h \
           src/mainwindow.h \
           src/matrix.h \
           src/texteditor.h \
  	   src/parser.h \
           src/webcrawler.h \
	   src/forms/dialogfilteredgesbyweight.h \
           src/forms/dialogwebcrawler.h \
	   src/forms/dialogdatasetselect.h \
	   src/forms/dialogpreviewfile.h \
	   src/forms/dialognodeedit.h \
	   src/forms/dialogranderdosrenyi.h \
	   src/forms/dialograndsmallworld.h \
	   src/forms/dialograndscalefree.h \
	   src/forms/dialograndregular.h \
	   src/forms/dialogsettings.h \
	   src/forms/dialogsimilaritypearson.h \
	   src/forms/dialogsimilaritymatches.h \
	   src/forms/dialogdissimilarities.h \
 	   src/forms/dialogclusteringhierarchical.h \ 
           src/forms/dialograndlattice.h \
    src/forms/dialognodefind.h

SOURCES += src/graphicswidget.cpp \
           src/graphicsedge.cpp \
           src/graphicsedgeweight.cpp \
           src/graphicsedgelabel.cpp \
           src/graphicsguide.cpp \
           src/graphicsnode.cpp \
           src/graphicsnodelabel.cpp \
           src/graphicsnodenumber.cpp \
           src/graph.cpp \
           src/graphvertex.cpp \
           src/main.cpp \
           src/mainwindow.cpp \
           src/matrix.cpp \
           src/texteditor.cpp \
           src/parser.cpp \
           src/webcrawler.cpp \
	   src/forms/dialogfilteredgesbyweight.cpp \
           src/forms/dialogwebcrawler.cpp \
	   src/forms/dialogdatasetselect.cpp \
	   src/forms/dialogpreviewfile.cpp \
	   src/forms/dialognodeedit.cpp \
	   src/forms/dialogranderdosrenyi.cpp \
	   src/forms/dialograndsmallworld.cpp \
	   src/forms/dialograndregular.cpp \
	   src/forms/dialograndscalefree.cpp \
	   src/forms/dialogsettings.cpp \
	   src/forms/dialogsimilaritypearson.cpp \
	   src/forms/dialogsimilaritymatches.cpp \ 
	   src/forms/dialogdissimilarities.cpp \
	   src/forms/dialogclusteringhierarchical.cpp \ 
           src/forms/dialograndlattice.cpp \
    src/forms/dialognodefind.cpp



# Extra optimization flags
#win32 {
#  QMAKE_CXXFLAGS += -msse -mfpmath=sse -ffast-math  
#}
#unix:!macx{
#  QMAKE_CXXFLAGS += -ffast-math  
#}
#macx {
#  QMAKE_CXXFLAGS += -msse  -ffast-math 
#}


RESOURCES = src/src.qrc
win32 {
     RC_FILE = src/icon.rc
}

macx:ICON = src/images/socnetv.icns


TRANSLATIONS    = translations/socnetv_es.ts \ 
		  translations/socnetv_el.ts 




