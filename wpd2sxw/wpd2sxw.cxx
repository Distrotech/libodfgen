/* wpd2sxw:
 *
 * Copyright (C) 2002 Jon K Hellan (hellan@acm.org)
 * Copyright (C) 2002-2004 William Lachance (william.lachance@sympatico.ca)
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
 		printf("ERROR: We have no confidence that you are giving us a valid WordPerfect document.\n");
 		return false;
 	}
	input.seek(0, WPX_SEEK_SET);

        WordPerfectCollector collector;
        DocumentHandler *pHandler;
        GsfOutput *pContentChild = NULL;
        if (pOutfile)
        {
                pContentChild = gsf_outfile_new_child(pOutfile, "content.xml", FALSE);
                pHandler = new DiskDocumentHandler(pContentChild); // WLACH_REFACTORING: rename to DiskHandler
        }
        else
                pHandler = new StdOutHandler();
	bool bRetVal = collector.filter(input, *pHandler);

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
		fprintf (stderr, "USAGE : %s [--stdout] <infile> [outfile]\n", argv[0]);
		fprintf (stderr, "USAGE : Where <infile> is the WordPerfect source document\n");
		fprintf (stderr, "USAGE : and [outfile] is the sxw target document. Alternately,\n");
		fprintf (stderr, "USAGE : pass '--stdout' to pipe the resultant document to\n");
		fprintf (stderr, "USAGE : standard output\n");
		fprintf (stderr, "USAGE : \n");
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
        if (!writeContent(szInputFile, pOutfile)) 
        {
                fprintf (stderr, "ERROR : Couldn't write document content\n");
                return 1;
        }

	if (pOutfile && !writeChildFile(pOutfile, "mimetype", mimetypeStr)) {
		fprintf (stderr, "ERROR : Couldn't write mimetype\n");
		return 1;
	}

	if (pOutfile && !gsf_output_close ((GsfOutput *) pOutfile)) {
		fprintf (stderr, "ERROR : Couldn't close outfile\n");
		return 1;
	}

        if (pOutfile)
                g_object_unref (G_OBJECT (pOutfile));

	gsf_shutdown ();

	return 0;
}
