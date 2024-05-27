///|/ Copyright (c) Prusa Research 2023 - 2023
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_Format_SVG_hpp_
#define slic3r_Format_SVG_hpp_

#include <string>

namespace Slic3r {

class Model;
// Load an SVG file as embossed shape into a provided model.
bool load_svg(const std::string &input_file, Model &output_model);

}; // namespace Slic3r

#endif /* slic3r_Format_SVG_hpp_ */
