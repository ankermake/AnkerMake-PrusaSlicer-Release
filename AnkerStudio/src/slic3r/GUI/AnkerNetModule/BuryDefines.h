#ifndef slic3r_BuryDefines_hpp_
#define slic3r_BuryDefines_hpp_
#include "AnkerNetDefines.h"


#define e_config_wizard_event "config_wizard_event"
#define c_config_wizard_entrance "config_wizard_entrance"
#define c_config_wizard_printer_added "config_wizard_printer_added"

#define e_online_preset_event "online_preset_update"
#define c_online_preset_status "update_status"
#define c_online_preset_status_info "update_status_info"

#define e_http_error_event "http_error"
#define c_http_error_code "error_code"
#define c_http_error_url "error_url"

//Property Event Name && Param Name
#define e_property  "property"
#define c_os_type "os_type"
#define c_slice_version "slice_version"
#define c_timestamp  "timestamp"

//slice_version_info
#define e_slice_version_info  "slice_version_info"
#define c_slice_version "slice_version"

#define e_crash_info "crash_info"
#define c_crash_flag "crash_flag"

#define e_webview_event "webview"
#define c_login_type "login_type"
#define c_retry_count "retry_count"
#define c_login_status "login_status"
#define c_status_info "status_info"
#define c_browser_version "browser_version"
#define c_load_url "load_url"

#define e_print_start "print_start"
#define c_print_type "print_type"
#define c_result "result"

//os_type
#define c_mac  "MacOS"
#define c_windows "Windows"

#define e_print_stop_reason "print_stop_reason"
#define c_reason "reason"

#define e_print_ratings  "print_ratings"
#define c_ratings "ratings"
#define c_task_id "task_id"

//start soft 2024/2/29
#define e_start_soft "start_soft"
#define c_ss_time "start_time"
#define c_ss_status   "start_status"
#define c_ss_error_code "error_code"
#define c_ss_error_msg "error_msg"

#define e_exit_soft "exit_soft"
#define c_es_error_code "error_code"
#define c_es_error_msg "error_msg"
#define c_exit_startup_duration "startup_duration"

#define e_hanlde_model "handle_model"
#define c_hm_type "handle_type"
#define c_hm_file_name "file_name"
#define c_hm_file_size "file_size"
#define c_hm_error_code "error_code"
#define c_hm_error_msg "error_msg"

#define e_slice_model "slice_model"
#define c_sm_file_name "file_name"
#define c_sm_file_size "file_size"
#define c_sm_time "time"
#define c_sm_status "status"
#define c_sm_error_code "error_code"
#define c_sm_error_msg "error_msg"
#define c_sm_unique_id "unique_id"

#define e_ag_handle "ag_handle"
#define c_ag_file_name "file_name"
#define c_ag_file_size "file_size"
#define c_ag_handle_type "handle_type"
#define c_ag_handle_duration "handle_duration"
#define c_ag_error_code "error_code"
#define c_ag_error_msg "error_msg"

#define e_start_print "start_print"
#define c_sp_error_code "error_code"
#define c_sp_error_msg "error_msg"

#define e_print_file_transport "print_file_transport"
#define c_pft_file_name "file_name"
#define c_pft_file_size "file_size"
#define c_pft_device_sn "device_sn"
#define c_pft_error_code "error_code"
#define c_pft_error_msg "error_msg"

#define e_play_video "play_video"
#define c_pv_device_sn "device_sn"
#define c_pv_error_code "error_code"
#define c_pv_error_msg "error_msg"

#define e_read_printer_file "read_printer_file"
#define c_rpf_disk_type "disk_type"
#define c_rpf_device_sn "device_sn"
#define c_rpf_error_code "error_code"
#define c_rpf_error_msg "error_msg"

#define e_crash_report "crash_report"
#define c_cr_error_code "error_code"
#define c_cr_error_msg "error_msg"

#endif
