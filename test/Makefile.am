if COMPILE_TEST
TESTSPAN1=testSpan1

AM_CXXFLAGS = $(REVENGE_CFLAGS) -I../inc/

bin_PROGRAMS= $(TESTSPAN1)

testSpan1_DEPENDENCIES = 
testSpan1_LDADD =  \
        ../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)
testSpan1_SOURCES = 			\
	StringDocumentHandler.cxx	\
	StringDocumentHandler.hxx	\
	testSpan1.cxx

endif
