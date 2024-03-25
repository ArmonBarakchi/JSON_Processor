//
// Created on 1/10/2024.
//

#pragma once

#include <string>
#include <fstream>
#include <array>
#include "Model.h"

namespace JSONProc {

    bool runModelQueryTest(const std::string& aPath);

    class Autograder {
    public:
        Autograder(const std::string& aWorkingDirectoryPath);

        bool runTest(const std::string& aTestName);

    protected:
        bool openFiles(const std::string& aTestName);
        bool parseJson(Model& aModel);

        bool runCommands(Model& aModel);

        std::string getExpectedOutput(const std::string& aQuery);

        std::string workingDirectory;
        std::fstream testFile, jsonFile;

    };

    class StringIterator {
    public:
        StringIterator(const std::string& aString, size_t aStartIndex = 0);

        bool matchesCharacter(char aChar);
        bool matchesKeyword(const std::string& aKeyword);
        std::string extractValueFromParenthesis();

        std::string getRemaningString() { return string.substr(index); }

        size_t getRemainingLength() const;

    protected:
        const std::string& string;
        size_t index = 0;

    };

    class CommandProcessor {
    public:
        CommandProcessor(Model& aModel);

        std::optional<std::string> process(const std::string& aQuery);

    protected:
        std::array<std::string, 5> commandList { "select", "filter", "count", "sum", "get" };
        enum class CommandType { select = 0, filter, count, sum, get, invalid };

        CommandType getCommandType(StringIterator& anIterator);
        std::optional<std::string> callCommand(CommandType aType, const std::string& aParameter);

        ModelQuery modelQuery;

    };

}
