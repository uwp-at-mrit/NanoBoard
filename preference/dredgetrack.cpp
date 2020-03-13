#include <map>

#include "preference/dredgetrack.hpp"

#include "graphlet/filesystem/project/dredgetracklet.hpp"
#include "graphlet/ui/colorpickerlet.hpp"
#include "graphlet/ui/togglet.hpp"

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

/*************************************************************************************************/
namespace {
	// order matters
	private enum class DT {
		Depth0, TrackInterval, TrackDistance, AfterImage,
		TrackWidth, TrackColor,

		_,

		// misc
		BeginTime, EndTime
	};
}

class WarGrey::DTPM::DredgeTrackEditor::Self {
public:
	Self(DredgeTrackEditor* master, Platform::String^ dregertrack) : master(master), label_max_width(0.0F), dregertrack(dregertrack), entity(nullptr) {
		this->input_style = make_highlight_dimension_style(label_font->FontSize, 8U, 2U);
		this->input_style.unit_color = label_color;
	}

public:
	void load(CanvasCreateResourcesReason reason, float width, float height, float inset) {
		float icon_width = width * 0.2F;
		float in_width, in_height;

		for (DT id = _E0(DT); id < DT::_; id++) {
			this->labels[id] = this->insert_label(id);

			switch (id) {
			case DT::AfterImage: this->metrics[id] = this->insert_input_field(id, 0.0, "hour"); break;
			case DT::TrackWidth: this->metrics[id] = this->insert_input_field(id, 0.0, "pixel"); break;
			case DT::TrackColor: {
				this->metrics[DT::Depth0]->fill_extent(0.0F, 0.0F, &in_width, &in_height);
				this->color_picker = this->master->insert_one(new ColorPickerlet(Palette::X11, in_width, in_height));
			}; break;
			default: this->metrics[id] = this->insert_input_field(id, 0.0, "meter");
			}
		}

		this->dates[DT::BeginTime] = this->insert_date_picker(DT::BeginTime);
		this->dates[DT::EndTime] = this->insert_date_picker(DT::EndTime);

		for (auto vid = _E0(DredgeTrackType); vid < DredgeTrackType::_; vid++) {
			this->toggles[vid] = this->insert_toggle(vid, icon_width);
		}

		this->track = new DredgeTracklet(nullptr, this->dregertrack, icon_width);
		this->master->insert(this->track);
	}

	void reflow(IGraphlet* frame, float width, float height, float inset) {
		float pwidth, pheight;
		float y0 = inset;
		float xgapsize = inset * 0.618F;
		float ygapsize = inset * 0.618F;

		this->metrics[DT::Depth0]->fill_extent(0.0F, 0.0F, &pwidth, &pheight);
		
		inset *= 2.0F;
		this->master->move_to(this->dates[DT::BeginTime], frame, GraphletAnchor::LT, GraphletAnchor::LT, this->label_max_width * 3.0F, y0);
		this->master->move_to(this->dates[DT::EndTime], this->dates[DT::BeginTime], GraphletAnchor::LB, GraphletAnchor::LT, 0.0F, ygapsize);

		this->reflow_input_fields(frame, DT::Depth0, DT::_, 0.0F, y0, xgapsize, ygapsize, pheight, DT::TrackWidth);
		this->master->move_to(this->color_picker, this->labels[DT::TrackColor], GraphletAnchor::RC, GraphletAnchor::LC, xgapsize);
		
		this->master->move_to(this->track, frame, GraphletAnchor::RT, GraphletAnchor::RT, -inset, inset);

		{ // reflow toggles
			auto last_dtt = DredgeTrackType::_;
	
			for (auto id = _E0(DredgeTrackType); id < DredgeTrackType::_; id++) {
				if (last_dtt == DredgeTrackType::_) {
					this->master->move_to(this->toggles[id], this->track, GraphletAnchor::RB, GraphletAnchor::RT, 0.0F, y0 * 2.0F);
				} else {
					this->master->move_to(this->toggles[id], this->toggles[last_dtt], GraphletAnchor::LB, GraphletAnchor::LT, 0.0F, ygapsize);
				}

				last_dtt = id;
			}
		}
	}

	void on_graphlet_ready(IGraphlet* g) {
		if (this->track == g) { // also see `this->load()`
			this->entity = this->track->clone_track(this->entity);
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

	bool on_edit(Credit<DatePickerlet, DT>* dp) {
		long long new_date = dp->get_value();
		bool modified = (this->entity == nullptr);

		if (!modified) {
			switch (dp->id) {
			case DT::BeginTime: modified = (new_date != this->entity->begin_timepoint); break;
			case DT::EndTime: modified = (new_date != this->entity->end_timepoint); break;
			}
		}

		if (modified) {
			this->refresh_entity();

			this->track->moor(GraphletAnchor::CB);
			this->track->preview(this->entity);
			this->track->clear_moor();
		}

		return modified;
	}

	bool on_apply() {
		this->refresh_entity(); // duplicate work
		this->track->refresh(this->entity);

		return true;
	}

	bool on_reset() {
		if (this->track != nullptr) {
			this->track->preview(nullptr);
			
			this->entity = this->track->clone_track(this->entity, true);
			this->refresh_input_fields();
		}

		return true;
	}

	bool on_default() {
		if (this->entity == nullptr) {
			this->entity = ref new DredgeTrack();
		}

		this->master->begin_update_sequence();

		this->metrics[DT::Depth0]->set_value(10.0);
		this->metrics[DT::TrackInterval]->set_value(1.0);
		this->metrics[DT::TrackDistance]->set_value(1000.0);
		this->metrics[DT::AfterImage]->set_value(24.0);

		this->metrics[DT::TrackWidth]->set_value(1.0);

		for (auto id = _E0(DredgeTrackType); id < DredgeTrackType::_; id++) {
			this->toggles[id]->toggle(false);

			switch (id) {
			case DredgeTrackType::PSDrag: case DredgeTrackType::SBDrag: this->toggles[id]->toggle(); break;
			}
		}

		{ // set contrastive period
			long long now = current_seconds();

			this->dates[DT::BeginTime]->set_value(seconds_add_days(now, -7));
			this->dates[DT::EndTime]->set_value(now);
		}

		this->master->end_update_sequence();

		return true;
	}

public:
	IGraphlet* thumbnail() {
		return this->track;
	}

private:
	void refresh_entity() {
		if (this->entity == nullptr) {
			this->entity = ref new DredgeTrack();
		}

		this->entity->depth0 = this->metrics[DT::Depth0]->get_value();
		this->entity->subinterval = this->metrics[DT::TrackInterval]->get_value();
		this->entity->partition_distance = this->metrics[DT::TrackDistance]->get_value();
		this->entity->after_image_period = this->metrics[DT::AfterImage]->get_value();

		this->entity->track_width = float(this->metrics[DT::TrackWidth]->get_value());

		for (auto id = _E0(DredgeTrackType); id < DredgeTrackType::_; id++) {
			this->entity->visibles[_I(id)] = this->toggles[id]->checked();
		}

		this->entity->begin_timepoint = this->dates[DT::BeginTime]->get_value();
		this->entity->end_timepoint = this->dates[DT::EndTime]->get_value();
	}

	void refresh_input_fields() {
		if (this->entity != nullptr) {
			this->master->begin_update_sequence();

			this->metrics[DT::Depth0]->set_value(this->entity->depth0);
			this->metrics[DT::TrackInterval]->set_value(this->entity->subinterval);
			this->metrics[DT::TrackDistance]->set_value(this->entity->partition_distance);
			this->metrics[DT::AfterImage]->set_value(this->entity->after_image_period);

			this->metrics[DT::TrackWidth]->set_value(this->entity->track_width);

			for (auto id = _E0(DredgeTrackType); id < DredgeTrackType::_; id++) {
				this->toggles[id]->toggle(this->entity->visibles[_I(id)]);
			}

			this->dates[DT::BeginTime]->set_value(this->entity->begin_timepoint);
			this->dates[DT::EndTime]->set_value(this->entity->end_timepoint);

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
		auto input = new Credit<DatePickerlet, DT>(DatePickerState::Input, current_seconds(), _speak(id));

		this->master->insert_one(input, id);

		return input;
	}
	
	Credit<Togglet, DredgeTrackType>* insert_toggle(DredgeTrackType id, float width) {
		auto input = new Credit<Togglet, DredgeTrackType>(true, _speak(id), width);

		this->master->insert_one(input, id);

		return input;
	}

	void reflow_input_fields(IGraphlet* frame, DT id0, DT idp1, float x0, float y0, float xgapsize, float ygapsize, float pheight, DT sep) {
		float idx0 = _F(id0);
		float xoff =  x0 + this->label_max_width + xgapsize * 1.618F;
		float yoff0 = y0;

		for (DT id = id0; id < idp1; id++) {
			float yrow = (pheight + ygapsize) * (_F(id) - idx0);
			
			if (id == sep) { // add a blank before `sep`
				yoff0 *= 2.0F;
			}

			this->master->move_to(this->labels[id], frame, GraphletAnchor::LT, GraphletAnchor::RT, xoff, yoff0 + yrow);
			this->master->move_to(this->metrics[id], this->labels[id], GraphletAnchor::RC, GraphletAnchor::LC, xgapsize);
		}
	}

private:
	float label_max_width;
	DimensionStyle input_style;
	DredgeTrack^ entity;
	Platform::String^ dregertrack;

private: // never delete these graphlet manually
	DredgeTracklet* track;
	ColorPickerlet* color_picker;
	std::map<DT, Labellet*> labels;
	std::map<DT, Credit<Dimensionlet, DT>*> metrics;
	std::map<DT, Credit<DatePickerlet, DT>*> dates;
	std::map<DredgeTrackType, Credit<Togglet, DredgeTrackType>*> toggles;
	
private:
	DredgeTrackEditor* master;
};

/*************************************************************************************************/
DredgeTrackEditor::DredgeTrackEditor(Platform::String^ dregertrack) : EditorPlanet(__MODULE__) {
	this->self = new DredgeTrackEditor::Self(this, dregertrack);
}

DredgeTrackEditor::~DredgeTrackEditor() {
	delete this->self;
}

void DredgeTrackEditor::load(CanvasCreateResourcesReason reason, float width, float height) {
	float bg_width, bg_height;

	EditorPlanet::load(reason, width, height);

	this->enable_default(true);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->load(reason, bg_width, bg_height, (width - bg_width) * 0.5F);
}

void DredgeTrackEditor::reflow(float width, float height) {
	float bg_width, bg_height;
	
	EditorPlanet::reflow(width, height);

	this->background->fill_extent(0.0F, 0.0F, &bg_width, &bg_height);
	this->self->reflow(this->background, width, height, (width - bg_width) * 0.5F);
}

void DredgeTrackEditor::on_graphlet_ready(IGraphlet* g) {
	this->self->on_graphlet_ready(g);
}

IGraphlet* DredgeTrackEditor::thumbnail_graphlet() {
	return this->self->thumbnail();
}

bool DredgeTrackEditor::on_apply() {
	return this->self->on_apply();
}

bool DredgeTrackEditor::on_reset() {
	return this->self->on_reset();
}

bool DredgeTrackEditor::on_default() {
	return this->self->on_default();
}

bool DredgeTrackEditor::on_edit(Dimensionlet* dim) {
	return this->self->on_edit(static_cast<Credit<Dimensionlet, DT>*>(dim));
}

bool DredgeTrackEditor::on_date_picked(DatePickerlet* dp) {
	return this->self->on_edit(static_cast<Credit<DatePickerlet, DT>*>(dp));
}
