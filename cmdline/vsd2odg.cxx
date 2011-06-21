/* writerperfect:
 *
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006-2007 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <stdio.h>
#include <string.h>

#include <libvisio/libvisio.h>

#include "OutputFileHelper.hxx"

#include "OdgGenerator.hxx"

const char mimetypeStr[] = "application/vnd.oasis.opendocument.graphics";

const char manifestStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
		" <manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.graphics\" manifest:version=\"1.0\" manifest:full-path=\"/\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"settings.xml\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>"
		"</manifest:manifest>";

class OdgOutputFileHelper : public OutputFileHelper
{
public:
	OdgOutputFileHelper(const char *outFileName, const char *password) :
		OutputFileHelper(outFileName, password) {}
	~OdgOutputFileHelper() {}

private:
	bool _isSupportedFormat(WPXInputStream *input, const char * /* password */)
	{
		if (!libvisio::VisioDocument::isSupported(input))
		{
 			fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid Visio Document.\n");
			return false;
		}
		return true;
	}

	bool _convertDocument(WPXInputStream *input, const char * /* password */, OdfDocumentHandler *handler, OdfStreamType streamType)
	{
		OdgGenerator exporter(handler, streamType);
		return libvisio::VisioDocument::parse(input, &exporter);
	}
};

int printUsage(char * name)
{
	fprintf(stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", name);
	fprintf(stderr, "USAGE : Where <infile> is the Microsoft Visio source document\n");
	fprintf(stderr, "USAGE : and [outfile] is the odg target document. Alternately,\n");
	fprintf(stderr, "USAGE : pass '--stdout' or simply omit the [outfile] to pipe the\n");
	fprintf(stderr, "USAGE : resultant document as flat XML to standard output\n");
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

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--stdout"))
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

	OdgOutputFileHelper helper(szOutFile, 0);

	if (!helper.writeChildFile("mimetype", mimetypeStr, (char)0)) {
		fprintf(stderr, "ERROR : Couldn't write mimetype\n");
		return 1;
	}

	if (!helper.writeChildFile("META-INF/manifest.xml", manifestStr)) {
		fprintf(stderr, "ERROR : Couldn't write manifest\n");
		return 1;
	}

	if (szOutFile && !helper.writeConvertedContent("settings.xml", szInputFile, ODF_SETTINGS_XML))
	{
		fprintf(stderr, "ERROR : Couldn't write document settings\n");
		return 1;
	}

	if (szOutFile && !helper.writeConvertedContent("styles.xml", szInputFile, ODF_STYLES_XML))
	{
		fprintf(stderr, "ERROR : Couldn't write document styles\n");
		return 1;
	}

	if (!helper.writeConvertedContent("content.xml", szInputFile, szOutFile ? ODF_CONTENT_XML : ODF_FLAT_XML))
	{
	        fprintf(stderr, "ERROR : Couldn't write document content\n");
	        return 1;
	}

	return 0;
}
