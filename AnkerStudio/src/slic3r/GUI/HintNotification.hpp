#ifndef slic3r_GUI_HintNotification_hpp_
#define slic3r_GUI_HintNotification_hpp_

#include "NotificationManager.hpp"

namespace Slic3r {
namespace GUI {

// Database of hints updatable
struct HintData
{
	std::string        id_string;
	std::string        title;
	std::string        text;					// main desc text
	int                weight{-1};
	bool               was_displayed;
	std::string        hypertext;
	std::string		   follow_text;
	std::string		   disabled_tags;
	std::string        enabled_tags;
	bool               runtime_disable{false};  // if true - hyperlink will check before every click if not in disabled mode
	std::string        documentation_link_text; // eg:"read more>>"
	std::string        documentation_link;		// "link for documentation_link_text"
	std::string        image;					// image path
	std::string        image_url;
	std::string        language;
	std::function<void(void)> callback{ nullptr };
};

enum class HintDataNavigation {
	Curr,
	Prev,
	Next,
	Random,
};

class HintDatabase
{
public:
    static HintDatabase& get_instance()
    {
        static HintDatabase    instance; // Guaranteed to be destroyed.
                                         // Instantiated on first use.
        return instance;
    }
private:
	HintDatabase() : m_hint_id(0) {}
public:
	~HintDatabase();
	HintDatabase(HintDatabase const&) = delete;
	void operator=(HintDatabase const&) = delete;

	// return true if HintData filled;
	HintData* get_hint(bool new_hint = true);
	HintData* get_hint(HintDataNavigation nav);
	size_t	  get_index() { return m_hint_id; }
	size_t    get_valid_hint_count();
	// resets m_initiailized to false and writes used if was initialized
	// used when reloading in runtime - like change language
	void    uninit();
	void	reinit();
private:
	void	init();
	void    downLoading();
	bool    download_hints();
	void    download_hints_finish();
	void    download_hints_image(const std::string& img_url, std::string& img);
	bool	load_hints_from_file(const boost::filesystem::path& path);
	void	save_server_hints(const boost::filesystem::path& path);
	bool    is_used(const std::string& id);
	void    set_used(const std::string& id);
	void    clear_used();
	void	init_random_hint_id();
	void    check_hint_imgage();
	// Returns position in m_loaded_hints with next hint chosed randomly with weights
	size_t  get_next_hint_id();
	size_t	get_prev_hint_id();
	size_t  get_next();
	size_t  get_random_next();
	std::string get_curr_language();

	size_t						m_hint_id { 0 };
	bool						m_sorted_hints { false };
	bool                        m_used_ids_loaded { false };
	std::atomic_bool		    m_initialized { false };
	std::vector<HintData>       m_loaded_hints;
	std::vector<std::string>    m_used_ids;

	std::thread m_thread;
	std::mutex m_mutex;
	std::atomic<bool> m_abort_download { false };
};
// Notification class - shows current Hint ("Did you know") 
class NotificationManager::HintNotification : public NotificationManager::PopNotification
{
public:
	HintNotification(const NotificationData& n, NotificationIDProvider& id_provider, wxEvtHandler* evt_handler, bool new_hint)
		: PopNotification(n, id_provider, evt_handler)
	{
		retrieve_data(new_hint);
	}
	virtual void	init() override;
	void			open_next() { retrieve_data(); }
protected:
	virtual void	set_next_window_size(ImGuiWrapper& imgui) override;
	virtual void	count_spaces() override;
	virtual void	count_lines() override;
	virtual bool	on_text_click() override;
	virtual void	render_text(ImGuiWrapper& imgui,
								const float win_size_x, const float win_size_y,
								const float win_pos_x, const float win_pos_y) override;
	virtual void	render_close_button(ImGuiWrapper& imgui,
								const float win_size_x, const float win_size_y,
								const float win_pos_x, const float win_pos_y) override;
	virtual void	render_minimize_button(ImGuiWrapper& imgui,
								const float win_pos_x, const float win_pos_y) override {}
	void			render_preferences_button(ImGuiWrapper& imgui,
								const float win_pos_x, const float win_pos_y);
	void			render_right_arrow_button(ImGuiWrapper& imgui,
								const float win_size_x, const float win_size_y,
								const float win_pos_x, const float win_pos_y);
	void			render_documentation_button(ImGuiWrapper& imgui,
								const float win_size_x, const float win_size_y,
								const float win_pos_x, const float win_pos_y);
	void			render_logo(ImGuiWrapper& imgui,
								const float win_size_x, const float win_size_y,
								const float win_pos_x, const float win_pos_y);
	// recursion counter -1 tells to retrieve same hint as last time
	void			retrieve_data(bool new_hint = true);
	void			open_documentation();

	bool						m_has_hint_data { false };
	std::function<void(void)>	m_hypertext_callback;
	std::string					m_disabled_tags;
	std::string					m_enabled_tags;
	bool                        m_runtime_disable;
	std::string                 m_documentation_link;
	float						m_close_b_y { 0 };
	float						m_close_b_w { 0 };
	// hover of buttons
	long                      m_docu_hover_time { 0 };
	long                      m_prefe_hover_time{ 0 };
};

} //namespace Slic3r 
} //namespace GUI 

#endif //slic3r_GUI_HintNotification_hpp_