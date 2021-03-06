/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "MeterSegmentedLine.h"
#include "Measure.h"
#include "Logger.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

/*
** The default color
**
*/
std::vector<Color> MeterSegmentedLine::defColor = std::vector<Color>(1, Color::White);

/*
** The constructor
**
*/
MeterSegmentedLine::MeterSegmentedLine(Skin* skin, const WCHAR* name) : Meter(skin, name),
	m_Autoscale(false),
	m_LineWidth(1.0),
	m_CurrentPos(),
	m_GraphStartLeft(false)
{
}

/*
** The destructor
**
*/
MeterSegmentedLine::~MeterSegmentedLine()
{
}

/*
** create the buffer for the lines
**
*/
void MeterSegmentedLine::Initialize()
{
	Meter::Initialize();												//flag this class as initialized

	size_t colorsSize = m_Colors.size();								//get the number of colors
	size_t allValuesSize = m_AllValues.size();							//get the number of series
	size_t num = (allValuesSize > 0) ? m_AllValues[0].size() : 0;		//if there is at least one series, get the length of the first series
	int maxSize = m_DataWidth;											//get the primary axis length

	if (colorsSize != allValuesSize)
	{
		if (colorsSize > allValuesSize)									//if there are more colors than series
		{
			for (size_t i = allValuesSize; i < colorsSize; ++i)			//add empty series until there are as many series as colors
			{
				m_AllValues.push_back(std::vector<double>());

				if (maxSize > 0)
				{
					m_AllValues.back().assign(maxSize, 0.0);
				}
			}
		}
		else															//if there are more series than colors
		{
			m_AllValues.resize(colorsSize);
		}
	}

	if (maxSize < 0 || num != (size_t)maxSize)							//resize all data series to match meter width
	{
		if (m_CurrentPos >= maxSize) m_CurrentPos = 0;

		num = (maxSize < 0) ? 0 : maxSize;
		for (size_t i = 0; i < allValuesSize; ++i)
		{
			if (num != m_AllValues[i].size())
			{
				m_AllValues[i].resize(num, 0.0);
			}
		}
	}
}

/*
** Read the options specified in the ini file.
**
*/
void MeterSegmentedLine::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	WCHAR tmpName[64];

	//Store the current number of lines so we know if the buffer needs to be updated
	int oldLineCount = (int)m_Colors.size();
	int oldSize = m_DataWidth;

	//Read series options
	Meter::ReadOptions(parser, section);

	int lineCount = parser.ReadInt(section, L"LineCount", 1);

	m_Colors.clear();
	m_ScaleValues.clear();
	for (auto buffer = m_Points.cbegin(); buffer != m_Points.cend(); buffer++)
		delete[] * buffer;
	m_Points.clear();

	for (int i = 0; i < lineCount; ++i)			//read colors, and scales one by one for each series
	{
		if (i == 0)
		{
			wcsncpy_s(tmpName, L"LineColor", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"LineColor%i", i + 1);
		}

		m_Colors.push_back(parser.ReadMultipleColors(section, tmpName, defColor, std::vector<Color>()));

		if (i == 0)
		{
			wcsncpy_s(tmpName, L"Scale", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"Scale%i", i + 1);
		}

		m_ScaleValues.push_back(parser.ReadFloat(section, tmpName, 1.0));

		REAL* pointsBuffer = new REAL[m_W];
		for (int j = 0; j < m_W; j++)
			pointsBuffer[j] = 0;
		m_Points.push_back(pointsBuffer);
	}

	//Read in segment definitions
	int segmentCount = parser.ReadInt(section, L"SegmentCount", 1);

	m_Segments.clear();
	m_SegmentDividers.clear();
	m_DataWidth = 0;
	int lastMarker = 0,
		currMarker = 0,
		divider = 1;

	for (int i = 1; i < segmentCount; ++i)
	{
		if (i == 1)
		{
			wcsncpy_s(tmpName, L"Segment", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"Segment%i", i);
		}

		currMarker = parser.ReadUInt(section, tmpName, 0);
		m_Segments.push_back(currMarker);

		if (i == 1)
		{
			wcsncpy_s(tmpName, L"SegmentUpdateDivider", _TRUNCATE);
		}
		else
		{
			_snwprintf_s(tmpName, _TRUNCATE, L"SegmentUpdateDivider%i", i);
		}

		divider = parser.ReadUInt(section, tmpName, 1);
		m_SegmentDividers.push_back(divider);

		//calculate data buffer width
		m_DataWidth += divider * (currMarker - lastMarker);
		lastMarker = currMarker;

		m_DataWriteOffset = max(m_DataWriteOffset, divider);
	}

	_snwprintf_s(tmpName, _TRUNCATE, L"SegmentUpdateDivider%i", segmentCount);
	divider = parser.ReadUInt(section, tmpName, 1);
	m_SegmentDividers.push_back(divider);

	m_DataWidth += divider * (m_W - lastMarker);
	m_DataWidth += m_DataWriteOffset;

	//Read in other options
	m_Autoscale = parser.ReadBool(section, L"AutoScale", false);
	m_LineWidth = parser.ReadFloat(section, L"LineWidth", 1.0);
	m_CurveFitMethod = parser.ReadUInt(section, L"CurveFit", 0);
	m_SmoothAmount = parser.ReadFloat(section, L"Smooth", 0.0);

	//More options
	const WCHAR* graph = parser.ReadString(section, L"GraphStart", L"RIGHT").c_str();
	if (_wcsicmp(graph, L"RIGHT") == 0)
	{
		m_GraphStartLeft = false;
	}
	else if (_wcsicmp(graph, L"LEFT") ==  0)
	{
		m_GraphStartLeft = true;
	}
	else
	{
		LogErrorF(this, L"GraphStart=%s is not valid", graph);
	}

	//If this is just a refresh, update our size and reinitialize if needed
	if (m_Initialized)
	{
		int maxSize = m_DataWidth;
		if (oldLineCount != lineCount || oldSize != maxSize)
		{
			m_AllValues.clear();
			m_CurrentPos = 0;
			Initialize();
		}
	}
}

/*
** Updates the value(s) from the measures.
** Returns true if the meter was updated
**
*/
bool MeterSegmentedLine::Update()
{
	if (Meter::Update() && !m_Measures.empty())
	{
		int maxSize = m_DataWidth;

		if (maxSize > 0)
		{
			int allValuesSize = (int)m_AllValues.size();
			int counter = 0;
			int currInd = m_CurrentPos - m_DataWriteOffset;
			if (currInd < 0)
				currInd += maxSize;
			int prevInd = currInd - 1;
			if (prevInd < 0)
				prevInd += maxSize;
			for (auto i = m_Measures.cbegin(); counter < allValuesSize && i != m_Measures.cend(); ++i, ++counter)
			{
				double prevVal = m_AllValues[counter][prevInd];
				m_AllValues[counter][currInd] = (*i)->GetValue() * (1 - m_SmoothAmount) + prevVal * m_SmoothAmount;
			}

			++m_CurrentPos;
			m_CurrentPos %= maxSize;
		}
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterSegmentedLine::Draw(Gfx::Canvas& canvas)
{
	int maxSize = m_DataWidth;
	if (!Meter::Draw(canvas) || maxSize <= 0) return false;
	
	Gdiplus::Graphics& graphics = canvas.BeginGdiplusContext();

	double maxValue = 0.0;
	int counter = 0;

	// Find the maximum value
	if (m_Autoscale)
	{
		double newValue = 0;
		counter = 0;
		for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
		{
			double scale = m_ScaleValues[counter];
			for (auto j = (*i).cbegin(); j != (*i).cend(); ++j)
			{
				double val = (*j) * scale;
				newValue = max(newValue, val);
			}
			++counter;
		}

		// Scale the value up to nearest power of 2
		if (newValue > DBL_MAX / 2.0)
		{
			maxValue = DBL_MAX;
		}
		else
		{
			maxValue = 2.0;
			while (maxValue < newValue)
			{
				maxValue *= 2.0;
			}
		}
	}
	else
	{
		for (auto i = m_Measures.cbegin(); i != m_Measures.cend(); ++i)
		{
			double val = (*i)->GetMaxValue();
			maxValue = max(maxValue, val);
		}

		if (maxValue == 0.0)
		{
			maxValue = 1.0;
		}
	}

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	// Draw all the lines
	const REAL H = meterRect.Height - 1.0f;
	counter = 0;
	auto pointsBuffer = m_Points.cbegin();
	for (auto i = m_AllValues.cbegin(); i != m_AllValues.cend(); ++i)
	{
		// Draw a line
		REAL Y, oldY;

		const double scale = m_ScaleValues[counter] * H / maxValue;

		int pos = m_CurrentPos;

		auto calcY = [&](REAL& _y, REAL stepSize, int currPos)		//TODO: move this lambda elsewhere
		{
			_y = 0;
			switch (m_CurveFitMethod)
			{
				//first value
				case 0:	_y = (REAL)((*i)[currPos]);
						break;

				//maximum value
				case 1: for (int ind = 0; ind < stepSize; ind++)
							_y = max(_y, (REAL)((*i)[(currPos + ind) % m_DataWidth]));
						break;

				//arithmetic mean
				case 2: for (int ind = 0; ind < stepSize; ind++)
							_y += (REAL)((*i)[(currPos + ind) % m_DataWidth]);
						_y /= stepSize;
						break;

				default: _y = (REAL)((*i)[currPos]);
			}
			
			_y *= scale;
			_y = min(_y, H);
			_y = max(_y, 0.0f);
			_y = meterRect.Y + (H - _y);
		};

		// Cache all lines
		GraphicsPath path;
		int segmentInd = 0,
			step,
			divider;

		//compute y values
		step = m_SegmentDividers[m_SegmentDividers.size() - 1];
		divider = m_Segments.size() > 0 ? m_W - m_Segments[m_Segments.size() - 1] : m_W;
		for (int j = 0; j < m_W; ++j)
		{
			calcY(Y, step, pos - pos % step);
			(*pointsBuffer)[j] = Y;

			if (segmentInd < m_Segments.size() && j >= divider)
			{
				segmentInd++;

				step = m_SegmentDividers[m_SegmentDividers.size() - segmentInd - 1];
				divider = segmentInd != m_Segments.size() ? m_W - m_Segments[m_Segments.size() - segmentInd - 1] : m_W;
			}

			pos += step;
			pos %= m_DataWidth;
		}
		
		//draw y values
		segmentInd = 0;
		divider = m_Segments.size() > 0 ? m_W - m_Segments[m_Segments.size() - segmentInd - 1] : m_W;
		if (!m_GraphStartLeft)
		{
			for (int j = 1; j < m_W; ++j)
			{
				if (segmentInd < m_Segments.size() && j >= divider)
				{
					segmentInd++;
					path.SetMarker();
					path.StartFigure();

					divider = segmentInd != m_Segments.size() ? m_W - m_Segments[m_Segments.size() - segmentInd - 1] : m_W;
				}

				path.AddLine((REAL)(meterRect.X + j - 1), (*pointsBuffer)[j - 1], (REAL)(meterRect.X + j), (*pointsBuffer)[j]);
			}
		}
		else
		{
			for (int j = 1; j < m_W; ++j)
			{
				if (segmentInd < m_Segments.size() && j >= divider)
				{
					segmentInd++;
					path.SetMarker();
					path.StartFigure();
					divider = segmentInd != m_Segments.size() ? m_W - m_Segments[m_Segments.size() - segmentInd - 1] : m_W;
				}

				path.AddLine((REAL)(meterRect.X + meterRect.Width - j), (*pointsBuffer)[j - 1], (REAL)(meterRect.X + meterRect.Width - j - 1), (*pointsBuffer)[j]);
			}
		}

		// Draw cached lines
		GraphicsPathIterator pathIter(&path);
		GraphicsPath subPath;
		for (auto color = m_Colors[counter].rbegin(); color != m_Colors[counter].rend(); ++color)
		{
			pathIter.NextMarker(&subPath);

			Pen pen(*color, (REAL)m_LineWidth);
			pen.SetLineJoin(LineJoinRound);
			graphics.DrawPath(&pen, &subPath);
		}

		++counter;
		++pointsBuffer;
	}

	canvas.EndGdiplusContext();

	return true;
}

/*
** Overwritten method to handle the other measure bindings.
**
*/
void MeterSegmentedLine::BindMeasures(ConfigParser& parser, const WCHAR* section)
{
	if (BindPrimaryMeasure(parser, section, false))
	{
		BindSecondaryMeasures(parser, section);
	}
}
