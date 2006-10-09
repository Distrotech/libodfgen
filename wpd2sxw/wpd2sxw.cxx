/* wpd2sxw:
 *
 * Copyright (C) 2002 Jon K Hellan (hellan@acm.org)
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2003-2004 Net Integration Technologies (http://www.net-itech.com)
 * Copyright (C) 2004 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <libwpd/libwpd.h>
#include <libwpd/GSFStream.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#include <gsf/gsf-input-stdio.h>

#include <stdio.h>
#include <string.h>

#include "WordPerfectCollector.hxx"
#include "DiskDocumentHandler.hxx"
#include "StdOutHandler.hxx"

const char *mimetypeStr = "application/vnd.sun.xml.writer";


const char *manifestStr ="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE manifest:manifest PUBLIC \"-//OpenOffice.org//DTD Manifest 1.0//EN\" \"Manifest.dtd\">\n\
<manifest:manifest xmlns:manifest=\"http://openoffice.org/2001/manifest\">\n\
 <manifest:file-entry manifest:media-type=\"application/vnd.sun.xml.writer\" manifest:full-path=\"/\"/>\n\
 <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>\n\
 <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>\n\
</manifest:manifest>\n";

const char *stylesStr ="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<!DOCTYPE office:document-styles PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">\
<office:document-styles xmlns:office=\"http://openoffice.org/2000/office\" xmlns:style=\"http://openoffice.org/2000/style\"\
 xmlns:text=\"http://openoffice.org/2000/text\" xmlns:table=\"http://openoffice.org/2000/table\"\
 xmlns:draw=\"http://openoffice.org/2000/drawing\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\"\
 xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:number=\"http://openoffice.org/2000/datastyle\"\
 xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:chart=\"http://openoffice.org/2000/chart\" xmlns:dr3d=\"http://openoffice.org/2000/dr3d\"\
 xmlns:math=\"http://www.w3.org/1998/Math/MathML\" xmlns:form=\"http://openoffice.org/2000/form\"\
 xmlns:script=\"http://openoffice.org/2000/script\" office:version=\"1.0\">\
<office:styles>\
<style:default-style style:family=\"paragraph\">\
<style:properties style:use-window-font-color=\"true\" style:text-autospace=\"ideograph-alpha\"\
 style:punctuation-wrap=\"hanging\" style:line-break=\"strict\" style:writing-mode=\"page\"/>\
</style:default-style>\
<style:default-style style:family=\"table\"/>\
<style:default-style style:family=\"table-row\"/>\
<style:default-style style:family=\"table-column\"/>\
<style:style style:name=\"Standard\" style:family=\"paragraph\" style:class=\"text\"/>\
<style:style style:name=\"Text body\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"text\"/>\
<style:style style:name=\"List\" style:family=\"paragraph\" style:parent-style-name=\"Text body\" style:class=\"list\"/>\
<style:style style:name=\"Header\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>\
<style:style style:name=\"Footer\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>\
<style:style style:name=\"Caption\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>\
<style:style style:name=\"Footnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>\
<style:style style:name=\"Endnote\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"extra\"/>\
<style:style style:name=\"Index\" style:family=\"paragraph\" style:parent-style-name=\"Standard\" style:class=\"index\"/>\
<style:style style:name=\"Footnote Symbol\" style:family=\"text\">\
<style:properties style:text-position=\"super 58%\"/>\
</style:style>\
<style:style style:name=\"Endnote Symbol\" style:family=\"text\">\
<style:properties style:text-position=\"super 58%\"/>\
</style:style>\
<style:style style:name=\"Footnote anchor\" style:family=\"text\">\
<style:properties style:text-position=\"super 58%\"/>\
</style:style>\
<style:style style:name=\"Endnote anchor\" style:family=\"text\">\
<style:properties style:text-position=\"super 58%\"/>\
</style:style>\
<text:footnotes-configuration text:citation-style-name=\"Footnote Symbol\" text:citation-body-style-name=\"Footnote anchor\"\
 style:num-format=\"1\" text:start-value=\"0\" text:footnotes-position=\"page\" text:start-numbering-at=\"document\"/>\
<text:endnotes-configuration text:citation-style-name=\"Endnote Symbol\" text:citation-body-style-name=\"Endnote anchor\"\
 text:master-page-name=\"Endnote\" style:num-format=\"i\" text:start-value=\"0\"/>\
<text:linenumbering-configuration text:number-lines=\"false\" text:offset=\"0.1965inch\" style:num-format=\"1\"\
 text:number-position=\"left\" text:increment=\"5\"/>\
</office:styles>\
<office:automatic-styles>\
<style:page-master style:name=\"PM0\">\
<style:properties fo:margin-bottom=\"1.0000inch\" fo:margin-left=\"1.0000inch\" fo:margin-right=\"1.0000inch\" fo:margin-top=\"1.0000inch\"\
 fo:page-height=\"11.0000inch\" fo:page-width=\"8.5000inch\" style:print-orientation=\"portrait\">\
<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:distance-after-sep=\"0.0398inch\"\
 style:distance-before-sep=\"0.0398inch\" style:rel-width=\"25%\" style:width=\"0.0071inch\"/>\
</style:properties>\
</style:page-master>\
<style:page-master style:name=\"PM1\">\
<style:properties fo:margin-bottom=\"1.0000inch\" fo:margin-left=\"1.0000inch\" fo:margin-right=\"1.0000inch\" fo:margin-top=\"1.0000inch\"\
 fo:page-height=\"11.0000inch\" fo:page-width=\"8.5000inch\" style:print-orientation=\"portrait\">\
<style:footnote-sep style:adjustment=\"left\" style:color=\"#000000\" style:rel-width=\"25%\"/>\
</style:properties>\
</style:page-master>\
</office:automatic-styles>\
<office:master-styles>\
<style:master-page style:name=\"Standard\" style:page-master-name=\"PM0\"/>\
<style:master-page style:name=\"Endnote\" style:page-master-name=\"PM1\"/>\
</office:master-styles>\
</office:document-styles>";


static bool writeChildFile(GsfOutfile *outfile, const char *fileName, const char *str)
{
	GsfOutput  *child = gsf_outfile_new_child  (outfile, fileName, FALSE);

	if (!gsf_output_write (child, strlen (str), (uint8_t *)str))
		return false;
	if (!gsf_output_close ((GsfOutput *) child))
		return false;

	g_object_unref (child);

	return true;
}

static bool writeContent(const char *pInFileName, GsfOutfile *pOutfile)
{
	GError *err;
	GsfInput *pGsfInput;
	pGsfInput = GSF_INPUT(gsf_input_stdio_new (pInFileName, &err));
	if (pGsfInput == NULL) 
	{
		g_return_val_if_fail (err != NULL, 1);
		
		g_warning ("'%s' error: %s", pInFileName, err->message);
		g_error_free (err);
		return false;
	}
	GSFInputStream input(pGsfInput);

	WPDConfidence confidence = WPDocument::isFileFormatSupported(&input, false);
 	if (confidence == WPD_CONFIDENCE_NONE)
 	{
 		fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid WordPerfect document.\n");
 		return false;
 	}
	input.seek(0, WPX_SEEK_SET);

        DocumentHandler *pHandler;
        GsfOutput *pContentChild = NULL;
        if (pOutfile)
        {
                pContentChild = gsf_outfile_new_child(pOutfile, "content.xml", FALSE);
                pHandler = new DiskDocumentHandler(pContentChild); // WLACH_REFACTORING: rename to DiskHandler
        }
        else
                pHandler = new StdOutHandler();

        WordPerfectCollector collector(&input, pHandler);
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
	GError   *err;

	gsf_init ();

	if (argc != 3) 
        {
		fprintf(stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", argv[0]);
		fprintf(stderr, "USAGE : Where <infile> is the WordPerfect source document\n");
		fprintf(stderr, "USAGE : and [outfile] is the sxw target document. Alternately,\n");
		fprintf(stderr, "USAGE : pass '--stdout' to pipe the resultant document to\n");
		fprintf(stderr, "USAGE : standard output\n");
		fprintf(stderr, "USAGE : \n");
		return 1;
	}

        char *szInputFile;

        if (!strcmp(argv[1], "--stdout"))
        {
                szInputFile = argv[2];
                pOutput = NULL;
        }
        else
        {
                szInputFile = argv[1];

                pOutput = GSF_OUTPUT(gsf_output_stdio_new (argv[2], &err));
                if (pOutput == NULL) {
                        g_return_val_if_fail (err != NULL, 1);
                        
                        g_warning ("'%s' error: %s", argv[2], err->message);
                        g_error_free (err);
                        return 1;
                }
                pOutfile = GSF_OUTFILE(gsf_outfile_zip_new (pOutput, &err));
                if (pOutput == NULL) {
                        g_return_val_if_fail (err != NULL, 1);
                        
                        g_warning ("'%s' error: %s",
                                   "gsf_outfile_zip_new", err->message);
                        g_error_free (err);
                        return 1;
                }
                g_object_unref (G_OBJECT (pOutput));
        }
	if (pOutfile && !writeChildFile(pOutfile, "mimetype", mimetypeStr)) {
		fprintf(stderr, "ERROR : Couldn't write mimetype\n");
		return 1;
	}
	
	if (pOutfile && !writeChildFile(pOutfile, "META-INF/manifest.xml", manifestStr)) {
		fprintf(stderr, "ERROR : Couldn't write manifest\n");
		return 1;
	}
	
	if (pOutfile && !writeChildFile(pOutfile, "styles.xml", stylesStr)) {
		fprintf(stderr, "ERROR : Couldn't write styles\n");
		return 1;
	}

        if (!writeContent(szInputFile, pOutfile)) 
        {
                fprintf(stderr, "ERROR : Couldn't write document content\n");
                return 1;
        }

	if (pOutfile && !gsf_output_close ((GsfOutput *) pOutfile)) {
		fprintf(stderr, "ERROR : Couldn't close outfile\n");
		return 1;
	}

        if (pOutfile)
                g_object_unref (G_OBJECT (pOutfile));

	gsf_shutdown ();

	return 0;
}
