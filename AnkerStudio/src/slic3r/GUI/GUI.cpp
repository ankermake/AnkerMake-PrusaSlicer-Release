#include "GUI.hpp"
#include "GUI_App.hpp"
#include "format.hpp"
#include "I18N.hpp"

#include "libslic3r/LocalesUtils.hpp"

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>

#if __APPLE__
#import <IOKit/pwr_mgt/IOPMLib.h>
#elif _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "boost/nowide/convert.hpp"
#endif

#include "AboutDialog.hpp"
#include "MsgDialog.hpp"
#include "format.hpp"

#include "libslic3r/Print.hpp"

namespace Slic3r {

class AppConfig;

namespace GUI {

#if __APPLE__
IOPMAssertionID assertionID;
#endif

void disable_screensaver()
{
    #if __APPLE__
    CFStringRef reasonForActivity = CFSTR("Slic3r");
    [[maybe_unused]]IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
        kIOPMAssertionLevelOn, reasonForActivity, &assertionID);
    // ignore result: success == kIOReturnSuccess
    #elif _WIN32
    SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
    #endif
}

void enable_screensaver()
{
    #if __APPLE__
    IOPMAssertionRelease(assertionID);
    #elif _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
    #endif
}

bool debugged()
{
    #ifdef _WIN32
    return IsDebuggerPresent() == TRUE;
	#else
	return false;
    #endif /* _WIN32 */
}

void break_to_debugger()
{
    #ifdef _WIN32
    if (IsDebuggerPresent())
        DebugBreak();
    #endif /* _WIN32 */
}

const std::string& shortkey_ctrl_prefix()
{
	static const std::string str =
#ifdef __APPLE__
		"⌘"
#else
		"Ctrl+"
#endif
		;
	return str;
}

const std::string& shortkey_alt_prefix()
{
	static const std::string str =
#ifdef __APPLE__
		"⌥"
#else
		"Alt+"
#endif
		;
	return str;
}

// opt_index = 0, by the reason of zero-index in ConfigOptionVector by default (in case only one element)
void change_opt_value(DynamicPrintConfig& config, const t_config_option_key& opt_key, const boost::any& value, int opt_index /*= 0*/)
{
	try{

        if (config.def()->get(opt_key)->type == coBools && config.def()->get(opt_key)->nullable) {
            ConfigOptionBoolsNullable* vec_new = new ConfigOptionBoolsNullable{ boost::any_cast<unsigned char>(value) };
            config.option<ConfigOptionBoolsNullable>(opt_key)->set_at(vec_new, opt_index, 0);
            return;
        }

        const ConfigOptionDef *opt_def = config.def()->get(opt_key);
		switch (opt_def->type) {
		case coFloatOrPercent:{
			std::string str = boost::any_cast<std::string>(value);
			bool percent = false;
			if (str.back() == '%') {
				str.pop_back();
				percent = true;
			}
            double val = std::stod(str); // locale-dependent (on purpose - the input is the actual content of the field)
			config.set_key_value(opt_key, new ConfigOptionFloatOrPercent(val, percent));
			break;}
		case coPercent:
			config.set_key_value(opt_key, new ConfigOptionPercent(boost::any_cast<double>(value)));
			break;
		case coFloat:{
			//double& val = config.opt_float(opt_key); val = boost::any_cast<double>(value);
			config.set_key_value(opt_key, new ConfigOptionFloat(boost::any_cast<double>(value)));
			break;
		}
		case coFloatsOrPercents:{
			std::string str = boost::any_cast<std::string>(value);
			bool percent = false;
			if (str.back() == '%') {
				str.pop_back();
				percent = true;
			}
            double val = std::stod(str); // locale-dependent (on purpose - the input is the actual content of the field)
			ConfigOptionFloatsOrPercents* vec_new = new ConfigOptionFloatsOrPercents({ {val, percent} });
			config.option<ConfigOptionFloatsOrPercents>(opt_key)->set_at(vec_new, opt_index, opt_index);
			break;
		}
		case coPercents:{
			ConfigOptionPercents* vec_new = new ConfigOptionPercents{ boost::any_cast<double>(value) };
			config.option<ConfigOptionPercents>(opt_key)->set_at(vec_new, opt_index, opt_index);
			break;
		}
		case coFloats:{
			ConfigOptionFloats* vec_new = new ConfigOptionFloats{ boost::any_cast<double>(value) };
			config.option<ConfigOptionFloats>(opt_key)->set_at(vec_new, opt_index, opt_index);
 			break;
		}
		case coString:
			config.set_key_value(opt_key, new ConfigOptionString(boost::any_cast<std::string>(value)));
			break;
		case coStrings:{
			if (opt_key == "compatible_prints" || opt_key == "compatible_printers" || opt_key == "gcode_substitutions") {
				config.option<ConfigOptionStrings>(opt_key)->values =
					boost::any_cast<std::vector<std::string>>(value);
			}
			else if (config.def()->get(opt_key)->gui_flags.compare("serialized") == 0) {
				std::string str = boost::any_cast<std::string>(value);
                std::vector<std::string> values {};
                if (!str.empty()) {
				    if (str.back() == ';') str.pop_back();
				    // Split a string to multiple strings by a semi - colon.This is the old way of storing multi - string values.
				    // Currently used for the post_process config value only.
				    boost::split(values, str, boost::is_any_of(";"));
				    if (values.size() == 1 && values[0] == "")
					    values.resize(0);
                }
				config.option<ConfigOptionStrings>(opt_key)->values = values;
			}
			else{
				ConfigOptionStrings* vec_new = new ConfigOptionStrings{ boost::any_cast<std::string>(value) };
				config.option<ConfigOptionStrings>(opt_key)->set_at(vec_new, opt_index, 0);
			}
			}
			break;
		case coBool:
			config.set_key_value(opt_key, new ConfigOptionBool(boost::any_cast<bool>(value)));
			break;
		case coBools:{
			ConfigOptionBools* vec_new = new ConfigOptionBools{ boost::any_cast<unsigned char>(value) != 0 };
			config.option<ConfigOptionBools>(opt_key)->set_at(vec_new, opt_index, 0);
			break;}
		case coInt:
			config.set_key_value(opt_key, new ConfigOptionInt(boost::any_cast<int>(value)));
			break;
		case coInts:{
			ConfigOptionInts* vec_new = new ConfigOptionInts{ boost::any_cast<int>(value) };
			config.option<ConfigOptionInts>(opt_key)->set_at(vec_new, opt_index, 0);
			}
			break;
		case coEnum:{
			auto *opt = opt_def->default_value.get()->clone();
			opt->setInt(boost::any_cast<int>(value));
			config.set_key_value(opt_key, opt);
			}
			break;
		case coPoints:{
			if (opt_key == "bed_shape" || opt_key == "thumbnails") {
				config.option<ConfigOptionPoints>(opt_key)->values = boost::any_cast<std::vector<Vec2d>>(value);
				break;
			}
			ConfigOptionPoints* vec_new = new ConfigOptionPoints{ boost::any_cast<Vec2d>(value) };
			config.option<ConfigOptionPoints>(opt_key)->set_at(vec_new, opt_index, 0);
			}
			break;
		case coNone:
			break;
		default:
			break;
		}
	}
	catch (const std::exception &e)
	{
		wxLogError(format_wxstr("Internal error when changing value for %1%: %2%", opt_key, e.what()));
	}
}

void show_error(wxWindow* parent, const wxString& message, bool monospaced_font)
{
	ErrorDialog msg(parent, message, monospaced_font);
	msg.ShowModal();
}

void show_error(wxWindow* parent, const char* message, bool monospaced_font)
{
	assert(message);
	show_error(parent, wxString::FromUTF8(message), monospaced_font);
}

void show_error_id(int id, const std::string& message)
{
	auto *parent = id != 0 ? wxWindow::FindWindowById(id) : nullptr;
	show_error(parent, message);
}

void show_info(wxWindow* parent, const wxString& message, const wxString& title, const bool showingTitle)
{
	//wxMessageDialog msg_wingow(parent, message, wxString(SLIC3R_APP_NAME " - ") + (title.empty() ? _L("Notice") : title), wxOK | wxICON_INFORMATION);
	if (showingTitle) {
		MessageDialog msg_wingow(parent, message, wxString(SLIC3R_APP_NAME " - ") + (title.empty() ? _L("Notice") : title), wxOK | wxICON_INFORMATION);
		msg_wingow.ShowModal();
	}
	else {
		MessageDialog msg_wingow(parent, message, wxString(SLIC3R_APP_NAME), wxOK | wxICON_INFORMATION);
		msg_wingow.ShowModal();
	}
}

void show_info(wxWindow* parent, const char* message, const char* title)
{
	assert(message);
	show_info(parent, wxString::FromUTF8(message), title ? wxString::FromUTF8(title) : wxString());
}

void warning_catcher(wxWindow* parent, const wxString& message)
{
	//wxMessageDialog msg(parent, message, _L("Warning"), wxOK | wxICON_WARNING);
	MessageDialog msg(parent, message, _L("Warning"), wxOK | wxICON_WARNING);
	msg.ShowModal();
}

static wxString bold(const wxString& str)
{
	return wxString::Format("<b>%s</b>", str);
};

static wxString bold_string(const wxString& str) 
{ 
	return wxString::Format("<b>\"%s\"</b>", str); 
};

static void add_config_substitutions(const ConfigSubstitutions& conf_substitutions, wxString& changes)
{
	changes += "<table>";
	for (const ConfigSubstitution& conf_substitution : conf_substitutions) {
		wxString new_val;
		const ConfigOptionDef* def = conf_substitution.opt_def;
		if (!def)
			continue;
		switch (def->type) {
		case coEnum:
		{
			auto opt = def->enum_def->enum_to_index(conf_substitution.new_value->getInt());
			new_val = opt.has_value() ?
				wxString("\"") + def->enum_def->value(*opt) + "\"" + " (" +
					_(from_u8(def->enum_def->label(*opt))) + ")" :
				_L("Undefined");
			break;
		}
		case coBool:
			new_val = conf_substitution.new_value->getBool() ? "true" : "false";
			break;
		case coBools:
			if (conf_substitution.new_value->nullable())
				for (const char v : static_cast<const ConfigOptionBoolsNullable*>(conf_substitution.new_value.get())->values)
					new_val += std::string(v == ConfigOptionBoolsNullable::nil_value() ? "nil" : v ? "true" : "false") + ", ";
			else
				for (const char v : static_cast<const ConfigOptionBools*>(conf_substitution.new_value.get())->values)
					new_val += std::string(v ? "true" : "false") + ", ";
			if (! new_val.empty())
				new_val.erase(new_val.begin() + new_val.size() - 2, new_val.end());
			break;
		default:
			assert(false);
		}

		changes += format_wxstr("<tr><td><b>\"%1%\" (%2%)</b></td><td>: ", def->opt_key, _(def->label)) +
				   format_wxstr(_L("%1% was substituted with %2%"), bold_string(conf_substitution.old_value), bold(new_val)) + 
				   "</td></tr>";
	}
	changes += "</table>";
}

static wxString substitution_message(const wxString& changes)
{
	return
		_L("Most likely the configuration was produced by a newer version of eufyMake Studio or by some eufyMake Studio fork.") + " " +
		_L("The following values were substituted:") + "\n" + changes + "\n\n" +
		_L("Review the substitutions and adjust them if needed.");
}

void show_substitutions_info(const PresetsConfigSubstitutions& presets_config_substitutions) 
{
	wxString changes;

	auto preset_type_name = [](Preset::Type type) {
		switch (type) {
			case Preset::TYPE_PRINT:			return _L("Print settings");
			case Preset::TYPE_SLA_PRINT:		return _L("SLA print settings");
			case Preset::TYPE_FILAMENT:			return _L("Filament");
			case Preset::TYPE_SLA_MATERIAL:		return _L("SLA material");
			case Preset::TYPE_PRINTER: 			return _L("Printer");
			case Preset::TYPE_PHYSICAL_PRINTER:	return _L("Physical Printer");
			default: assert(false);				return wxString();
		}
	};

	for (const PresetConfigSubstitutions& substitution : presets_config_substitutions) {
		changes += "\n\n" + format_wxstr("%1% : %2%", preset_type_name(substitution.preset_type), bold_string(substitution.preset_name));
		if (!substitution.preset_file.empty())
			changes += format_wxstr(" (%1%)", substitution.preset_file);

		add_config_substitutions(substitution.substitutions, changes);
	}

	InfoDialog msg(nullptr, _L("Configuration bundle was loaded, however some configuration values were not recognized."), substitution_message(changes), true);
	msg.ShowModal();
}

void show_substitutions_info(const ConfigSubstitutions& config_substitutions, const std::string& filename)
{
	wxString changes = "\n";
	add_config_substitutions(config_substitutions, changes);

	InfoDialog msg(nullptr, 
		format_wxstr(_L("Configuration file \"%1%\" was loaded, however some configuration values were not recognized."), from_u8(filename)), 
		substitution_message(changes), true);
	msg.ShowModal();
}

void create_combochecklist(wxComboCtrl* comboCtrl, const std::string& text, const std::string& items)
{
    if (comboCtrl == nullptr)
        return;
    wxGetApp().UpdateDarkUI(comboCtrl);

    wxCheckListBoxComboPopup* popup = new wxCheckListBoxComboPopup;
    if (popup != nullptr) {
		// FIXME If the following line is removed, the combo box popup list will not react to mouse clicks.
        //  On the other side, with this line the combo box popup cannot be closed by clicking on the combo button on Windows 10.
        comboCtrl->UseAltPopupWindow();

		int max_width = 0;

		// the following line messes up the popup size the first time it is shown on wxWidgets 3.1.3
//		comboCtrl->EnablePopupAnimation(false);
#ifdef _WIN32
		popup->SetFont(comboCtrl->GetFont());
#endif // _WIN32
		comboCtrl->SetPopupControl(popup);
		wxString title = from_u8(text);
		max_width = std::max(max_width, 60 + comboCtrl->GetTextExtent(title).x);
		popup->SetStringValue(title);
		popup->Bind(wxEVT_CHECKLISTBOX, [popup](wxCommandEvent& evt) { popup->OnCheckListBox(evt); });
		popup->Bind(wxEVT_LISTBOX, [popup](wxCommandEvent& evt) { popup->OnListBoxSelection(evt); });
        popup->Bind(wxEVT_KEY_DOWN, [popup](wxKeyEvent& evt) { popup->OnKeyEvent(evt); });
        popup->Bind(wxEVT_KEY_UP, [popup](wxKeyEvent& evt) { popup->OnKeyEvent(evt); });

        std::vector<std::string> items_str;
        boost::split(items_str, items, boost::is_any_of("|"), boost::token_compress_off);

		// each item must be composed by 2 parts
		assert(items_str.size() %2 == 0);

		for (size_t i = 0; i < items_str.size(); i += 2) {
			wxString label = from_u8(items_str[i]);
			max_width = std::max(max_width, 60 + popup->GetTextExtent(label).x);
			popup->Append(label);
			popup->Check(i / 2, items_str[i + 1] == "1");
		}

		comboCtrl->SetMinClientSize(wxSize(max_width, -1));
        wxGetApp().UpdateDarkUI(popup);
	}
}

unsigned int combochecklist_get_flags(wxComboCtrl* comboCtrl)
{
	unsigned int flags = 0;

	wxCheckListBoxComboPopup* popup = wxDynamicCast(comboCtrl->GetPopupControl(), wxCheckListBoxComboPopup);
	if (popup != nullptr) {
		for (unsigned int i = 0; i < popup->GetCount(); ++i) {
			if (popup->IsChecked(i))
				flags |= 1 << i;
		}
	}

	return flags;
}

void combochecklist_set_flags(wxComboCtrl* comboCtrl, unsigned int flags)
{
	wxCheckListBoxComboPopup* popup = wxDynamicCast(comboCtrl->GetPopupControl(), wxCheckListBoxComboPopup);
	if (popup != nullptr) {
		for (unsigned int i = 0; i < popup->GetCount(); ++i) {
			popup->Check(i, (flags & (1 << i)) != 0);
		}
	}
}

AppConfig* get_app_config()
{
    return wxGetApp().app_config;
}

wxString from_u8(const std::string &str)
{
	return wxString::FromUTF8(str.c_str());
}

std::string into_u8(const wxString &str)
{
	auto buffer_utf8 = str.utf8_str();
	return std::string(buffer_utf8.data());
}

wxString from_path(const boost::filesystem::path &path)
{
#ifdef _WIN32
	return wxString(path.string<std::wstring>());
#else
	return from_u8(path.string<std::string>());
#endif
}

boost::filesystem::path into_path(const wxString &str)
{
	return boost::filesystem::path(str.wx_str());
}

void about()
{
    AboutDialog dlg;
    dlg.ShowModal();
}

void desktop_open_datadir_folder()
{
	boost::filesystem::path path(data_dir());
	desktop_open_folder(std::move(path));
}

void desktop_open_folder(const boost::filesystem::path& path)
{
	if (!boost::filesystem::is_directory(path)) 
		return;

	// Execute command to open a file explorer, platform dependent.
#ifdef _WIN32
	const wxString widepath = path.wstring();
	const wchar_t* argv[] = { L"explorer", widepath.GetData(), nullptr };
	::wxExecute(const_cast<wchar_t**>(argv), wxEXEC_ASYNC, nullptr);
#elif __APPLE__
	const char* argv[] = { "open", path.string().c_str(), nullptr };
	::wxExecute(const_cast<char**>(argv), wxEXEC_ASYNC, nullptr);
#else
	const char* argv[] = { "xdg-open", path.string().c_str(), nullptr };
	desktop_execute(argv);
#endif
}

#ifdef __linux__
namespace {
wxExecuteEnv get_appimage_exec_env()
{
	// If we're running in an AppImage container, we need to remove AppImage's env vars,
	// because they may mess up the environment expected by the file manager.
	// Mostly this is about LD_LIBRARY_PATH, but we remove a few more too for good measure.
	wxEnvVariableHashMap env_vars;
	wxGetEnvMap(&env_vars);

	env_vars.erase("APPIMAGE");
	env_vars.erase("APPDIR");
	env_vars.erase("LD_LIBRARY_PATH");
	env_vars.erase("LD_PRELOAD");
	env_vars.erase("UNION_PRELOAD");

	wxExecuteEnv exec_env;
	exec_env.env = std::move(env_vars);

	wxString owd;
	if (wxGetEnv("OWD", &owd)) {
		// This is the original work directory from which the AppImage image was run,
		// set it as CWD for the child process:
		exec_env.cwd = std::move(owd);
	}
	return exec_env;
}
} // namespace
void desktop_execute(const char* argv[])
{
	// Check if we're running in an AppImage container, if so, we need to remove AppImage's env vars,
	// because they may mess up the environment expected by the file manager.
	// Mostly this is about LD_LIBRARY_PATH, but we remove a few more too for good measure.
	if (wxGetEnv("APPIMAGE", nullptr)) {
		// We're running from AppImage
		wxExecuteEnv exec_env = get_appimage_exec_env();
		::wxExecute(const_cast<char**>(argv), wxEXEC_ASYNC, nullptr, &exec_env);
	}
	else {
		// Looks like we're NOT running from AppImage, we'll make no changes to the environment.
		::wxExecute(const_cast<char**>(argv), wxEXEC_ASYNC, nullptr, nullptr);
	}
}
void desktop_execute_get_result(wxString command, wxArrayString& output)
{
	output.Clear();
   //Check if we're running in an AppImage container, if so, we need to remove AppImage's env vars,
   // because they may mess up the environment expected by the file manager.
   // Mostly this is about LD_LIBRARY_PATH, but we remove a few more too for good measure.
	if (wxGetEnv("APPIMAGE", nullptr)) {
		// We're running from AppImage
		wxExecuteEnv exec_env = get_appimage_exec_env();
		::wxExecute(command, output, wxEXEC_SYNC | wxEXEC_NOEVENTS, &exec_env);
	} else {
		// Looks like we're NOT running from AppImage, we'll make no changes to the environment.
		::wxExecute(command, output, wxEXEC_SYNC | wxEXEC_NOEVENTS);
	}
}
#endif // __linux__

#ifdef _WIN32
bool create_process(const boost::filesystem::path& path, const std::wstring& cmd_opt, std::string& error_msg)
{
	// find updater exe
	if (boost::filesystem::exists(path)) {
		// Using quoted string as mentioned in CreateProcessW docs.
		std::wstring wcmd = L"\"" + path.wstring() + L"\"";
		if (!cmd_opt.empty())
			wcmd += L" " + cmd_opt;

		// additional information
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		if (CreateProcessW(NULL,   // the path
			wcmd.data(),    // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		)) {
			// Close process and thread handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return true;
		}
		else
			error_msg = "CreateProcessW failed to create process " + boost::nowide::narrow(path.wstring());
	}
	else
		error_msg = "Executable doesn't exists. Path: " + boost::nowide::narrow(path.wstring());
	return false;
}
#endif //_WIN32

} } // namespaces GUI / Slic3r
