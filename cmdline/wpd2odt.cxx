/* writerperfect:
 *
 * Copyright (C) 2002 Jon K Hellan (hellan@acm.org)
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2003-2004 Net Integration Technologies (http://www.net-itech.com)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libwpd/libwpd.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>

#include <stdio.h>
#include <string.h>

#include "WordPerfectCollector.hxx"
#include "DiskDocumentHandler.hxx"
#include "StdOutHandler.hxx"
#include <libwpd/WPXStreamImplementation.h>

const char mimetypeStr[] = "application/vnd.oasis.opendocument.text";

const char manifestStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
		" <manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.text\" manifest:full-path=\"/\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>"
		"</manifest:manifest>";

const char stylesStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<office:document-styles xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
	"xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
	"xmlns:table=\"urn:oasis:names:tc:opendocument:xmlns:table:1.0\" xmlns:draw=\"urn:oasis:names:tc:opendocument:xmlns:drawing:1.0\" "
	"xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
	"xmlns:number=\"urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0\" "
	"xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\" "
	"xmlns:chart=\"urn:oasis:names:tc:opendocument:xmlns:chart:1.0\" xmlns:dr3d=\"urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0\" "
	"xmlns:form=\"urn:oasis:names:tc:opendocument:xmlns:form:1.0\" xmlns:script=\"urn:oasis:names:tc:opendocument:xmlns:script:1.0\">"
	"<office:styles>"
	"<style:default-style style:family=\"paragraph\">"
	"<style:paragraph-properties style:use-window-font-color=\"true\" style:text-autospace=\"ideograph-alpha\" "
	"style:punctuation-wrap=\"hanging\" style:line-break=\"strict\" style:writing-mode=\"page\"/>"
	"</style:default-style>"
	"<style:default-style style:family=\"table\"/>"
	"<style:default-style style:family=\"table-row\">"
	"<style:table-row-properties fo:keep-together=\"auto\"/>"
	"</style:default-style>"
	"<style:default-style style:family=\"table-column\"/>"
	"<style:style style:name=\"Standard\" style:family=\"paragraph\" style:class=\"text\"/>"
	"<style:style style:name=\"Text_body\" style:display-name=\"Text body\" style:family=\"paragraph\" "
	"style:parent-style-name=\"Standard\" style:class=\"text\"/>"
	"<style:style style:name=\"List\" style:family=\"paragraph\" style:parent-style-name=\"Text_body\" style:class=\"list\"/>"
	"<style:style style:name=\"Header\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Footer\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Caption\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Footnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Endnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>"
	"<style:style style:name=\"Index\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"index\"/>"
	"<style:style style:name=\"Footnote_Symbol\" style:display-name=\"Footnote Symbol\" style:family=\"text\">"
	"<style:text-properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Endnote_Symbol\" style:display-name=\"Endnote Symbol\" style:family=\"text\">"
	"<style:text-properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Footnote_anchor\" style:display-name=\"Footnote anchor\" style:family=\"text\">"
	"<style:text-properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<style:style style:name=\"Endnote_anchor\" style:display-name=\"Endnote anchor\" style:family=\"text\">"
	"<style:text-properties style:text-position=\"super 58%\"/>"
	"</style:style>"
	"<text:notes-configuration text:note-class=\"footnote\" text:citation-style-name=\"Footnote_Symbol\" "
	"text:citation-body-style-name=\"Footnote_anchor\" style:num-format=\"1\" text:start-value=\"0\" "
	"text:footnotes-position=\"page\" text:start-numbering-at=\"document\"/>"
	"<text:notes-configuration text:note-class=\"endnote\" text:citation-style-name=\"Endnote_Symbol\" "
	"text:citation-body-style-name=\"Endnote_anchor\" text:master-page-name=\"Endnote\" "
	"style:num-format=\"i\" text:start-value=\"0\"/>"
	"<text:linenumbering-configuration text:number-lines=\"false\" text:offset=\"0.1965in\" "
	"style:num-format=\"1\" text:number-position=\"left\" text:increment=\"5\"/>"
	"</office:styles>"
	"<office:automatic-styles>"
	"<style:page-layout style:name=\"PM0\">"
	"<style:page-layout-properties fo:margin-bottom=\"1.0000in\" fo:margin-left=\"1.0000in\" "
	"fo:margin-right=\"1.0000in\" fo:margin-top=\"1.0000in\" fo:page-height=\"11.0000in\" "
	"fo:page-width=\"8.5000in\" style:print-orientation=\"portrait\">"
	"<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:distance-after-sep=\"0.0398in\" "
	"style:distance-before-sep=\"0.0398in\" style:rel-width=\"25%\" style:width=\"0.0071in\"/>"
	"</style:page-layout-properties>"
	"</style:page-layout>"
	"<style:page-layout style:name=\"PM1\">"
	"<style:page-layout-properties fo:margin-bottom=\"1.0000in\" fo:margin-left=\"1.0000in\" "
	"fo:margin-right=\"1.0000in\" fo:margin-top=\"1.0000in\" fo:page-height=\"11.0000in\" "
	"fo:page-width=\"8.5000in\" style:print-orientation=\"portrait\">"
	"<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:rel-width=\"25%\"/>"
	"</style:page-layout-properties>"
	"</style:page-layout>"
	"</office:automatic-styles>"
	"<office:master-styles>"
	"<style:master-page style:name=\"Standard\" style:page-layout-name=\"PM0\"/>"
	"<style:master-page style:name=\"Endnote\" style:page-layout-name=\"PM1\"/>"
	"</office:master-styles>"
	"</office:document-styles>";

static bool writeChildFile(GsfOutfile *outfile, const char *fileName, const char *str)
{
	GsfOutput *child;
	if (NULL != (child = gsf_outfile_new_child  (outfile, fileName, FALSE)))
	{
		bool res = gsf_output_puts (child, str) &&
			gsf_output_close (child);
		g_object_unref (child);
		return res;
	}
	return false;
}

static bool writeChildFile(GsfOutfile *outfile, const char *fileName, const char *str, const char compression_level)
{
	GsfOutput *child;
#ifdef GSF_HAS_COMPRESSION_LEVEL
	if (NULL != (child = gsf_outfile_new_child_full  (outfile, fileName, FALSE,"compression-level", 0, (void*)0)))
#else
	if (NULL != (child = gsf_outfile_new_child  (outfile, fileName, FALSE)))
#endif
	{
		bool res = gsf_output_puts (child, str) &&
			gsf_output_close (child);
		g_object_unref (child);
		return res;
	}
	return false;
}

static bool writeContent(const char *pInFileName, GsfOutfile *pOutfile)
{
	WPXFileStream input(pInFileName);

	WPDConfidence confidence = WPDocument::isFileFormatSupported(&input);
 	if (confidence != WPD_CONFIDENCE_EXCELLENT)
 	{
 		fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid WordPerfect document.\n");
 		return false;
 	}
	input.seek(0, WPX_SEEK_SET);

	DocumentHandler *pHandler;
	GsfOutput *pContentChild = NULL;
	bool tmpIsFlatXML = true;
	if (pOutfile)
	{
	        pContentChild = gsf_outfile_new_child(pOutfile, "content.xml", FALSE);
	        pHandler = new DiskDocumentHandler(pContentChild); // WLACH_REFACTORING: rename to DiskHandler
		tmpIsFlatXML = false;
	}
	else
	        pHandler = new StdOutHandler();

	WordPerfectCollector collector(&input, pHandler, tmpIsFlatXML);
	bool bRetVal = collector.filter();

	if (pContentChild)
	{
	        gsf_output_close(pContentChild);
	        g_object_unref(G_OBJECT (pContentChild));
	}
	delete pHandler;

	return bRetVal;
}

int
main (int argc, char *argv[])
{
	GsfOutput  *pOutput = NULL;
	GsfOutfile *pOutfile = NULL;
	GError   *err = NULL;

	gsf_init ();

	if (argc < 2) 
	{
		fprintf(stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", argv[0]);
		fprintf(stderr, "USAGE : Where <infile> is the WordPerfect source document\n");
		fprintf(stderr, "USAGE : and [outfile] is the odt target document. Alternately,\n");
		fprintf(stderr, "USAGE : pass '--stdout' or simply omit the [outfile] to pipe the\n");
		fprintf(stderr, "USAGE : resultant document as flat XML to standard output\n");
		fprintf(stderr, "USAGE : \n");
		gsf_shutdown ();
		return 1;
	}

	char *szInputFile;

	if (argc == 2)
	{
		szInputFile = argv[1];
		pOutput = NULL;
	}
	else if (!strcmp(argv[1], "--stdout"))
	{
	        szInputFile = argv[2];
	        pOutput = NULL;
	}
	else
	{
	        szInputFile = argv[1];

	        pOutput = GSF_OUTPUT(gsf_output_stdio_new (argv[2], &err));
	        if (pOutput == NULL) {
			if (err) {
		                g_warning ("'%s' error: %s", argv[2], err->message);
	                        g_error_free (err);
			}
			gsf_shutdown ();
	                return 1;
	        }
		if (err)
			g_error_free (err);
		err = NULL;
	        pOutfile = GSF_OUTFILE(gsf_outfile_zip_new (pOutput, &err));
	        if (pOutfile == NULL) {
			if (err) {
		                g_warning ("'%s' error: %s",
					"gsf_outfile_zip_new", err->message);
	                        g_error_free (err);
			}
			gsf_shutdown ();
	                return 1;
	        }
		if (err)
			g_error_free (err);
		err = NULL;
	        g_object_unref (pOutput);
	}

	if (pOutfile && !writeChildFile(pOutfile, "mimetype", mimetypeStr, (char)0)) {
		fprintf(stderr, "ERROR : Couldn't write mimetype\n");
	       	g_object_unref (pOutfile);
		gsf_shutdown ();
		return 1;
	}

	if (pOutfile && !writeChildFile(pOutfile, "META-INF/manifest.xml", manifestStr)) {
		fprintf(stderr, "ERROR : Couldn't write manifest\n");
	       	g_object_unref (pOutfile);
		gsf_shutdown ();
		return 1;
	}
	
	if (pOutfile && !writeChildFile(pOutfile, "styles.xml", stylesStr)) {
		fprintf(stderr, "ERROR : Couldn't write styles\n");
	       	g_object_unref (pOutfile);
		gsf_shutdown ();
		return 1;
	}

	if (!writeContent(szInputFile, pOutfile)) 
	{
	        fprintf(stderr, "ERROR : Couldn't write document content\n");
		if (pOutfile)
		      	g_object_unref (pOutfile);
		gsf_shutdown ();
	        return 1;
	}

	if (pOutfile && !gsf_output_close ((GsfOutput *) pOutfile)) {
		fprintf(stderr, "ERROR : Couldn't close outfile\n");
	       	g_object_unref (pOutfile);
		gsf_shutdown ();
		return 1;
	}

	if (pOutfile)
	        g_object_unref (pOutfile);

	gsf_shutdown ();

	return 0;
}
