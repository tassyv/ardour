/*
    Copyright (C) 2000-2007 Paul Davis 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <ardour/ardour.h>

#include "utils.h"
#include "editor.h"
#include "editing.h"
#include "actions.h"
#include "ardour_ui.h"
#include "gui_thread.h"
#include "i18n.h"

using namespace Gtk;
using namespace Glib;
using namespace std;
using namespace sigc;
using namespace ARDOUR;
using namespace PBD;
using namespace Editing;

void
Editor::register_actions ()
{
	RefPtr<Action> act;

	editor_actions = ActionGroup::create (X_("Editor"));
	
	/* non-operative menu items for menu bar */

	ActionManager::register_action (editor_actions, X_("Edit"), _("Edit"));
	ActionManager::register_action (editor_actions, X_("Select"), _("Select"));
	ActionManager::register_action (editor_actions, X_("EditSelectRegionOptions"), _("Select Regions"));
	ActionManager::register_action (editor_actions, X_("EditSelectRangeOptions"), _("Select Range Operations"));
	ActionManager::register_action (editor_actions, X_("EditCursorMovementOptions"), _("Move Selected Marker"));
	ActionManager::register_action (editor_actions, X_("RegionEditOps"), _("Region operations"));
	ActionManager::register_action (editor_actions, X_("Tools"), _("Tools"));
	ActionManager::register_action (editor_actions, X_("View"), _("View"));
	ActionManager::register_action (editor_actions, X_("ZoomFocus"), _("ZoomFocus"));
	ActionManager::register_action (editor_actions, X_("MeterHold"), _("Meter hold"));
	ActionManager::register_action (editor_actions, X_("MeterFalloff"), _("Meter falloff"));
	ActionManager::register_action (editor_actions, X_("Solo"), _("Solo"));
	ActionManager::register_action (editor_actions, X_("Crossfades"), _("Crossfades"));
	ActionManager::register_action (editor_actions, X_("Monitoring"), _("Monitoring"));
	ActionManager::register_action (editor_actions, X_("Autoconnect"), _("Autoconnect"));
	ActionManager::register_action (editor_actions, X_("Layering"), _("Layering"));
	ActionManager::register_action (editor_actions, X_("Timecode"), _("Timecode fps"));
	ActionManager::register_action (editor_actions, X_("Pullup"), _("Pullup / Pulldown"));
	ActionManager::register_action (editor_actions, X_("Subframes"), _("Subframes"));

	/* add named actions for the editor */

	ActionManager::register_toggle_action (editor_actions, "link-region-and-track-selection", _("Link Region/Track Selection"), mem_fun (*this, &Editor::toggle_link_region_and_track_selection));

	act = ActionManager::register_toggle_action (editor_actions, "show-editor-mixer", _("Show Editor Mixer"), mem_fun (*this, &Editor::editor_mixer_button_toggled));
	ActionManager::session_sensitive_actions.push_back (act);

	RadioAction::Group crossfade_model_group;

	act = ActionManager::register_radio_action (editor_actions, crossfade_model_group, "CrossfadesFull", _("Span Entire Overlap"), bind (mem_fun(*this, &Editor::set_crossfade_model), FullCrossfade));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_radio_action (editor_actions, crossfade_model_group, "CrossfadesShort", _("Short"), bind (mem_fun(*this, &Editor::set_crossfade_model), ShortCrossfade));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (editor_actions, "toggle-xfades-active", _("Active"), mem_fun(*this, &Editor::toggle_xfades_active));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (editor_actions, "toggle-xfades-visible", _("Show"), mem_fun(*this, &Editor::toggle_xfade_visibility));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_toggle_action (editor_actions, "toggle-auto-xfades", _("Created Automatically"), mem_fun(*this, &Editor::toggle_auto_xfade));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "playhead-to-next-region-boundary", _("Playhead to Next Region Boundary"), bind (mem_fun(*this, &Editor::cursor_to_next_region_boundary), playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-previous-region-boundary", _("Playhead to Previous Region Boundary"), bind (mem_fun(*this, &Editor::cursor_to_previous_region_boundary), playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "playhead-to-next-region-start", _("Playhead to Next Region Start"), bind (mem_fun(*this, &Editor::cursor_to_next_region_point), playhead_cursor, RegionPoint (Start)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-next-region-end", _("Playhead to Next Region End"), bind (mem_fun(*this, &Editor::cursor_to_next_region_point), playhead_cursor, RegionPoint (End)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-next-region-sync", _("Playhead to Next Region Sync"), bind (mem_fun(*this, &Editor::cursor_to_next_region_point), playhead_cursor, RegionPoint (SyncPoint)));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "playhead-to-previous-region-start", _("Playhead to Previous Region Start"), bind (mem_fun(*this, &Editor::cursor_to_previous_region_point), playhead_cursor, RegionPoint (Start)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-previous-region-end", _("Playhead to Previous Region End"), bind (mem_fun(*this, &Editor::cursor_to_previous_region_point), playhead_cursor, RegionPoint (End)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-previous-region-sync", _("Playhead to Previous Region Sync"), bind (mem_fun(*this, &Editor::cursor_to_previous_region_point), playhead_cursor, RegionPoint (SyncPoint)));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "selected-marker-to-next-region-boundary", _("to Next Region Boundary"), mem_fun(*this, &Editor::selected_marker_to_next_region_boundary));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "selected-marker-to-previous-region-boundary", _("to Previous Region Boundary"), mem_fun(*this, &Editor::selected_marker_to_previous_region_boundary));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "edit-cursor-to-next-region-start", _("to Next Region Start"), bind (mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (Start)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-cursor-to-next-region-end", _("to Next Region End"), bind (mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (End)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-cursor-to-next-region-sync", _("to Next Region Sync"), bind (mem_fun(*this, &Editor::selected_marker_to_next_region_point), RegionPoint (SyncPoint)));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "edit-cursor-to-previous-region-start", _("to Previous Region Start"), bind (mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (Start)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-cursor-to-previous-region-end", _("to Previous Region End"), bind (mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (End)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-cursor-to-previous-region-sync", _("to Previous Region Sync"), bind (mem_fun(*this, &Editor::selected_marker_to_previous_region_point), RegionPoint (SyncPoint)));
	ActionManager::session_sensitive_actions.push_back (act);

        act = ActionManager::register_action (editor_actions, "edit-cursor-to-range-start", _("to Range Start"), mem_fun(*this, &Editor::selected_marker_to_selection_start));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-cursor-to-range-end", _("to Range End"), mem_fun(*this, &Editor::selected_marker_to_selection_end));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "playhead-to-range-start", _("Playhead to Range Start"), bind (mem_fun(*this, &Editor::cursor_to_selection_start), playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "playhead-to-range-end", _("Playhead to Range End"), bind (mem_fun(*this, &Editor::cursor_to_selection_end), playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "select-all", _("Select All"), bind (mem_fun(*this, &Editor::select_all), Selection::Set));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "deselect-all", _("Deselect All"), mem_fun(*this, &Editor::deselect_all));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "invert-selection", _("Invert Selection"), mem_fun(*this, &Editor::invert_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "select-all-after-edit-cursor", _("Select All After Edit Point"), bind (mem_fun(*this, &Editor::select_all_selectables_using_edit), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "select-all-before-edit-cursor", _("Select All Before Edit Point"), bind (mem_fun(*this, &Editor::select_all_selectables_using_edit), false));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "select-all-after-playhead", _("Select All After Playhead"), bind (mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "select-all-before-playhead", _("Select All Before Playhead"), bind (mem_fun(*this, &Editor::select_all_selectables_using_cursor), playhead_cursor, false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "select-all-between-cursors", _("Select All Between Playhead & Edit Point"), bind (mem_fun(*this, &Editor::select_all_selectables_between), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "select-all-within-cursors", _("Select All Within Playhead & Edit Point"), bind (mem_fun(*this, &Editor::select_all_selectables_between), true));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "select-range-between-cursors", _("Select Range Between Playhead & Edit Point"), mem_fun(*this, &Editor::select_range_between));
	ActionManager::session_sensitive_actions.push_back (act);

       	act = ActionManager::register_action (editor_actions, "select-all-in-punch-range", _("Select All in Punch Range"), mem_fun(*this, &Editor::select_all_selectables_using_punch));
	ActionManager::session_sensitive_actions.push_back (act);
       	act = ActionManager::register_action (editor_actions, "select-all-in-loop-range", _("Select All in Loop Range"), mem_fun(*this, &Editor::select_all_selectables_using_loop));
	ActionManager::session_sensitive_actions.push_back (act);
	
       	act = ActionManager::register_action (editor_actions, "select-next-route", _("Select Next Track/Bus"), mem_fun(*this, &Editor::select_next_route));
	ActionManager::session_sensitive_actions.push_back (act);
       	act = ActionManager::register_action (editor_actions, "select-prev-route", _("Select Previous Track/Bus"), mem_fun(*this, &Editor::select_prev_route));
	ActionManager::session_sensitive_actions.push_back (act);
	

	act = ActionManager::register_action (editor_actions, "goto-mark-1", _("Locate to Mark 1"), bind (mem_fun (*this, &Editor::goto_nth_marker), 0));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-2", _("Locate to Mark 2"), bind (mem_fun (*this, &Editor::goto_nth_marker), 1));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-3", _("Locate to Mark 3"), bind (mem_fun (*this, &Editor::goto_nth_marker), 2));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-4", _("Locate to Mark 4"), bind (mem_fun (*this, &Editor::goto_nth_marker), 3));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-5", _("Locate to Mark 5"), bind (mem_fun (*this, &Editor::goto_nth_marker), 4));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-6", _("Locate to Mark 6"), bind (mem_fun (*this, &Editor::goto_nth_marker), 5));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-7", _("Locate to Mark 7"), bind (mem_fun (*this, &Editor::goto_nth_marker), 6));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-8", _("Locate to Mark 8"), bind (mem_fun (*this, &Editor::goto_nth_marker), 7));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto-mark-9", _("Locate to Mark 9"), bind (mem_fun (*this, &Editor::goto_nth_marker), 8));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "jump-forward-to-mark", _("Jump Forward to Mark"), mem_fun(*this, &Editor::jump_forward_to_mark));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "jump-backward-to-mark", _("Jump Backward to Mark"), mem_fun(*this, &Editor::jump_backward_to_mark));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "add-location-from-playhead", _("Add Mark from Playhead"), mem_fun(*this, &Editor::add_location_from_playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "nudge-forward", _("Nudge Forward"), bind (mem_fun(*this, &Editor::nudge_forward), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "nudge-next-forward", _("Nudge Next Forward"), bind (mem_fun(*this, &Editor::nudge_forward), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "nudge-backward", _("Nudge Backward"), bind (mem_fun(*this, &Editor::nudge_backward), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "nudge-next-backward", _("Nudge Next Backward"), bind (mem_fun(*this, &Editor::nudge_backward), true));
	ActionManager::session_sensitive_actions.push_back (act);


	act = ActionManager::register_action (editor_actions, "temporal-zoom-out", _("Zoom Out"), bind (mem_fun(*this, &Editor::temporal_zoom_step), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "temporal-zoom-in", _("Zoom In"), bind (mem_fun(*this, &Editor::temporal_zoom_step), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "zoom-to-session", _("Zoom to Session"), mem_fun(*this, &Editor::temporal_zoom_session));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "zoom-to-region", _("Zoom to Region"), mem_fun(*this, &Editor::toggle_zoom_region));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "toggle-zoom", _("Toggle Zoom State"), mem_fun(*this, &Editor::swap_visual_state));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "scroll-tracks-up", _("Scroll Tracks Up"), mem_fun(*this, &Editor::scroll_tracks_up));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "scroll-tracks-down", _("Scroll Tracks Down"), mem_fun(*this, &Editor::scroll_tracks_down));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "step-tracks-up", _("Step Tracks Up"), mem_fun(*this, &Editor::scroll_tracks_up_line));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "step-tracks-down", _("Step Tracks Down"), mem_fun(*this, &Editor::scroll_tracks_down_line));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "scroll-backward", _("Scroll Backward"), bind (mem_fun(*this, &Editor::scroll_backward), 0.8f));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "scroll-forward", _("Scroll Forward"), bind (mem_fun(*this, &Editor::scroll_forward), 0.8f));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "goto", _("goto"), mem_fun(*this, &Editor::goto_frame));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "center-playhead", _("to Center"), mem_fun(*this, &Editor::center_playhead));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "center-edit-cursor", _("to Center"), mem_fun(*this, &Editor::center_edit_point));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "scroll-playhead-forward", _("Playhead forward"), bind (mem_fun(*this, &Editor::scroll_playhead), true));;
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "scroll-playhead-backward", _("Playhead Backward"), bind (mem_fun(*this, &Editor::scroll_playhead), false));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "playhead-to-edit", _("to Edit"), bind (mem_fun(*this, &Editor::cursor_align), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "edit-to-playhead", _("to Playhead"), bind (mem_fun(*this, &Editor::cursor_align), false));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "trim-front", _("Trim start at edit point"), mem_fun(*this, &Editor::trim_region_front));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "trim-back", _("Trim end at edit point"), mem_fun(*this, &Editor::trim_region_back));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "trim-from-start", _("Start to edit point"), mem_fun(*this, &Editor::trim_region_from_edit_point));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "trim-to-end", _("Edit point to end"), mem_fun(*this, &Editor::trim_region_to_edit_point));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "trim-region-to-loop", _("Trim To Loop"), mem_fun(*this, &Editor::trim_region_to_loop));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "trim-region-to-punch", _("Trim To Punch"), mem_fun(*this, &Editor::trim_region_to_punch));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "set-loop-from-edit-range", _("Set Loop From Edit Range"), bind (mem_fun(*this, &Editor::set_loop_from_edit_range), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "set-loop-from-region", _("Set Loop From Region"), bind (mem_fun(*this, &Editor::set_loop_from_region), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "loop-region", _("Loop Region"), bind (mem_fun(*this, &Editor::set_loop_from_region), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "set-punch-from-edit-range", _("Set Punch From Edit Range"), mem_fun(*this, &Editor::set_punch_from_edit_range));
	ActionManager::session_sensitive_actions.push_back (act);
	
	
	act = ActionManager::register_action (editor_actions, "pitch-shift-region", _("Transpose"), mem_fun(*this, &Editor::pitch_shift_regions));
	ActionManager::session_sensitive_actions.push_back (act);
	
	act = ActionManager::register_action (editor_actions, "set-fade-in-length", _("Set Fade In Length"), bind (mem_fun(*this, &Editor::set_fade_length), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "toggle-fade-in-active", _("Toggle Fade In Active"), bind (mem_fun(*this, &Editor::toggle_fade_active), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "set-fade-out-length", _("Set Fade Out Length"), bind (mem_fun(*this, &Editor::set_fade_length), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "toggle-fade-out-active", _("Toggle Fade Out Active"), bind (mem_fun(*this, &Editor::toggle_fade_active), false));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "align-regions-start", _("Align Regions Start"), bind (mem_fun(*this, &Editor::align), ARDOUR::Start));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "align-regions-start-relative", _("Align Regions Start Relative"), bind (mem_fun(*this, &Editor::align_relative), ARDOUR::Start));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "align-regions-end", _("Align Regions End"), bind (mem_fun(*this, &Editor::align), ARDOUR::End));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "align-regions-end-relative", _("Align Regions End Relative"), bind (mem_fun(*this, &Editor::align_relative), ARDOUR::End));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "align-regions-sync", _("Align Regions Sync"), bind (mem_fun(*this, &Editor::align), ARDOUR::SyncPoint));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "align-regions-sync-relative", _("Align Regions Sync Relative"), bind (mem_fun(*this, &Editor::align_relative), ARDOUR::SyncPoint));
	ActionManager::session_sensitive_actions.push_back (act);
	
        act = ActionManager::register_action (editor_actions, "play-selected-regions", _("Play Selected Region(s)"), mem_fun(*this, &Editor::play_selected_region));
        ActionManager::session_sensitive_actions.push_back (act);
        act = ActionManager::register_action (editor_actions, "brush-at-mouse", _("Brush at Mouse"), mem_fun(*this, &Editor::kbd_brush));
        ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "mute-unmute-region", _("Mute/Unmute Region"), mem_fun(*this, &Editor::kbd_mute_unmute_region));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "set-playhead", _("Set Playhead"), mem_fun(*this, &Editor::set_playhead_cursor));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "set-edit-point", _("Set Edit Point"), mem_fun(*this, &Editor::set_edit_point));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "split-region", _("Split Region"), mem_fun(*this, &Editor::split));
        ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "set-region-sync-position", _("Set Region Sync Position"), mem_fun(*this, &Editor::set_region_sync_from_edit_point));
        ActionManager::session_sensitive_actions.push_back (act);

	undo_action = act = ActionManager::register_action (editor_actions, "undo", _("Undo"), bind (mem_fun(*this, &Editor::undo), 1U));
	ActionManager::session_sensitive_actions.push_back (act);
	redo_action = act = ActionManager::register_action (editor_actions, "redo", _("Redo"), bind (mem_fun(*this, &Editor::redo), 1U));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "export-session", _("Export Session"), mem_fun(*this, &Editor::export_session));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "export-range", _("Export Range"), mem_fun(*this, &Editor::export_selection));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "editor-separate", _("Separate"), mem_fun(*this, &Editor::separate_region_from_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "editor-crop", _("Crop"), mem_fun(*this, &Editor::crop_region_to_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "editor-cut", _("Cut"), mem_fun(*this, &Editor::cut));
	ActionManager::session_sensitive_actions.push_back (act);
	/* Note: for now, editor-delete does the exact same thing as editor-cut */
	act = ActionManager::register_action (editor_actions, "editor-delete", _("Delete"), mem_fun(*this, &Editor::cut));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "editor-copy", _("Copy"), mem_fun(*this, &Editor::copy));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "editor-paste", _("Paste"), mem_fun(*this, &Editor::keyboard_paste));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "duplicate-region", _("Duplicate Region"), bind (mem_fun(*this, &Editor::duplicate_dialog), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "multi-duplicate-region", _("Multi-Duplicate Region"), bind (mem_fun(*this, &Editor::duplicate_dialog), true));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "duplicate-range", _("Duplicate Range"), bind (mem_fun(*this, &Editor::duplicate_dialog), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "insert-region", _("Insert Region"), mem_fun(*this, &Editor::keyboard_insert_region_list_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "reverse-region", _("Reverse Region"), mem_fun(*this, &Editor::reverse_region));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "normalize-region", _("Normalize Region"), mem_fun(*this, &Editor::normalize_region));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "crop", _("Crop"), mem_fun(*this, &Editor::crop_region_to_selection));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "insert-chunk", _("Insert Chunk"), bind (mem_fun(*this, &Editor::paste_named_selection), 1.0f));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "split-at-edit-cursor", _("Split At Edit Point"), mem_fun(*this, &Editor::split_region));
	ActionManager::edit_point_in_region_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "start-range", _("Start Range"), mem_fun(*this, &Editor::keyboard_selection_begin));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "finish-range", _("Finish Range"), bind (mem_fun(*this, &Editor::keyboard_selection_finish), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "finish-add-range", _("Finish add Range"), bind (mem_fun(*this, &Editor::keyboard_selection_finish), true));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_action (editor_actions, "extend-range-to-end-of-region", _("Extend Range to End of Region"), bind (mem_fun(*this, &Editor::extend_selection_to_end_of_region), false));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "extend-range-to-start-of-region", _("Extend Range to Start of Region"), bind (mem_fun(*this, &Editor::extend_selection_to_start_of_region), false));
	ActionManager::session_sensitive_actions.push_back (act);

	act = ActionManager::register_toggle_action (editor_actions, "toggle-follow-playhead", _("Follow Playhead"), (mem_fun(*this, &Editor::toggle_follow_playhead)));
	ActionManager::session_sensitive_actions.push_back (act);
	act = ActionManager::register_action (editor_actions, "remove-last-capture", _("Remove Last Capture"), (mem_fun(*this, &Editor::remove_last_capture)));
	ActionManager::session_sensitive_actions.push_back (act);

	Glib::RefPtr<ActionGroup> zoom_actions = ActionGroup::create (X_("Zoom"));
	RadioAction::Group zoom_group;

	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-left", _("Zoom Focus Left"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusLeft));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-right", _("Zoom Focus Right"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusRight));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-center", _("Zoom Focus Center"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusCenter));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-playhead", _("Zoom Focus Playhead"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusPlayhead));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-mouse", _("Zoom Focus Mouse"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusMouse));
	ActionManager::session_sensitive_actions.push_back (act);
	ActionManager::register_radio_action (zoom_actions, zoom_group, "zoom-focus-edit", _("Zoom Focus Edit"), bind (mem_fun(*this, &Editor::zoom_focus_chosen), Editing::ZoomFocusEdit));
	ActionManager::session_sensitive_actions.push_back (act);

	Glib::RefPtr<ActionGroup> mouse_mode_actions = ActionGroup::create (X_("MouseMode"));
	RadioAction::Group mouse_mode_group;

	ActionManager::register_radio_action (mouse_mode_actions, mouse_mode_group, "set-mouse-mode-object", _("Object Tool"), bind (mem_fun(*this, &Editor::set_mouse_mode), Editing::MouseObject, false));
	ActionManager::register_radio_action (mouse_mode_actions, mouse_mode_group, "set-mouse-mode-range", _("Range Tool"), bind (mem_fun(*this, &Editor::set_mouse_mode), Editing::MouseRange, false));
	ActionManager::register_radio_action (mouse_mode_actions, mouse_mode_group, "set-mouse-mode-gain", _("Gain Tool"), bind (mem_fun(*this, &Editor::set_mouse_mode), Editing::MouseGain, false));
	ActionManager::register_radio_action (mouse_mode_actions, mouse_mode_group, "set-mouse-mode-zoom", _("Zoom Tool"), bind (mem_fun(*this, &Editor::set_mouse_mode), Editing::MouseZoom, false));
	ActionManager::register_radio_action (mouse_mode_actions, mouse_mode_group, "set-mouse-mode-timefx", _("Timefx Tool"), bind (mem_fun(*this, &Editor::set_mouse_mode), Editing::MouseTimeFX, false));

	RadioAction::Group edit_point_group;
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-playhead"), _("Playhead"), (bind (mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtPlayhead)));
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-mouse"), _("Mouse"), (bind (mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtPlayhead)));
	ActionManager::register_radio_action (editor_actions, edit_point_group, X_("edit-at-selected-marker"), _("Marker"), (bind (mem_fun(*this, &Editor::edit_point_chosen), Editing::EditAtPlayhead)));

	ActionManager::register_action (editor_actions, "cycle-edit-point", _("Change edit point"), bind (mem_fun (*this, &Editor::cycle_edit_point), false));
	ActionManager::register_action (editor_actions, "cycle-edit-point-with-marker", _("Change edit point (w/Marker)"), bind (mem_fun (*this, &Editor::cycle_edit_point), true));

	ActionManager::register_action (editor_actions, "set-edit-splice", _("Splice"), bind (mem_fun (*this, &Editor::set_edit_mode), Splice));
	ActionManager::register_action (editor_actions, "set-edit-slide", _("Slide"), bind (mem_fun (*this, &Editor::set_edit_mode), Slide));
	ActionManager::register_action (editor_actions, "toggle-edit-mode", _("Toggle Edit Mode"), mem_fun (*this, &Editor::cycle_edit_mode));

	ActionManager::register_action (editor_actions, X_("SnapTo"), _("Snap To"));
	ActionManager::register_action (editor_actions, X_("SnapMode"), _("Snap Mode"));

	RadioAction::Group snap_mode_group;
	ActionManager::register_radio_action (editor_actions, snap_mode_group, X_("snap-off"), _("No Grid"), (bind (mem_fun(*this, &Editor::snap_mode_chosen), Editing::SnapOff)));
	ActionManager::register_radio_action (editor_actions, snap_mode_group, X_("snap-normal"), _("Grid"), (bind (mem_fun(*this, &Editor::snap_mode_chosen), Editing::SnapNormal)));
	ActionManager::register_radio_action (editor_actions, snap_mode_group, X_("snap-magnetic"), _("Magnetic"), (bind (mem_fun(*this, &Editor::snap_mode_chosen), Editing::SnapMagnetic)));

	ActionManager::register_action (editor_actions, X_("cycle-snap-mode"), _("Next Snap Mode"), mem_fun (*this, &Editor::cycle_snap_mode));
	ActionManager::register_action (editor_actions, X_("cycle-snap-choice"), _("Next Snap Choice"), mem_fun (*this, &Editor::cycle_snap_choice));

	Glib::RefPtr<ActionGroup> snap_actions = ActionGroup::create (X_("Snap"));
	RadioAction::Group snap_choice_group;

	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-cd-frame"), _("Snap to cd frame"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToCDFrame)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-smpte-frame"), _("Snap to SMPTE frame"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToSMPTEFrame)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-smpte-seconds"), _("Snap to SMPTE seconds"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToSMPTESeconds)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-smpte-minutes"), _("Snap to SMPTE minutes"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToSMPTEMinutes)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-seconds"), _("Snap to seconds"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToSeconds)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-minutes"), _("Snap to minutes"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToMinutes)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-thirtyseconds"), _("Snap to thirtyseconds"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToAThirtysecondBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-asixteenthbeat"), _("Snap to asixteenthbeat"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToASixteenthBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-eighths"), _("Snap to eighths"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToAEighthBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-quarters"), _("Snap to quarters"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToAQuarterBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-thirds"), _("Snap to thirds"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToAThirdBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-beat"), _("Snap to beat"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToBeat)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-bar"), _("Snap to bar"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToBar)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-mark"), _("Snap to mark"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToMark)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-region-start"), _("Snap to region start"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToRegionStart)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-region-end"), _("Snap to region end"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToRegionEnd)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-region-sync"), _("Snap to region sync"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToRegionSync)));
	ActionManager::register_radio_action (snap_actions, snap_choice_group, X_("snap-to-region-boundary"), _("Snap to region boundary"), (bind (mem_fun(*this, &Editor::snap_type_chosen), Editing::SnapToRegionBoundary)));

	/* REGION LIST */

	Glib::RefPtr<ActionGroup> rl_actions = ActionGroup::create (X_("RegionList"));
	RadioAction::Group sort_type_group;
	RadioAction::Group sort_order_group;

	/* the region list popup menu */
	ActionManager::register_action (rl_actions, X_("RegionListSort"), _("Sort"));

	act = ActionManager::register_action (rl_actions, X_("rlAudition"), _("Audition"), mem_fun(*this, &Editor::audition_region_from_region_list));
	ActionManager::region_list_selection_sensitive_actions.push_back (act);
	act = ActionManager::register_action (rl_actions, X_("rlHide"), _("Hide"), mem_fun(*this, &Editor::hide_region_from_region_list));
	ActionManager::region_list_selection_sensitive_actions.push_back (act);
	act = ActionManager::register_action (rl_actions, X_("rlRemove"), _("Remove"), mem_fun (*this, &Editor::remove_region_from_region_list));
	ActionManager::region_list_selection_sensitive_actions.push_back (act);
	ActionManager::register_toggle_action (rl_actions, X_("rlShowAll"), _("Show all"), mem_fun(*this, &Editor::toggle_full_region_list));
	ActionManager::register_toggle_action (rl_actions, X_("rlShowAuto"), _("Show automatic regions"), mem_fun(*this, &Editor::toggle_show_auto_regions));

	ActionManager::register_radio_action (rl_actions, sort_order_group, X_("SortAscending"),  _("Ascending"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_direction), true));
	ActionManager::register_radio_action (rl_actions, sort_order_group, X_("SortDescending"),   _("Descending"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_direction), false));
	
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionName"),  _("By Region Name"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByName));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionLength"),  _("By Region Length"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByLength));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionPosition"),  _("By Region Position"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByPosition));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionTimestamp"),  _("By Region Timestamp"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByTimestamp));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionStartinFile"),  _("By Region Start in File"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByStartInFile));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortByRegionEndinFile"),  _("By Region End in File"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), ByEndInFile));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortBySourceFileName"),  _("By Source File Name"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), BySourceFileName));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortBySourceFileLength"),  _("By Source File Length"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), BySourceFileLength));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortBySourceFileCreationDate"),  _("By Source File Creation Date"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), BySourceFileCreationDate));
	ActionManager::register_radio_action (rl_actions, sort_type_group, X_("SortBySourceFilesystem"),  _("By Source Filesystem"),
			       bind (mem_fun(*this, &Editor::reset_region_list_sort_type), BySourceFileFS));


	/* the next two are duplicate items with different names for use in two different contexts */

	ActionManager::register_action (editor_actions, X_("addExistingAudioFiles"), _("Add Existing Audio"), mem_fun (*this, &Editor::external_audio_dialog));

	act = ActionManager::register_action (editor_actions, X_("addExternalAudioToRegionList"), _("Add External Audio"), bind (mem_fun(*this, &Editor::add_external_audio_action), ImportAsRegion));
	ActionManager::session_sensitive_actions.push_back (act);

	ActionManager::register_toggle_action (editor_actions, X_("ToggleWaveformVisibility"), _("Show Waveforms"), mem_fun (*this, &Editor::toggle_waveform_visibility));
	ActionManager::register_toggle_action (editor_actions, X_("ToggleWaveformsWhileRecording"), _("Show Waveforms While Recording"), mem_fun (*this, &Editor::toggle_waveforms_while_recording));
	ActionManager::register_toggle_action (editor_actions, X_("ToggleMeasureVisibility"), _("Show Measures"), mem_fun (*this, &Editor::toggle_measure_visibility));

	/* if there is a logo in the editor canvas, its always visible at startup */

	act = ActionManager::register_toggle_action (editor_actions, X_("ToggleLogoVisibility"), _("Show Logo"), mem_fun (*this, &Editor::toggle_logo_visibility));
	Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
	tact->set_active (true);
	
	RadioAction::Group layer_model_group;

	ActionManager::register_radio_action (editor_actions, layer_model_group,  X_("LayerLaterHigher"), _("Later is Higher"), bind (mem_fun (*this, &Editor::set_layer_model), LaterHigher));
	ActionManager::register_radio_action (editor_actions, layer_model_group,  X_("LayerMoveAddHigher"), _("Most Recently Moved/Added is Higher"), bind (mem_fun (*this, &Editor::set_layer_model), MoveAddHigher));
	ActionManager::register_radio_action (editor_actions, layer_model_group,  X_("LayerAddHigher"), _("Most Recently Added is Higher"), bind (mem_fun (*this, &Editor::set_layer_model), AddHigher));

	RadioAction::Group smpte_group;

	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte23976"), _("23.976"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_23976));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte24"), _("24"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_24));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte24976"), _("24.976"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_24976));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte25"), _("25"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_25));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte2997"), _("29.97"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_2997));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte2997drop"), _("29.97 drop"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_2997drop));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte30"), _("30"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_30));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte30drop"), _("30 drop"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_30drop));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte5994"), _("59.94"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_5994));
	ActionManager::register_radio_action (editor_actions, smpte_group,  X_("Smpte60"), _("60"), bind (mem_fun (*this, &Editor::smpte_fps_chosen), smpte_60));

	RadioAction::Group pullup_group;

	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupPlus4Plus1"), _("+4.1667% + 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Plus4Plus1));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupPlus4"), _("+4.1667%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Plus4));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupPlus4Minus1"), _("+4.1667% - 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Plus4Minus1));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupPlus1"), _("+ 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Plus1));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupNone"), _("None"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_None));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupMinus1"), _("- 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Minus1));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupMinus4Plus1"), _("-4.1667% + 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Minus4Plus1));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupMinus4"), _("-4.1667%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Minus4));
	ActionManager::register_radio_action (editor_actions, pullup_group,  X_("PullupMinus4Minus1"), _("-4.1667% - 0.1%"), bind (mem_fun (*this, &Editor::video_pullup_chosen), Session::pullup_Minus4Minus1));

	RadioAction::Group subframe_group;

	ActionManager::register_radio_action (editor_actions, subframe_group,  X_("Subframes80"), _("80 per frame"), bind (mem_fun (*this, 
&Editor::subframes_per_frame_chosen), 80));
	ActionManager::register_radio_action (editor_actions, subframe_group,  X_("Subframes100"), _("100 per frame"), bind (mem_fun (*this, 
&Editor::subframes_per_frame_chosen), 100));

	ActionManager::add_action_group (rl_actions);
	ActionManager::add_action_group (zoom_actions);
	ActionManager::add_action_group (mouse_mode_actions);
	ActionManager::add_action_group (snap_actions);
	ActionManager::add_action_group (editor_actions);
}

void
Editor::toggle_waveform_visibility ()
{
	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("ToggleWaveformVisibility"));
	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		set_show_waveforms (tact->get_active());
	}
}

void
Editor::toggle_waveforms_while_recording ()
{
	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("ToggleWaveformsWhileRecording"));
	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		set_show_waveforms_recording (tact->get_active());
	}
}

void
Editor::toggle_measure_visibility ()
{
	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("ToggleMeasureVisibility"));
	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		set_show_measures (tact->get_active());
	}
}

void
Editor::toggle_logo_visibility ()
{
	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Editor"), X_("ToggleLogoVisibility"));

	if (act) {
		Glib::RefPtr<ToggleAction> tact = Glib::RefPtr<ToggleAction>::cast_dynamic(act);
		if (logo_item) {
			if (tact->get_active()) {
				logo_item->show ();
			} else {
				logo_item->hide ();
			}
		}
	}
}

void
Editor::set_crossfade_model (CrossfadeModel model)
{
	RefPtr<Action> act;

	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	switch (model) {
	case FullCrossfade:
		act = ActionManager::get_action (X_("Editor"), X_("CrossfadesFull"));
		break;
	case ShortCrossfade:
		act = ActionManager::get_action (X_("Editor"), X_("CrossfadesShort"));
		break;
	}
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && ract->get_active()) {
			Config->set_xfade_model (model);
		}
	}
}

void
Editor::update_crossfade_model ()
{
	RefPtr<Action> act;

	switch (Config->get_xfade_model()) {
	case FullCrossfade:
		act = ActionManager::get_action (X_("Editor"), X_("CrossfadesFull"));
		break;
	case ShortCrossfade:
		act = ActionManager::get_action (X_("Editor"), X_("CrossfadesShort"));
		break;
	}

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && !ract->get_active()) {
			ract->set_active (true);
		}
	}
}


void
Editor::update_smpte_mode ()
{
	ENSURE_GUI_THREAD(mem_fun(*this, &Editor::update_smpte_mode));

	RefPtr<Action> act;
	const char* action = 0;

	switch (Config->get_smpte_format()) {
	case smpte_23976:
		action = X_("Smpte23976");
		break;
	case smpte_24:
		action = X_("Smpte24");
		break;
	case smpte_24976:
		action = X_("Smpte24976");
		break;
	case smpte_25:
		action = X_("Smpte25");
		break;
	case smpte_2997:
		action = X_("Smpte2997");
		break;
	case smpte_2997drop:
		action = X_("Smpte2997drop");
		break;
	case smpte_30:
		action = X_("Smpte30");
		break;
	case smpte_30drop:
		action = X_("Smpte30drop");
		break;
	case smpte_5994:
		action = X_("Smpte5994");
		break;
	case smpte_60:
		action = X_("Smpte60");
		break;
	}

	act = ActionManager::get_action (X_("Editor"), action);

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && !ract->get_active()) {
			ract->set_active (true);
		}
	}
}

void
Editor::update_video_pullup ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &Editor::update_video_pullup));

	RefPtr<Action> act;
	const char* action = 0;

	float pullup = Config->get_video_pullup();

	if ( pullup < (-4.1667 - 0.1) * 0.99) {
		action = X_("PullupMinus4Minus1");
	} else if ( pullup < (-4.1667) * 0.99 ) {
		action = X_("PullupMinus4");
	} else if ( pullup < (-4.1667 + 0.1) * 0.99 ) {
		action = X_("PullupMinus4Plus1");
	} else if ( pullup < (-0.1) * 0.99 ) {
		action = X_("PullupMinus1");
	} else if (pullup > (4.1667 + 0.1) * 0.99 ) {
		action = X_("PullupPlus4Plus1");
	} else if ( pullup > (4.1667) * 0.99 ) {
		action = X_("PullupPlus4");
	} else if ( pullup > (4.1667 - 0.1) * 0.99) {
		action = X_("PullupPlus4Minus1");
	} else if ( pullup > (0.1) * 0.99 ) {
		action = X_("PullupPlus1");
	} else {
		action = X_("PullupNone");
	}

	act = ActionManager::get_action (X_("Editor"), action);

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && !ract->get_active()) {
			ract->set_active (true);
		}
	}
}

void
Editor::update_layering_model ()
{
	RefPtr<Action> act;

	switch (Config->get_layer_model()) {
	case LaterHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerLaterHigher"));
		break;
	case MoveAddHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerMoveAddHigher"));
		break;
	case AddHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerAddHigher"));
		break;
	}

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && !ract->get_active()) {
			ract->set_active (true);
		}
	}
}

void
Editor::set_layer_model (LayerModel model)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<Action> act;

	switch (model) {
	case LaterHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerLaterHigher"));
		break;
	case MoveAddHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerMoveAddHigher"));
		break;
	case AddHigher:
		act = ActionManager::get_action (X_("Editor"), X_("LayerAddHigher"));
		break;
	}
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && ract->get_active() && Config->get_layer_model() != model) {
			Config->set_layer_model (model);
		}
	}
}

RefPtr<RadioAction>
Editor::snap_type_action (SnapType type)
{

	const char* action = 0;
	RefPtr<Action> act;
	
	switch (type) {
	case Editing::SnapToCDFrame:
		action = "snap-to-cd-frame";
		break;
	case Editing::SnapToSMPTEFrame:
		action = "snap-to-smpte-frame";
		break;
	case Editing::SnapToSMPTESeconds:
		action = "snap-to-smpte-seconds";
		break;
	case Editing::SnapToSMPTEMinutes:
		action = "snap-to-smpte-minutes";
		break;
	case Editing::SnapToSeconds:
		action = "snap-to-seconds";
		break;
	case Editing::SnapToMinutes:
		action = "snap-to-minutes";
		break;
	case Editing::SnapToAThirtysecondBeat:
		action = "snap-to-thirtyseconds";
		break;
	case Editing::SnapToASixteenthBeat:
		action = "snap-to-asixteenthbeat";
		break;
	case Editing::SnapToAEighthBeat:
		action = "snap-to-eighths";
		break;
	case Editing::SnapToAQuarterBeat:
		action = "snap-to-quarters";
		break;
	case Editing::SnapToAThirdBeat:
		action = "snap-to-thirds";
		break;
	case Editing::SnapToBeat:
		action = "snap-to-beat";
		break;
	case Editing::SnapToBar:
		action = "snap-to-bar";
		break;
	case Editing::SnapToMark:
		action = "snap-to-mark";
		break;
	case Editing::SnapToRegionStart:
		action = "snap-to-region-start";
		break;
	case Editing::SnapToRegionEnd:
		action = "snap-to-region-end";
		break;
	case Editing::SnapToRegionSync:
		action = "snap-to-region-sync";
		break;
	case Editing::SnapToRegionBoundary:
		action = "snap-to-region-boundary";
		break;
	default:
		fatal << string_compose (_("programming error: %1: %2"), "Editor: impossible snap-to type", (int) type) << endmsg;
		/*NOTREACHED*/
	}

	act = ActionManager::get_action (X_("Snap"), action);

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		return ract;

	} else  {
		error << string_compose (_("programming error: %1"), "Editor::snap_type_chosen could not find action to match type.") << endmsg;
		return RefPtr<RadioAction>();
	}
}

void
Editor::cycle_snap_choice()
{
	switch (snap_type) {
	case Editing::SnapToCDFrame:
		set_snap_to (Editing::SnapToSMPTEFrame);
		break;
	case Editing::SnapToSMPTEFrame:
		set_snap_to (Editing::SnapToSMPTESeconds);
		break;
	case Editing::SnapToSMPTESeconds:
		set_snap_to (Editing::SnapToSMPTEMinutes);
		break;
	case Editing::SnapToSMPTEMinutes:
		set_snap_to (Editing::SnapToSeconds);
		break;
	case Editing::SnapToSeconds:
		set_snap_to (Editing::SnapToMinutes);
		break;
	case Editing::SnapToMinutes:
		set_snap_to (Editing::SnapToAThirtysecondBeat);
		break;
	case Editing::SnapToAThirtysecondBeat:
		set_snap_to (Editing::SnapToASixteenthBeat);
		break;
	case Editing::SnapToASixteenthBeat:
		set_snap_to (Editing::SnapToAEighthBeat);
		break;
	case Editing::SnapToAEighthBeat:
		set_snap_to (Editing::SnapToAQuarterBeat);
		break;
	case Editing::SnapToAQuarterBeat:
		set_snap_to (Editing::SnapToAThirdBeat);
		break;
	case Editing::SnapToAThirdBeat:
		set_snap_to (Editing::SnapToBeat);
		break;
	case Editing::SnapToBeat:
		set_snap_to (Editing::SnapToBar);
		break;
	case Editing::SnapToBar:
		set_snap_to (Editing::SnapToMark);
		break;
	case Editing::SnapToMark:
		set_snap_to (Editing::SnapToRegionStart);
		break;
	case Editing::SnapToRegionStart:
		set_snap_to (Editing::SnapToRegionEnd);
		break;
	case Editing::SnapToRegionEnd:
		set_snap_to (Editing::SnapToRegionSync);
		break;
	case Editing::SnapToRegionSync:
		set_snap_to (Editing::SnapToRegionBoundary);
		break;
	case Editing::SnapToRegionBoundary:
		set_snap_to (Editing::SnapToCDFrame);
		break;
	}
}

void
Editor::snap_type_chosen (SnapType type)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<RadioAction> ract = snap_type_action (type);

	if (ract && ract->get_active()) {
		set_snap_to (type);
	}
}

RefPtr<RadioAction>
Editor::snap_mode_action (SnapMode mode)
{
	const char* action = 0;
	RefPtr<Action> act;
	
	switch (mode) {
	case Editing::SnapOff:
		action = X_("snap-off");
		break;
	case Editing::SnapNormal:
		action = X_("snap-normal");
		break;
	case Editing::SnapMagnetic:
		action = X_("snap-magnetic");
		break;
	default:
		fatal << string_compose (_("programming error: %1: %2"), "Editor: impossible snap mode type", (int) mode) << endmsg;
		/*NOTREACHED*/
	}
	
	act = ActionManager::get_action (X_("Editor"), action);
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		return ract;
		
	} else  {
		error << string_compose (_("programming error: %1: %2"), "Editor::snap_mode_chosen could not find action to match mode.", action) << endmsg;
		return RefPtr<RadioAction> ();
	}
}

void
Editor::cycle_snap_mode ()
{
	switch (snap_mode) {
	case SnapOff:
		set_snap_mode (SnapNormal);
		break;
	case SnapNormal:
		set_snap_mode (SnapMagnetic);
		break;
	case SnapMagnetic:
		set_snap_mode (SnapOff);
		break;
	}
}

void
Editor::snap_mode_chosen (SnapMode mode)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<RadioAction> ract = snap_mode_action (mode);

	if (ract && ract->get_active()) {
		set_snap_mode (mode);
	}
}

RefPtr<RadioAction>
Editor::edit_point_action (EditPoint ep)
{
	const char* action = 0;
	RefPtr<Action> act;
	
	switch (ep) {
	case Editing::EditAtPlayhead:
		action = X_("edit-at-playhead");
		break;
	case Editing::EditAtSelectedMarker:
		action = X_("edit-at-selected-marker");
		break;
	case Editing::EditAtMouse:
		action = X_("edit-at-mouse");
		break;
	default:
		fatal << string_compose (_("programming error: %1: %2"), "Editor: impossible edit point type", (int) ep) << endmsg;
		/*NOTREACHED*/
	}
	
	act = ActionManager::get_action (X_("Editor"), action);
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		return ract;
		
	} else  {
		error << string_compose (_("programming error: %1: %2"), "Editor::edit_point_action could not find action to match edit point.", action) << endmsg;
		return RefPtr<RadioAction> ();
	}
}

void
Editor::edit_point_chosen (EditPoint ep)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<RadioAction> ract = edit_point_action (ep);

	if (ract && ract->get_active()) {
		set_edit_point_preference (ep);
	}
}


RefPtr<RadioAction>
Editor::zoom_focus_action (ZoomFocus focus)
{
	const char* action = 0;
	RefPtr<Action> act;
	
	switch (focus) {
	case ZoomFocusLeft:
		action = X_("zoom-focus-left");
		break;
	case ZoomFocusRight:
		action = X_("zoom-focus-right");
		break;
	case ZoomFocusCenter:
		action = X_("zoom-focus-center");
		break;
	case ZoomFocusPlayhead:
		action = X_("zoom-focus-playhead");
		break;
	case ZoomFocusMouse:
		action = X_("zoom-focus-mouse");
		break;
	case ZoomFocusEdit:
		action = X_("zoom-focus-edit");
		break;
	default:
		fatal << string_compose (_("programming error: %1: %2"), "Editor: impossible focus type", (int) focus) << endmsg;
		/*NOTREACHED*/
	}
	
	act = ActionManager::get_action (X_("Zoom"), action);
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		return ract;
	} else {
		error << string_compose (_("programming error: %1: %2"), "Editor::zoom_focus_action could not find action to match focus.", action) << endmsg;
	}

	return RefPtr<RadioAction> ();
}

void
Editor::zoom_focus_chosen (ZoomFocus focus)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	RefPtr<RadioAction> ract = zoom_focus_action (focus);

	if (ract && ract->get_active()) {
		set_zoom_focus (focus);
	}
}

void
Editor::smpte_fps_chosen (SmpteFormat format)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	if (session) {

		RefPtr<Action> act;

		switch (format) {
			case smpte_23976: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte23976"));
			 break;
			case smpte_24: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte24"));
			 break;
			case smpte_24976: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte24976"));
			 break;
			case smpte_25: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte25"));
			 break;
			case smpte_2997: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte2997"));
			 break;
			case smpte_2997drop: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte2997drop"));
			 break;
			case smpte_30: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte30"));
			 break;
			case smpte_30drop: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte30drop"));
			 break;
			case smpte_5994: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte5994"));
			 break;
			case smpte_60: 
				act = ActionManager::get_action (X_("Editor"), X_("Smpte60"));
			 break;
			default:
				cerr << "Editor received unexpected smpte type" << endl;
		}

		if (act) {
			RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
			if (ract && ract->get_active()) {
			        session->set_smpte_format (format);
			}
		}
	}
}

void
Editor::video_pullup_chosen (Session::PullupFormat pullup)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	const char* action = 0;

	RefPtr<Action> act;
	
	float pull = 0.0;
	
	switch (pullup) {
	case Session::pullup_Plus4Plus1:
		pull = 4.1667 + 0.1;
		action = X_("PullupPlus4Plus1");
		break;
	case Session::pullup_Plus4:
		pull = 4.1667;
		action = X_("PullupPlus4");
		break;
	case Session::pullup_Plus4Minus1:
		pull = 4.1667 - 0.1;
		action = X_("PullupPlus4Minus1");
		break;
	case Session::pullup_Plus1:
		pull = 0.1;
		action = X_("PullupPlus1");
		break;
	case Session::pullup_None:
		pull = 0.0;
		action = X_("PullupNone");
		break;
	case Session::pullup_Minus1:
		pull = -0.1;
		action = X_("PullupMinus1");
		break;
	case Session::pullup_Minus4Plus1:
		pull = -4.1667 + 0.1;
		action = X_("PullupMinus4Plus1");
		break;
	case Session::pullup_Minus4:
		pull = -4.1667;
		action = X_("PullupMinus4");
		break;
	case Session::pullup_Minus4Minus1:
		pull = -4.1667 - 0.1;
		action = X_("PullupMinus4Minus1");
		break;
	default:
		fatal << string_compose (_("programming error: %1"), "Session received unexpected pullup type") << endmsg;
		/*NOTREACHED*/
	}
	
	act = ActionManager::get_action (X_("Editor"), action);
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && ract->get_active()) {
			Config->set_video_pullup ( pull );
		}
		
	} else  {
		error << string_compose (_("programming error: %1"), "Editor::video_pullup_chosen could not find action to match pullup.") << endmsg;
	}
}

void
Editor::update_subframes_per_frame ()
{
	ENSURE_GUI_THREAD (mem_fun(*this, &Editor::update_subframes_per_frame));

	RefPtr<Action> act;
	const char* action = 0;

	uint32_t sfpf = Config->get_subframes_per_frame();

	if (sfpf == 80) {
		action = X_("Subframes80");
	} else if (sfpf == 100) {
		action = X_("Subframes100");
	} else {
		warning << string_compose (_("Configuraton is using unhandled subframes per frame value: %1"), sfpf) << endmsg;
		/*NOTREACHED*/
		return;
	}

	act = ActionManager::get_action (X_("Editor"), action);

	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && !ract->get_active()) {
			ract->set_active (true);
		}
	}
}

void
Editor::subframes_per_frame_chosen (uint32_t sfpf)
{
	/* this is driven by a toggle on a radio group, and so is invoked twice,
	   once for the item that became inactive and once for the one that became
	   active.
	*/

	const char* action = 0;

	RefPtr<Action> act;
	
	if (sfpf == 80) {
		action = X_("Subframes80");
	} else if (sfpf == 100) {	
		action = X_("Subframes100");
	} else {
		fatal << string_compose (_("programming error: %1 %2"), "Session received unexpected subframes per frame value: ", sfpf) << endmsg;
		/*NOTREACHED*/
	}
	
	act = ActionManager::get_action (X_("Editor"), action);
	
	if (act) {
		RefPtr<RadioAction> ract = RefPtr<RadioAction>::cast_dynamic(act);
		if (ract && ract->get_active()) {
			Config->set_subframes_per_frame ((uint32_t) rint (sfpf));
		}
		
	} else  {
		error << string_compose (_("programming error: %1"), "Editor::subframes_per_frame_chosen could not find action to match value.") << endmsg;
	}
}

void
Editor::toggle_auto_xfade ()
{
	ActionManager::toggle_config_state ("Editor", "toggle-auto-xfades", &Configuration::set_auto_xfade, &Configuration::get_auto_xfade);
}

void
Editor::toggle_xfades_active ()
{
	ActionManager::toggle_config_state ("Editor", "toggle-xfades-active", &Configuration::set_xfades_active, &Configuration::get_xfades_active);
}

void
Editor::toggle_xfade_visibility ()
{
	ActionManager::toggle_config_state ("Editor", "toggle-xfades-visible", &Configuration::set_xfades_visible, &Configuration::get_xfades_visible);
}

void
Editor::toggle_link_region_and_track_selection ()
{
	ActionManager::toggle_config_state ("Editor", "link-region-and-track-selection", &Configuration::set_link_region_and_track_selection, &Configuration::get_link_region_and_track_selection);
}

/** A Configuration parameter has changed.
 * @param parameter_name Name of the changed parameter.
 */
void
Editor::parameter_changed (const char* parameter_name)
{
#define PARAM_IS(x) (!strcmp (parameter_name, (x)))
	//cerr << "Editor::parameter_changed: " << parameter_name << endl;
	ENSURE_GUI_THREAD (bind (mem_fun (*this, &Editor::parameter_changed), parameter_name));

	if (PARAM_IS ("auto-loop")) {
		update_loop_range_view (true);
	} else if (PARAM_IS ("punch-in")) {
		update_punch_range_view (true);
	} else if (PARAM_IS ("punch-out")) {
		update_punch_range_view (true);
	} else if (PARAM_IS ("layer-model")) {
		update_layering_model ();
	} else if (PARAM_IS ("smpte-format")) {
	        update_smpte_mode ();
		update_just_smpte ();
	} else if (PARAM_IS ("video-pullup")) {
		update_video_pullup ();
	} else if (PARAM_IS ("xfades-active")) {
		ActionManager::map_some_state ("Editor", "toggle-xfades-active", &Configuration::get_xfades_active);
	} else if (PARAM_IS ("xfades-visible")) {
		ActionManager::map_some_state ("Editor", "toggle-xfades-visible", &Configuration::get_xfades_visible);
		update_xfade_visibility ();
	} else if (PARAM_IS ("auto-xfade")) {
		ActionManager::map_some_state ("Editor", "toggle-auto-xfades", &Configuration::get_auto_xfade);
	} else if (PARAM_IS ("xfade-model")) {
		update_crossfade_model ();
	} else if (PARAM_IS ("edit-mode")) {
		edit_mode_selector.set_active_text (edit_mode_to_string (Config->get_edit_mode()));
	} else if (PARAM_IS ("subframes-per-frame")) {
		update_subframes_per_frame ();
		update_just_smpte ();
	} else if (PARAM_IS ("show-track-meters")) {
		toggle_meter_updating();
	} else if (PARAM_IS ("link-region-and-track-selection")) {
		ActionManager::map_some_state ("Editor", "link-region-and-track-selection", &Configuration::get_link_region_and_track_selection);
	}

#undef PARAM_IS
}

void
Editor::reset_focus ()
{
	track_canvas.grab_focus();
}
