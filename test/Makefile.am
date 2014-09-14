if COMPILE_TEST

AM_CXXFLAGS = $(REVENGE_CFLAGS) -I../inc/

targets=testChart1 testGraphic1 testLayer1 testLink1 testList1 testMasterPage1 testPageSpan1 testPara1 testSpan1 testTable1 testTextbox1
noinst_PROGRAMS= $(targets)

EXTRA_SRC=StringDocumentHandler.cxx StringDocumentHandler.hxx
EXTRA_LDD=../src/libodfgen-@LIBODFGEN_MAJOR_VERSION@.@LIBODFGEN_MINOR_VERSION@.la \
	$(REVENGE_LIBS) $(REVENGE_STREAM_LIBS)

testChart1_DEPENDENCIES =
testChart1_LDADD =  $(EXTRA_LDD)
testChart1_SOURCES = $(EXTRA_SRC) testChart1.cxx

testGraphic1_DEPENDENCIES =
testGraphic1_LDADD =  $(EXTRA_LDD)
testGraphic1_SOURCES = $(EXTRA_SRC) testGraphic1.cxx

testLayer1_DEPENDENCIES =
testLayer1_LDADD =  $(EXTRA_LDD)
testLayer1_SOURCES = $(EXTRA_SRC) testLayer1.cxx

testLink1_DEPENDENCIES =
testLink1_LDADD =  $(EXTRA_LDD)
testLink1_SOURCES = $(EXTRA_SRC) testLink1.cxx

testList1_DEPENDENCIES =
testList1_LDADD =  $(EXTRA_LDD)
testList1_SOURCES = $(EXTRA_SRC) testList1.cxx

testMasterPage1_DEPENDENCIES =
testMasterPage1_LDADD =  $(EXTRA_LDD)
testMasterPage1_SOURCES = $(EXTRA_SRC) testMasterPage1.cxx

testPageSpan1_DEPENDENCIES = 
testPageSpan1_LDADD = $(EXTRA_LDD)
testPageSpan1_SOURCES = $(EXTRA_SRC) testPageSpan1.cxx

testPara1_DEPENDENCIES = 
testPara1_LDADD = $(EXTRA_LDD)
testPara1_SOURCES = $(EXTRA_SRC) testParagraph1.cxx

testSpan1_DEPENDENCIES = 
testSpan1_LDADD = $(EXTRA_LDD)
testSpan1_SOURCES = $(EXTRA_SRC) testSpan1.cxx

testTable1_DEPENDENCIES =
testTable1_LDADD = $(EXTRA_LDD)
testTable1_SOURCES = $(EXTRA_SRC) testTable1.cxx

testTextbox1_DEPENDENCIES =
testTextbox1_LDADD = $(EXTRA_LDD)
testTextbox1_SOURCES = $(EXTRA_SRC) testTextbox1.cxx

clean::
	@rm -f test*.odg test*.odp test*.ods test*.odt $(targets)

launch_all:: $(targets)
	./testGraphic1 && ./testLayer1 && ./testLink1 && ./testList1 && ./testMasterPage1 && ./testPageSpan1 && ./testPara1 && ./testSpan1 && ./testTable1 && ./testTextbox1
endif
