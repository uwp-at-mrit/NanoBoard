#include <map>

#include "device/gps_cs.hpp"

#include "graphlet/shapelet.hpp"
#include "graphlet/planetlet.hpp"

#include "module.hpp"

using namespace WarGrey::SCADA;

using namespace Windows::Foundation;

using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;

static CanvasTextFormat^ label_font = make_bold_text_format("Microsoft Yahei", 16.0F);

static CanvasSolidColorBrush^ label_color = Colours::DarkGray;
static CanvasSolidColorBrush^ region_border_color = Colours::DimGray;

#define GPS_Display_Vertex(v, ref, vs, id) vs[id]->set_value(v->parameter.ref)
#define GPS_Refresh_Vertex(v, ref, vs, id) v->parameter.ref = vs[id]->get_value()

/*************************************************************************************************/
namespace {
	// order matters
	private enum class GCS {
		// Ellipsoid
		a, f, CM,

		// Cartographic coordinate reference system 
		Tx, Ty, Tz, S, Rx, Ry, Rz,

		// Gauss-Krüger projection
		Dx, Dy, Dz, UTM_S,

		_,

		// testcase
		B, L, H, X, Y, Z, 

		__
	};
}

private class WarGrey::SCADA::GPSCSSelf {
public:
	GPSCSSelf(GPSCSPlanet* master, Platform::String^ gps, IGPSConvertor* gc)
		: master(master), label_max_width(0.0F), entity(nullptr), convertor(gc) {
		this->parameter_style = make_highlight_dimension_style(label_font->FontSize, 16U);
		this->parameter_style.unit_color = Colours::Transparent;

		this->input_style = make_highlight_dimension_style(label_font->FontSize, 12U);
		this->input_style.unit_color = Colours::Transparent;

		this->output_style = this->input_style;
		this->output_style.label_color = label_color;
		this->output_style.number_color = this->output_style.label_color;
		this->output_style.number_background_color = Colours::Transparent;
		this->output_style.label_background_color = Colours::Transparent;
		this->output_style.number_xfraction = 0.0F;

		this->gps = new GPSlet(gps, 128.0F);
	}

public:
	void load(CanvasCreateResourcesReason reason, float width, float height, float inset) {
		for (GCS id = _E0(GCS); id < GCS::_; id++) {
			int precision = -1;

			switch (id) {
			case GCS::a: case GCS::f: case GCS::CM: precision = 2; break;
			case GCS::Tx: case GCS::Ty: case GCS::Tz: precision = 5; break;
			case GCS::Rx: case GCS::Ry: case GCS::Rz: precision = 5; break;
			case GCS::Dx: case GCS::Dy: case GCS::Dz: precision = 2; break;
			case GCS::S: case GCS::UTM_S: precision = 2; break;
			}

			this->labels[id] = this->insert_label(id);
			this->ps[id] = this->insert_parameter_field(id, precision);
		}

		this->load_test_field(GCS::B, 3117.6797, 4);
		this->load_test_field(GCS::L, 12147.5565, 4);
		this->load_test_field(GCS::H, 30.0, 2);

		this->load_test_field(GCS::X, 3464324.75, 2);
		this->load_test_field(GCS::Y, 384959.47, 2);
		this->load_test_field(GCS::Z, 30.0, 2);

		{ // load regions
			float rcorner = inset * 0.25F;
			float gapsize = inset * 0.618F;
			float rwidth, rheight;

			this->is[GCS::B]->fill_extent(0.0F, 0.0F, &rwidth, &rheight);
			rwidth = (rwidth + gapsize) * 2.0F;
			rheight = (rheight + gapsize) * 3.0F + gapsize;

			this->regions[GCS::B] = this->master->insert_one(new RoundedRectanglet(rwidth, rheight, rcorner, nullptr, region_border_color));
			this->regions[GCS::X] = this->master->insert_one(new RoundedRectanglet(rwidth, rheight, rcorner, nullptr, region_border_color));
		}

		this->master->insert(this->gps);
		this->gps->set_north(-45.0);

		for (auto it = this->regions.begin(); it != this->regions.end(); it++) {
			it->second->camouflage(true);
		}
	}

	void reflow(IGraphlet* frame, float width, float height, float inset) {
		float xoff = inset + this->label_max_width;
		float pheight, iheight;

		this->ps[GCS::a]->fill_extent(0.0F, 0.0F, nullptr, &pheight);
		this->is[GCS::B]->fill_extent(0.0F, 0.0F, nullptr, &iheight);

		this->reflow_parameter_fields(frame, xoff, _E0(GCS), GCS::_, inset, pheight, GCS::Tx);
		this->master->move_to(this->regions[GCS::X], frame, GraphletAnchor::RB, GraphletAnchor::RB, -inset, -inset);
		this->master->move_to(this->regions[GCS::B], this->regions[GCS::X], GraphletAnchor::CT, GraphletAnchor::CB, 0.0F, -inset);
		this->reflow_test_fields(GCS::B, GCS::H, GCS::X, GCS::Z, inset, iheight);
		this->reflow_test_fields(GCS::X, GCS::Z, GCS::B, GCS::H, inset, iheight);

		this->master->move_to(this->gps, this->regions[GCS::B], 0.5F, frame, 0.0F, GraphletAnchor::CT, 0.0F, inset);
	}

	void on_graphlet_ready(IGraphlet* g) {
		if (this->gps == g) {
			this->entity = this->gps->clone_gpscs(this->entity);
			this->refresh_parameter_fields();
			this->refresh_output_fields();
		}
	}

public:
	bool on_edit(Credit<Dimensionlet, GCS>* dim) {
		long double new_value = dim->get_input_number();
		bool modified = (new_value != dim->get_value());

		if (modified) {
			dim->set_value(new_value);

			this->refresh_entity();
			this->refresh_output_fields();
		}

		return modified && (dim->id < GCS::_);
	}

	bool on_apply() {
		this->refresh_entity(); // duplicate work
		this->gps->refresh(this->entity);

		this->refresh_output_fields();

		return true;
	}

	bool on_reset() {
		if (this->gps != nullptr) {
			if (!this->master->up_to_date()) {
				this->entity = this->gps->clone_gpscs(this->entity);
				this->refresh_parameter_fields();
				this->refresh_output_fields();
			}
		}

		return true;
	}

public:
	IGraphlet* thumbnail() {
		return this->gps;
	}

private:
	void refresh_entity() {
		if (this->entity == nullptr) {
			this->entity = ref new GPSCS();
		}

		GPS_Refresh_Vertex(this->entity, a, this->ps, GCS::a);
		GPS_Refresh_Vertex(this->entity, f, this->ps, GCS::f);
		GPS_Refresh_Vertex(this->entity, cm, this->ps, GCS::CM);

		GPS_Refresh_Vertex(this->entity, cs_tx, this->ps, GCS::Tx);
		GPS_Refresh_Vertex(this->entity, cs_ty, this->ps, GCS::Ty);
		GPS_Refresh_Vertex(this->entity, cs_tz, this->ps, GCS::Tz);
		GPS_Refresh_Vertex(this->entity, cs_s, this->ps, GCS::S);
		GPS_Refresh_Vertex(this->entity, cs_rx, this->ps, GCS::Rx);
		GPS_Refresh_Vertex(this->entity, cs_ry, this->ps, GCS::Ry);
		GPS_Refresh_Vertex(this->entity, cs_rz, this->ps, GCS::Rz);

		GPS_Refresh_Vertex(this->entity, gk_dx, this->ps, GCS::Dx);
		GPS_Refresh_Vertex(this->entity, gk_dy, this->ps, GCS::Dy);
		GPS_Refresh_Vertex(this->entity, gk_dz, this->ps, GCS::Dz);
		GPS_Refresh_Vertex(this->entity, utm_s, this->ps, GCS::UTM_S);
	}

	void refresh_parameter_fields() {
		if (this->entity != nullptr) {
			this->master->begin_update_sequence();

			GPS_Display_Vertex(this->entity, a, this->ps, GCS::a);
			GPS_Display_Vertex(this->entity, f, this->ps, GCS::f);
			GPS_Display_Vertex(this->entity, cm, this->ps, GCS::CM);

			GPS_Display_Vertex(this->entity, cs_tx, this->ps, GCS::Tx);
			GPS_Display_Vertex(this->entity, cs_ty, this->ps, GCS::Ty);
			GPS_Display_Vertex(this->entity, cs_tz, this->ps, GCS::Tz);
			GPS_Display_Vertex(this->entity, cs_s, this->ps, GCS::S);
			GPS_Display_Vertex(this->entity, cs_rx, this->ps, GCS::Rx);
			GPS_Display_Vertex(this->entity, cs_ry, this->ps, GCS::Ry);
			GPS_Display_Vertex(this->entity, cs_rz, this->ps, GCS::Rz);

			GPS_Display_Vertex(this->entity, gk_dx, this->ps, GCS::Dx);
			GPS_Display_Vertex(this->entity, gk_dy, this->ps, GCS::Dy);
			GPS_Display_Vertex(this->entity, gk_dz, this->ps, GCS::Dz);
			GPS_Display_Vertex(this->entity, utm_s, this->ps, GCS::UTM_S);
			
			this->master->notify_updated();
			this->master->end_update_sequence();
		}
	}

	void refresh_output_fields() {
		if ((this->convertor != nullptr) && (this->entity != nullptr)) {
			double latitude = this->is[GCS::B]->get_value();
			double longitude = this->is[GCS::L]->get_value();
			double altitude = this->is[GCS::H]->get_value();
			double x = this->is[GCS::X]->get_value();
			double y = this->is[GCS::Y]->get_value();
			double z = this->is[GCS::Z]->get_value();
			double3 xyz = this->convertor->gps_to_xyz(latitude, longitude, altitude, this->entity->parameter);
			double3 blh = this->convertor->xyz_to_gps(x, y, z, this->entity->parameter);

			this->master->begin_update_sequence();

			this->os[GCS::X]->set_value(xyz.x);
			this->os[GCS::Y]->set_value(xyz.y);
			this->os[GCS::Z]->set_value(xyz.z);
			//this->os[GCS::B]->set_value(blh.x);
			//this->os[GCS::L]->set_value(blh.y);
			//this->os[GCS::H]->set_value(blh.z);

			this->master->notify_updated();
			this->master->end_update_sequence();
		}
	}

private:
	Labellet* insert_label(GCS id) {
		Labellet* label = new Labellet(_speak(id), label_font, label_color);
		float lbl_width;

		label->fill_extent(0.0F, 0.0F, &lbl_width, nullptr);
		this->label_max_width = flmax(this->label_max_width, lbl_width);

		return this->master->insert_one(label);
	}
	
	Credit<Dimensionlet, GCS>* insert_parameter_field(GCS id, int precision) {
		this->parameter_style.precision = precision;
		
		return this->master->insert_one(new Credit<Dimensionlet, GCS>(DimensionState::Input, this->parameter_style, "meter"), id);
	}

	void load_test_field(GCS id, double v, int precision) {
		this->input_style.precision = precision;		
		this->is[id] = this->master->insert_one(new Credit<Dimensionlet, GCS>(DimensionState::Input, this->input_style, "meter", id.ToString()), id);
		this->is[id]->set_value(v);

		this->output_style.precision = precision;
		this->os[id] = this->master->insert_one(new Credit<Dimensionlet, GCS>(DimensionState::Default, this->output_style, "meter", id.ToString()), id);
		this->os[id]->set_value(flnan);
	}

	void reflow_parameter_fields(IGraphlet* frame, float xoff, GCS id0, GCS idp1, float gapsize, float pheight, GCS sep) {
		float idx0 = _F(id0);
		float yoff0 = gapsize * 0.5F;
		float hgap = gapsize * 0.618F;

		for (GCS id = id0; id < idp1; id++) {
			float yrow = (pheight + gapsize) * (_F(id) - idx0);

			if (id == sep) { // add a blank before `sep`
				yoff0 = gapsize;
			}

			this->master->move_to(this->ps[id], frame, GraphletAnchor::LT, GraphletAnchor::LT, xoff + hgap, yrow + gapsize + yoff0);
			this->master->move_to(this->labels[id], this->ps[id], GraphletAnchor::LC, GraphletAnchor::RC, -hgap);
		}
	}

	void reflow_test_fields(GCS id0, GCS idn, GCS lbl0, GCS lbln, float inset, float iheight) {
		float fln = _F(idn) - _F(id0) + 1.0F;
		float idx_1 = _F(id0) - 1.0F;
		float rwidth, rheight, vgapsize;
		GCS lbl = lbl0;

		this->regions[id0]->fill_extent(0.0F, 0.0F, &rwidth, &rheight);
		vgapsize = (rheight - fln * iheight) / (fln + 1.0F);

		for (GCS id = id0; id <= idn; id++, lbl++) {
			float yrow_p1 = (iheight + vgapsize) * (_F(id) - idx_1);

			this->master->move_to(this->is[id], this->regions[id0], GraphletAnchor::CT, GraphletAnchor::RB, 0.0F, yrow_p1);
			this->master->move_to(this->os[lbl], this->is[id], GraphletAnchor::RC, GraphletAnchor::LC, inset);
		}
	}

private:
	float label_max_width;
	DimensionStyle parameter_style;
	DimensionStyle input_style;
	DimensionStyle output_style;
	GPSCS^ entity;

private: // never delete these graphlet manually
	GPSlet* gps;
	std::map<GCS, Shapelet*> regions;
	std::map<GCS, Labellet*> labels;
	std::map<GCS, Credit<Dimensionlet, GCS>*> ps;
	std::map<GCS, Credit<Dimensionlet, GCS>*> is;
	std::map<GCS, Credit<Dimensionlet, GCS>*> os;
	
private:
	GPSCSPlanet* master;
	IGPSConvertor* convertor;
};

/*************************************************************************************************/
GPSCSPlanet::GPSCSPlanet(IGPSConvertor* gc, Platform::String^ gps) : EditorPlanet(__MODULE__) {
	this->self = new GPSCSSelf(this, gps, gc);
}

GPSCSPlanet::~GPSCSPlanet() {
	delete this->self;
}

void GPSCSPlanet::load(CanvasCreateResourcesReason reason, float width, float height) {
	float bg_width, bg_height;

	EditorPlanet::load(reason, width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->load(reason, bg_width, bg_height, (width - bg_width) * 0.5F);
}

void GPSCSPlanet::reflow(float width, float height) {
	float bg_width, bg_height;
	
	EditorPlanet::reflow(width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->reflow(this->background, width, height, (width - bg_width) * 0.5F);
}

void GPSCSPlanet::on_graphlet_ready(IGraphlet* g) {
	this->self->on_graphlet_ready(g);
}

IGraphlet* GPSCSPlanet::thumbnail_graphlet() {
	return this->self->thumbnail();
}

bool GPSCSPlanet::on_apply() {
	return this->self->on_apply();
}

bool GPSCSPlanet::on_reset() {
	return this->self->on_reset();
}

bool GPSCSPlanet::on_edit(Dimensionlet* dim) {
	return this->self->on_edit(static_cast<Credit<Dimensionlet, GCS>*>(dim));
}
