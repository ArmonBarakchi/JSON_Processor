//
// Created on 1/15/2024.
//

#pragma once

#include <string>
#include <optional>
#include "JSONParser.h"
#include <variant>
#include <vector>
#include <map>
#include <stack>
#include <utility>
#include "Formatting.h"
#include <regex>
#include <cmath>

namespace JSONProc {

	class ModelQuery;
    class filterPolicy;
    enum class filterType;

	// Model is built from a bunch of these...
	struct ModelNode {

        using ListType = std::vector<ModelNode*>;
        using ObjectType = std::map<std::string, ModelNode*>;
        struct NullType {};
        std::variant<NullType, long, double, bool, std::string, ListType, ObjectType> value = std::map<std::string, ModelNode*>();

        [[nodiscard]] std::string toString() const {
            struct StringVisitor {
                std::string operator()(NullType) {return "null";}
                std::string operator()(const bool &value) {return value ? "true" : "false";}
                std::string operator()(const double &value) {return std::to_string(value);} //ask about behavior of trailing zeroes
                std::string operator()(const long &value) {return  std::to_string(value);}
                std::string operator()(const std::string &value) {return "\"" + value + "\"";}
                std::string operator()(const ListType &list) {

                    std::string output = "[";
                    for (auto it = list.begin(); it != list.end(); ++it) {
                        if (std::next(it) != list.end()) {
                           output += (*it)->toString() + ", ";
                        }
                        else{
                           return output += (*it)->toString() + "]";
                        }
                    }
                    return output; // just so warning will go away, it will never hit this
                }
                std::string operator()(const ObjectType &object) {
                    std::string output = "{";
                    for (auto it = object.begin(); it != object.end(); ++it) {
                        output += "\"" + it->first + "\": " + (*it->second).toString();
                        if (std::next(it) != object.end()) {
                            output += ", ";
                        } else {
                            return output + "}";
                        }
                    }
                    return output; // just so warning will go away, it will never hit this
                }
            };

           return std::visit(StringVisitor(), value);

        }

        friend std::ostream& operator << (std::ostream &anOut, const ModelNode &aNode) { //for debugging purposes
            return anOut << aNode.toString();
        }

	};

	class Model : public JSONListener {
        public:
            Model();
            Model(ModelNode &_rootNode);
            ~Model() override = default;
            Model(const Model& aModel);
            Model &operator=(const Model& aModel);

            ModelQuery createQuery();

            bool addKeyValuePair(const std::string &aKey, const std::string &aValue, Element aType) override;
            bool addItem(const std::string &aValue, Element aType) override;
            bool openContainer(const std::string &aKey, Element aType) override;
            bool closeContainer(const std::string &aKey, Element aType) override;
            ModelNode& getRoot();
            void setRoot(const ModelNode& newRootNode);



        protected:
            ModelNode rootNode;
            std::stack<ModelNode*> nodetracker;
            void populateNode(ModelNode* temp, const std::string &aValue, JSONProc::Element aType);

	};

    enum class filterType{
        indexFilter, keyNameFilter, none
    };


    class filterPolicy {
        public:
            filterPolicy() = default;
            filterPolicy (std::variant<std::string, int> filterCondition, filterType _aFilterType);
            bool isAdmittable (std::variant<std::string, size_t> currentPosition);
            void setOp (std::string &_anOperation);
            void clearFilter();

        protected:
            bool compare(int a, int b, std::function<bool(int, int)> op);
            filterType aFilterType;
            std::variant<std::string, int> filterCondition;
            std::string anOperation;
    };

	class ModelQuery {
	public:
		ModelQuery(Model& aModel);

		// ---Traversal---
		ModelQuery& select(const std::string& aQuery);

		// ---Filtering---
		ModelQuery& filter(const std::string& aQuery);

		// ---Consuming---
		size_t count();// count number of nodes in a node
		double sum();
		std::optional<std::string> get(const std::string& aKeyOrIndex);


	protected:
        // --- data members ---
		Model model;
        Model tempModel;
        filterPolicy aFilter;
        bool errorChecking;
        bool calledByGet;

        // --- primitives ----
        std::string removeApostrophes(const std::string& str);
        ModelNode* traverseQuery(ModelNode* root, const std::string& query);
        void raiseErrorFlag();
        filterType determineFilterType(const std::string& aQuery);
        std::string extractCondition(const std::string& input_string);
        int extractNumber(const std::string& s);
        std::string extractOperation(const std::string &aQuery);

	};

}
