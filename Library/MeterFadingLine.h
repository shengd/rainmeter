#ifndef __METERFADINGLINE_H__
#define __METERFADINGLINE_H__

#include "Meter.h"

class MeterFadingLine : public Meter
{
public:
	MeterFadingLine(Skin* skin, const WCHAR* name);
	virtual ~MeterFadingLine();

	MeterFadingLine(const MeterFadingLine& other) = delete;
	MeterFadingLine& operator=(MeterFadingLine other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterFadingLine>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	std::vector<Gdiplus::Color> m_Colors;
	std::vector<double> m_ScaleValues;

	bool m_Autoscale;
	bool m_HorizontalLines;
	bool m_Flip;
	double m_LineWidth;
	Gdiplus::Color m_HorizontalColor;

	std::vector< std::vector<double> > m_AllValues;
	int m_CurrentPos;

	bool m_GraphStartLeft;
	bool m_GraphHorizontalOrientation;
};

#endif
