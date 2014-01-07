/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* libodfgen
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
 * Copyright (C) 2006 Fridrich Strba (fridrich.strba@bluewin.ch)
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

/* "This product is not manufactured, approved, or supported by
 * Corel Corporation or Corel Corporation Limited."
 */

#include <iostream>
#include <fstream>

#include <librevenge/librevenge.h>
#include <libodfgen/libodfgen.hxx>

#include "StringDocumentHandler.hxx"

template <class Generator>
static void sendChart(Generator &generator)
{
	librevenge::RVNGPropertyList style1;
	style1.insert("librevenge:chart-id", "1");
	style1.insert("draw:stroke", "none");
	style1.insert("draw:fill", "none");
	generator.defineChartStyle(style1);

	librevenge::RVNGPropertyList chart;
	chart.insert("svg:width", 400, librevenge::RVNG_POINT);
	chart.insert("svg:height", 300, librevenge::RVNG_POINT);
	chart.insert("chart:class", "bar");
	chart.insert("librevenge:chart-id", 1);
	generator.openChart(chart);

	librevenge::RVNGPropertyList style(style1);
	style.insert("librevenge:chart-id", "2");
	style.insert("chart:auto-position", "true");
	style.insert("draw:stroke", "none");
	style.insert("draw:fill", "none");
	style.insert("fo:font-size", "10pt");
	generator.defineChartStyle(style);
	style.remove("chart:auto-position");

	librevenge::RVNGPropertyList legend;
	legend.insert("librevenge:zone-type", "legend");
	legend.insert("svg:x", 40, librevenge::RVNG_POINT);
	legend.insert("svg:y", 50, librevenge::RVNG_POINT);
	legend.insert("chart:legend-position", "end");
	legend.insert("style:legend-expansion", "high");
	legend.insert("librevenge:chart-id", 2);
	generator.openChartTextObject(legend);
	generator.closeChartTextObject();


	librevenge::RVNGPropertyList plotStyle, plotArea, floor, floorStyle, wall, wallStyle;
	plotStyle.insert("librevenge:chart-id", 3);
	plotStyle.insert("chart:include-hidden-cells","false");
	plotStyle.insert("chart:auto-position","true");
	plotStyle.insert("chart:auto-size","true");
	plotStyle.insert("chart:treat-empty-cells","leave-gap");
	plotStyle.insert("chart:right-angled-axes","true");
	generator.defineChartStyle(plotStyle);
	floorStyle.insert("librevenge:chart-id", 9);
	floorStyle.insert("draw:stroke","solid");
	floorStyle.insert("svg:stroke-color","#b3b3b3");
	floorStyle.insert("draw:fill","none");
	generator.defineChartStyle(floorStyle);
	wallStyle.insert("librevenge:chart-id", 10);
	wallStyle.insert("draw:stroke","solid");
	wallStyle.insert("svg:stroke-color","#b3b3b3");
	wallStyle.insert("draw:fill","solid");
	wallStyle.insert("draw:fill-color","#ffffff");
	generator.defineChartStyle(wallStyle);

	librevenge::RVNGPropertyListVector vect;
	plotArea.insert("svg:x", 20, librevenge::RVNG_POINT);
	plotArea.insert("svg:y", 20, librevenge::RVNG_POINT);
	plotArea.insert("svg:width", 350, librevenge::RVNG_POINT);
	plotArea.insert("svg:height", 250, librevenge::RVNG_POINT);
	plotArea.insert("librevenge:chart-id", 3);
	floor.insert("librevenge:type", "floor");
	floor.insert("librevenge:chart-id", "9");
	vect.append(floor);
	wall.insert("librevenge:type", "wall");
	wall.insert("librevenge:chart-id", "10");
	vect.append(wall);
	plotArea.insert("librevenge:childs", vect);
	generator.openChartPlotArea(plotArea);

	librevenge::RVNGPropertyList axisStyle(style);
	axisStyle.insert("librevenge:chart-id", "4");
	axisStyle.insert("draw:stroke", "solid");
	axisStyle.insert("svg:stroke-color", "#b3b3b3");
	axisStyle.insert("chart:display-label","true");
	axisStyle.insert("chart:logarithmic","false");
	axisStyle.insert("chart:reverse-direction","false");
	axisStyle.insert("text:line-break","false");
	axisStyle.insert("chart:axis-position","0");
	generator.defineChartStyle(axisStyle);
	style.insert("librevenge:chart-id", "5");
	style.insert("draw:stroke", "solid");
	style.insert("draw:stroke-color", "#b3b3b3");
	generator.defineChartStyle(style);

	librevenge::RVNGPropertyList axis, grid;
	axis.insert("chart:dimension","x");
	axis.insert("chart:name","primary-x");
	axis.insert("librevenge:chart-id", 4);
	generator.insertChartAxis(axis);

	axis.insert("chart:dimension","y");
	axis.insert("chart:name","primary-y");
	grid.insert("librevenge:type","grid");
	grid.insert("chart:class","major");
	grid.insert("librevenge:chart-id", 5);
	vect.clear();
	vect.append(grid);
	axis.insert("librevenge:childs", vect);
	generator.insertChartAxis(axis);

	style.insert("librevenge:chart-id", "7");
	style.insert("draw:stroke", "none");
	style.insert("draw:fill", "solid");
	style.insert("draw:fill-color", "#004586");
	style.insert("dr3d:edge-rounding", 0.05, librevenge::RVNG_PERCENT);
	generator.defineChartStyle(style);

	librevenge::RVNGPropertyList serie, range, datapoint;
	range.insert("librevenge:sheet-name", "MySheet");
	range.insert("librevenge:start-row", 1);
	range.insert("librevenge:start-column", 0);
	range.insert("librevenge:end-row", 1);
	range.insert("librevenge:end-column", 2);
	vect.clear();
	vect.append(range);
	serie.insert("chart:class","chart:bar");
	serie.insert("chart:values-cell-range-address", vect);
	vect.clear();
	datapoint.insert("librevenge:type", "data-point");
	datapoint.insert("chart:repeated", 2);
	vect.append(datapoint);
	serie.insert("librevenge:childs", vect);
	serie.insert("librevenge:chart-id", "7");
	generator.openChartSerie(serie);
	generator.closeChartSerie();

	style.insert("librevenge:chart-id", "8");
	style.insert("draw:fill-color", "#ff420e");
	generator.defineChartStyle(style);

	range.insert("librevenge:sheet-name", "MySheet");
	range.insert("librevenge:start-row", 2);
	range.insert("librevenge:start-column", 0);
	range.insert("librevenge:end-row", 2);
	range.insert("librevenge:end-column", 2);
	vect.clear();
	vect.append(range);
	serie.insert("chart:values-cell-range-address", vect);
	serie.insert("librevenge:chart-id", "8");
	generator.openChartSerie(serie);
	generator.closeChartSerie();

	generator.closeChartPlotArea();

	generator.closeChart();
}

static void createOds()
{
	StringDocumentHandler content;
	OdsGenerator generator;
	generator.addDocumentHandler(&content, ODF_FLAT_XML);

	generator.startDocument(librevenge::RVNGPropertyList());
	librevenge::RVNGPropertyList page;
	page.insert("librevenge:num-pages", 1);
	page.insert("fo:page-height", 11.5, librevenge::RVNG_INCH);
	page.insert("fo:page-width", 9, librevenge::RVNG_INCH);
	page.insert("style:print-orientation", "portrait");
	page.insert("fo:margin-left", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-right", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-top", 0.1, librevenge::RVNG_INCH);
	page.insert("fo:margin-bottom", 0.1, librevenge::RVNG_INCH);
	generator.openPageSpan(page);

	librevenge::RVNGPropertyList list;
	librevenge::RVNGPropertyListVector columns;
	for (size_t c = 0; c < 3; c++)
	{
		librevenge::RVNGPropertyList column;
		column.insert("style:column-width", 3, librevenge::RVNG_INCH);
		columns.append(column);
	}
	list.insert("librevenge:columns", columns);
	list.insert("librevenge:sheet-name", "MySheet");
	generator.openSheet(list);

	librevenge::RVNGPropertyList frame;
	frame.insert("svg:x",0.2, librevenge::RVNG_INCH);
	frame.insert("svg:y",1.4, librevenge::RVNG_INCH);
	frame.insert("svg:width",400, librevenge::RVNG_POINT);
	frame.insert("svg:height",400, librevenge::RVNG_POINT);
	frame.insert("text:anchor-type", "page");
	frame.insert("text:anchor-page-number", 1);
	frame.insert("style:vertical-rel", "page");
	frame.insert("style:horizontal-rel", "page");
	frame.insert("style:horizontal-pos", "from-left");
	frame.insert("style:vertical-pos", "from-top");

	frame.insert("draw:stroke", "none");
	frame.insert("draw:fill", "none");
	generator.openFrame(frame);
	sendChart(generator);
	generator.closeFrame();

	librevenge::RVNGPropertyList numbering, span, row, cell;
	span.insert("style:font-name","Courier");
	span.insert("fo:font-size", 12, librevenge::RVNG_POINT);
	span.insert("librevenge:span-id",1);
	generator.defineCharacterStyle(span);
	span.clear();
	span.insert("librevenge:span-id",1);
	numbering.insert("librevenge:value-type","number");
	numbering.insert("librevenge:name", "Numbering0");
	generator.defineSheetNumberingStyle(numbering);

	row.insert("style:row-height", 40, librevenge::RVNG_POINT);
	generator.openSheetRow(row);
	cell.insert("librevenge:row", 0);
	for (int c=0; c<3; ++c)
	{
		cell.insert("librevenge:column", c);
		generator.openSheetCell(cell);
		static char const *(wh[3])= {"a","b","c"};
		generator.openParagraph(librevenge::RVNGPropertyList());
		generator.openSpan(span);
		generator.insertText(wh[c]);
		generator.closeSpan();
		generator.closeParagraph();
		generator.closeSheetCell();
	}
	generator.closeSheetRow();

	generator.openSheetRow(row);
	cell.insert("librevenge:row", 1);
	cell.insert("librevenge:numbering-name", "Numbering0");
	cell.insert("librevenge:value-type", "float");
	for (int c=0; c<3; ++c)
	{
		static double wh[3]= {0.1, 0.2, 0.3};
		cell.insert("librevenge:column", c);
		cell.insert("librevenge:value", wh[c], librevenge::RVNG_GENERIC);
		generator.openSheetCell(cell);
		generator.closeSheetCell();
	}
	generator.closeSheetRow();

	generator.openSheetRow(row);
	cell.insert("librevenge:row", 2);
	for (int c=0; c<3; ++c)
	{
		static double wh[3]= {2, 3, 10.3};
		cell.insert("librevenge:column", c);
		cell.insert("librevenge:value", wh[c], librevenge::RVNG_GENERIC);
		generator.openSheetCell(cell);
		generator.closeSheetCell();
	}
	generator.closeSheetRow();

	generator.closeSheet();

	generator.closePageSpan();
	generator.endDocument();

	std::ofstream file("testChart1.ods");
	file << content.cstr();
}

int main()
{
	createOds();
	return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 noexpandtab: */

