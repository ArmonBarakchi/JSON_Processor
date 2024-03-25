//
// Created on 1/15/2024.
//

#include "Model.h"


#include "Debug.h"

namespace JSONProc {

	// ----------Model Class------------
    //used to create the model

    Model::Model() = default;

	Model::Model(ModelNode &_rootNode){
        this->rootNode = _rootNode;
    }

	Model::Model(const Model& aModel) {
        this->rootNode = aModel.rootNode;
	}

	Model &Model::operator=(const Model& aModel) {
        this->rootNode = aModel.rootNode;
		return *this;
	}

	ModelQuery Model::createQuery() {
		return ModelQuery(*this);
	}

    ModelNode& Model::getRoot() {
        return rootNode;
    }

    void Model::setRoot(const ModelNode& newRootNode) {
        rootNode = newRootNode;
    }

    //figure out what type the value is and set the variant in temp.value to that type
    void Model::populateNode(ModelNode* temp, const std::string &aValue, JSONProc::Element aType) {
        switch(aType){
            case Element::quoted:
                (*temp).value = aValue;
                break;
            case Element::constant:
                if(aValue == "true") {
                    (*temp).value = true;
                    break;

                }
                if(aValue == "false") {
                    (*temp).value = false;
                    break;
                }
                if(aValue == "null") {
                    (*temp).value = ModelNode::NullType{};
                    break;
                }
                if (std::fabs(std::stod(aValue) - std::trunc(std::stod(aValue)))>0){ //check if there is a remainder after truncation -> if yes, its a double
                    (*temp).value = std::stod(aValue);
                    break;
                   }
                else {
                    (*temp).value = std::stol(aValue);
                    break;
                }
            case Element::object:
                (*temp).value = ModelNode::ObjectType {};
                break;
            case Element::array:
                (*temp).value = ModelNode::ListType {};
                break;
            case Element::closing:
            case Element::unknown:
                if(aValue.empty()) {
                    break;
                }
        }
    }

	bool Model::addKeyValuePair(const std::string& aKey, const std::string& aValue, Element aType) { //no need to error check because current
        ModelNode* temp = new ModelNode;

        populateNode(temp, aValue, aType);

        ModelNode &aNode = *(nodetracker.top());
        if (std::holds_alternative<ModelNode::ObjectType>(aNode.value)) {
            auto& objMap = std::get<ModelNode::ObjectType>(aNode.value);
            objMap.insert(std::make_pair(aKey, temp));
            return true;
        }
        return false;
	}

	bool Model::addItem(const std::string& aValue, Element aType) {
        ModelNode* temp = new ModelNode;
        populateNode(temp, aValue, aType);
        ModelNode &aNode = *(nodetracker.top());
        if (std::holds_alternative<ModelNode::ListType >(aNode.value)) {
            auto &objList = std::get<ModelNode::ListType>(aNode.value);
            objList.push_back(temp);
            return true;
        }
        else {
            aNode.value = ModelNode::ListType{new ModelNode{(*temp).value}};
            return true;
        }
	}

	bool Model::openContainer(const std::string& aContainerName, Element aType) {

        if (nodetracker.empty()) { //is this correct?
            nodetracker.push(&rootNode);
            return true;
        }

		ModelNode &temp = *(nodetracker.top()); //used to see if we should add item or add key-value pair
        ModelNode* newNode = new ModelNode {};

        if(aType == Element::array) {
            (*newNode).value = ModelNode::ListType{};
        }

        if (std::holds_alternative<ModelNode::ObjectType>(temp.value)) {
            auto& objMap = std::get<ModelNode::ObjectType>(temp.value);
            objMap.insert(std::make_pair(aContainerName, newNode));
            nodetracker.push(newNode);
            return true;

        }

        if (std::holds_alternative<ModelNode::ListType>(temp.value)) {
            auto &objList = std::get<ModelNode::ListType>(temp.value);
            objList.push_back(newNode);
            nodetracker.push(newNode);
            return true;
        }
        return false;
	}

	bool Model::closeContainer([[maybe_unused]] const std::string& aContainerName, [[maybe_unused]] Element aType) {
        //regardless of the name or type the process is the same

        if (!nodetracker.empty()) {
            nodetracker.pop(); // Safe to pop if the stack is not empty
            return true;
        } else {
            std::cout << "nodetracker is empty, cannot pop!" << std::endl; //for debugging
            return false;
        }

	}

    // ---------------ModelQuery Class----------------

    //used to query the model
	ModelQuery::ModelQuery(Model &aModel) : model(aModel) {
        calledByGet = false;
        errorChecking = false;
    }

    //-----select command-----

    ModelQuery& ModelQuery::select(const std::string& aQuery) {
        if(aQuery=="") { //just for empty query
            ModelNode *temp = &(this->model.getRoot());
            this->tempModel.setRoot(*(temp));
            return *this;
        }

        if (calledByGet) { //need this if statement just because I want to use the temp model instead of the root model
            ModelNode* rootNode = &(this->tempModel.getRoot());
            ModelNode* temp = traverseQuery(rootNode, aQuery); //get a pointer to the node that you want
            if (temp) {
                this->tempModel.setRoot(*(temp));
                return *this;
            }
            else {
                this->raiseErrorFlag(); //node that was queried doesn't exist
                return *this;
            }
        }
        else {
            ModelNode *rootNode = &(this->model.getRoot());
            ModelNode *temp = traverseQuery(rootNode, aQuery); //get a pointer to the node that you want
            if (temp) {
                this->tempModel.setRoot(*(temp));
                return *this;
            } else {
                this->raiseErrorFlag(); //node that was queried doesn't exist
                return *this;
            }
        }
	}

    // ---- filter command --------
    ModelQuery& ModelQuery::filter(const std::string& aQuery) {

        filterType aFilterType = determineFilterType(aQuery);
        if (aFilterType == filterType::keyNameFilter) {
            filterPolicy aFilterPolicy (extractCondition(aQuery), aFilterType);
            std::string anOperation = "LocateSubstring";
            aFilterPolicy.setOp(anOperation);
            this->aFilter = aFilterPolicy;
            return *this;
        }
        else if (aFilterType == filterType::indexFilter) {
            filterPolicy aFilterPolicy (extractNumber(aQuery), aFilterType);
            std::string anOperation = (extractOperation(aQuery));
            aFilterPolicy.setOp(anOperation);
            this->aFilter = aFilterPolicy;
            return *this;

        }
        else {
            return *this; // just so warning will go away, it will never hit this
        }

    }

    //---------------Consuming methods ------------------------

    /*Policy Decisions of this function:
    * made the choice that when you call count on a value node, or a container with nothing
    * in it that count returns 0
    */

    size_t ModelQuery::count() {
        struct GetCount {
            GetCount(ModelQuery& modelQuery) : modelQuery(modelQuery) {}
            size_t operator()(const ModelNode::NullType) const {return 0;}
            size_t operator()(const bool&) const {return 0;}
            size_t operator()(const long&) const {return 0;}
            size_t operator()(const double&) const {return 0;}
            size_t operator()(const std::string&) const {return 0;}
            size_t operator()(const ModelNode::ListType &list) const {
                size_t result{0};
                for (size_t i =0; i<list.size(); i++)
                    if (this->modelQuery.aFilter.isAdmittable(i)) {
                        result++;
                    }

                return result;
            }
            size_t operator()(const ModelNode::ObjectType aMap) const {
                size_t result{0};
                for (const auto& pair : aMap) {
                    if (this->modelQuery.aFilter.isAdmittable(pair.first)) {
                        result++;
                    }
                }

                return result;
            }
        private:
            ModelQuery& modelQuery;
        };
        ModelNode temp;
        temp.value = this->tempModel.getRoot().value;
        size_t result = std::visit(GetCount(*this), temp.value);
        this->aFilter.clearFilter();
        return result;
    }

    /*Policy Decisions of this function:
     *if this gets called on an object:
     *  return sum of all the values at each key if they are a long or double
     *if this gets called on an empty list:
     *  return zero
     *If this gets called on a list of any kind:
     *  sum up the values that are of type long or double
     * If this gets called on a value node:
     *  return value if it is Long or Double, return zero if it is any other type
     *
     * Rounds sum to two decimal places
     */
    double ModelQuery::sum() {
        struct GetSum {
            GetSum(ModelQuery& modelQuery) : modelQuery(modelQuery) {}
            double operator()(ModelNode::NullType) const {return 0;}
            double operator()([[maybe_unused]] bool &value) const {return 0;}
            double operator()(const long &value) const {return static_cast<double>(value);}
            double operator()(const double &value) const {return value;}
            double operator()([[maybe_unused]] const std::string &value) const {return 0;}
            double operator()(const ModelNode::ListType &list) const {
                double sum{0.0};
                for (size_t i=0; i<list.size(); i++ ) {
                    if (this->modelQuery.aFilter.isAdmittable(i)) {
                        auto temp = list[i]->value;
                        if (std::holds_alternative<double>(temp)) {
                            sum += std::get<double>(temp);
                        } else if (std::holds_alternative<long>(temp)) {
                            sum += static_cast<double>(std::get<long>(temp));
                        }
                    }
                }
                return sum;
            }
            double operator()(const ModelNode::ObjectType &aMap) const {
                double sum{0.0};
                for (const auto& [key, aNodePtr] : aMap) {
                    if (this->modelQuery.aFilter.isAdmittable(key)) {
                        auto temp = aNodePtr->value;
                        if (std::holds_alternative<double>(temp)) {
                            sum += std::get<double>(temp);
                        } else if (std::holds_alternative<long>(temp)) {
                            sum += static_cast<double>(std::get<long>(temp));
                        }
                    }
                }
                return sum;
            }
        private:
            ModelQuery& modelQuery;
        };
        ModelNode temp;
        temp.value = this->tempModel.getRoot().value;
        double sum = std::visit(GetSum(*this), temp.value);
        this->aFilter.clearFilter();
        return  (std::round(sum * 100.0) / 100.0); //round the sum to two decimal places


    }

    std::optional<std::string> ModelQuery::get(const std::string& aKeyOrIndex) {
        struct GetNewModel {
            GetNewModel(ModelQuery& modelQuery) : modelQuery(modelQuery) {}
            ModelNode* operator()(ModelNode::NullType) const {
                ModelNode* temp = new ModelNode{ModelNode::NullType{}};
                return temp;
            }
            ModelNode* operator()(const bool &value) const {
                ModelNode* temp = new ModelNode{value};
                return temp;
            }
            ModelNode* operator()(const long &value) const {
                ModelNode* temp = new ModelNode{value};
                return temp;
            }
            ModelNode* operator()(const double &value) const {
                ModelNode* temp = new ModelNode{value};
                return temp;
            }
            ModelNode* operator()(const std::string &value) const {
                ModelNode* temp = new ModelNode{value};
                return temp;
            }
            ModelNode* operator()( ModelNode::ListType &list) {
                ModelNode* templist = new ModelNode;
                (*templist).value = ModelNode::ListType{};
                for (size_t i=0; i<list.size(); i++ ) {
                    if (this->modelQuery.aFilter.isAdmittable(static_cast<size_t>(i))) {
                        auto &objList = std::get<ModelNode::ListType>(templist->value);
                        objList.push_back(list[i]);
                    }
                }
                return templist;

            }
            ModelNode* operator()(ModelNode::ObjectType &aMap)  {
                ModelNode* tempObj = new ModelNode;
                for (const auto& [key, aNodePtr] : aMap) {
                    if (this->modelQuery.aFilter.isAdmittable(key)) {
                        auto& objMap = std::get<ModelNode::ObjectType>(tempObj->value);
                        objMap.insert(std::make_pair(key, aNodePtr));
                    }
                }

                return tempObj;
            }
        private:
            ModelQuery& modelQuery;
        };

        if (aKeyOrIndex == "*") {
            calledByGet = true;
            if (!errorChecking) {
                ModelNode* filteredTemp = std::visit(GetNewModel(*this), this->tempModel.getRoot().value);
                calledByGet = false;
                this->aFilter.clearFilter();
                return (*filteredTemp).toString();
            }
            else {
                calledByGet=false;
                errorChecking = false;
                this->aFilter.clearFilter();
                return std::nullopt;
            }

        } else {
            calledByGet = true;

            this->select(aKeyOrIndex);
            if (!errorChecking) {
                calledByGet = false;
                this->aFilter.clearFilter();
                return this->tempModel.getRoot().toString();
            } else {
                calledByGet = false;
                errorChecking = false;
                this->aFilter.clearFilter();
                return std::nullopt;
            }
        }

    }



    //-----------------Primitives for Model Query --------------------

    std::string ModelQuery::removeApostrophes(const std::string& str) {
        if (str.size() >= 2 && str.front() == '\'' && str.back() == '\'') {
            return str.substr(1, str.size() - 2);
        } else {
            // If the string doesn't have apostrophes around it, return as is
            return str;
        }
    }

    ModelNode* ModelQuery::traverseQuery(ModelNode* root, const std::string& query) {
        size_t pos = query.find(".");
        if (pos == std::string::npos) {
            auto current = removeApostrophes(query);
            if (auto* listPtr = std::get_if<ModelNode::ListType>(&root->value)) {
                try {
                    size_t index = std::stoi(current);
                    if (index < listPtr->size()) {
                        return (*listPtr)[index];
                    } else {
                        std::cerr << "Index out of bounds in list!" << std::endl;
                        return nullptr;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid index in list!" << std::endl;
                    return nullptr;
                }
            } else if (auto* objectPtr = std::get_if<ModelNode::ObjectType>(&root->value)) {
                auto it = objectPtr->find(current);
                if (it != objectPtr->end()) {
                    return it->second;
                } else {
                    std::cerr << "Object '" << query << "' not found!" << std::endl;
                    return nullptr;
                }
            } else {
                std::cerr << "Unsupported value type!" << std::endl;
                return nullptr;
            }
        } else {
            std::string current = removeApostrophes((query.substr(0, pos)));

            std::string next = query.substr(pos + 1);

            if (auto *listPtr = std::get_if<ModelNode::ListType>(&root->value)) {
                try {
                    size_t index = std::stoi(current);
                    if (index < listPtr->size()) {
                        return traverseQuery((*listPtr)[index], next);
                    } else {
                        std::cerr << "Index out of bounds in list!" << std::endl;
                        return nullptr;
                    }
                } catch (const std::invalid_argument &e) {
                    std::cerr << "Invalid index in list!" << std::endl;
                    return nullptr;
                }
            } else if (auto *objectPtr = std::get_if<ModelNode::ObjectType>(&root->value)) {
                auto it = objectPtr->find(current);
                if (it != objectPtr->end()) {
                    return traverseQuery(it->second, next);
                } else {
                    std::cerr << "Object '" << current << "' not found!" << std::endl;
                    return nullptr;
                }
            } else {
                std::cerr << "Unsupported value type!" << std::endl;
                return nullptr;
            }
        }
    }

    void ModelQuery::raiseErrorFlag() {
        errorChecking = true;
    }

    //-------------filter command primitives-----------

    filterType ModelQuery::determineFilterType(const std::string& aQuery){
        if (std::string::npos != aQuery.find("index")) {
            return filterType::indexFilter;
        }

        if (std::string::npos != aQuery.find("contains")) {
            return filterType::keyNameFilter;
        }
        return filterType::none; // just so warning will go away, it will never hit this
    }

    std::string ModelQuery::extractCondition(const std::string& input_string) {
        // Find the position of the first single quote
        size_t start_pos = input_string.find('\'');
        if (start_pos == std::string::npos) {
            return ""; // No single quote found
        }

        // Find the position of the second single quote
        size_t end_pos = input_string.find('\'', start_pos + 1);
        if (end_pos == std::string::npos) {
            return ""; // No second single quote found
        }

        // Extract the substring between the single quotes to get the filter condition
        return input_string.substr(start_pos + 1, end_pos - start_pos - 1);
    }
    int ModelQuery::extractNumber(const std::string& s) {
        std::istringstream iss(s);
        std::string word;
        while (iss >> word) {
            if (std::isdigit(word[0])) {
                return std::stoi(word);
            }
        }
        throw std::invalid_argument("No number found in the string.");
    }
    std::string ModelQuery::extractOperation(const std::string &aQuery) {
        std::regex re("(==|!=|<=?|>=?)");
        std::smatch match;
        if (std::regex_search(aQuery, match, re)) {
            return match.str();
        }
        return "";
    }


    //-------------filter policy class primitives ---------------

    filterPolicy::filterPolicy(std::variant<std::string, int> _filterCondition, JSONProc::filterType _aFilterType) {
        this->filterCondition = _filterCondition;
        this->aFilterType = _aFilterType;
    }
    void filterPolicy::setOp (std::string &_anOperation) {
        this->anOperation = _anOperation;
    }
    void filterPolicy::clearFilter() {
        this->aFilterType = filterType::none;
    }
    bool filterPolicy::compare(int a, int b, std::function<bool(int, int)> op) {
        return op(a, b);
    }

    //determines if a node is allowed through the filter or not
    bool filterPolicy::isAdmittable(std::variant<std::string, size_t> currentPosition) {
        if (this->aFilterType == filterType::indexFilter) {

            std::map<std::string, std::function<bool(int, int)>> ops = {
                    {"==", [](int a, int b) { return a == b; }},
                    {"!=", [](int a, int b) { return a != b; }},
                    {"<", [](int a, int b) { return a < b; }},
                    {">", [](int a, int b) { return a > b; }},
                    {"<=", [](int a, int b) { return a <= b; }},
                    {">=", [](int a, int b) { return a >= b; }},
            };

            bool result = compare(std::get<size_t>(currentPosition), std::get<int>(filterCondition), ops[anOperation]);
            return result;

            
        }

        if (this->aFilterType == filterType::keyNameFilter) {
            std::string current = std::get<std::string>(currentPosition);
            std::string filter = std::get<std::string>(filterCondition);
            if (std::string::npos == current.find(filter)) {
                return false;
            }
            else {
                return true;
            }
        }

        if (this->aFilterType == filterType::none) {
            return true;
        }
        return false; // just so warning will go away, it will never hit this
    }

}



