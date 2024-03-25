
## 1. The Parser
NOTE: THIS was not made by me, it was made by my instructor, Rick Gessner

Before implementing any functionality, 
we firstly need to be able to **read/write** JSON files and **parse** the JSON syntax.

### Using the parser

The `Model`  inherits from `JSONListener` and implements the pure virtual methods.
These listener methods will be called by the parser each time a new JSON element is parsed and such that the element is added  to the `Model`.


## 2. The In-Memory Model


For something as dynamic and flexible as JSON, I utilized a form of "graph".
This graph stores all the various JSON elements/values.


Each value/element in the JSON can be considered a "node" in the graph. 
To paint a clearer picture of what I mean, let's take a look at the following example JSON:

```json
{
  "stringNode": "Hello!",
  "numberNode": 42,
  "objectNode": {
    "innerStringNode": "Hello again!",
    "innerBoolNode": true,
    "innerListNode": [
      "item1", "item2", "item3"
    ]
  }
}
```

Let's start at the very beginning. The first character we see is the open-curly-brace `{`.
This character is associated with the start of an `object` type. 

Since the root element of JSON is *always* an `object`, we know our model/graph will always start with an `object`.

So we can add an object to the root of our model/graph:

```
node(object) -> ...
```

Let's look inside this root `object` at the key-value pairs. 
The first one is `"stringNode": "Hello!"`, 
where the key is `"stringNode"` and the value is the string `"Hello!"`.

This means that inside our `object` node, we need to store this and all other key-value pairs,
where each key is a string and each value is another node in our graph.

```
node(object) -> {
    key: "stringNode", value -> node(string)
    key: "numberNode", value -> node(int)
    key: "objectNode", value -> ???
}
```

Now that another  `"objectNode"` has been encountered, the graph is updated as such: 
the value is a node like the other key-value pairs, just this time the node stores a value of type `object`:

```
node(object) -> {
    key: "stringNode", value -> node(string)
    key: "numberNode", value -> node(int)
    key: "objectNode", value -> node(object) -> {
        ...
    }
}
```

This same logic can be extended to lists and further embedded objects. 

### Using `JSONListener` and Creating the Model


First method:

```cpp
// Add basic key-value data types (null, bool, number, or string)
bool addKeyValuePair(const std::string& aKey, const std::string& aValue, Element aType);
```

Like the comment says, this method will be called by the `JSONParser` each time it encounters a key-value pair
where the value is a basic type (either `null`, `boolean`, `number`, or `string`).

To help differentiate `string`s from other values, the `Element aType` argument will be `Element::quoted` for `string`s, and `Element::constant` otherwise.

This method will only be called when we are inside an `object` container.

```cpp
// Add values to a list
bool addItem(const std::string& aValue, Element aType);
```

If we are within a list rather than an object, 
then there should **not** be any storing of key-value pairs, just values.
This is what the `addItem()` method is for.

This method will only be called when inside a `list` container.

```cpp
// Start of an object or list container ('{' or '[')
bool openContainer(const std::string& aKey, Element aType);
```

When the parser encounters the start of an object or list, `openContainer()` will be called.
This uses the `Element aType` argument to differentiate between the two.

If within an object, then this container will have a key associate with it, hence the
`const std::string& aKey` argument. However, if within a list, then this argument will be an empty string.

```cpp
// End of an object or list container ('}' or ']')
bool closeContainer(const std::string& aKey, Element aType);
```

Lastly, the `closeContainer()` method. As you may expect, this method is called at the end of an
object or list. 


## 3. Query Language

Now that an in-memory model has been created, a way to interact with it is necessary. 

Below I describe the query API/syntax. These methods are implemented within the `ModelQuery` class.

### Traversal

Used to navigate between nodes in the JSON structure.

```cpp
ModelQuery& select(const std::string& aQuery);
```
Traverse the JSON tree.

- The `aQuery` argument is a string that contains a series of **keys** (for key-value pairs within objects) and/or **indices** (for elements in lists).
  - The indices start counting from `0`, just like indexing an array in C++.
  - All keys will be surrounded by single quotes `'`.
  - All indices will **not** be surrounded by single quotes.
  - The different keys/indices will be separated by periods `.`.
- Think of this command as moving some sort of pointer that points to a JSON element/node.
- Ex: `select("'firstNode'.'secondNode'.5")`. This will navigate to the element with `'firstNode'` as the key, then the child that has the key of `'secondNode'` and lastly, the child that has the index of `5`, as `'secondNode'` contains a list.
- If `select()` is called with invalid keys/indices (meaning they don't exist within the JSON), these errors are printed to the terminal

### Filtering

Used to 'skip' or 'ignore' certain JSON elements.

```cpp
ModelQuery& filter(const std::string& aQuery);
```
Filter certain elements within the currently selected container (list or object).

- The `aQuery` is a string which contains some sort of comparison. This comparison can be applied to the element's key or index.
- The filters do not apply to nested children elements, only the elements within the currently selected container (examples below).

Filtering by key: `filter("key {action} '{value}'")`.
- Actions: `contains`
- Ex: `filter("key contains 'hello'")`: Will only include JSON elements where the key contains the substring `"hello"`.
- This will only apply to key-value pairs within objects.

Filtering by index: `filter("index {comparison} {value}")`
- Comparisons: All 6 (`<`, `>`, `<=`, `>=`, `==`, `!=`) comparisons.
- Ex: `filter("index > 2")`: Will only include JSON elements where the index is greater than 2. 
- This only applies to elements within lists.

### Consuming

After navigating and filtering the JSON, 
these commands will be used at the end of the command chain to actually return some data/values.

- these methods return actual values instead of `ModelQuery&`.

```cpp
size_t count();
```
- Counts number of child elements of the currently selected node.

```cpp
double sum();
```
- Sum values in a list. This will only be used in lists of numbers. 


```cpp
std::optional<std::string> get(const std::string& aKeyOrIndex);
```
- Get values of a certain key-value pair or value at index of a list. 
If the value is a list/object, be sure to return all the elements (view examples below).
- Passing `"*"` as the argument will return all child nodes of the currently selected node (see examples below). 

### Example:

```json
{
  "sammy": {
    "name": "Sammy",
    "username": "SammyShark",
    "online": true,
    "followers": {
      "avg-age": 25,
      "count": 100
    }
  },
  "items": [
    {"key1": "100"}
  ],
  "list": [
    100, 250, 3000
  ]
}
```

- `select("'sammy'").get("'username'")`: Should result in `"SammyShark"`.

- `select("'sammy'.'uh_oh'").get("'nope'")`: Should result in `std::nullopt`, as the JSON does not contain these elements.

- `select("'list'").sum()`: Should result in `3350`.

- `select("'list'").filter("index >= 1").sum()`: Should skip the value at index `0` and result in `3250`.

- `select("'sammy'").count()`: There are four elements within the `"sammy"` object (`"name"`,`"username"`, `"online"`, `"followers"`), this should return `4`.

- `select("'sammy'.'followers'").get("*")`: Should return `{"avg-age":25,"count":100}`.

- `select("'sammy'.'followers'").get("'count'")`: Should return `100`.

- `select("'items'").get("0")`: Should return `{"key1":"100"}`.

- `select("'list'").filter("index < 2").get("*")`: Should return `[100,250]`.

- `select("'sammy'").filter("key contains 'name').get("*")`: Should return `{"name":"Sammy","username":"SammyShark"}` since these are the only key-value pairs within the `"sammy"` object that have keys containing the substring `"name"`.

These query commands should be called on the `ModelQuery` object. Here is an example:

```cpp
// Build the model...
std::fstream theJsonFile(getWorkingDirectoryPath() + "/Resources/classroom.json");
ECE141::JSONParser theParser(theJsonFile);
ECE141::Model theModel;
theParser.parse(&theModel);

// Query the model...
auto theQuery = theModel.createQuery();
auto theResult = theQuery.select("'sammy'").get("'username'");
```

#### Notes:


- The order of these commands will always be the same: `select`, then optionally `filter`, and lastly a consumer (`sum`, `count`, or `get`).

- Multiple filters are not supports

- Filtering will only happen on collections (lists or objects). 



