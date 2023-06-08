#include "Timer.hpp"
#include <boost/log/trivial.hpp>

using namespace std::chrono;

Slic3r::Timer::Timer(const std::string &name) : m_name(name), m_start(steady_clock::now()) {}

Slic3r::Timer::~Timer()
{
    BOOST_LOG_TRIVIAL(debug) << "Timer '" << m_name << "' spend " << 
        duration_cast<milliseconds>(steady_clock::now() - m_start).count() << "ms";
}
