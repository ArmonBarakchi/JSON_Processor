#pragma once

#include <iostream>
#include <string>
#include <optional>
#include <sstream>

namespace JSONProc {

    class Testable
    {
    public:
        Testable() = default;
        virtual ~Testable() = default;

        // Retrieve the name of a specific test.
        virtual std::optional<std::string> getTestName(size_t anIndex) const = 0;

        // Run a specific test.
        virtual bool operator()(const std::string &aName) = 0;

        // Run all tests, returns number of tests passed.
        size_t runAllTests() {
            size_t theNumberOfTestsPassed = 0;
            std::stringstream theOutput;
            size_t i = 0;
            while (const auto theName = getTestName(i++)) {
                const bool hasPassed = (*this)(theName.value());
                theNumberOfTestsPassed += static_cast<int>(hasPassed);

                theOutput << i + 1 << ". " << theName.value() << ": "
                          << (hasPassed ? "PASS" : "FAIL") << "\n";
            }

            if (theNumberOfTestsPassed == i - 1)
                std::cout << "All";
            else
                std::cout << theNumberOfTestsPassed << " of " << i - 1;

            std::cout << " tests passed.\n" << theOutput.str() << "\n";
            return theNumberOfTestsPassed;
        }

    };

}
