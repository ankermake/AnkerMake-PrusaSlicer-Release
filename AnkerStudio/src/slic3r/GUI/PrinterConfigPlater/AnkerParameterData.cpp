#include "AnkerParameterData.hpp"

AnkerParameterData::AnkerParameterData()
{
	initData();
}



void AnkerParameterData::initData()
{	
	m_fillPatternData = { {("Rectilinear"), _L("Rectilinear")},
						{("Aligned Rectilinear"), _L("Aligned Rectilinear")},
						{("Alignedrectilinear"), _L("Aligned Rectilinear")},
						{("Grid"), _L("Grid")},
						{("Triangles"), _L("Triangles")},
						{("Stars"), _L("Stars")},
						{("Cubic"), _L("Cubic")},
						{("Line"), _L("Line")},
						{("Concentric"), _L("Concentric")},
						{("Honeycomb"), _L("Honeycomb")},
						{("3D Honeycomb"), _L("3D Honeycomb")},
						{("3dhoneycomb"), _L("3D Honeycomb")},
						{("Gyroid"), _L("Gyroid")},
						{("Hilbert Curve"), _L("Hilbert Curve")},
						{("Hilbertcurve"), _L("Hilbert Curve")},
						{("Archimedean Chords"),_L("Archimedean Chords")},
						{("Archimedeanchords"),_L("Archimedean Chords")},
						{("Octagram Spiral"),_L("Octagram Spiral")},
						{("Octagramspiral"),_L("Octagram Spiral")},
						{("Adaptive Cubic"), _L("Adaptive Cubic")},
						{("Adaptivecubic"), _L("Adaptive Cubic")},
						{("Support Cubic"), _L("Support Cubic")},
						{("Supportcubic"), _L("Support Cubic")},
						{("Lightning"), _L("Lightning")},
						{("Monotonic"), _L("Monotonic")},
						{("Monotonic Lines"), _L("Monotonic Lines")},
						{("Monotoniclines"), _L("Monotonic Lines")},
	};


	m_fillPatternMap = {
		{ 0,        Slic3r::InfillPattern::ipRectilinear },
		{ 1,          Slic3r::InfillPattern::ipAlignedRectilinear },
		{ 2,     			Slic3r::InfillPattern::ipGrid },
		{ 3, 			Slic3r::InfillPattern::ipTriangles },
		{ 4,              Slic3r::InfillPattern::ipStars },
		{ 5,              Slic3r::InfillPattern::ipCubic },
		{ 6,               Slic3r::InfillPattern::ipLine },
		{ 7,         Slic3r::InfillPattern::ipConcentric },
		{ 8,          Slic3r::InfillPattern::ipHoneycomb },
		{ 9,        Slic3r::InfillPattern::ip3DHoneycomb },
		{ 10,            Slic3r::InfillPattern::ipGyroid },
		{ 11,       Slic3r::InfillPattern::ipHilbertCurve },
		{ 12,  Slic3r::InfillPattern::ipArchimedeanChords },
		{ 13,     Slic3r::InfillPattern::ipOctagramSpiral },
		{ 14,      Slic3r::InfillPattern::ipAdaptiveCubic },
		{ 15,       Slic3r::InfillPattern::ipSupportCubic },
		{ 16,          Slic3r::InfillPattern::ipLightning }
	};

	m_tabfillPatternMap = {
		{ 0,        Slic3r::InfillPattern::ipRectilinear },
		{ 1,          Slic3r::InfillPattern::ipMonotonic },
		{ 2,     			Slic3r::InfillPattern::ipMonotonicLines },
		{ 3, 			Slic3r::InfillPattern::ipAlignedRectilinear },
		{ 4,              Slic3r::InfillPattern::ipConcentric },
		{ 5,              Slic3r::InfillPattern::ipHilbertCurve },
		{ 6,               Slic3r::InfillPattern::ipArchimedeanChords },
		{ 7,         Slic3r::InfillPattern::ipOctagramSpiral },
	};


	m_singlePerimeterTypeMap = {
	{ 0,        Slic3r::SinglePerimeterType::None },
	{ 1,		Slic3r::SinglePerimeterType::TopSurfaces },
	{ 2,		Slic3r::SinglePerimeterType::TopmostOnly },
	};

	m_StyleMap = {
		{0, Slic3r::SupportMaterialStyle::smsGrid},
		{1, Slic3r::SupportMaterialStyle::smsSnug},
		{2, Slic3r::SupportMaterialStyle::smsOrganic},
	};
	m_ReVerStyleMap = {
		{Slic3r::SupportMaterialStyle::smsGrid, 0},
		{Slic3r::SupportMaterialStyle::smsSnug, 1},
		{Slic3r::SupportMaterialStyle::smsOrganic, 2},
	};
}


void AnkerParameterData::getItemList(wxStringList& list, ControlListType listType)
{
	list.Clear();
	switch (listType)
	{
	case List_Seam_Position:
	{
		list.Add(_L("Random"));
		list.Add(_L("Nearest"));
		list.Add(_L("Aligned"));
		list.Add(_L("Rear"));
	}
	break;
	case List_Perimeter_generator:
	{
		list.Add(_L("Classic"));
		list.Add(_L("Arachne"));
	}
	break;
	case List_Fuzzy_skin:
	{
		list.Add(_L("None"));
		list.Add(_L("Outside walls"));
		list.Add(_L("All walls"));
	}
	break;
	case List_Fill_density:
	{
		list.Add(_L("0%"));
		list.Add(_L("5%"));
		list.Add(_L("10%"));
		list.Add(_L("15%"));
		list.Add(_L("20%"));
		list.Add(_L("25%"));
		list.Add(_L("30%"));
		list.Add(_L("40%"));
		list.Add(_L("50%"));
		list.Add(_L("60%"));
		list.Add(_L("70%"));
		list.Add(_L("80%"));
		list.Add(_L("90%"));
		list.Add(_L("100%"));
	}
	break;
	case List_Fill_pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Aligned Rectilinear"));
		list.Add(_L("Grid"));
		list.Add(_L("Triangles"));
		list.Add(_L("Stars"));
		list.Add(_L("Cubic"));
		list.Add(_L("Line"));
		list.Add(_L("Concentric"));
		list.Add(_L("Honeycomb"));
		list.Add(_L("3D Honeycomb"));
		list.Add(_L("Gyroid"));
		list.Add(_L("Hilbert Curve"));
		list.Add(_L("Archimedean Chords"));
		list.Add(_L("Octagram Spiral"));
		list.Add(_L("Adaptive Cubic"));
		list.Add(_L("Support Cubic"));
		list.Add(_L("Lightning"));
	}
	break;
	case List_Length_of_th_infill_anchor:
	{
		list.Add(_L("0 (no open anchors)"));
		list.Add(_L("1 mm"));
		list.Add(_L("2 mm"));
		list.Add(_L("5 mm"));
		list.Add(_L("10 mm"));
		list.Add(_L("1000 (unlimited)"));
	}
	break;
	case List_Maximum_length_of_the_infill_anchor:
	{
		list.Add(_L("0 (not anchored)"));
		list.Add(_L("1 mm"));
		list.Add(_L("2 mm"));
		list.Add(_L("5 mm"));
		list.Add(_L("10 mm"));
		list.Add(_L("1000 (unlimited)"));
	}
	break;
	case List_Top_fill_pattern:
	case List_Bottom_fill_pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Monotonic"));
		list.Add(_L("Monotonic Lines"));
		list.Add(_L("Aligned Rectilinear"));
		list.Add(_L("Concentric"));
		list.Add(_L("Hilbert Curve"));
		list.Add(_L("Archimedean Chords"));
		list.Add(_L("Octagram Spiral"));
	}
	break;
	case List_Top_surface_single_perimeter:
	{
		list.Add(_L("None"));
		list.Add(_L("All Top Surfaces"));
		list.Add(_L("Only Topmost Surface"));
	}
	break;
	case List_Ironing_Type:
	{
		list.Add(_L("No ironing"));
		list.Add(_L("All top surfaces"));
		list.Add(_L("Topmost surface only"));
		list.Add(_L("All solid surfaces"));
	}
	break;
	case List_Draft_shield:
	{
		list.Add(_L("Disabled"));
		list.Add(_L("Limited"));
		list.Add(_L("Enabled"));
	}
	break;
	case List_Brim_type:
	{
		list.Add(_L("Auto"));
		list.Add(_L("engin_brim_type_mouse_ear"));
		list.Add(_L("No brim"));
		list.Add(_L("Outer brim only"));
		list.Add(_L("Inner brim only"));
		list.Add(_L("Outer and inner brim"));
	}
	break;
	case List_Style:
	{
		list.Add(_L("Grid"));
		list.Add(_L("Snug"));
		list.Add(_L("Organic"));
	}
	break;
	case List_Top_contact_Z_distance:
	{
		list.Add(_L("0 (soluble)"));
		list.Add(_L("0.1 (detachable)"));
		list.Add(_L("0.2 (detachable)"));
	}
	break;
	case List_Bottom_contact_Z_distance:
	{
		list.Add(_L("Same as top"));
		list.Add(_L("0.1"));
		list.Add(_L("0.2"));
	}
	break;
	case List_Pattern:
	{
		list.Add(_L("Rectilinear"));
		list.Add(_L("Rectilinear grid"));
		list.Add(_L("Honeycomb"));
	}
	break;
	case List_Top_interface_layers:
	{
		list.Add(_L("0 (off)"));
		list.Add(_L("1 (light)"));
		list.Add(_L("2 (default)"));
		list.Add(_L("3 (heavy)"));
	}
	break;
	case List_Bottom_interface_layers:
	{
		list.Add(_L("Same as top"));
		list.Add(_L("0 (off)"));
		list.Add(_L("1 (light)"));
		list.Add(_L("2 (default)"));
		list.Add(_L("3 (heavy)"));
	}
	break;
	case List_interface_pattern:
	{
		list.Add(_L("Default"));
		list.Add(_L("Rectilinear"));
		list.Add(_L("Concentric"));
	}
	break;
	case List_Slicing_Mode:
	{
		list.Add(_L("Regular"));
		list.Add(_L("Even-odd"));
		list.Add(_L("Close holes"));
	}
	break;
	case List_ironing_pattern:
	{
		list.Add(_L("Concentric"));
		list.Add(_L("Rectilinear"));
	}
	break;
	case List_print_order:
	{
		list.Add(_L("engin_option_wall_sequence_inner_outer"));
		list.Add(_L("engin_option_wall_sequence_outer_inner"));
		list.Add(_L("engin_option_wall_sequence_inner_outer_innner"));
	}
	break;
	default:
		break;
	}
}


