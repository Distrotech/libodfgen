/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* writerperfect
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2002-2004 William Lachance (wrlach@gmail.com)
 * Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libwpd.sourceforge.net
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef TOOLS_VERSION
#define TOOLS_VERSION
#endif

#include <stdio.h>
#include <string.h>

#include <libwpd/libwpd.h>

#include "OutputFileHelper.hxx"

#include <libodfgen/libodfgen.hxx>

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

class OdtOutputFileHelper : public OutputFileHelper
{
public:
	OdtOutputFileHelper(const char *outFileName,const char *password) :
		OutputFileHelper(outFileName, password) {};
	~OdtOutputFileHelper() {};

private:
	bool _isSupportedFormat(WPXInputStream *input, const char *password)
	{
		WPDConfidence confidence = WPDocument::isFileFormatSupported(input);
		if (WPD_CONFIDENCE_EXCELLENT != confidence && WPD_CONFIDENCE_SUPPORTED_ENCRYPTION != confidence)
		{
			fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid WordPerfect document.\n");
			return false;
		}
		if (WPD_CONFIDENCE_SUPPORTED_ENCRYPTION == confidence && !password)
		{
			fprintf(stderr, "ERROR: The WordPerfect document is encrypted and you did not give us a password.\n");
			return false;
		}
		if (confidence == WPD_CONFIDENCE_SUPPORTED_ENCRYPTION && password && (WPD_PASSWORD_MATCH_OK != WPDocument::verifyPassword(input, password)))
		{
			fprintf(stderr, "ERROR: The WordPerfect document is encrypted and we either\n");
			fprintf(stderr, "ERROR: don't know how to decrypt it or the given password is wrong.\n");
			return false;
		}

		return true;
	}

	static bool handleEmbeddedWPGObject(const WPXBinaryData &data, OdfDocumentHandler *pHandler,  const OdfStreamType streamType)
	{
		OdgGenerator exporter(pHandler, streamType);

		libwpg::WPGFileFormat fileFormat = libwpg::WPG_AUTODETECT;

		if (!libwpg::WPGraphics::isSupported(const_cast<WPXInputStream *>(data.getDataStream())))
			fileFormat = libwpg::WPG_WPG1;

		return libwpg::WPGraphics::parse(const_cast<WPXInputStream *>(data.getDataStream()), &exporter, fileFormat);
	}

	static bool handleEmbeddedWPGImage(const WPXBinaryData &input, WPXBinaryData &output)
	{
		WPXString svgOutput;
		libwpg::WPGFileFormat fileFormat = libwpg::WPG_AUTODETECT;

		if (!libwpg::WPGraphics::isSupported(const_cast<WPXInputStream *>(input.getDataStream())))
			fileFormat = libwpg::WPG_WPG1;

		if (!libwpg::WPGraphics::generateSVG(const_cast<WPXInputStream *>(input.getDataStream()), svgOutput, fileFormat))
			return false;

		output.clear();
		output.append((unsigned char *)svgOutput.cstr(), strlen(svgOutput.cstr()));

		return true;
	}

	bool _convertDocument(WPXInputStream *input, const char *password, OdfDocumentHandler *handler, const OdfStreamType streamType)
	{
		OdtGenerator collector(handler, streamType);
		collector.registerEmbeddedObjectHandler("image/x-wpg", &handleEmbeddedWPGObject);
		collector.registerEmbeddedImageHandler("image/x-wpg", &handleEmbeddedWPGImage);
		if (WPD_OK == WPDocument::parse(input, &collector, password))
			return true;
		return false;
	}
};


int printUsage(char *name)
{
	fprintf(stderr, "USAGE : %s [--stdout] --password <password> <infile> [outfile]\n", name);
	fprintf(stderr, "USAGE : Where <infile> is the WordPerfect source document\n");
	fprintf(stderr, "USAGE : and [outfile] is the odt target document. Alternately,\n");
	fprintf(stderr, "USAGE : pass '--stdout' or simply omit the [outfile] to pipe the\n");
	fprintf(stderr, "USAGE : resultant document as flat XML to standard output\n");
	fprintf(stderr, "USAGE : pass '--password <password>' to try to decrypt password\n");
	fprintf(stderr, "USAGE : protected documents.\n");
	fprintf(stderr, "USAGE : \n");
	return 1;
}


int main (int argc, char *argv[])
{
	if (argc < 2)
		return printUsage(argv[0]);

	char *szInputFile = 0;
	char *szOutFile = 0;
	bool stdOutput = false;
	char *password = 0;

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--password"))
		{
			if (i < argc - 1)
				password = argv[++i];
		}
		else if (!strncmp(argv[i], "--password=", 11))
			password = &argv[i][11];
		else if (!strcmp(argv[i], "--stdout"))
			stdOutput = true;
		else if (!szInputFile && strncmp(argv[i], "--", 2))
			szInputFile = argv[i];
		else if (szInputFile && !szOutFile && strncmp(argv[i], "--", 2))
			szOutFile = argv[i];
		else
			return printUsage(argv[0]);
	}

	if (!szInputFile)
		return printUsage(argv[0]);

	if (szOutFile && stdOutput)
		szOutFile = 0;

	OdtOutputFileHelper helper(szOutFile, password);

	if (!helper.writeChildFile("mimetype", mimetypeStr, (char)0))
	{
		fprintf(stderr, "ERROR : Couldn't write mimetype\n");
		return 1;
	}

	if (!helper.writeChildFile("META-INF/manifest.xml", manifestStr))
	{
		fprintf(stderr, "ERROR : Couldn't write manifest\n");
		return 1;
	}

	if (!helper.writeChildFile("styles.xml", stylesStr))
	{
		fprintf(stderr, "ERROR : Couldn't write styles\n");
		return 1;
	}

	if (!helper.writeConvertedContent("content.xml", szInputFile, szOutFile ? ODF_CONTENT_XML : ODF_FLAT_XML))
	{
		fprintf(stderr, "ERROR : Couldn't write document content\n");
		return 1;
	}

	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
