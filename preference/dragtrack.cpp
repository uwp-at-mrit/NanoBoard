#include <map>

#include "preference/dragtrack.hpp"

#include "graphlet/filesystem/project/dragtracklet.hpp"
#include "graphlet/time/datepickerlet.hpp"

#include "graphlet/shapelet.hpp"
#include "graphlet/planetlet.hpp"

#include "datum/flonum.hpp"
#include "datum/time.hpp"

#include "module.hpp"

using namespace WarGrey::SCADA;
using namespace WarGrey::DTPM;

using namespace Windows::Foundation;

using namespace Microsoft::Graphics::Canvas::UI;
using namespace Microsoft::Graphics::Canvas::Text;
using namespace Microsoft::Graphics::Canvas::Brushes;

static CanvasTextFormat^ label_font = make_bold_text_format("Microsoft Yahei", 16.0F);
static CanvasTextFormat^ sketch_number_font = make_bold_text_format("Consolas", 14.0F);
static CanvasTextFormat^ sketch_font = make_bold_text_format("Consolas", 8.0F);

static CanvasSolidColorBrush^ label_color = Colours::DarkGray;
static CanvasSolidColorBrush^ original_color = Colours::Khaki;
static CanvasSolidColorBrush^ designed_color = Colours::SkyBlue;
static CanvasSolidColorBrush^ axes_color = Colours::Salmon;
static CanvasSolidColorBrush^ water_color = Colours::SeaGreen;

#define Section_Display_Vertex(v, ref, ms, id) ms[id]->set_value(v->ref)
#define Section_Refresh_Vertex(v, ref, ms, id) v->ref = ms[id]->get_value()

/*************************************************************************************************/
namespace {
	// order matters
	private enum class DT {
		BeginTime, EndTime,
		TrackWidth, TrackColor,

		PSVisible, SBVisible,

		Depth0, TrackInterval, AfterImage,

		_,

		// for sketch icon
		lb, rt, tide
	};

	private class SketchIcon : public Planet {
	public:
		SketchIcon() : Planet("dragtrack") {}

	public:
		void load(CanvasCreateResourcesReason reason, float width, float height) override {
			float stepsize = 36.0F;
			float thickness = 2.0F;
		}

		void reflow(float width, float height) override {
		}

	private: // never delete these graphlet manually
		Tracklet<DT>* designed_section;
		Tracklet<DT>* original_section;
		std::map<DT, Shapelet*> decorates;
	};
}

class WarGrey::DTPM::DragTrackEditor::Self {
public:
	Self(DragTrackEditor* master, Platform::String^ section) : master(master), label_max_width(0.0F), section(section), entity(nullptr) {
		this->input_style = make_highlight_dimension_style(label_font->FontSize, 8U, 1U);
		this->input_style.unit_color = label_color;
	}

public:
	void load(CanvasCreateResourcesReason reason, float width, float height, float inset) {
		for (DT id = _E0(DT); id < DT::_; id++) {
			this->labels[id] = this->insert_label(id);

			switch (id) {
			case DT::BeginTime: case DT::EndTime: this->dates[id] = this->insert_date_picker(id); break;
			default: this->metrics[id] = this->insert_input_field(id, 0.0, "meter");
			}
		}
		
		this->track = new DragTracklet(nullptr, this->section, width - inset * 4.0F);
		this->master->insert(this->track);

		this->sketch = this->master->insert_one(new Planetlet(new SketchIcon()));
	}

	void reflow(IGraphlet* frame, float width, float height, float inset) {
		float pwidth, pheight;
		float y0 = inset;
		float xgapsize = inset;
		float ygapsize = inset;

		this->metrics[DT::Depth0]->fill_extent(0.0F, 0.0F, &pwidth, &pheight);
		
		inset *= 2.0F;
		this->master->move_to(this->dates[DT::BeginTime], frame, GraphletAnchor::LT, GraphletAnchor::LT, this->label_max_width + xgapsize, y0);
		this->master->move_to(this->dates[DT::EndTime], this->dates[DT::BeginTime], GraphletAnchor::LB, GraphletAnchor::LT, 0.0F, ygapsize);

		this->reflow_input_fields(frame, DT::Depth0, DT::_, this->label_max_width * 3.0F, y0, xgapsize, ygapsize, pheight, DT::TrackWidth);
		
		this->master->move_to(this->sketch, frame, GraphletAnchor::RT, GraphletAnchor::RT, -inset, inset);
		this->master->move_to(this->track, frame, GraphletAnchor::CB, GraphletAnchor::CB, 0.0F, -inset);
	}

	void on_graphlet_ready(IGraphlet* g) {
		if (this->track == g) { // also see `this->load()`
			this->entity = this->track->clone_profile(this->entity);
			this->refresh_input_fields();
		}
	}

public:
	bool on_edit(Credit<Dimensionlet, DT>* dim) {
		long double new_value = dim->get_input_number();
		bool modified = (new_value != dim->get_value());

		if (modified) {
			dim->set_value(new_value);

			this->refresh_entity();

			this->track->moor(GraphletAnchor::CB);
			this->track->preview(this->entity);
			this->track->clear_moor();
		}

		return (modified && (dim->id < DT::_));
	}

	bool on_apply() {
		this->refresh_entity(); // duplicate work
		this->track->refresh(this->entity);

		return true;
	}

	bool on_reset() {
		if (this->track != nullptr) {
			this->track->preview(nullptr);
			
			this->entity = this->track->clone_profile(this->entity, true);
			this->refresh_input_fields();
		}

		return true;
	}

public:
	IGraphlet* thumbnail() {
		return this->sketch;
	}

private:
	void refresh_entity() {
		if (this->entity == nullptr) {
			this->entity = ref new DragTrack();
		}

		//Section_Refresh_Vertex(this->entity, width, this->metrics, DT::Width);
		//Section_Refresh_Vertex(this->entity, max_depth, this->metrics, DT::MaxDepth);
		//Section_Refresh_Vertex(this->entity, min_depth, this->metrics, DT::MinDepth);
		
		//Section_Refresh_Vertex(this->entity, depth_distance, this->metrics, DT::DepthDistance);
		//Section_Refresh_Vertex(this->entity, dragheads_distance, this->metrics, DT::DragHeadsDistance);
	}

	void refresh_input_fields() {
		if (this->entity != nullptr) {
			this->master->begin_update_sequence();

			//Section_Display_Vertex(this->entity, width, this->metrics, DT::Width);
			//Section_Display_Vertex(this->entity, max_depth, this->metrics, DT::MaxDepth);
			//Section_Display_Vertex(this->entity, min_depth, this->metrics, DT::MinDepth);
			
			//Section_Display_Vertex(this->entity, depth_distance, this->metrics, DT::DepthDistance);
			//Section_Display_Vertex(this->entity, dragheads_distance, this->metrics, DT::DragHeadsDistance);
			
			this->master->end_update_sequence();
		}
	}

private:
	Labellet* insert_label(DT id) {
		Labellet* label = new Labellet(_speak(id), label_font, label_color);
		float lbl_width;

		label->fill_extent(0.0F, 0.0F, &lbl_width, nullptr);
		this->label_max_width = flmax(this->label_max_width, lbl_width);

		return this->master->insert_one(label);
	}
	
	Credit<Dimensionlet, DT>* insert_input_field(DT id, double v, Platform::String^ unit) {
		auto input = new Credit<Dimensionlet, DT>(DimensionState::Input, this->input_style, unit);

		this->master->insert_one(input, id);
		input->set_value(v);

		return input;
	}

	Credit<DatePickerlet, DT>* insert_date_picker(DT id) {
		auto input = new Credit<DatePickerlet, DT>(current_seconds(), _speak(id));

		this->master->insert_one(input, id);

		return input;
	}

	void reflow_input_fields(IGraphlet* frame, DT id0, DT idp1, float x0, float y0, float xgapsize, float ygapsize, float pheight, DT sep) {
		float idx0 = _F(id0);
		float xoff =  x0 + this->label_max_width + xgapsize;
		float yoff0 = y0;

		for (DT id = id0; id < idp1; id++) {
			float yrow = (pheight + ygapsize * 0.5F) * (_F(id) - idx0);
			
			if (id == sep) { // add a blank before `sep`
				yoff0 *= 2.0F;
			}

			this->master->move_to(this->labels[id], frame, GraphletAnchor::LT, GraphletAnchor::RT, xoff, yoff0 + yrow);
			this->master->move_to(this->metrics[id], this->labels[id], GraphletAnchor::RC, GraphletAnchor::LC, xgapsize * 0.618F);
		}
	}

private:
	float label_max_width;
	DimensionStyle input_style;
	DragTrack^ entity;
	Platform::String^ section;

private: // never delete these graphlet manually
	DragTracklet* track;
	std::map<DT, Labellet*> labels;
	std::map<DT, Credit<Dimensionlet, DT>*> metrics;
	std::map<DT, Credit<DatePickerlet, DT>*> dates;
	Planetlet* sketch;
	
private:
	DragTrackEditor* master;
};

/*************************************************************************************************/
DragTrackEditor::DragTrackEditor(Platform::String^ section) : EditorPlanet(__MODULE__) {
	this->self = new DragTrackEditor::Self(this, section);
}

DragTrackEditor::~DragTrackEditor() {
	delete this->self;
}

void DragTrackEditor::load(CanvasCreateResourcesReason reason, float width, float height) {
	float bg_width, bg_height;

	EditorPlanet::load(reason, width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->load(reason, bg_width, bg_height, (width - bg_width) * 0.5F);
}

void DragTrackEditor::reflow(float width, float height) {
	float bg_width, bg_height;
	
	EditorPlanet::reflow(width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->reflow(this->background, width, height, (width - bg_width) * 0.5F);
}

void DragTrackEditor::on_graphlet_ready(IGraphlet* g) {
	this->self->on_graphlet_ready(g);
}

IGraphlet* DragTrackEditor::thumbnail_graphlet() {
	return this->self->thumbnail();
}

bool DragTrackEditor::on_apply() {
	return this->self->on_apply();
}

bool DragTrackEditor::on_reset() {
	return this->self->on_reset();
}

bool DragTrackEditor::on_edit(Dimensionlet* dim) {
	return this->self->on_edit(static_cast<Credit<Dimensionlet, DT>*>(dim));
}
