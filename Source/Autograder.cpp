//
// Created on 1/10/2024.
//

#include "Autograder.h"

#include "JSONParser.h"
#include "Debug.h"
#include "Formatting.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

#define assertWithMessage(expression, message) \
    if (!(expression)) { \
        std::cout << "\n[Autograder] " << message << "\n"; \
        return false; \
    }

namespace JSONProc {

    bool runModelQueryTest(const std::string& aPath) {
        std::fstream theJsonFile(aPath + "/Resources/classroom.json");
        Model theModel;
        JSONParser theParser(theJsonFile);
        theParser.parse(&theModel);

        {
            auto theQuery = theModel.createQuery();
            const auto theResult = theQuery.select("'location'")
                                                 .get("'roomNumber'")
                                                 .value_or("std::nullopt");
            assertWithMessage(theResult == "247", "Expected '247', got: '" + theResult + "'");
        }
        {
            auto theQuery = theModel.createQuery();
            const auto theResult = theQuery.select("'students'")
                                                 .filter("index > 1")
                                                 .count();
            assertWithMessage(theResult == 2, "Expected '2', got: '" + std::to_string(theResult) + "'");
        }
        {
            auto theQuery = theModel.createQuery();
            const auto theResult = theQuery.select("'students'.3")
                                                 .get("'grade'")
                                                 .value_or("std::nullopt");
            assertWithMessage(theResult == "null", "Expected '2', got: '" + theResult + "'");
        }
        {
            auto theQuery = theModel.createQuery();
            const auto theResult = theQuery.select("'location'.'uh_oh'")
                                                 .get("'nope'");
            assertWithMessage(theResult == std::nullopt, "Expected std::nullopt, got: '" + theResult.value() + "'");
        }

        return true;
    }

    // ---Autograder---

    Autograder::Autograder(const std::string& aWorkingDirectoryPath)
        : workingDirectory(aWorkingDirectoryPath) {
    }

    bool Autograder::runTest(const std::string& aTestName) {
        if (!openFiles(aTestName))
            return false;

        Model theModel;
        if (!parseJson(theModel))
            return false;

        return runCommands(theModel);
    }

    std::string getJsonFileName(std::fstream& aTestFile) {
        std::string theFileName;
        std::getline(aTestFile, theFileName);

        if (theFileName.find(".json") == std::string::npos)
            return "";

        return theFileName;
    }

    bool Autograder::openFiles(const std::string& aTestName) {
        testFile.open(workingDirectory + "/Tests/" + aTestName + ".txt");
        assertWithMessage(testFile.is_open(), "Could not open test file.");

        const auto theJsonFileName = getJsonFileName(testFile);
        assertWithMessage(!theJsonFileName.empty(), "Test does not specify a JSON target.");

        jsonFile.open(workingDirectory + "/Resources/" + theJsonFileName);
        assertWithMessage(jsonFile.is_open(), "Could not open JSON file.");

        return true;
    }

    bool Autograder::parseJson(Model& aModel){
        JSONParser theParser(jsonFile);
        assertWithMessage(theParser.parse(&aModel), "Error parsing JSON");

        return true;
    }

    void removeWhitespace(std::string& aString) {
        aString.erase(std::remove_if(aString.begin(), aString.end(), [](unsigned char x) {
            return std::isspace(x);
        }), aString.end());
    }

    bool Autograder::runCommands(Model& aModel) {
        std::string theQuery;
        while (std::getline(testFile, theQuery)) {
            CommandProcessor theProcessor(aModel);
            auto theOutput = theProcessor.process(theQuery).value_or("~~empty~~");
            removeWhitespace(theOutput);

            auto theExpectedOutput = getExpectedOutput(theQuery);
            removeWhitespace(theExpectedOutput);

            assertWithMessage(theOutput == theExpectedOutput, "Test failed: '" + theQuery +
                "'\nExpected: '" + theExpectedOutput + "', got: '" + theOutput + "'");
        }

        return true;
    }

    std::string Autograder::getExpectedOutput(const std::string& aQuery) {
        const auto theStartPosition = aQuery.find("//");
        ASSERT(theStartPosition != std::string::npos);
        ASSERT(aQuery.length() - theStartPosition > 3);

        return aQuery.substr(theStartPosition + 3);
    }


    // ---StringIterator---

    StringIterator::StringIterator(const std::string& aString, const size_t aStartIndex)
        : string(aString), index(aStartIndex) {}

    bool StringIterator::matchesCharacter(char aChar) {
        if (getRemainingLength() == 0)
            return false;

        const bool isEqual = string[index] == aChar;
        if (isEqual)
            ++index;

        return isEqual;
    }

    bool StringIterator::matchesKeyword(const std::string& aKeyword) {
        if (aKeyword.length() > getRemainingLength())
            return false;

        const bool isEqual = string.compare(index, aKeyword.length(), aKeyword) == 0;
        if (isEqual)
            index += aKeyword.length();

        return isEqual;
    }

    std::string StringIterator::extractValueFromParenthesis() {
        ASSERT(getRemainingLength() > 0);
        ASSERT(string[index] == '(');
        const auto theEndPosition = string.find(')', index);
        ASSERT(theEndPosition != std::string::npos);

        const auto theStartPosition = index + 1;
        const auto theLength = theEndPosition - index;
        index = theEndPosition + 1;
        return string.substr(theStartPosition, theLength - 1);
    }

    size_t StringIterator::getRemainingLength() const {
        return string.length() - index;
    }


    // ---CommandProcessor---

    CommandProcessor::CommandProcessor(Model& aModel) : modelQuery(aModel.createQuery()) {
    }

    std::optional<std::string> CommandProcessor::process(const std::string& aQuery) {
        StringIterator theIterator(aQuery);
        std::optional<std::string> theOutput = std::nullopt;

        do {
            const auto theCommandType = getCommandType(theIterator);
            if (theCommandType == CommandType::invalid) {
                std::clog << "Invalid command type in query: '" << aQuery << "'\n";
                return std::nullopt;
            }

            const auto theParameters = theIterator.extractValueFromParenthesis();
            theOutput = callCommand(theCommandType, theParameters);

        } while (theIterator.matchesCharacter('.'));

        return theOutput;
    }

    CommandProcessor::CommandType CommandProcessor::getCommandType(StringIterator& anIterator) {
        for (size_t i = 0; i < commandList.size(); ++i)
            if (anIterator.matchesKeyword(commandList[i]))
                return static_cast<CommandType>(i);

        return CommandType::invalid;
    }

    std::optional<std::string> CommandProcessor::callCommand(const CommandType aType, const std::string& aParameter) {
        switch (aType)
        {
        case CommandType::select:
            modelQuery.select(aParameter);
            break;

        case CommandType::filter:
            modelQuery.filter(aParameter);
            break;

        case CommandType::count:
            return std::to_string(modelQuery.count());

        case CommandType::sum:
            return doubleToString(modelQuery.sum());

        case CommandType::get:
            return modelQuery.get(aParameter);

        default:
            break;
        }

        return std::nullopt;
    }

} // ECE141