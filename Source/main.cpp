#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include "JSONParser.h"
#include "Autograder.h"
#include "Testable.h"
#include "Debug.h"

// Be sure to update this path if necessary (should point to the repo directory)
inline std::string getWorkingDirectoryPath() {
    return "/Users/armonbarakchi/Desktop/JSON_Processor";
}

bool runAutoTest(const std::string& aPath, const std::string& aTestName) {
    JSONProc::Autograder autoGrader(aPath);
    return autoGrader.runTest(aTestName);
}

bool runNoFilterTest(const std::string& aPath) {
    return runAutoTest(aPath, "NoFilterTest");
}

bool runBasicTest(const std::string& aPath) {
    return runAutoTest(aPath, "BasicTest");
}

bool runAdvancedTest(const std::string& aPath) {
    return runAutoTest(aPath, "AdvancedTest");
}

int runTest(const int argc, const char* argv[]) {
    const std::string thePath = argc > 2 ? argv[2] : getWorkingDirectoryPath();
    const std::string theTest = argv[1];

    std::map<std::string, std::function<bool(const std::string &)>> theTestFunctions{
            {"compile",  [](const std::string &) { return true; }},
            {"nofilter", runNoFilterTest},
            {"query",    JSONProc::runModelQueryTest},
            {"basic",    runBasicTest},
            {"advanced", runAdvancedTest}
    };

    if (theTestFunctions.count(theTest) == 0) {
        std::clog << "Unkown test '" << theTest << "'\n";
        return 1;
    }

    const bool hasPassed = theTestFunctions[theTest](thePath);
    std::cout << "Test '" << theTest << "' " << (hasPassed ? "PASSED" : "FAILED") << "\n";
    return !hasPassed;
}

    int main(const int argc, const char *argv[]) {
        if (argc > 1)
            return runTest(argc, argv);
      if(JSONProc::runModelQueryTest(getWorkingDirectoryPath()) ){
          std::cout << "test ModelQuery passed" << "\n";
      }
      if( runAdvancedTest(getWorkingDirectoryPath())) {
          std::cout << "test advanced passed" << "\n";
      }
      if(runBasicTest(getWorkingDirectoryPath())) {
          std::cout << "test Basic passed" << "\n";
      }

      if(runNoFilterTest(getWorkingDirectoryPath())){
          std::cout << "test noFilter passed" << "\n";
      }


        return 0;
    }







