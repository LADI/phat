lib_LTLIBRARIES = libphat.la

libphat_la_SOURCES = phatfanslider.c phatfanslider.h phatvfanslider.c	\
phatvfanslider.h phathfanslider.c phathfanslider.h phatprivate.h	\
phatprivate.c phatsliderbutton.c phatsliderbutton.h phatkeyboard.c	\
phatkeyboard.h phatvkeyboard.c phatvkeyboard.h phathkeyboard.c		\
phathkeyboard.h phatpad.c phatpad.h phatknob.c phatknob.h

libphat_la_CFLAGS = $(CFLAGS) -I.. -DINSTALL_DIR=\"$(datadir)\"

libphatincludedir = $(includedir)/phat

libphatinclude_HEADERS = phat.h phatfanslider.h phatvfanslider.h	\
phathfanslider.h phatsliderbutton.h phatkeyboard.h phatvkeyboard.h	\
phathkeyboard.h phatpad.h phatknob.h
