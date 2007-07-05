/* wpg2odg:
 *
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

#include <libwpg/libwpg.h>
#include <libwpd/GSFStream.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#include <gsf/gsf-input-stdio.h>

#include <stdio.h>
#include <string.h>

#include "OdgExporter.hxx"
#include "DiskDocumentHandler.hxx"
#include "StdOutHandler.hxx"

const char mimetypeStr[] = "application/vnd.oasis.opendocument.drawing";

const char manifestStr[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">"
		" <manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.drawing\" manifest:full-path=\"/\"/>"
		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>"
//		" <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>"
		"</manifest:manifest>";

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
	if (NULL != (child = gsf_outfile_new_child_full  (outfile, fileName, FALSE,"compression-level", 0, (void*)0)))
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
	GError *err = NULL;
	GsfInput *pGsfInput = NULL;
	if (!(pGsfInput = GSF_INPUT(gsf_input_stdio_new (pInFileName, &err)))) 
	{
		g_return_val_if_fail (err != NULL, 1);
		
		g_warning ("'%s' error: %s", pInFileName, err->message);
		if (err)
			g_error_free (err);
		g_object_unref(pGsfInput);
		return false;
	}
	if (err)
		g_error_free(err);
	GSFInputStream input(pGsfInput);

 	if (!libwpg::WPGraphics::isSupported(&input))
 	{
 		fprintf(stderr, "ERROR: We have no confidence that you are giving us a valid WordPerfect document.\n");
		g_object_unref(pGsfInput);
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

	OdgExporter exporter(pHandler, tmpIsFlatXML);
	bool bRetVal = libwpg::WPGraphics::parse(&input, &exporter);

	if (pContentChild)
	{
	        gsf_output_close(pContentChild);
	        g_object_unref(G_OBJECT (pContentChild));
	}
	delete pHandler;

	g_object_unref(pGsfInput);

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
		fprintf(stderr, "USAGE : Where <infile> is the WordPerfect Graphics source image\n");
		fprintf(stderr, "USAGE : and [outfile] is the odg target document. Alternately,\n");
		fprintf(stderr, "USAGE : pass '--stdout' to pipe the resultant document as flat XML to\n");
		fprintf(stderr, "USAGE : standard output\n");
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
	                g_return_val_if_fail (err != NULL, 1);
	                
	                g_warning ("'%s' error: %s", argv[2], err->message);
			if (err)
	                        g_error_free (err);
			gsf_shutdown ();
	                return 1;
	        }
		if (err)
			g_error_free (err);
		err = NULL;
	        pOutfile = GSF_OUTFILE(gsf_outfile_zip_new (pOutput, &err));
	        if (pOutfile == NULL) {
	                g_return_val_if_fail (err != NULL, 1);
	                
	                g_warning ("'%s' error: %s",
	                           "gsf_outfile_zip_new", err->message);
			if (err)
	                        g_error_free (err);
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
	
	if (!writeContent(szInputFile, pOutfile)) 
	{
	        fprintf(stderr, "ERROR : Couldn't write document content\n");
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
