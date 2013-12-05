if COMPILE_TEST

AM_CXXFLAGS = $(REVENGE_CFLAGS) -I../inc/

bin_PROGRAMS= testList1 testPara1 testSpan1 testTable1 testTextbox1

EXTRA_SOURCES=StringDocumentHandler.cxx StringDocumentHandler.hxx
EXTRA_LDADD=../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)

testList1_DEPENDENCIES =
testList1_LDADD =  $(EXTRA_LDADD)
testList1_SOURCES = $(EXTRA_SOURCES) testList1.cxx

testPara1_DEPENDENCIES = 
testPara1_LDADD = $(EXTRA_LDADD)
testPara1_SOURCES = $(EXTRA_SOURCES) testParagraph1.cxx

testSpan1_DEPENDENCIES = 
testSpan1_LDADD = $(EXTRA_LDADD)
testSpan1_SOURCES = $(EXTRA_SOURCES) testSpan1.cxx

testTable1_DEPENDENCIES =
testTable1_LDADD = $(EXTRA_LDADD)
testTable1_SOURCES = $(EXTRA_SOURCES) testTable1.cxx

testTextbox1_DEPENDENCIES =
testTextbox1_LDADD = $(EXTRA_LDADD)
testTextbox1_SOURCES = $(EXTRA_SOURCES) testTextbox1.cxx

endif
