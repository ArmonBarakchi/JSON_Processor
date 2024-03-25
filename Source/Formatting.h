//
// Created on 1/12/2024.
//

#pragma once

#include <sstream>
#include <iomanip>

namespace JSONProc {

    // Rounds a double to nearest hundredth
    inline std::string doubleToString(const double aValue) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << aValue;
        auto theString = stream.str();

        // Remove excess 0's
        while (theString[theString.length() - 1] == '0')
            theString = theString.erase(theString.length() - 1);

        if (theString[theString.length() - 1] == '.')
            theString = theString.erase(theString.length() - 1);

        return theString;
    }

}