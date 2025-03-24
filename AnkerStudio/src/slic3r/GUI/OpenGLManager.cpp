#include "libslic3r/libslic3r.h"
#include "OpenGLManager.hpp"

#include "GUI.hpp"
#if ENABLE_GL_CORE_PROFILE
#include "GUI_Init.hpp"
#endif // ENABLE_GL_CORE_PROFILE
#include "I18N.hpp"
#include "3DScene.hpp"
#include "format.hpp"

#include "libslic3r/Platform.hpp"
#include <GL/glew.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/log/trivial.hpp>

#include <wx/glcanvas.h>
#include <wx/msgdlg.h>

#ifdef __APPLE__
// Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
#include <wx/platinfo.h>

#include "../Utils/MacDarkMode.hpp"
#endif // __APPLE__

namespace Slic3r {
namespace GUI {

// A safe wrapper around glGetString to report a "N/A" string in case glGetString returns nullptr.
std::string gl_get_string_safe(GLenum param, const std::string& default_value)
{
    const char* value = (const char*)::glGetString(param);
    glcheck();
    return std::string((value != nullptr) ? value : default_value);
}

const std::string& OpenGLManager::GLInfo::get_version_string() const
{
    if (!m_detected)
        detect();

    return m_version_string;
}

const std::string& OpenGLManager::GLInfo::get_glsl_version_string() const
{
    if (!m_detected)
        detect();

    return m_glsl_version_string;
}

const std::string& OpenGLManager::GLInfo::get_vendor() const
{
    if (!m_detected)
        detect();

    return m_vendor;
}

const std::string& OpenGLManager::GLInfo::get_renderer() const
{
    if (!m_detected)
        detect();

    return m_renderer;
}

bool OpenGLManager::GLInfo::is_mesa() const
{
    return m_version_is_mesa;
}

int OpenGLManager::GLInfo::get_max_tex_size() const
{
    if (!m_detected)
        detect();

    // clamp to avoid the texture generation become too slow and use too much GPU memory
#ifdef __APPLE__
    // and use smaller texture for non retina systems
    return (Slic3r::GUI::mac_max_scaling_factor() > 1.0) ? std::min(m_max_tex_size, 8192) : std::min(m_max_tex_size / 2, 4096);
#else
    // and use smaller texture for older OpenGL versions
    return is_version_greater_or_equal_to(3, 0) ? std::min(m_max_tex_size, 8192) : std::min(m_max_tex_size / 2, 4096);
#endif // __APPLE__
}

float OpenGLManager::GLInfo::get_max_anisotropy() const
{
    if (!m_detected)
        detect();

    return m_max_anisotropy;
}

static Semver parse_version_string(const std::string& version);

void OpenGLManager::GLInfo::detect() const
{
    *const_cast<std::string*>(&m_version_string) = gl_get_string_safe(GL_VERSION, "N/A");
    *const_cast<std::string*>(&m_glsl_version_string) = gl_get_string_safe(GL_SHADING_LANGUAGE_VERSION, "N/A");
    *const_cast<std::string*>(&m_vendor)       = gl_get_string_safe(GL_VENDOR, "N/A");
    *const_cast<std::string*>(&m_renderer)     = gl_get_string_safe(GL_RENDERER, "N/A");

    *const_cast<Semver*>(&m_version)       = parse_version_string(m_version_string);
    *const_cast<bool*>(&m_version_is_mesa) = boost::icontains(m_version_string, "mesa");
    *const_cast<Semver*>(&m_glsl_version)  = parse_version_string(m_glsl_version_string);
    
    int* max_tex_size = const_cast<int*>(&m_max_tex_size);
    glsafe(::glGetIntegerv(GL_MAX_TEXTURE_SIZE, max_tex_size));

    *max_tex_size /= 2;

    if (Slic3r::total_physical_memory() / (1024 * 1024 * 1024) < 6)
        *max_tex_size /= 2;

    if (GLEW_EXT_texture_filter_anisotropic) {
        float* max_anisotropy = const_cast<float*>(&m_max_anisotropy);
        glsafe(::glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropy));
    }
    *const_cast<bool*>(&m_detected) = true;
}

static Semver parse_version_string(const std::string& version)
{
    if (version == "N/A")
        return Semver::invalid();

    std::vector<std::string> tokens;
    boost::split(tokens, version, boost::is_any_of(" "), boost::token_compress_on);

    if (tokens.empty())
        return Semver::invalid();

#if ENABLE_OPENGL_ES
    const std::string version_container = (tokens.size() > 1 && boost::istarts_with(tokens[1], "ES")) ? tokens[2] : tokens[0];
#endif // ENABLE_OPENGL_ES

    std::vector<std::string> numbers;
#if ENABLE_OPENGL_ES
    boost::split(numbers, version_container, boost::is_any_of("."), boost::token_compress_on);
#else
    boost::split(numbers, tokens[0], boost::is_any_of("."), boost::token_compress_on);
#endif // ENABLE_OPENGL_ES

    unsigned int gl_major = 0;
    unsigned int gl_minor = 0;

    if (numbers.size() > 0)
        gl_major = ::atoi(numbers[0].c_str());

    if (numbers.size() > 1)
        gl_minor = ::atoi(numbers[1].c_str());

    return Semver(gl_major, gl_minor, 0);
}

bool OpenGLManager::GLInfo::is_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
{
    if (!m_detected)
        detect();
    
    return m_version >= Semver(major, minor, 0);
}

bool OpenGLManager::GLInfo::is_glsl_version_greater_or_equal_to(unsigned int major, unsigned int minor) const
{
    if (!m_detected)
        detect();

    return m_glsl_version >= Semver(major, minor, 0);
}

// If formatted for github, plaintext with OpenGL extensions enclosed into <details>.
// Otherwise HTML formatted for the system info dialog.
std::string OpenGLManager::GLInfo::to_string(bool for_github) const
{
    if (!m_detected)
        detect();

    std::stringstream out;

    const bool format_as_html = ! for_github;
    std::string h2_start = format_as_html ? "<b>" : "";
    std::string h2_end = format_as_html ? "</b>" : "";
    std::string b_start = format_as_html ? "<b>" : "";
    std::string b_end = format_as_html ? "</b>" : "";
    std::string line_end = format_as_html ? "<br>" : "\n";

    out << h2_start << "OpenGL installation" << h2_end << line_end;
    out << b_start << "GL version:   " << b_end << m_version << line_end;
#if ENABLE_GL_CORE_PROFILE
    out << b_start << "Profile:      " << b_end << (is_core_profile() ? "Core" : "Compatibility") << line_end;
#endif // ENABLE_GL_CORE_PROFILE
    out << b_start << "Vendor:       " << b_end << m_vendor << line_end;
    out << b_start << "Renderer:     " << b_end << m_renderer << line_end;
    out << b_start << "GLSL version: " << b_end << m_glsl_version << line_end;
    out << b_start << "Textures compression:       " << b_end << (are_compressed_textures_supported() ? "Enabled" : "Disabled") << line_end;

    {
#if ENABLE_GL_CORE_PROFILE
        std::vector<std::string>  extensions_list = get_extensions_list();
#else
        const std::string extensions_str = gl_get_string_safe(GL_EXTENSIONS, "");
        std::vector<std::string> extensions_list;
        boost::split(extensions_list, extensions_str, boost::is_any_of(" "), boost::token_compress_on);
#endif // ENABLE_GL_CORE_PROFILE

        if (!extensions_list.empty()) {
            if (for_github)
                out << "<details>\n<summary>Installed extensions:</summary>\n";
            else
                out << h2_start << "Installed extensions:" << h2_end << line_end;

            std::sort(extensions_list.begin(), extensions_list.end());
            for (const std::string& ext : extensions_list)
                if (! ext.empty())
                    out << ext << line_end;

            if (for_github)
                out << "</details>\n";
        }
    }

    return out.str();
}

#if ENABLE_GL_CORE_PROFILE
std::vector<std::string> OpenGLManager::GLInfo::get_extensions_list() const
{
    std::vector<std::string> ret;

    if (is_core_profile()) {
        GLint n = 0;
        glsafe(::glGetIntegerv(GL_NUM_EXTENSIONS, &n));
        ret.reserve(n);
        for (GLint i = 0; i < n; ++i) {
            const char* extension = (const char*)::glGetStringi(GL_EXTENSIONS, i);
            glcheck();
            if (extension != nullptr)
                ret.emplace_back(extension);
        }
    }
    else {
        const std::string extensions_str = gl_get_string_safe(GL_EXTENSIONS, "");
        boost::split(ret, extensions_str, boost::is_any_of(" "), boost::token_compress_on);
    }

    return ret;
}
#endif // ENABLE_GL_CORE_PROFILE

OpenGLManager::GLInfo OpenGLManager::s_gl_info;
bool OpenGLManager::s_compressed_textures_supported = false;
bool OpenGLManager::s_force_power_of_two_textures = false;
OpenGLManager::EMultisampleState OpenGLManager::s_multisample = OpenGLManager::EMultisampleState::Unknown;
OpenGLManager::EFramebufferType OpenGLManager::s_framebuffers_type = OpenGLManager::EFramebufferType::Unknown;

#ifdef __APPLE__ 
// Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
OpenGLManager::OSInfo OpenGLManager::s_os_info;
#endif // __APPLE__ 

OpenGLManager::~OpenGLManager()
{
    m_shaders_manager.shutdown();

#ifdef __APPLE__ 
    // This is an ugly hack needed to solve the crash happening when closing the application on OSX 10.9.5 with newer wxWidgets
    // The crash is triggered inside wxGLContext destructor
    if (s_os_info.major != 10 || s_os_info.minor != 9 || s_os_info.micro != 5)
    {
#endif //__APPLE__
        if (m_context != nullptr)
            delete m_context;
#ifdef __APPLE__ 
    }
#endif //__APPLE__
}

#if ENABLE_OPENGL_DEBUG_OPTION
#ifdef _WIN32
static void APIENTRY CustomGLDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
#else
static void CustomGLDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
#endif // _WIN32
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    std::string out = "OpenGL DEBUG message [";
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               out += "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: out += "Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  out += "Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         out += "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         out += "Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              out += "Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          out += "Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           out += "Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               out += "Other"; break;
    }
    out += "/";
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             out += "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   out += "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: out += "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     out += "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     out += "Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           out += "Other"; break;
    }
    out += "/";
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         out += "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       out += "medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          out += "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: out += "notification"; break;
    }
    out += "]:\n";
    std::cout << out << "(" << id << "): " << message << "\n\n";
}
#endif // ENABLE_OPENGL_DEBUG_OPTION

bool OpenGLManager::init_gl()
{
    if (!m_gl_initialized) {
#if ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
        glewExperimental = true;
#endif // ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            BOOST_LOG_TRIVIAL(error) << "Unable to init glew library: " << glewGetErrorString(err);
            return false;
        }

#if ENABLE_GL_CORE_PROFILE
        do {
            // glewInit() generates an OpenGL GL_INVALID_ENUM error
            err = ::glGetError();
        } while (err != GL_NO_ERROR);
#endif // ENABLE_GL_CORE_PROFILE

        m_gl_initialized = true;

        if (GLEW_EXT_texture_compression_s3tc)
            s_compressed_textures_supported = true;
        else
            s_compressed_textures_supported = false;

        if (GLEW_ARB_framebuffer_object)
            s_framebuffers_type = EFramebufferType::Arb;
        else if (GLEW_EXT_framebuffer_object)
            s_framebuffers_type = EFramebufferType::Ext;
        else
            s_framebuffers_type = EFramebufferType::Unknown;

#if ENABLE_OPENGL_ES
        bool valid_version = s_gl_info.is_version_greater_or_equal_to(2, 0);
#elif ENABLE_GL_CORE_PROFILE
        bool valid_version = s_gl_info.is_core_profile() ? s_gl_info.is_version_greater_or_equal_to(3, 2) : s_gl_info.is_version_greater_or_equal_to(2, 0);
#else
        bool valid_version = s_gl_info.is_version_greater_or_equal_to(2, 0);
#endif // ENABLE_OPENGL_ES

        if (!valid_version) {
            // Complain about the OpenGL version.
            wxString message = format_wxstr(
#if ENABLE_OPENGL_ES
                _L("eufyMake Studio requires OpenGL ES 2.0 capable graphics driver to run correctly, \n"
                    "while OpenGL version %s, render %s, vendor %s was detected."), s_gl_info.get_version_string(), s_gl_info.get_renderer(), s_gl_info.get_vendor());
#elif ENABLE_GL_CORE_PROFILE
                _L("eufyMake Studio requires OpenGL %s capable graphics driver to run correctly, \n"
                    "while OpenGL version %s, render %s, vendor %s was detected."), (s_gl_info.is_core_profile() ? "3.3" : "2.0"), s_gl_info.get_version_string(), s_gl_info.get_renderer(), s_gl_info.get_vendor());
#else
                _L("eufyMake Studio requires OpenGL 2.0 capable graphics driver to run correctly, \n"
                    "while OpenGL version %s, render %s, vendor %s was detected."), s_gl_info.get_version_string(), s_gl_info.get_renderer(), s_gl_info.get_vendor());
#endif // ENABLE_OPENGL_ES
            message += "\n";
        	message += _L("You may need to update your graphics card driver.");
#ifdef _WIN32
            message += "\n";
            message += _L("As a workaround, you may run eufyMake Studio with a software rendered 3D graphics by running anker-studio.exe with the --sw-renderer parameter.");
#endif
        	wxMessageBox(message, wxString("eufyMake Studio - ") + _L("Unsupported OpenGL version"), wxOK | wxICON_ERROR);
        }

        if (valid_version) {
            // load shaders
            auto [result, error] = m_shaders_manager.init();
            if (!result) {
                wxString message = format_wxstr(_L("Unable to load the following shaders:\n%s"), error);
                wxMessageBox(message, wxString("eufyMake Studio - ") + _L("Error loading shaders"), wxOK | wxICON_ERROR);
            }
#if ENABLE_OPENGL_DEBUG_OPTION
            if (m_debug_enabled && GLEW_KHR_debug) {
                ::glEnable(GL_DEBUG_OUTPUT);
                ::glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                ::glDebugMessageCallback(CustomGLDebugOutput, nullptr);
                ::glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            }
#endif // ENABLE_OPENGL_DEBUG_OPTION
        }

#ifdef _WIN32
        // Since AMD driver version 22.7.1, there is probably some bug in the driver that causes the issue with the missing
        // texture of the bed (see: https://github.com/prusa3d/PrusaSlicer/issues/8417).
        // It seems that this issue only triggers when mipmaps are generated manually
        // (combined with a texture compression) with texture size not being power of two.
        // When mipmaps are generated through OpenGL function glGenerateMipmap() the driver works fine,
        // but the mipmap generation is quite slow on some machines.
        // There is no an easy way to detect the driver version without using Win32 API because the strings returned by OpenGL
        // have no standardized format, only some of them contain the driver version.
        // Until we do not know that driver will be fixed (if ever) we force the use of power of two textures on all cards
        // containing the string 'Radeon' in the string returned by glGetString(GL_RENDERER)
        const auto& gl_info = OpenGLManager::get_gl_info();
        if (boost::contains(gl_info.get_vendor(), "ATI Technologies Inc.") && boost::contains(gl_info.get_renderer(), "Radeon"))
            s_force_power_of_two_textures = true;
#endif // _WIN32
    }

    return true;
}

#if ENABLE_GL_CORE_PROFILE
#if ENABLE_OPENGL_DEBUG_OPTION
wxGLContext* OpenGLManager::init_glcontext(wxGLCanvas& canvas, const std::pair<int, int>& required_opengl_version, bool enable_debug)
#else
wxGLContext* OpenGLManager::init_glcontext(wxGLCanvas& canvas, const std::pair<int, int>& required_opengl_version)
#endif // ENABLE_OPENGL_DEBUG_OPTION
#else
wxGLContext* OpenGLManager::init_glcontext(wxGLCanvas& canvas)
#endif // ENABLE_GL_CORE_PROFILE
{
    if (m_context == nullptr) {
#if ENABLE_OPENGL_ES
        wxGLContextAttrs attrs;
        attrs.PlatformDefaults().ES2().MajorVersion(2).EndList();
        m_context = new wxGLContext(&canvas, nullptr, &attrs);
#elif ENABLE_GL_CORE_PROFILE
#if ENABLE_OPENGL_DEBUG_OPTION
        m_debug_enabled = enable_debug;
#endif // ENABLE_OPENGL_DEBUG_OPTION

        const int gl_major = required_opengl_version.first;
        const int gl_minor = required_opengl_version.second;
        const bool supports_core_profile = (gl_major < 3) ? false : (gl_major > 3) ? true : gl_minor >= 2;

        if (gl_major == 0) {
            // search for highest supported core profile version
            // disable wxWidgets logging to avoid showing the log dialog in case the following code fails generating a valid gl context
            wxLogNull logNo;
            for (auto v = OpenGLVersions::core.rbegin(); v != OpenGLVersions::core.rend(); ++v) {
                wxGLContextAttrs attrs;
#if ENABLE_OPENGL_DEBUG_OPTION
                attrs.PlatformDefaults().MajorVersion(v->first).MinorVersion(v->second).CoreProfile().ForwardCompatible();
                if (m_debug_enabled)
                    attrs.DebugCtx();
                attrs.EndList();
#else
                attrs.PlatformDefaults().MajorVersion(gl_major).MinorVersion(gl_minor).CoreProfile().ForwardCompatible().EndList();
#endif // ENABLE_OPENGL_DEBUG_OPTION
                m_context = new wxGLContext(&canvas, nullptr, &attrs);
                if (m_context->IsOK()) {
                    std::map<std::string, std::string> map;
                    map.insert(std::make_pair(c_major_version, std::to_string(v->first)));
                    map.insert(std::make_pair(c_minor_version, std::to_string(v->second)));
                    reportBuryEvent(e_context_version, map);
                    s_gl_info.set_core_profile(true);
                    break;
                }
                else {
                    delete m_context;
                    m_context = nullptr;
                }
            }
        }

        if (m_context == nullptr) {
            // search for requested core profile version 
            if (supports_core_profile) {
                // disable wxWidgets logging to avoid showing the log dialog in case the following code fails generating a valid gl context
                wxLogNull logNo;
                wxGLContextAttrs attrs;
#if ENABLE_OPENGL_DEBUG_OPTION
                attrs.PlatformDefaults().MajorVersion(gl_major).MinorVersion(gl_minor).CoreProfile().ForwardCompatible();
                if (m_debug_enabled)
                    attrs.DebugCtx();
                attrs.EndList();
#else
                attrs.PlatformDefaults().MajorVersion(gl_major).MinorVersion(gl_minor).CoreProfile().ForwardCompatible().EndList();
#endif // ENABLE_OPENGL_DEBUG_OPTION
                m_context = new wxGLContext(&canvas, nullptr, &attrs);
                if (!m_context->IsOK()) {
                    BOOST_LOG_TRIVIAL(error) << "Unable to create context for required OpenGL " << gl_major << "." << gl_minor;
                    delete m_context;
                    m_context = nullptr;
                }
                else
                    s_gl_info.set_core_profile(true);
            }
        }

#if ENABLE_OPENGL_DEBUG_OPTION
        if (m_context == nullptr) {
            wxGLContextAttrs attrs;
            attrs.PlatformDefaults();
            if (m_debug_enabled)
                attrs.DebugCtx();
            attrs.EndList();
            // if no valid context was created use the default one
            m_context = new wxGLContext(&canvas, nullptr, &attrs);
        }
#else
        if (m_context == nullptr)
            // if no valid context was created use the default one
            m_context = new wxGLContext(&canvas);
#endif // ENABLE_OPENGL_DEBUG_OPTION
#else
        m_context = new wxGLContext(&canvas);
#endif // ENABLE_OPENGL_ES

#ifdef __APPLE__ 
        // Part of hack to remove crash when closing the application on OSX 10.9.5 when building against newer wxWidgets
        s_os_info.major = wxPlatformInfo::Get().GetOSMajorVersion();
        s_os_info.minor = wxPlatformInfo::Get().GetOSMinorVersion();
        s_os_info.micro = wxPlatformInfo::Get().GetOSMicroVersion();
#endif //__APPLE__
    }
    return m_context;
}

wxGLCanvas* OpenGLManager::create_wxglcanvas(wxWindow& parent)
{
#if ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
    wxGLAttributes attribList;
    attribList.PlatformDefaults().RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(24).SampleBuffers(1).Samplers(4).EndList();
#ifdef __APPLE__
    // on MAC the method RGBA() has no effect
    attribList.SetNeedsARB(true);
#endif // __APPLE__
#else
    int attribList[] = {
        WX_GL_RGBA,
    	WX_GL_DOUBLEBUFFER,
    	// RGB channels each should be allocated with 8 bit depth. One should almost certainly get these bit depths by default.
      	WX_GL_MIN_RED, 			8,
      	WX_GL_MIN_GREEN, 		8,
      	WX_GL_MIN_BLUE, 		8,
      	// Requesting an 8 bit alpha channel. Interestingly, the NVIDIA drivers would most likely work with some alpha plane, but glReadPixels would not return
      	// the alpha channel on NVIDIA if not requested when the GL context is created.
      	WX_GL_MIN_ALPHA, 		8,
    	WX_GL_DEPTH_SIZE, 		24,
    	WX_GL_SAMPLE_BUFFERS, 	GL_TRUE,
    	WX_GL_SAMPLES, 			4,
    	0
    };
#endif // ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES

    if (s_multisample == EMultisampleState::Unknown) {
        detect_multisample(attribList);
//        // debug output
//        std::cout << "Multisample " << (can_multisample() ? "enabled" : "disabled") << std::endl;
    }

    if (!can_multisample())
#if ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
    {
        attribList.Reset();
        attribList.PlatformDefaults().RGBA().DoubleBuffer().MinRGBA(8, 8, 8, 8).Depth(24).EndList();
#ifdef __APPLE__
        // on MAC the method RGBA() has no effect
        attribList.SetNeedsARB(true);
#endif // __APPLE__
    }

    return new wxGLCanvas(&parent, attribList, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
#else
        attribList[12] = 0;

    return new wxGLCanvas(&parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
#endif // ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
}

#if ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
void OpenGLManager::detect_multisample(const wxGLAttributes& attribList)
#else
void OpenGLManager::detect_multisample(int* attribList)
#endif // ENABLE_GL_CORE_PROFILE || ENABLE_OPENGL_ES
{
    int wxVersion = wxMAJOR_VERSION * 10000 + wxMINOR_VERSION * 100 + wxRELEASE_NUMBER;
    bool enable_multisample = wxVersion >= 30003;
    s_multisample = 
        enable_multisample &&
        // Disable multi-sampling on ChromeOS, as the OpenGL virtualization swaps Red/Blue channels with multi-sampling enabled,
        // at least on some platforms.
        platform_flavor() != PlatformFlavor::LinuxOnChromium &&
        wxGLCanvas::IsDisplaySupported(attribList)
        ? EMultisampleState::Enabled : EMultisampleState::Disabled;
    // Alternative method: it was working on previous version of wxWidgets but not with the latest, at least on Windows
    // s_multisample = enable_multisample && wxGLCanvas::IsExtensionSupported("WGL_ARB_multisample");
}

} // namespace GUI
} // namespace Slic3r
