#ifndef libslic3r_Timer_hpp_
#define libslic3r_Timer_hpp_

#include <string>
#include <chrono>

namespace Slic3r {

/// <summary>
/// Instance of this class is used for measure time consumtion
/// of block code until instance is alive and write result to debug output
/// </summary>
class Timer
{
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;
public:
    /// <summary>
    /// name describe timer
    /// </summary>
    /// <param name="name">Describe timer in consol log</param>
    Timer(const std::string& name);

    /// <summary>
    /// name describe timer
    /// </summary>
    ~Timer();
};

} // namespace Slic3r
#endif // libslic3r_Timer_hpp_