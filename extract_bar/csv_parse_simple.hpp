#include <string>
#include <vector>
#include <sstream>

std::vector<std::string> csv_parse_simple_row(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                }
                else {
                    in_quotes = false;
                }
            }
            else {
                field += c;
            }
        }
        else {
            if (c == '"') {
                in_quotes = true;
            }
            else if (c == ',') {
                result.push_back(field);
                field.clear();
            }
            else {
                field += c;
            }
        }
    }

    result.push_back(field); // Add the last field
    return result;
}