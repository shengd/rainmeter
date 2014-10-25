#ifndef __METERSEGMENTEDLINE_H__
#define __METERSEGMENTEDLINE_H__

#include "Meter.h"

class MeterSegmentedLine : public Meter
{
public:
	MeterSegmentedLine(Skin* skin, const WCHAR* name);
	virtual ~MeterSegmentedLine();

	MeterSegmentedLine(const MeterSegmentedLine& other) = delete;
	MeterSegmentedLine& operator=(MeterSegmentedLine other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterSegmentedLine>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw(Gfx::Canvas& canvas);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void BindMeasures(ConfigParser& parser, const WCHAR* section);

private:
	static std::vector<Gdiplus::Color> defColor;

	std::vector<std::vector<Gdiplus::Color>> m_Colors;	//outer vector is series, inner vector is segments
	std::vector<double> m_ScaleValues;

	std::vector<INT> m_Segments;
	std::vector<INT> m_SegmentDividers;

	bool m_Autoscale;
	double m_LineWidth;

	std::vector<std::vector<double>> m_AllValues;		//outer vector is series, inner vector is data points
	std::vector<Gdiplus::REAL*> m_Points;
	int m_CurrentPos;
	int m_DataWidth;

	int m_CurveFitMethod;
	double m_SmoothAmount;

	bool m_GraphStartLeft;
};

#endif
