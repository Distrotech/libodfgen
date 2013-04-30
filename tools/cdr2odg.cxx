/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* writerperfect
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006-2007 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <libcdr/libcdr.h>

#include <libodfgen/libodfgen.hxx>

#include "OutputFileHelper.hxx"

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
		if (!libcdr::CDRDocument::isSupported(input))
		{
			fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid Corel Draw Document.\n");
			return false;
		}
		return true;
	}

	bool _convertDocument(WPXInputStream *input, const char * /* password */, OdfDocumentHandler *handler, OdfStreamType streamType)
	{
		OdgGenerator exporter(handler, streamType);
		return libcdr::CDRDocument::parse(input, &exporter);
	}
};

int printUsage(char *name)
{
	fprintf(stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", name);
	fprintf(stderr, "USAGE : Where <infile> is the Corel Draw source document\n");
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

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */
