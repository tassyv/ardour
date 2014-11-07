/*
    Copyright (C) 2014 Waves Audio Ltd.

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
#include <stdlib.h>
#include <string>
#include <stdio.h>

#include "tracks_control_panel.h"
#include "waves_button.h"
#include "pbd/unwind.h"

#include <gtkmm2ext/utils.h>

#include "ardour/types.h"
#include "ardour/engine_state_controller.h"
#include "ardour/rc_configuration.h"
#include "ardour/recent_sessions.h"
#include "ardour/filename_extensions.h"

#include "ardour/utils.h"
#include "ardour_ui.h"
#include "gui_thread.h"
#include "utils.h"
#include "i18n.h"
#include "pbd/convert.h"

#include "timecode/time.h"
#include "time.h"

#include "open_file_dialog_proxy.h"

using namespace ARDOUR;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace PBD;
using namespace Glib;
using namespace Timecode;

#define dbg_msg(a) MessageDialog (a, PROGRAM_NAME).run();

namespace {

    static const char* audio_capture_name_prefix = "system:capture:";
    static const char* audio_playback_name_prefix = "system:playback:";
    static const char* midi_port_name_prefix = "system_midi:";
    static const char* midi_capture_suffix = " capture";
    static const char* midi_playback_suffix = " playback";
    
    struct MidiDeviceDescriptor {
        std::string name;
        std::string capture_name;
        bool capture_active;
        std::string playback_name;
        bool playback_active;
        
        MidiDeviceDescriptor(const std::string& name) :
        name(name),
        capture_name(""),
        capture_active(false),
        playback_name(""),
        playback_active(false)
        {}
        
        bool operator==(const MidiDeviceDescriptor& rhs) {
            return name == rhs.name;
        }
    };
    
    typedef std::vector<MidiDeviceDescriptor> MidiDeviceDescriptorVec;
}

void
TracksControlPanel::init ()
{
	_ok_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_ok));
	_cancel_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_cancel));
	_apply_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_apply));

	_audio_settings_tab_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_a_settings_tab_button_clicked));
	_midi_settings_tab_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_a_settings_tab_button_clicked));
	_session_settings_tab_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_a_settings_tab_button_clicked));
	_general_settings_tab_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_a_settings_tab_button_clicked));
    
    _all_inputs_on_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_all_inputs_on_button));
    _all_inputs_off_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_all_inputs_off_button));
    _all_outputs_on_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_all_outputs_on_button));
    _all_outputs_off_button.signal_clicked.connect (sigc::mem_fun (*this, &TracksControlPanel::on_all_outputs_off_button));
    
    _multi_out_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_multi_out));
    _stereo_out_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_stereo_out));
    
    _browse_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_browse_button));    
    
	EngineStateController::instance ()->EngineRunning.connect (running_connection, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::engine_running, this), gui_context());
	EngineStateController::instance ()->EngineStopped.connect (stopped_connection, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::engine_stopped, this), gui_context());
	EngineStateController::instance ()->EngineHalted.connect (stopped_connection, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::engine_stopped, this), gui_context());

	/* Subscribe for udpates from EngineStateController */
    EngineStateController::instance()->PortRegistrationChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_port_registration_update, this), gui_context());
	EngineStateController::instance()->BufferSizeChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_buffer_size_update, this), gui_context());
    EngineStateController::instance()->DeviceListChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_device_list_update, this, _1), gui_context());
    EngineStateController::instance()->InputConfigChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_audio_input_configuration_changed, this), gui_context());
    EngineStateController::instance()->OutputConfigChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_audio_output_configuration_changed, this), gui_context());
    EngineStateController::instance()->MIDIInputConfigChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_midi_input_configuration_changed, this), gui_context());
    EngineStateController::instance()->MIDIOutputConfigChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_midi_output_configuration_changed, this), gui_context());
    EngineStateController::instance()->DeviceError.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_device_error, this), gui_context());

    /* Global configuration parameters update */
    Config->ParameterChanged.connect (update_connections, MISSING_INVALIDATOR, boost::bind (&TracksControlPanel::on_parameter_changed, this, _1), gui_context());
    
    _engine_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_engine_dropdown_item_clicked));
    _device_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_device_dropdown_item_clicked));
	_sample_rate_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_sample_rate_dropdown_item_clicked));
	_buffer_size_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_buffer_size_dropdown_item_clicked));
    
    /* Session configuration parameters update */
	_file_type_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_file_type_dropdown_item_clicked));
    _bit_depth_dropdown.selected_item_changed.connect (sigc::mem_fun(*this, &TracksControlPanel::on_bit_depth_dropdown_item_clicked));
    _frame_rate_dropdown.selected_item_changed.connect (sigc::mem_fun (*this, &TracksControlPanel::on_frame_rate_item_clicked));

    _name_tracks_after_driver.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_name_tracks_after_driver));
    _reset_tracks_name_to_default.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_reset_tracks_name_to_default));

    _control_panel_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_control_panel_button));
    
    _yes_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_yes_button));
    _no_button.signal_clicked.connect(sigc::mem_fun (*this, &TracksControlPanel::on_no_button));
    _yes_button.set_visible(false);
    _no_button.set_visible(false);
    
	populate_engine_dropdown ();
	populate_device_dropdown ();

	populate_output_mode ();

    populate_file_type_dropdown ();
    populate_input_channels ();
    populate_output_channels ();
    populate_midi_ports ();
    populate_default_session_path ();
    
    // Init session Settings
    populate_bit_depth_dropdown();
    populate_frame_rate_dropdown();
    populate_auto_lock_timer_dropdown();
    populate_auto_save_timer_dropdown();
    populate_pre_record_buffer_dropdown();
    
	_audio_settings_tab_button.set_active(true);

	display_general_preferences ();
}

DeviceConnectionControl& TracksControlPanel::add_device_capture_control(std::string port_name, bool active, uint16_t capture_number, std::string track_name)
{
    std::string device_capture_name("");
    std::string pattern(audio_capture_name_prefix);
    ARDOUR::remove_pattern_from_string(port_name, pattern, device_capture_name);
    
	DeviceConnectionControl &capture_control = *manage (new DeviceConnectionControl(device_capture_name, active, capture_number, track_name));
    
    char * id_str = new char [port_name.length()+1];
    std::strcpy (id_str, port_name.c_str());
    capture_control.set_data(DeviceConnectionControl::id_name, id_str);
	
    _device_capture_list.pack_start (capture_control, false, false);
	capture_control.signal_active_changed.connect (sigc::mem_fun (*this, &TracksControlPanel::on_capture_active_changed));
	return capture_control;
}

DeviceConnectionControl& TracksControlPanel::add_device_playback_control(std::string port_name, bool active, uint16_t playback_number)
{
    std::string device_playback_name("");
    std::string pattern(audio_playback_name_prefix);
    ARDOUR::remove_pattern_from_string(port_name, pattern, device_playback_name);
    
	DeviceConnectionControl &playback_control = *manage (new DeviceConnectionControl(device_playback_name, active, playback_number));
    
    char * id_str = new char [port_name.length()+1];
    std::strcpy (id_str, port_name.c_str());
    playback_control.set_data(DeviceConnectionControl::id_name, id_str);
    
	_device_playback_list.pack_start (playback_control, false, false);
	playback_control.signal_active_changed.connect(sigc::mem_fun (*this, &TracksControlPanel::on_playback_active_changed));
	return playback_control;
}

MidiDeviceConnectionControl& TracksControlPanel::add_midi_device_control(const std::string& midi_device_name,
                                                                         const std::string& capture_name, bool capture_active,
                                                                         const std::string& playback_name, bool playback_active)
{
	MidiDeviceConnectionControl &midi_device_control = *manage (new MidiDeviceConnectionControl(midi_device_name, !capture_name.empty(), capture_active, !playback_name.empty(), playback_active));
    
    if (!capture_name.empty()) {
        char * capture_id_str = new char [capture_name.length()+1];
        std::strcpy (capture_id_str, capture_name.c_str());
        midi_device_control.set_data(MidiDeviceConnectionControl::capture_id_name, capture_id_str);
    }
    
    if (!playback_name.empty()) {
        char * playback_id_str = new char [playback_name.length()+1];
        std::strcpy (playback_id_str, playback_name.c_str());
        midi_device_control.set_data(MidiDeviceConnectionControl::playback_id_name, playback_id_str);
    }
    
	_midi_device_list.pack_start (midi_device_control, false, false);
	midi_device_control.signal_capture_active_changed.connect (sigc::mem_fun (*this, &TracksControlPanel::on_midi_capture_active_changed));
    midi_device_control.signal_playback_active_changed.connect(sigc::mem_fun (*this, &TracksControlPanel::on_midi_playback_active_changed));
	return midi_device_control;
}

namespace  {
    // Strings which are shown to user in the Preference panel
    const std::string string_CAF = "Caf";
    const std::string string_BWav = "BWav";
    const std::string string_Aiff = "Aiff";
    const std::string string_Wav64 = "Wave64";

    std::string
    HeaderFormat_to_string(HeaderFormat header_format)
    {
        switch (header_format) {
            case CAF:
                return string_CAF;
            case BWF:
                return string_BWav;
            case AIFF:
                return string_Aiff;
            case WAVE64:
                return string_Wav64;
            default:
                return std::string("");
        }
        
        return std::string("");
    }
    
    HeaderFormat
    string_to_HeaderFormat(std::string s)
    {
        if(s == string_CAF)
            return CAF;
        
        if(s == string_BWav)
            return BWF;
        
        if(s == string_Aiff)
            return AIFF;
        
        if(s == string_Wav64)
            return WAVE64;
        
        //defaul value
        return BWF;
    }
    
    std::string
    xml_string_to_user_string(std::string xml_string);
    
    enum SessionProperty {
        Native_File_Header_Format,
        Native_File_Data_Format,
        Timecode_Format
    };
    
    std::string
    read_property_from_last_session(SessionProperty session_property)
    {        
        ARDOUR::RecentSessions rs;
        ARDOUR::read_recent_sessions (rs);
        
        if( rs.size() > 0 )
        {
            std::string full_session_name = Glib::build_filename( rs[0].second, rs[0].first );
            full_session_name += statefile_suffix;
            
            // read property from session projectfile
            boost::shared_ptr<XMLTree> state_tree(new XMLTree());
            
            if (!state_tree->read (full_session_name))
                return std::string("");
            
            XMLNode& root (*state_tree->root());

            if (root.name() != X_("Session"))
                return std::string("");
            
            XMLNode* config_main_node = root.child ("Config");
            if( !config_main_node )
                return std::string("");

            XMLNodeList config_nodes_list = config_main_node->children();
            XMLNodeConstIterator config_node_iter = config_nodes_list.begin();
            
            std::string required_property_name;
            
            switch (session_property) {
                case Native_File_Header_Format:
                    required_property_name = "native-file-header-format";
                    break;
                case Native_File_Data_Format:
                    required_property_name = "native-file-data-format";
                    break;
                case Timecode_Format:
                    required_property_name = "timecode-format";
                    break;
                default:
                    return std::string("");
            }
            
            for (; config_node_iter != config_nodes_list.end(); ++config_node_iter)
            {
                XMLNode* config_node = *config_node_iter;
                XMLProperty* prop = NULL;
                
                if ( (prop = config_node->property ("name")) != 0 )
                    if( prop->value() == required_property_name )
                        if ( (prop = config_node->property ("value")) != 0 )
                            return xml_string_to_user_string( prop->value() );
            }
        } 
        
        return std::string("");
    }
}

namespace {
    // Strings which are shown to user in the Preference panel
    const std::string string_bit32 = "32 bit floating point";
    const std::string string_bit24 = "24 bit";
    const std::string string_bit16 = "16 bit";
    
    std::string
    SampleFormat_to_string(SampleFormat sample_format)
    {
        switch (sample_format) {
            case FormatFloat:
                return string_bit32;
            case FormatInt24:
                return string_bit24;
            case FormatInt16:
                return string_bit16;
        }
        
        return std::string("");
    }

    SampleFormat
    string_to_SampleFormat(std::string s)
    {
        if(s == string_bit32)
            return FormatFloat;
        
        if(s == string_bit24)
            return FormatInt24;
        
        if(s == string_bit16)
            return FormatInt16;
        
        // default value
        return FormatInt24;
    }
}

void
TracksControlPanel::populate_bit_depth_dropdown()
{
    // Get BIT_DEPTH from last used session
    std::string sample_format_string = read_property_from_last_session(Native_File_Data_Format);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    SampleFormat sample_format = string_to_SampleFormat(sample_format_string);
    ardour_ui->set_sample_format( sample_format );
    
    {
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
        _bit_depth_dropdown.set_text ( SampleFormat_to_string(sample_format) );
	}
   
    return;
}

namespace  {
    const std::string string_24fps = "24 fps";
    const std::string string_25fps = "25 fps";
    const std::string string_30fps = "30 fps";
    const std::string string_23976fps = "23.976 fps";
    const std::string string_2997fps = "29.97 fps";
    
    std::string
    TimecodeFormat_to_string(Timecode::TimecodeFormat timecode_format)
    {
        
        switch (timecode_format) {
            case timecode_24:
                return string_24fps;
            case timecode_25:
                return string_25fps;
            case timecode_30:
                return string_30fps;
            case timecode_23976:
                return string_23976fps;
            case timecode_2997:
                return string_2997fps;
                
            default:
                return std::string("");
        }
        
        return std::string("");
    }
    
    Timecode::TimecodeFormat
    string_to_TimecodeFormat(std::string s)
    {
        if(s == string_24fps)
            return timecode_24;
        if(s == string_25fps)
            return timecode_25;
        if(s == string_30fps)
            return timecode_30;
        if(s == string_23976fps)
            return timecode_23976;
        if(s == string_2997fps)
            return timecode_2997;
        
        //defaul value
        return timecode_25;
    }
    
    std::string
    xml_string_to_user_string(std::string xml_string)
    {
        // Bit depth format
        if(xml_string == enum_2_string (FormatFloat))
            return string_bit32;
        
        if(xml_string == enum_2_string (FormatInt24))
            return string_bit24;
        
        if(xml_string == enum_2_string (FormatInt16))
            return string_bit16;
        
        
        // Header format (File type)
        if(xml_string == enum_2_string(CAF))
            return string_CAF;
        
        if(xml_string == enum_2_string(BWF))
            return string_BWav;
        
        if(xml_string == enum_2_string(AIFF))
            return string_Aiff;
        
        if(xml_string == enum_2_string(WAVE64))
            return string_Wav64;
        
        // fps (Timecode)
        if(xml_string == enum_2_string(Timecode::timecode_24))
            return string_24fps;
        
        if(xml_string == enum_2_string(Timecode::timecode_25))
            return string_25fps;
        
        if(xml_string == enum_2_string(Timecode::timecode_30))
            return string_30fps;
        
        if(xml_string == enum_2_string(Timecode::timecode_23976))
            return string_23976fps;
        
        if(xml_string == enum_2_string(Timecode::timecode_2997))
            return string_2997fps;
        
        return std::string("");
    }

}

void
TracksControlPanel::populate_frame_rate_dropdown()
{
    // Get FRAME_RATE from last used session
    std::string last_used_frame_rate = read_property_from_last_session(Timecode_Format);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    Timecode::TimecodeFormat timecode_format = string_to_TimecodeFormat(last_used_frame_rate);
    ardour_ui->set_timecode_format( timecode_format );
    
    {
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
        _frame_rate_dropdown.set_text ( TimecodeFormat_to_string(timecode_format) );
	}
    
    return;
}

void
TracksControlPanel::populate_auto_lock_timer_dropdown()
{
    int time = ARDOUR_UI::config()->get_auto_lock_timer();
    std::stringstream ss;
    ss << time;
    std::string str_time = ss.str() + " Min";
    
    _auto_lock_timer_dropdown.set_text( str_time );
}

void
TracksControlPanel::populate_auto_save_timer_dropdown()
{
    int time = ARDOUR_UI::config()->get_auto_save_timer();
    std::stringstream ss;
    ss << time;
    std::string str_time = ss.str() + " Min";
    
    _auto_save_timer_dropdown.set_text( str_time );
}

void
TracksControlPanel::populate_pre_record_buffer_dropdown()
{
    int time = ARDOUR_UI::config()->get_pre_record_buffer();
    std::stringstream ss;
    ss << time;
    std::string str_time = ss.str() + " Min";
    
    _pre_record_buffer_dropdown.set_text( str_time );
}

void
TracksControlPanel::refresh_session_settings_info()
{
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    if( !ardour_ui )
        return;
    
    Session* session = ardour_ui->the_session();
    if( !session )
        return;
    _bit_depth_dropdown.set_text( SampleFormat_to_string(session->config.get_native_file_data_format()) );
    _file_type_dropdown.set_text( HeaderFormat_to_string(session->config.get_native_file_header_format()) );
    _frame_rate_dropdown.set_text( TimecodeFormat_to_string(session->config.get_timecode_format()) );
}

void
TracksControlPanel::populate_default_session_path()
{
    std::string std_path = Config->get_default_session_parent_dir();
    bool folderExist = Glib::file_test(std_path, FILE_TEST_EXISTS);
    
    if ( !folderExist )
        Config->set_default_session_parent_dir(Glib::get_home_dir());
    
    _default_open_path.set_text(Config->get_default_session_parent_dir());
}

void
TracksControlPanel::populate_engine_dropdown()
{
	if (_ignore_changes) {
		return;
	}

    std::vector<const AudioBackendInfo*> backends;
    EngineStateController::instance()->available_backends(backends);

	if (backends.empty()) {
		MessageDialog msg (string_compose (_("No audio/MIDI backends detected. %1 cannot run\n\n(This is a build/packaging/system error. It should never happen.)"), PROGRAM_NAME));
		msg.run ();
		throw failed_constructor ();
	}
	for (std::vector<const AudioBackendInfo*>::const_iterator b = backends.begin(); b != backends.end(); ++b) {
		_engine_dropdown.add_menu_item ((*b)->name, 0);
	}

	{
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		_engine_dropdown.set_sensitive (backends.size() > 1);
	}

	if (!backends.empty() )
	{
		_engine_dropdown.set_text (EngineStateController::instance()->get_current_backend_name() );
	}
}

void
TracksControlPanel::populate_device_dropdown()
{
    std::vector<AudioBackend::DeviceStatus> all_devices;
	EngineStateController::instance()->enumerate_devices (all_devices);

	_device_dropdown.clear_items ();
	for (std::vector<AudioBackend::DeviceStatus>::const_iterator i = all_devices.begin(); i != all_devices.end(); ++i) {
		_device_dropdown.add_menu_item (i->name, 0);
	}

	{
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		_device_dropdown.set_sensitive (all_devices.size() > 1);
	}

    if(!all_devices.empty() ) {
		_device_dropdown.set_text (EngineStateController::instance ()->get_current_device_name());
        device_changed();
    }
}

void
TracksControlPanel::populate_file_type_dropdown()
{
    // Get FILE_TYPE from last used session
    std::string header_format_string = read_property_from_last_session(Native_File_Header_Format);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    HeaderFormat header_format = string_to_HeaderFormat(header_format_string);
    ardour_ui->set_header_format( header_format );
	{
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		_file_type_dropdown.set_text( HeaderFormat_to_string(header_format) );
	}
    return;
}

void
TracksControlPanel::populate_sample_rate_dropdown()
{
    std::vector<float> sample_rates;
    EngineStateController::instance()->available_sample_rates_for_current_device(sample_rates);

	_sample_rate_dropdown.clear_items ();

	for (std::vector<float>::const_iterator x = sample_rates.begin(); x != sample_rates.end(); ++x) {
		_sample_rate_dropdown.add_menu_item (ARDOUR_UI_UTILS::rate_as_string (*x), 0);
	}

	// set _ignore_changes flag to ignore changes in combo-box callbacks
	PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
	_sample_rate_dropdown.set_sensitive (sample_rates.size() > 1);

	if (!sample_rates.empty() ) {
		std::string active_sr = ARDOUR_UI_UTILS::rate_as_string(EngineStateController::instance()->get_current_sample_rate() );
		_sample_rate_dropdown.set_text(active_sr);
	}
}

void
TracksControlPanel::populate_buffer_size_dropdown()
{
	std::vector<pframes_t> buffer_sizes;
	EngineStateController::instance()->available_buffer_sizes_for_current_device(buffer_sizes);

	_buffer_size_dropdown.clear_items ();
	for (std::vector<pframes_t>::const_iterator x = buffer_sizes.begin(); x != buffer_sizes.end(); ++x) {
		_buffer_size_dropdown.add_menu_item (bufsize_as_string (*x), 0);
	}

	{
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		_buffer_size_dropdown.set_sensitive (buffer_sizes.size() > 1);

		if (!buffer_sizes.empty() ) {
			std::string active_bs = bufsize_as_string(EngineStateController::instance()->get_current_buffer_size());
			_buffer_size_dropdown.set_text(active_bs);
		}
	}
}

void
TracksControlPanel::populate_output_mode()
{
    _multi_out_button.set_active(Config->get_output_auto_connect() & AutoConnectPhysical);
    _stereo_out_button.set_active(Config->get_output_auto_connect() & AutoConnectMaster);
    
    _all_outputs_on_button.set_sensitive(Config->get_output_auto_connect() & AutoConnectPhysical);
    _all_outputs_off_button.set_sensitive(Config->get_output_auto_connect() & AutoConnectPhysical);
}


void
TracksControlPanel::populate_input_channels()
{
    cleanup_input_channels_list();
    
    // process captures (inputs)
    std::vector<EngineStateController::PortState> input_states;
    EngineStateController::instance()->get_physical_audio_input_states(input_states);
    
    std::vector<EngineStateController::PortState>::const_iterator input_iter;
    
    uint16_t number_count = 1;
    for (input_iter = input_states.begin(); input_iter != input_states.end(); ++input_iter ) {
        
        uint16_t number = DeviceConnectionControl::NoNumber;
        std::string track_name;

        if (input_iter->active) {
            
            std::string port_name("");
            std::string pattern(audio_capture_name_prefix);
            ARDOUR::remove_pattern_from_string(input_iter->name, pattern, port_name);
            
            number = number_count++;
            
            if (Config->get_tracks_auto_naming() & UseDefaultNames) {
                track_name = string_compose ("%1 %2", Session::default_trx_track_name_pattern, number);
            } else if (Config->get_tracks_auto_naming() & NameAfterDriver) {
                track_name = port_name;
            }
        }
        
        add_device_capture_control (input_iter->name, input_iter->active, number, track_name);
    }
    
    _all_inputs_on_button.set_sensitive(!input_states.empty() );
    _all_inputs_off_button.set_sensitive(!input_states.empty() );
}


void
TracksControlPanel::populate_output_channels()
{
    cleanup_output_channels_list();
        
    // process captures (outputs)
    std::vector<EngineStateController::PortState> output_states;
    EngineStateController::instance()->get_physical_audio_output_states(output_states);
    
    std::vector<EngineStateController::PortState>::const_iterator output_iter;
    
    uint16_t number_count = 1;
    for (output_iter = output_states.begin(); output_iter != output_states.end(); ++output_iter ) {
        
        uint16_t number = DeviceConnectionControl::NoNumber;
        
        if (output_iter->active) {
            number = number_count++;
        }
        
        add_device_playback_control (output_iter->name, output_iter->active, number);
    }
    
    bool stereo_out_disabled = (Config->get_output_auto_connect() & AutoConnectPhysical);
    _all_outputs_on_button.set_sensitive(!output_states.empty() && stereo_out_disabled );
    _all_outputs_off_button.set_sensitive(!output_states.empty() && stereo_out_disabled );
}


void
TracksControlPanel::populate_midi_ports()
{
    cleanup_midi_device_list();
    
    std::vector<EngineStateController::PortState> midi_input_states, midi_output_states;
    EngineStateController::instance()->get_physical_midi_input_states(midi_input_states);
    EngineStateController::instance()->get_physical_midi_output_states(midi_output_states);
    
    // now group corresponding inputs and outputs into a std::vector of midi device descriptors
    MidiDeviceDescriptorVec midi_device_descriptors;
    std::vector<EngineStateController::PortState>::const_iterator state_iter;
    // process inputs
    for (state_iter = midi_input_states.begin(); state_iter != midi_input_states.end(); ++state_iter) {
        // strip the device name from input port name
        std::string device_name("");
        ARDOUR::remove_pattern_from_string(state_iter->name, midi_port_name_prefix, device_name);
        ARDOUR::remove_pattern_from_string(device_name, midi_capture_suffix, device_name);
        
        MidiDeviceDescriptor device_descriptor(device_name);
        device_descriptor.capture_name = state_iter->name;
        device_descriptor.capture_active = state_iter->active;
        midi_device_descriptors.push_back(device_descriptor);
    }
    
    // process outputs
    for (state_iter = midi_output_states.begin(); state_iter != midi_output_states.end(); ++state_iter){
        // strip the device name from input port name
        std::string device_name("");
        ARDOUR::remove_pattern_from_string(state_iter->name, midi_port_name_prefix, device_name);
        ARDOUR::remove_pattern_from_string(device_name, midi_playback_suffix, device_name);
        
        // check if we already have descriptor for this device
        MidiDeviceDescriptor device_descriptor(device_name);
        MidiDeviceDescriptorVec::iterator found_iter;
        found_iter = std::find(midi_device_descriptors.begin(), midi_device_descriptors.end(), device_descriptor );
        
        if (found_iter != midi_device_descriptors.end() ) {
            found_iter->playback_name = state_iter->name;
            found_iter->playback_active = state_iter->active;
        } else {
            device_descriptor.capture_name.clear();
            device_descriptor.playback_name = state_iter->name;
            device_descriptor.playback_active = state_iter->active;
            midi_device_descriptors.push_back(device_descriptor);
        }
    }
    
    // now add midi device controls
    MidiDeviceDescriptorVec::iterator iter;
    for (iter = midi_device_descriptors.begin(); iter != midi_device_descriptors.end(); ++iter ) {
        add_midi_device_control(iter->name, iter->capture_name, iter->capture_active,
                                            iter->playback_name, iter->playback_active);
    }
}


void
TracksControlPanel::cleanup_input_channels_list()
{
    std::vector<Gtk::Widget*> capture_controls = _device_capture_list.get_children();
        
    while (capture_controls.size() != 0) {
        Gtk::Widget* item = capture_controls.back();
        
        DeviceConnectionControl* control = dynamic_cast<DeviceConnectionControl*>(item);
        
        if (control) {
            control->remove_data(DeviceConnectionControl::id_name);
        }
        
        capture_controls.pop_back();
        _device_capture_list.remove(*item);
        delete item;
    }
}


void
TracksControlPanel::cleanup_output_channels_list()
{
    std::vector<Gtk::Widget*> playback_controls = _device_playback_list.get_children();

    while (playback_controls.size() != 0) {
        Gtk::Widget* item = playback_controls.back();
        
        DeviceConnectionControl* control = dynamic_cast<DeviceConnectionControl*>(item);
        
        if (control) {
            control->remove_data(DeviceConnectionControl::id_name);
        }
        
        playback_controls.pop_back();
        _device_playback_list.remove(*item);
        delete item;
    }
}


void
TracksControlPanel::cleanup_midi_device_list()
{
    std::vector<Gtk::Widget*> midi_device_controls = _midi_device_list.get_children();
    
    while (midi_device_controls.size() != 0) {
        Gtk::Widget* item = midi_device_controls.back();
        
        MidiDeviceConnectionControl* control = dynamic_cast<MidiDeviceConnectionControl*>(item);
        
        if (control) {
            control->remove_data(MidiDeviceConnectionControl::capture_id_name);
            control->remove_data(MidiDeviceConnectionControl::playback_id_name);
        }
        
        midi_device_controls.pop_back();
        _midi_device_list.remove(*item);
        delete item;
    }
}


void TracksControlPanel::display_general_preferences ()
{
	ARDOUR::WaveformShape shape = Config->get_waveform_shape ();
	switch (shape) {
	case Traditional:
		_waveform_shape_dropdown.set_current_item (0);
		break;
	case Rectified:
		_waveform_shape_dropdown.set_current_item (1);
		break;
	default:
		dbg_msg ("TracksControlPanel::display_general_preferences ():\nUnexpected WaveFormShape !");
		break;
	}

	float peak_hold_time = Config->get_meter_hold ();
	int selected_item = 0;
	if (peak_hold_time <= (MeterHoldOff + 0.1)) {
		selected_item = 0;
	} else if (peak_hold_time <= (MeterHoldShort + 0.1)) {
		selected_item = 1;
	} else if (peak_hold_time <= (MeterHoldMedium + 0.1)) {
		selected_item = 2;
	} else if (peak_hold_time <= (MeterHoldLong + 0.1)) {
		selected_item = 3;
	} 
	_peak_hold_time_dropdown.set_current_item (selected_item);

	float meter_falloff = Config->get_meter_falloff ();
	selected_item = 0;

	if (meter_falloff <= (METER_FALLOFF_OFF + 0.1)) {
		selected_item = 0;
	} else if (meter_falloff <= (METER_FALLOFF_SLOWEST + 0.1)) {
		selected_item = 1;
	} else if (meter_falloff <= (METER_FALLOFF_SLOW + 0.1)) {
		selected_item = 2;
	} else if (meter_falloff <= (METER_FALLOFF_SLOWISH + 0.1)) {
		selected_item = 3;
	} else if (meter_falloff <= (METER_FALLOFF_MODERATE + 0.1)) {
		selected_item = 4;
	} else if (meter_falloff <= (METER_FALLOFF_MEDIUM + 0.1)) {
		selected_item = 5;
	} else if (meter_falloff <= (METER_FALLOFF_FAST + 0.1)) {
		selected_item = 6;
	} else if (meter_falloff <= (METER_FALLOFF_FASTER + 0.1)) {
		selected_item = 7;
	} else if (meter_falloff <= (METER_FALLOFF_FASTEST + 0.1)) {
		selected_item = 8;
	} 
	_dpm_fall_off_dropdown.set_current_item (selected_item);


	long period = Config->get_audio_capture_buffer_seconds ();
	selected_item = 0;

	if (period <= 0.1) {
		selected_item = 0;
	} else if ((period <= 5.1)) {
		selected_item = 1;
	} else if ((period <= 10.1)) {
		selected_item = 2;
	} else if ((period <= 15.1)) {
		selected_item = 3;
	} else if ((period <= 30.1)) {
		selected_item = 4;
	} else if ((period <= 45.1)) {
		selected_item = 5;
	} else if ((period <= 60.1)) {
		selected_item = 6;
	}
	_recording_seconds_dropdown.set_current_item (selected_item);
	
	period = Config->get_audio_playback_buffer_seconds ();
	selected_item = 0;

	if (period <= 0.1) {
		selected_item = 0;
	} else if ((period <= 5.1)) {
		selected_item = 1;
	} else if ((period <= 10.1)) {
		selected_item = 2;
	} else if ((period <= 15.1)) {
		selected_item = 3;
	} else if ((period <= 30.1)) {
		selected_item = 4;
	} else if ((period <= 45.1)) {
		selected_item = 5;
	} else if ((period <= 60.1)) {
		selected_item = 6;
	}
	_playback_seconds_dropdown.set_current_item (selected_item);

	_obey_mmc_commands_button.set_active_state (Config->get_mmc_control () ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	_send_mmc_commands_button.set_active_state (Config->get_send_mmc () ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	_send_mmc_feedback_button.set_active_state (Config->get_midi_feedback () ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	_inbound_mmc_device_spinbutton.set_value (Config->get_mmc_receive_device_id ());
	_outbound_mmc_device_spinbutton.set_value (Config->get_mmc_send_device_id ());
	_limit_undo_history_spinbutton.set_value (Config->get_history_depth ());
	_save_undo_history_spinbutton.set_value (Config->get_saved_history_depth ());
	_copy_imported_files_button.set_active_state (Config->get_only_copy_imported_files () ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	_dc_bias_against_denormals_button.set_active_state (Config->get_denormal_protection () ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
}


void TracksControlPanel::save_general_preferences ()
{
	int selected_item = _waveform_shape_dropdown.get_current_item ();
	switch (selected_item) {
	case 0:
		Config->set_waveform_shape (Traditional);
		break;
	case 1:
		Config->set_waveform_shape (Rectified);
		break;
	default:
		dbg_msg ("TracksControlPanel::general_preferences ():\nUnexpected WaveFormShape !");
		break;
	}

	selected_item = _peak_hold_time_dropdown.get_current_item ();
	switch (selected_item) {
	case 0:
		Config->set_meter_hold (MeterHoldOff);
		break;
	case 1:
		Config->set_meter_hold (MeterHoldShort);
		break;
	case 2:
		Config->set_meter_hold (MeterHoldMedium);
		break;
	case 3:
		Config->set_meter_hold (MeterHoldLong);
		break;
	default:
		dbg_msg ("TracksControlPanel::general_preferences ():\nUnexpected peak hold time!");
		break;
	}

	selected_item = _dpm_fall_off_dropdown.get_current_item ();
	switch (selected_item) {
	case 0:
		Config->set_meter_falloff (METER_FALLOFF_OFF);
		break;
	case 1:
		Config->set_meter_falloff (METER_FALLOFF_SLOWEST);
		break;
	case 2:
		Config->set_meter_falloff (METER_FALLOFF_SLOW);
		break;
	case 3:
		Config->set_meter_falloff (METER_FALLOFF_SLOWISH);
		break;
	case 4:
		Config->set_meter_falloff (METER_FALLOFF_MODERATE);
		break;
	case 5:
		Config->set_meter_falloff (METER_FALLOFF_MEDIUM);
		break;
	case 6:
		Config->set_meter_falloff (METER_FALLOFF_FAST);
		break;
	case 7:
		Config->set_meter_falloff (METER_FALLOFF_FASTER);
		break;
	case 8:
		Config->set_meter_falloff (METER_FALLOFF_FASTEST);
		break;
	default:
		dbg_msg ("TracksControlPanel::general_preferences ():\nUnexpected meter fall off time!");
		break;
	}

	Config->set_mmc_control (_obey_mmc_commands_button.active_state () == Gtkmm2ext::ExplicitActive);
	Config->set_send_mmc (_send_mmc_commands_button.active_state () == Gtkmm2ext::ExplicitActive);
	Config->set_midi_feedback (_send_mmc_feedback_button.active_state () == Gtkmm2ext::ExplicitActive);
	Config->set_only_copy_imported_files (_copy_imported_files_button.active_state () ==  Gtkmm2ext::ExplicitActive);
	Config->set_denormal_protection (_dc_bias_against_denormals_button.active_state () == Gtkmm2ext::ExplicitActive);

	Config->set_mmc_receive_device_id (_inbound_mmc_device_spinbutton.get_value ());
	Config->set_mmc_send_device_id (_outbound_mmc_device_spinbutton.get_value ());
	Config->set_history_depth (_limit_undo_history_spinbutton.get_value ());
	Config->set_saved_history_depth (_save_undo_history_spinbutton.get_value ());
	Config->set_save_history (_save_undo_history_spinbutton.get_value () > 0);
}


void TracksControlPanel::on_engine_dropdown_item_clicked (WavesDropdown*, void*)
{
	if (_ignore_changes) {
		return;
	}

	std::string backend_name = _engine_dropdown.get_text();
    
	if ( EngineStateController::instance()->set_new_backend_as_current (backend_name) )
	{
		_have_control = EngineStateController::instance()->is_setup_required ();
        populate_device_dropdown();
        return;
	}
    
    std::cerr << "\tfailed to set backend [" << backend_name << "]\n";
}

void
TracksControlPanel::on_device_dropdown_item_clicked (WavesDropdown*, void*)
{
	if (_ignore_changes) {
		return;
	}

    std::string device_name = _device_dropdown.get_text ();
    
    std::string message = _("Would you like to switch to ") + device_name;

    MessageDialog msg (message,
                        false,
                        Gtk::MESSAGE_WARNING,
                        Gtk::BUTTONS_YES_NO,
                        true);
        
    msg.set_position (Gtk::WIN_POS_MOUSE);
    msg.set_keep_above(true);

    switch (msg.run()) {
        case RESPONSE_NO:
            // set _ignore_changes flag to ignore changes in combo-box callbacks
            PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
                
            _device_dropdown.set_text (EngineStateController::instance()->get_current_device_name());
            return;
    } 

	device_changed ();
}

void
TracksControlPanel::device_changed ()
{
	if (_ignore_changes) {
		return;
	}
    
    std::string device_name = _device_dropdown.get_text ();
    if (EngineStateController::instance()->set_new_device_as_current(device_name) )
    {
        populate_buffer_size_dropdown();
        populate_sample_rate_dropdown();
        return;
    }
    
    {
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		// restore previous device name in combo box
        _device_dropdown.set_text (EngineStateController::instance()->get_current_device_name() );
	}
    
    MessageDialog( _("Error activating selected device"), PROGRAM_NAME).run();
}

void
TracksControlPanel::on_all_inputs_on_button(WavesButton*)
{
    EngineStateController::instance()->set_state_to_all_inputs(true);
}

void
TracksControlPanel::on_name_tracks_after_driver(WavesButton*)
{
    _yes_button.set_visible(true);
    _no_button.set_visible(true);

    _tracks_naming_rule = NameAfterDriver;
}

void
TracksControlPanel::on_reset_tracks_name_to_default(WavesButton*)
{
    _yes_button.set_visible(true);
    _no_button.set_visible(true);
    
    _tracks_naming_rule = UseDefaultNames;
}

void
TracksControlPanel::on_yes_button(WavesButton*)
{
    Config->set_tracks_auto_naming(_tracks_naming_rule);
    
    _yes_button.set_visible(false);
    _no_button.set_visible(false);
}

void
TracksControlPanel::on_no_button(WavesButton*)
{
    _yes_button.set_visible(false);
    _no_button.set_visible(false);
}

void
TracksControlPanel::on_control_panel_button(WavesButton*)
{
    boost::shared_ptr<AudioBackend> backend = AudioEngine::instance()->current_backend();
    assert(backend);
    
    backend->launch_control_app ();
}

void
TracksControlPanel::on_all_inputs_off_button(WavesButton*)
{
    EngineStateController::instance()->set_state_to_all_inputs(false);
}

void
TracksControlPanel::on_all_outputs_on_button(WavesButton*)
{
    EngineStateController::instance()->set_state_to_all_outputs(true);
}

void
TracksControlPanel::on_all_outputs_off_button(WavesButton*)
{
    EngineStateController::instance()->set_state_to_all_outputs(false);
}

void
TracksControlPanel::on_file_type_dropdown_item_clicked (WavesDropdown*, void*)
{ 
    if (_ignore_changes) {
		return;
	}
    
    std::string s = _file_type_dropdown.get_text();
    ARDOUR::HeaderFormat header_format = string_to_HeaderFormat(s);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    ardour_ui->set_header_format( header_format );
}

void
TracksControlPanel::on_bit_depth_dropdown_item_clicked (WavesDropdown*, void*)
{
    if (_ignore_changes) {
		return;
	}
    
    std::string s = _bit_depth_dropdown.get_text();
    ARDOUR::SampleFormat sample_format = string_to_SampleFormat(s);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    ardour_ui->set_sample_format( sample_format );
}

void
TracksControlPanel::on_frame_rate_item_clicked (WavesDropdown*, void*)
{
    if (_ignore_changes) {
		return;
	}

    std::string s = _frame_rate_dropdown.get_text();
    Timecode::TimecodeFormat timecode_format = string_to_TimecodeFormat(s);
    
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    ardour_ui->set_timecode_format(timecode_format);    
}

void 
TracksControlPanel::on_buffer_size_dropdown_item_clicked (WavesDropdown*, void*)
{
	if (_ignore_changes) {
		return;
	}

    pframes_t new_buffer_size = get_buffer_size();
    if (EngineStateController::instance()->set_new_buffer_size_in_controller(new_buffer_size) )
    {
        show_buffer_duration();
        return;
    }
    
    {
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
        // restore previous buffer size value in combo box
        std::string buffer_size_str = bufsize_as_string (EngineStateController::instance()->get_current_buffer_size() );
        _buffer_size_dropdown.set_text(buffer_size_str);
    }
    
	MessageDialog( _("Buffer size set to the value which is not supported"), PROGRAM_NAME).run();
}

void
TracksControlPanel::on_sample_rate_dropdown_item_clicked (WavesDropdown*, void*)
{
	if (_ignore_changes) {
		return;
	}

    framecnt_t new_sample_rate = get_sample_rate ();
    if (EngineStateController::instance()->set_new_sample_rate_in_controller(new_sample_rate) )
    {
        show_buffer_duration();
        return;
    }
    
	{
		// set _ignore_changes flag to ignore changes in combo-box callbacks
		PBD::Unwinder<uint32_t> protect_ignore_changes (_ignore_changes, _ignore_changes + 1);
		// restore previous buffer size value in combo box
		std::string sample_rate_str = ARDOUR_UI_UTILS::rate_as_string (EngineStateController::instance()->get_current_sample_rate() );
		_sample_rate_dropdown.set_text(sample_rate_str);
	}
	
    MessageDialog( _("Sample rate set to the value which is not supported"), PROGRAM_NAME).run();
}


void
TracksControlPanel::engine_running ()
{
	populate_buffer_size_dropdown();
	populate_sample_rate_dropdown();
    show_buffer_duration ();
}


void
TracksControlPanel::engine_stopped ()
{
}


void
TracksControlPanel::on_a_settings_tab_button_clicked (WavesButton* clicked_button)
{
	bool visible = (&_midi_settings_tab_button == clicked_button);
	_midi_settings_tab.set_visible (visible);
	_midi_settings_tab_button.set_active(visible);

	visible = (&_session_settings_tab_button == clicked_button);
	_session_settings_tab.set_visible (visible);;
	_session_settings_tab_button.set_active(visible);

	visible = (&_audio_settings_tab_button == clicked_button);
	_audio_settings_tab.set_visible (visible);
	_audio_settings_tab_button.set_active(visible);

	visible = (&_general_settings_tab_button == clicked_button);
	_general_settings_tab.set_visible (visible);
	_general_settings_tab_button.set_active(visible);
}


void
TracksControlPanel::on_device_error ()
{
    std::string message = _("Device cannot operate properly. Switched to None device.");
    
    MessageDialog msg (message,
                       false,
                       Gtk::MESSAGE_WARNING,
                       Gtk::BUTTONS_OK,
                       true);
    
    msg.set_position (Gtk::WIN_POS_MOUSE);
    msg.set_keep_above (true);
    msg.run ();
}

void
TracksControlPanel::on_multi_out (WavesButton*)
{
    if (Config->get_output_auto_connect() & AutoConnectPhysical) {
        return;
    }
    
    Config->set_output_auto_connect(AutoConnectPhysical);
}

void
TracksControlPanel::on_stereo_out (WavesButton*)
{
    if (Config->get_output_auto_connect() & AutoConnectMaster) {
        return;
    }
    
    Config->set_output_auto_connect(AutoConnectMaster);
}

void
TracksControlPanel::on_browse_button (WavesButton*)
{
    set_keep_above (false);
    _default_path_name = ARDOUR::choose_folder_dialog(Config->get_default_session_parent_dir(), _("Choose Default Path"));
    set_keep_above (true);    
    
    if (!_default_path_name.empty()) {
        _default_open_path.set_text(_default_path_name);
    } else {
        _default_open_path.set_text(Config->get_default_session_parent_dir());
    }
}

void
TracksControlPanel::save_default_session_path()
{
    if(!_default_path_name.empty())
    {
        Config->set_default_session_parent_dir(_default_path_name);
        Config->save_state();
    }
}

void
TracksControlPanel::save_auto_lock_time()
{
    std::string s = _auto_lock_timer_dropdown.get_text();
    char * pEnd;
    int time = strtol( s.c_str(), &pEnd, 10 );
    ARDOUR_UI::config()->set_auto_lock_timer(time);
}

void
TracksControlPanel::save_auto_save_time()
{
    std::string s = _auto_save_timer_dropdown.get_text();
    char * pEnd;
    int time = strtol( s.c_str(), &pEnd, 10 );
    ARDOUR_UI::config()->set_auto_save_timer(time);
}

void
TracksControlPanel::save_pre_record_buffer()
{
    std::string s = _pre_record_buffer_dropdown.get_text();
    char * pEnd;
    int time = strtol( s.c_str(), &pEnd, 10 );
    ARDOUR_UI::config()->set_pre_record_buffer(time);
}

void TracksControlPanel::update_session_config ()
{
    ARDOUR_UI* ardour_ui = ARDOUR_UI::instance();
    
    if( ardour_ui )
    {
        ARDOUR::Session* session = ardour_ui->the_session();
        
        if( session )
        {
            session->config.set_native_file_header_format( string_to_HeaderFormat(_file_type_dropdown.get_text()));
            session->config.set_native_file_data_format ( string_to_SampleFormat(_bit_depth_dropdown.get_text()));
            session->config.set_timecode_format ( string_to_TimecodeFormat(_frame_rate_dropdown.get_text()));
        }
    }
}

void
TracksControlPanel::update_configs()
{
    // update session config
    update_session_config ();
    
    // update global config
    save_default_session_path ();
    save_auto_lock_time ();
    save_auto_save_time ();
    save_pre_record_buffer ();
	save_general_preferences ();

    // save ARDOUR_UI::config to disk persistently
    ARDOUR_UI::config()->save_state();
}

void
TracksControlPanel::on_ok (WavesButton*)
{
	hide();
	EngineStateController::instance()->push_current_state_to_backend(true);
	response(Gtk::RESPONSE_OK);
    
    update_configs();
}


void
TracksControlPanel::on_cancel (WavesButton*)
{
	hide();
	response(Gtk::RESPONSE_CANCEL);
    
    // restore previous value in combo-boxes
    std::stringstream ss;
    int temp;
    std::string str;
    temp = ARDOUR_UI::config()->get_auto_lock_timer();
    ss.str(std::string(""));
    ss.clear();
    ss << temp;
    str = ss.str() + " Min";
    _auto_lock_timer_dropdown.set_text(str);
    
    temp = ARDOUR_UI::config()->get_auto_save_timer();
    ss.str(std::string(""));
    ss.clear();
    ss << temp;
    str = ss.str() + " Min";
    _auto_save_timer_dropdown.set_text(str);
    
    temp = ARDOUR_UI::config()->get_pre_record_buffer();
    ss.str(std::string(""));
    ss.clear();
    ss << temp;
    str = ss.str() + " Min";
    _pre_record_buffer_dropdown.set_text(str);
    
    _default_open_path.set_text(Config->get_default_session_parent_dir());
	display_general_preferences ();
}


void 
TracksControlPanel::on_apply (WavesButton*)
{
	EngineStateController::instance()->push_current_state_to_backend(true);
	//response(Gtk::RESPONSE_APPLY);
    
    update_configs(); 
}


void TracksControlPanel::on_capture_active_changed(DeviceConnectionControl* capture_control, bool active)
{
    const char * id_name = (char*)capture_control->get_data(DeviceConnectionControl::id_name);
    EngineStateController::instance()->set_physical_audio_input_state(id_name, active);
}


void TracksControlPanel::on_playback_active_changed(DeviceConnectionControl* playback_control, bool active)
{
    const char * id_name = (char*)playback_control->get_data(DeviceConnectionControl::id_name);
    EngineStateController::instance()->set_physical_audio_output_state(id_name, active);
}


void TracksControlPanel::on_midi_capture_active_changed(MidiDeviceConnectionControl* control, bool active)
{
    const char * id_name = (char*)control->get_data(MidiDeviceConnectionControl::capture_id_name);
    EngineStateController::instance()->set_physical_midi_input_state(id_name, active);
}


void TracksControlPanel::on_midi_playback_active_changed(MidiDeviceConnectionControl* control, bool active)
{
    const char * id_name = (char*)control->get_data(MidiDeviceConnectionControl::playback_id_name);
    EngineStateController::instance()->set_physical_midi_output_state(id_name, active);
}


void TracksControlPanel::on_port_registration_update()
{
    populate_input_channels();
    populate_output_channels();
    populate_midi_ports();
}


void
TracksControlPanel::on_buffer_size_update ()
{
    populate_buffer_size_dropdown();
}


void
TracksControlPanel::on_device_list_update (bool current_device_disconnected)
{
    populate_device_dropdown();
    
    if (current_device_disconnected) {
        std::string message = _("Audio device has been removed");
        
        MessageDialog msg (message,
                           false,
                           Gtk::MESSAGE_WARNING,
                           Gtk::BUTTONS_OK,
                           true);
        
        msg.set_position (Gtk::WIN_POS_MOUSE);
        msg.set_keep_above(true);
        msg.run();
        
        return;
    }
}

                                      
void
TracksControlPanel::on_parameter_changed (const std::string& parameter_name)
{
    if (parameter_name == "output-auto-connect") {
        populate_output_mode();
    } else if (parameter_name == "tracks-auto-naming") {
        on_audio_input_configuration_changed ();
    } else if (parameter_name == "default-session-parent-dir") {
        _default_open_path.set_text(Config->get_default_session_parent_dir());
    } else if ((parameter_name == "meter-hold") ||
			   (parameter_name == "meter-falloff") ||
			   (parameter_name == "waveform-shape") ||
			   (parameter_name == "mmc-control") ||
			   (parameter_name == "send-mmc") ||
			   (parameter_name == "midi-feedback") ||
			   (parameter_name == "mmc-receive-device-id") ||
			   (parameter_name == "mmc-send-device-id") ||
			   (parameter_name == "playback-buffer-seconds") ||
			   (parameter_name == "capture-buffer-seconds") ||
			   (parameter_name == "only-copy-imported-files") ||
			   (parameter_name == "denormal-protection") ||
			   (parameter_name == "history-depth") ||
			   (parameter_name == "save-history") ||
			   (parameter_name == "save-history-depth")){
		// This is not that correct.
		// We should update UI when the panel is being shown.
		// We should not react immediately.
		// The use case is: load the values, edit them and then
		// save or cancel.
		display_general_preferences ();
    }
}


void
TracksControlPanel::on_audio_input_configuration_changed ()
{
    std::vector<Gtk::Widget*> capture_controls = _device_capture_list.get_children();
    
    std::vector<Gtk::Widget*>::iterator control_iter = capture_controls.begin();
    
    uint16_t number_count = 1;
    for (; control_iter != capture_controls.end(); ++control_iter) {
        DeviceConnectionControl* control = dynamic_cast<DeviceConnectionControl*> (*control_iter);
        
        if (control) {
            
            const char* id_name = (char*)control->get_data(DeviceConnectionControl::id_name);
            
            if (id_name) {
                bool new_state = EngineStateController::instance()->get_physical_audio_input_state(id_name );
                
                uint16_t number = DeviceConnectionControl::NoNumber;
                std::string track_name ("");
                
                if (new_state) {

                    number = number_count++;
                    
                    if (Config->get_tracks_auto_naming() & UseDefaultNames) {
                        track_name = string_compose ("%1 %2", Session::default_trx_track_name_pattern, number);
                    } else if (Config->get_tracks_auto_naming() & NameAfterDriver) {
                        track_name = control->get_port_name();
                    }
                }
                
                control->set_track_name(track_name);
                control->set_number(number);
                control->set_active(new_state);
            }
        }
    }
}


void
TracksControlPanel::on_audio_output_configuration_changed()
{
    std::vector<Gtk::Widget*> playback_controls = _device_playback_list.get_children();
    
    std::vector<Gtk::Widget*>::iterator control_iter = playback_controls.begin();
    
    uint16_t number_count = 1;
    for (; control_iter != playback_controls.end(); ++control_iter) {
        DeviceConnectionControl* control = dynamic_cast<DeviceConnectionControl*> (*control_iter);
        
        if (control) {
            
            const char * id_name = (char*)control->get_data(DeviceConnectionControl::id_name);
            
            if (id_name != NULL) {
                bool new_state = EngineStateController::instance()->get_physical_audio_output_state(id_name );
                
                uint16_t number = DeviceConnectionControl::NoNumber;
                
                if (new_state) {
                    number = number_count++;
                }
                
                control->set_number(number);
                control->set_active(new_state);
            }
        }
    }

}


void
TracksControlPanel::on_midi_input_configuration_changed ()
{
    std::vector<Gtk::Widget*> midi_controls = _midi_device_list.get_children();
    
    std::vector<Gtk::Widget*>::iterator control_iter = midi_controls.begin();
    
    for (; control_iter != midi_controls.end(); ++control_iter) {
        MidiDeviceConnectionControl* control = dynamic_cast<MidiDeviceConnectionControl*> (*control_iter);
        
        if (control && control->has_capture() ) {
            
            const char* capture_id_name = (char*)control->get_data(MidiDeviceConnectionControl::capture_id_name);
            
            if (capture_id_name != NULL) {
                bool new_state = EngineStateController::instance()->get_physical_midi_input_state(capture_id_name );
                control->set_capture_active(new_state);
            }
        }
    }
}


void
TracksControlPanel::on_midi_output_configuration_changed ()
{
    std::vector<Gtk::Widget*> midi_controls = _midi_device_list.get_children();
    
    std::vector<Gtk::Widget*>::iterator control_iter = midi_controls.begin();
    
    for (; control_iter != midi_controls.end(); ++control_iter) {
        MidiDeviceConnectionControl* control = dynamic_cast<MidiDeviceConnectionControl*> (*control_iter);
        
        if (control && control->has_playback() ) {
            
            const char* playback_id_name = (char*)control->get_data(MidiDeviceConnectionControl::playback_id_name);
            
            if (playback_id_name != NULL) {
                bool new_state = EngineStateController::instance()->get_physical_midi_output_state(playback_id_name);
                control->set_playback_active(new_state);
            }
        }
    }
}


std::string
TracksControlPanel::bufsize_as_string (uint32_t sz)
{
	/* Translators: "samples" is always plural here, so no
	need for plural+singular forms.
	*/
	char  buf[32];
	snprintf (buf, sizeof (buf), _("%u samples"), sz);
	return buf;
}


framecnt_t
TracksControlPanel::get_sample_rate () const
{
    const std::string sample_rate = _sample_rate_dropdown.get_text ();
    
    if ( "44.1 kHz" == sample_rate )
    {
        return 44100;
    } else if ( "48 kHz" == sample_rate )
    {
        return 48000;
    } else if ( "88.2 kHz" == sample_rate )
    {
        return 88200;
    } else if ( "96 kHz" == sample_rate )
    {
        return 96000;
    } else if ( "172.4 kHz" == sample_rate )
    {
        return 172400;
    } else if ( "192 kHz" == sample_rate )
    {
        return 192000;
    }
    
    float r = atof (sample_rate);
	    
    /* the std::string may have been translated with an abbreviation for
	* thousands, so use a crude heuristic to fix this.
	*/
	if (r < 1000.0) {
		r *= 1000.0;
	}
	return r;
}

pframes_t TracksControlPanel::get_buffer_size() const
{
    std::string bs_text = _buffer_size_dropdown.get_text ();
    pframes_t samples = atoi (bs_text); /* will ignore trailing text */
	return samples;
}

void
TracksControlPanel::show_buffer_duration ()
{
	 float latency = (get_buffer_size() * 1000.0) / get_sample_rate();

	 char buf[256];
	 snprintf (buf, sizeof (buf), _("INPUT LATENCY: %.1f MS      OUTPUT LATENCY: %.1f MS      TOTAL LATENCY: %.1f MS"), 
			   latency, latency, 2*latency);
	 _latency_label.set_text (buf);
}
