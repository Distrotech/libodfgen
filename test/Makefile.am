if COMPILE_TEST
TESTSPAN1=testList1 testPara1 testSpan1

AM_CXXFLAGS = $(REVENGE_CFLAGS) -I../inc/

bin_PROGRAMS= $(TESTSPAN1) $(TESTPARA1) $(TESTLIST1)

testSpan1_DEPENDENCIES = 
testSpan1_LDADD =  \
        ../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)
testSpan1_SOURCES = 			\
	StringDocumentHandler.cxx	\
	StringDocumentHandler.hxx	\
	testSpan1.cxx

testPara1_DEPENDENCIES = 
testPara1_LDADD =  \
        ../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)
testPara1_SOURCES = 			\
	StringDocumentHandler.cxx	\
	StringDocumentHandler.hxx	\
	testParagraph1.cxx

testList1_DEPENDENCIES = 
testList1_LDADD =  \
        ../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)
testList1_SOURCES = 			\
	StringDocumentHandler.cxx	\
	StringDocumentHandler.hxx	\
	testList1.cxx

endif
