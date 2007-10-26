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

#include <libwpg/libwpg.h>

#include "OutputFileHelper.hxx"

#include "OdgExporter.hxx"

const char mimetypeStr[] = "application/vnd.oasis.opendocument.graphics";

const char manifestStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
		" <manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.drawing\" manifest:full-path=\"/\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>"
//		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>"
		"</manifest:manifest>";

class OdgOutputFileHelper : public OutputFileHelper
{
public:
	OdgOutputFileHelper(const char *outFileName) :
		OutputFileHelper(outFileName) {}
	~OdgOutputFileHelper() {}

private:
	bool _isSupportedFormat(WPXInputStream *input)
	{
		bool retVal = libwpg::WPGraphics::isSupported(input);
		if (!retVal)
 			fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid WordPerfect Graphics.\n");
		return retVal;		
	}

	bool _convertDocument(WPXInputStream *input, DocumentHandler *handler, bool isFlatXML)
	{
		OdgExporter exporter(handler, isFlatXML);
		return libwpg::WPGraphics::parse(input, &exporter);
	}
};

int main (int argc, char *argv[])
{
	if (argc < 2) 
	{
		fprintf(stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", argv[0]);
		fprintf(stderr, "USAGE : Where <infile> is the WordPerfect Graphics source image\n");
		fprintf(stderr, "USAGE : and [outfile] is the odg target document. Alternately,\n");
		fprintf(stderr, "USAGE : pass '--stdout' or simply omit the [outfile] to pipe the\n");
		fprintf(stderr, "USAGE : resultant document as flat XML to standard output\n");
		fprintf(stderr, "USAGE : \n");
		return 1;
	}

	char *szInputFile;
	char *szOutFile;

	if (argc == 2)
	{
		szInputFile = argv[1];
		szOutFile = NULL;
	}
	else if (!strcmp(argv[1], "--stdout"))
	{
	        szInputFile = argv[2];
	        szOutFile = NULL;
	}
	else
	{
	        szInputFile = argv[1];
		szOutFile = argv[2];
	}
	
	OdgOutputFileHelper helper(szOutFile);

	if (!helper.writeChildFile("mimetype", mimetypeStr, (char)0)) {
		fprintf(stderr, "ERROR : Couldn't write mimetype\n");
		return 1;
	}

	if (!helper.writeChildFile("META-INF/manifest.xml", manifestStr)) {
		fprintf(stderr, "ERROR : Couldn't write manifest\n");
		return 1;
	}
	
	if (!helper.writeConvertedContent("content.xml", szInputFile)) 
	{
	        fprintf(stderr, "ERROR : Couldn't write document content\n");
	        return 1;
	}
	
	return 0;
}
