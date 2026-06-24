#pragma once
#include <string>

namespace ocm {

// Resolves the final chapter-marker name from the dock's name field.
//
// - Leading/trailing whitespace is trimmed from `custom`.
// - If the trimmed value is non-empty, it is used verbatim.
// - Otherwise an auto name "<prefix> <counter>" is returned (default prefix
//   "Chapter"), e.g. resolve("", 3) -> "Chapter 3".
//
// Used by both the button and the hotkey paths so naming is identical.
std::string resolveChapterName(const std::string& custom, int counter,
                               const std::string& autoPrefix = "Chapter");

} // namespace ocm
