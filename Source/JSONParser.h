//
//  JSONParser.h
//  Assignment4
//
//  Created by rick gessner on 2/16/20.
//

#pragma once

#include <iostream>
#include <stack>

namespace JSONProc {

	// Type of the value
	enum class Element {
		object, array, closing, constant, quoted, unknown
	};

	class JSONListener {
	public:
		// Remember, all virtual objects need a virtual destructor
		virtual ~JSONListener() = default;

		// Add basic key-value data types (bool, number, or string)
		virtual bool addKeyValuePair(const std::string& aKey, const std::string& aValue, Element aType) = 0;

		// Add values to a list
		virtual bool addItem(const std::string& aValue, Element aType) = 0;

		// Start of an object or list container ('{' or '[')
		virtual bool openContainer(const std::string& aKey, Element aType) = 0;

		// End of an object or list container ('}' or ']')
		virtual bool closeContainer(const std::string& aKey, Element aType) = 0;

	};

	//--------------------------------------------
	// Used for parsing to keep track of state
	struct JSONState {
		JSONState(const std::string& aKey, Element aType = Element::object) : key(aKey), type(aType) {}

		JSONState(const JSONState &aCopy)
			: key(aCopy.key), type(aCopy.type) {}

		std::string key;
		Element type;
	};

	//--------------------------------------------
	class JSONParser {
	public:
		JSONParser(std::istream &anInputStream);

		bool parse(JSONListener *aListener = nullptr);

	protected:
		bool willParse(JSONListener *aListener = nullptr);
		bool didParse(bool aStatus);

		bool parseElements(char aChar, JSONListener *aListener);

		bool handleOpenContainer(Element aType, JSONListener *aListener);
		bool handleCloseContainer(Element aType, JSONListener *);

		std::stack<JSONState> states;
		std::string tempKey;
		std::istream &input;
	};

}
