/*
 * Modern C++ JSON schema validator
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *
 * Copyright (c) 2016 Patrick Boettcher <patrick.boettcher@posteo.de>.
 *
 * Permission is hereby  granted, free of charge, to any  person obtaining a
 * copy of this software and associated  documentation files (the "Software"),
 * to deal in the Software  without restriction, including without  limitation
 * the rights to  use, copy,  modify, merge,  publish, distribute,  sublicense,
 * and/or  sell copies  of  the Software,  and  to  permit persons  to  whom
 * the Software  is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS
 * OR IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN
 * NO EVENT  SHALL THE AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY
 * CLAIM,  DAMAGES OR  OTHER LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NLOHMANN_JSON_SCHEMA_HPP__
#define NLOHMANN_JSON_SCHEMA_HPP__

#ifdef _WIN32
#    if defined(JSON_SCHEMA_VALIDATOR_EXPORTS)
#        define JSON_SCHEMA_VALIDATOR_API __declspec(dllexport)
#    elif defined(JSON_SCHEMA_VALIDATOR_IMPORTS)
#        define JSON_SCHEMA_VALIDATOR_API __declspec(dllimport)
#    else
#        define JSON_SCHEMA_VALIDATOR_API
#    endif
#else
#    define JSON_SCHEMA_VALIDATOR_API
#endif
#ifdef JSON_SCHEMA_BOOST_REGEX
 #include <boost/regex.hpp>
 #define REGEX_NAMESPACE boost
#elif defined(JSON_SCHEMA_NO_REGEX)
 #define NO_STD_REGEX
#else
 #include <regex>
 #define REGEX_NAMESPACE std
#endif

#include "nlohmann/json.hpp"
#include <set>
// make yourself a home - welcome to nlohmann's namespace
namespace nlohmann
{

// a class representing a JSON-pointer RFC6901
//
// examples of JSON pointers
//
//   #     - root of the current document
//   #item - refers to the object which is identified ("id") by `item`
//           in the current document
//   #/path/to/element
//         - refers to the element in /path/to from the root-document
//
//
// The json_pointer-class stores everything in a string, which might seem bizarre
// as parsing is done from a string to a string, but from_string() is also
// doing some formatting.
//
// TODO
//   ~ and %  - codec
//   needs testing and clarification regarding the '#' at the beginning

class local_json_pointer
{
	std::string str_;

	void from_string(const std::string &r);

public:
	local_json_pointer(const std::string &s = "")
	{
		from_string(s);
	}

	void append(const std::string &elem)
	{
		str_.append(elem);
	}

	const std::string &to_string() const
	{
		return str_;
	}
};

// A class representing a JSON-URI for schemas derived from
// section 8 of JSON Schema: A Media Type for Describing JSON Documents
// draft-wright-json-schema-00
//
// New URIs can be derived from it using the derive()-method.
// This is useful for resolving refs or subschema-IDs in json-schemas.
//
// This is done implement the requirements described in section 8.2.
//
class JSON_SCHEMA_VALIDATOR_API json_uri
{
	std::string urn_;

	std::string proto_;
	std::string hostname_;
	std::string path_;
	local_json_pointer pointer_;

protected:
	// decodes a JSON uri and replaces all or part of the currently stored values
	void from_string(const std::string &uri);

	std::tuple<std::string, std::string, std::string, std::string, std::string> tie() const
	{
		return std::tie(urn_, proto_, hostname_, path_, pointer_.to_string());
	}

public:
	json_uri(const std::string &uri)
	{
		from_string(uri);
	}

	const std::string protocol() const { return proto_; }
	const std::string hostname() const { return hostname_; }
	const std::string path() const { return path_; }
	const local_json_pointer pointer() const { return pointer_; }

	const std::string url() const;

	// decode and encode strings for ~ and % escape sequences
	static std::string unescape(const std::string &);
	static std::string escape(const std::string &);

	// create a new json_uri based in this one and the given uri
	// resolves relative changes (pathes or pointers) and resets part if proto or hostname changes
	json_uri derive(const std::string &uri) const
	{
		json_uri u = *this;
		u.from_string(uri);
		return u;
	}

	// append a pointer-field to the pointer-part of this uri
	json_uri append(const std::string &field) const
	{
		json_uri u = *this;
		u.pointer_.append("/" + field);
		return u;
	}

	std::string to_string() const;

	friend bool operator<(const json_uri &l, const json_uri &r)
	{
		return l.tie() < r.tie();
	}

	friend bool operator==(const json_uri &l, const json_uri &r)
	{
		return l.tie() == r.tie();
	}

	friend std::ostream &operator<<(std::ostream &os, const json_uri &u);
};

namespace json_schema_draft4
{
extern json draft4_schema_builtin;

class JSON_SCHEMA_VALIDATOR_API json_validator
{
	std::vector<std::shared_ptr<json>> schema_store_;
	std::shared_ptr<json> root_schema_;
	std::function<void(const json_uri &, json &)> schema_loader_ = nullptr;
	std::function<void(const std::string &, const std::string &)> format_check_ = nullptr;

	std::map<json_uri, const json *> schema_refs_;

	void validate(const json &instance, const json &schema_, const std::string &name);
	void validate_array(const json &instance, const json &schema_, const std::string &name);
	void validate_object(const json &instance, const json &schema_, const std::string &name);
    void validate_string(const json &instance, const json &schema, const std::string &name);

	void insert_schema(const json &input, const json_uri &id);

public:
	json_validator(std::function<void(const json_uri &, json &)> loader = nullptr,
	               std::function<void(const std::string &, const std::string &)> format = nullptr)
	    : schema_loader_(loader), format_check_(format)
	{
	}

	// insert and set a root-schema
	void set_root_schema(const json &);

	// validate a json-document based on the root-schema
	void validate(const json &instance);
};

json draft4_schema_builtin = R"( {
    "id": "http://json-schema.org/draft-04/schema#",
    "$schema": "http://json-schema.org/draft-04/schema#",
    "description": "Core schema meta-schema",
    "definitions": {
        "schemaArray": {
            "type": "array",
            "minItems": 1,
            "items": { "$ref": "#" }
        },
        "positiveInteger": {
            "type": "integer",
            "minimum": 0
        },
        "positiveIntegerDefault0": {
            "allOf": [ { "$ref": "#/definitions/positiveInteger" }, { "default": 0 } ]
        },
        "simpleTypes": {
            "enum": [ "array", "boolean", "integer", "null", "number", "object", "string" ]
        },
        "stringArray": {
            "type": "array",
            "items": { "type": "string" },
            "minItems": 1,
            "uniqueItems": true
        }
    },
    "type": "object",
    "properties": {
        "id": {
            "type": "string",
            "format": "uri"
        },
        "$schema": {
            "type": "string",
            "format": "uri"
        },
        "title": {
            "type": "string"
        },
        "description": {
            "type": "string"
        },
        "default": {},
        "multipleOf": {
            "type": "number",
            "minimum": 0,
            "exclusiveMinimum": true
        },
        "maximum": {
            "type": "number"
        },
        "exclusiveMaximum": {
            "type": "boolean",
            "default": false
        },
        "minimum": {
            "type": "number"
        },
        "exclusiveMinimum": {
            "type": "boolean",
            "default": false
        },
        "maxLength": { "$ref": "#/definitions/positiveInteger" },
        "minLength": { "$ref": "#/definitions/positiveIntegerDefault0" },
        "pattern": {
            "type": "string",
            "format": "regex"
        },
        "additionalItems": {
            "anyOf": [
                { "type": "boolean" },
                { "$ref": "#" }
            ],
            "default": {}
        },
        "items": {
            "anyOf": [
                { "$ref": "#" },
                { "$ref": "#/definitions/schemaArray" }
            ],
            "default": {}
        },
        "maxItems": { "$ref": "#/definitions/positiveInteger" },
        "minItems": { "$ref": "#/definitions/positiveIntegerDefault0" },
        "uniqueItems": {
            "type": "boolean",
            "default": false
        },
        "maxProperties": { "$ref": "#/definitions/positiveInteger" },
        "minProperties": { "$ref": "#/definitions/positiveIntegerDefault0" },
        "required": { "$ref": "#/definitions/stringArray" },
        "additionalProperties": {
            "anyOf": [
                { "type": "boolean" },
                { "$ref": "#" }
            ],
            "default": {}
        },
        "definitions": {
            "type": "object",
            "additionalProperties": { "$ref": "#" },
            "default": {}
        },
        "properties": {
            "type": "object",
            "additionalProperties": { "$ref": "#" },
            "default": {}
        },
        "patternProperties": {
            "type": "object",
            "additionalProperties": { "$ref": "#" },
            "default": {}
        },
        "dependencies": {
            "type": "object",
            "additionalProperties": {
                "anyOf": [
                    { "$ref": "#" },
                    { "$ref": "#/definitions/stringArray" }
                ]
            }
        },
        "enum": {
            "type": "array",
            "minItems": 1,
            "uniqueItems": true
        },
        "type": {
            "anyOf": [
                { "$ref": "#/definitions/simpleTypes" },
                {
                    "type": "array",
                    "items": { "$ref": "#/definitions/simpleTypes" },
                    "minItems": 1,
                    "uniqueItems": true
                }
            ]
        },
        "allOf": { "$ref": "#/definitions/schemaArray" },
        "anyOf": { "$ref": "#/definitions/schemaArray" },
        "oneOf": { "$ref": "#/definitions/schemaArray" },
        "not": { "$ref": "#" }
    },
    "dependencies": {
        "exclusiveMaximum": [ "maximum" ],
        "exclusiveMinimum": [ "minimum" ]
    },
    "default": {}
} )"_json;

	class resolver
	{
		void resolve(json &schema, json_uri id)
		{
			// look for the id-field in this schema
			auto fid = schema.find("id");

			// found?
			if (fid != schema.end() &&
				fid.value().type() == json::value_t::string)
				id = id.derive(fid.value()); // resolve to a full id with URL + path based on the parent

			// already existing - error
			if (schema_refs.find(id) != schema_refs.end())
				throw std::invalid_argument("schema " + id.to_string() + " already present in local resolver");

			// store a raw pointer to this (sub-)schema referenced by its absolute json_uri
			// this (sub-)schema is part of a schema stored inside schema_store_ so we can use the a raw-pointer-ref
			schema_refs[id] = &schema;

			for (auto i = schema.begin(), end = schema.end(); i != end; ++i) {
				// FIXME: this inhibits the user adding properties with the key "default"
				if (i.key() == "default") /* default value can be objects, but are not schemas */
					continue;

				switch (i.value().type()) {

					case json::value_t::object: // child is object, it is a schema
						resolve(i.value(), id.append(json_uri::escape(i.key())));
						break;

					case json::value_t::array: {
						std::size_t index = 0;
						auto child_id = id.append(json_uri::escape(i.key()));
						for (auto &v : i.value()) {
							if (v.type() == json::value_t::object) // array element is object
								resolve(v, child_id.append(std::to_string(index)));
							index++;
						}
					} break;

					case json::value_t::string:
						if (i.key() == "$ref") {
							json_uri ref = id.derive(i.value());
							i.value() = ref.to_string();
							refs.insert(ref);
						}
						break;

					default:
						break;
				}
			}
		}

		std::set<json_uri> refs;

	public:
		std::set<json_uri> undefined_refs;

		std::map<json_uri, const json *> schema_refs;

		resolver(json &schema, json_uri id)
		{
			// if schema has an id use it as name and to retrieve the namespace (URL)
			auto fid = schema.find("id");
			if (fid != schema.end())
				id = id.derive(fid.value());

			resolve(schema, id);

			// refs now contains all references
			//
			// local references should be resolvable inside the same URL
			//
			// undefined_refs will only contain external references
			for (auto r : refs) {
				if (schema_refs.find(r) == schema_refs.end()) {
					if (r.url() == id.url()) // same url means referencing a sub-schema
						// of the same document, which has not been found
						throw std::invalid_argument("sub-schema " + r.pointer().to_string() +
													" in schema " + id.to_string() + " not found");
					undefined_refs.insert(r.url());
				}
			}
		}
	};

	void validate_type(const json &schema, const std::string &expected_type, const std::string &name)
	{
		const auto &type_it = schema.find("type");
		if (type_it == schema.end())
			/* TODO something needs to be done here, I think */
			return;

		const auto &type_instance = type_it.value();

		// any of the types in this array
		if (type_instance.type() == json::value_t::array) {
			if ((std::find(type_instance.begin(),
						   type_instance.end(),
						   expected_type) != type_instance.end()) ||
				(expected_type == "integer" &&
				 std::find(type_instance.begin(),
						   type_instance.end(),
						   "number") != type_instance.end()))
				return;

			std::ostringstream s;
			s << expected_type << " is not any of " << type_instance << " for " << name;
			throw std::invalid_argument(s.str());

		} else { // type_instance is a string
			if (type_instance == expected_type ||
				(type_instance == "number" && expected_type == "integer"))
				return;

			throw std::invalid_argument(name + " is " + expected_type +
										", but required type is " + type_instance.get<std::string>());
		}
	}

	void validate_enum(const json &instance, const json &schema, const std::string &name)
	{
		const auto &enum_value = schema.find("enum");
		if (enum_value == schema.end())
			return;

		if (std::find(enum_value.value().begin(), enum_value.value().end(), instance) != enum_value.value().end())
			return;

		std::ostringstream s;
		s << "invalid enum-value '" << instance << "' "
		  << "for instance '" << name << "'. Candidates are " << enum_value.value() << ".";

		throw std::invalid_argument(s.str());
	}

	void validate_boolean(const json & /*instance*/, const json &schema, const std::string &name)
	{
		validate_type(schema, "boolean", name);
	}

	template <class T>
	bool violates_numeric_maximum(T max, T value, bool exclusive)
	{
		if (exclusive)
			return value >= max;

		return value > max;
	}

	template <class T>
	bool violates_numeric_minimum(T min, T value, bool exclusive)
	{
		if (exclusive)
			return value <= min;

		return value < min;
	}

// multipleOf - if the rest of the division is 0 -> OK
	bool violates_multiple_of(json::number_float_t x, json::number_float_t y)
	{
		json::number_integer_t n = static_cast<json::number_integer_t>(x / y);
		double res = (x - n * y);
		return fabs(res) > std::numeric_limits<json::number_float_t>::epsilon();
	}

	template <class T>
	void validate_numeric(const json &instance, const json &schema, const std::string &name)
	{
		T value = instance;

		if (value != 0) { // zero is multiple of everything
			const auto &multipleOf = schema.find("multipleOf");

			if (multipleOf != schema.end()) {
				double multiple = multipleOf.value();
				double value_float = value;

				if (violates_multiple_of(value_float, multiple))
					throw std::out_of_range(name + " is not a multiple of " + std::to_string(multiple));
			}
		}

		const auto &maximum = schema.find("maximum");
		if (maximum != schema.end()) {
			T maxi = maximum.value();

			const auto &excl = schema.find("exclusiveMaximum");
			bool exclusive = (excl != schema.end()) ? excl.value().get<bool>() : false;

			if (violates_numeric_maximum<T>(maxi, value, exclusive))
				throw std::out_of_range(name + " exceeds maximum of " + std::to_string(maxi));
		}

		const auto &minimum = schema.find("minimum");
		if (minimum != schema.end()) {
			T mini = minimum.value();

			const auto &excl = schema.find("exclusiveMinimum");
			bool exclusive = (excl != schema.end()) ? excl.value().get<bool>() : false;

			if (violates_numeric_minimum<T>(mini, value, exclusive))
				throw std::out_of_range(name + " is below minimum of " + std::to_string(mini));
		}
	}

	void validate_integer(const json &instance, const json &schema, const std::string &name)
	{
		validate_type(schema, "integer", name);
		//TODO: Validate schema values are json::value_t::number_integer/unsigned?

		validate_numeric<int64_t>(instance, schema, name);
	}

	bool is_unsigned(const json &schema)
	{
		const auto &minimum = schema.find("minimum");

		// Number is expected to be unsigned if a minimum >= 0 is set
		return minimum != schema.end() && minimum.value() >= 0;
	}

	void validate_unsigned(const json &instance, const json &schema, const std::string &name)
	{
		validate_type(schema, "integer", name);
		//TODO: Validate schema values are json::value_t::unsigned?

		//Is there a better way to determine whether an unsigned comparison should take place?
		if (is_unsigned(schema))
			validate_numeric<uint64_t>(instance, schema, name);
		else
			validate_numeric<int64_t>(instance, schema, name);
	}

	void validate_float(const json &instance, const json &schema, const std::string &name)
	{
		validate_type(schema, "number", name);
		//TODO: Validate schema values are json::value_t::number_float?

		validate_numeric<double>(instance, schema, name);
	}

	void validate_null(const json & /*instance*/, const json &schema, const std::string &name)
	{
		validate_type(schema, "null", name);
	}

void json_validator::insert_schema(const json &input, const json_uri &id)
{
	// allocate create a copy for later storage - if resolving reference works
	std::shared_ptr<json> schema = std::make_shared<json>(input);

	do {
		// resolve all local schemas and references
		resolver r(*schema, id);

		// check whether all undefined schema references can be resolved with existing ones
		std::set<json_uri> undefined;
		for (auto &ref : r.undefined_refs)
			if (schema_refs_.find(ref) == schema_refs_.end()) // exact schema reference not found
				undefined.insert(ref);

		if (undefined.size() == 0) { // no undefined references
			// now insert all schema-references
			// check whether all schema-references are new
			for (auto &sref : r.schema_refs) {
				if (schema_refs_.find(sref.first) != schema_refs_.end())
					throw std::invalid_argument("schema " + sref.first.to_string() + " already present in validator.");
			}
			// no undefined references and no duplicated schema - store the schema
			schema_store_.push_back(schema);

			// and insert all references
			schema_refs_.insert(r.schema_refs.begin(), r.schema_refs.end());

			break;
		}

		if (schema_loader_ == nullptr)
			throw std::invalid_argument("schema contains undefined references to other schemas, needed schema-loader.");

		for (auto undef : undefined) {
			json ext;

			// check whether a recursive-call has already insert this schema in the meantime
			if (schema_refs_.find(undef) != schema_refs_.end())
				continue;

			schema_loader_(undef, ext);
			insert_schema(ext, undef.url()); // recursively call insert_schema to fill in new external references
		}
	} while (1);

	// store the document root-schema
	if (id == json_uri("#"))
		root_schema_ = schema;
}

void json_validator::validate(const json &instance)
{
	if (root_schema_ == nullptr)
		throw std::invalid_argument("no root-schema has been inserted. Cannot validate an instance without it.");

	validate(instance, *root_schema_, "root");
}

void json_validator::set_root_schema(const json &schema)
{
	insert_schema(schema, json_uri("#"));
}

void json_validator::validate(const json &instance, const json &schema_, const std::string &name)
{
	const json *schema = &schema_;

	// $ref resolution
	do {
		const auto &ref = schema->find("$ref");
		if (ref == schema->end())
			break;

		auto it = schema_refs_.find(ref.value().get<std::string>());

		if (it == schema_refs_.end())
			throw std::invalid_argument("schema reference " + ref.value().get<std::string>() + " not found. Make sure all schemas have been inserted before validation.");

		schema = it->second;
	} while (1); // loop in case of nested refs

	// not
	const auto attr = schema->find("not");
	if (attr != schema->end()) {
		bool ok;

		try {
			validate(instance, attr.value(), name);
			ok = false;
		} catch (std::exception &) {
			ok = true;
		}
		if (!ok)
			throw std::invalid_argument("schema match for " + name + " but a not-match is defined by schema.");
		return; // return here - not cannot be mixed with based-schemas?
	}

	// allOf, anyOf, oneOf
	const json *combined_schemas = nullptr;
	enum {
		none,
		allOf,
		anyOf,
		oneOf
	} combine_logic = none;

	{
		const auto &attr = schema->find("allOf");
		if (attr != schema->end()) {
			combine_logic = allOf;
			combined_schemas = &attr.value();
		}
	}
	{
		const auto &attr = schema->find("anyOf");
		if (attr != schema->end()) {
			combine_logic = anyOf;
			combined_schemas = &attr.value();
		}
	}
	{
		const auto &attr = schema->find("oneOf");
		if (attr != schema->end()) {
			combine_logic = oneOf;
			combined_schemas = &attr.value();
		}
	}

	if (combine_logic != none) {
		std::size_t count = 0;
		std::ostringstream sub_schema_err;

		for (const auto &s : *combined_schemas) {
			try {
				validate(instance, s, name);
				count++;
			} catch (std::exception &e) {
				sub_schema_err << "  one schema failed because: " << e.what() << "\n";

				if (combine_logic == allOf)
					throw std::out_of_range("At least one schema has failed for " + name + " where allOf them were requested.\n" + sub_schema_err.str());
			}
			if (combine_logic == oneOf && count > 1)
				throw std::out_of_range("More than one schema has succeeded for " + name + " where only oneOf them was requested.\n" + sub_schema_err.str());
		}
		if ((combine_logic == anyOf || combine_logic == oneOf) && count == 0)
			throw std::out_of_range("No schema has succeeded for " + name + " but anyOf/oneOf them should have worked.\n" + sub_schema_err.str());
	}

	// check (base) schema
	validate_enum(instance, *schema, name);

	switch (instance.type()) {
	case json::value_t::object:
		validate_object(instance, *schema, name);
		break;

	case json::value_t::array:
		validate_array(instance, *schema, name);
		break;

	case json::value_t::string:
		validate_string(instance, *schema, name);
		break;

	case json::value_t::number_unsigned:
		validate_unsigned(instance, *schema, name);
		break;

	case json::value_t::number_integer:
		validate_integer(instance, *schema, name);
		break;

	case json::value_t::number_float:
		validate_float(instance, *schema, name);
		break;

	case json::value_t::boolean:
		validate_boolean(instance, *schema, name);
		break;

	case json::value_t::null:
		validate_null(instance, *schema, name);
		break;

	default:
		assert(0 && "unexpected instance type for validation");
		break;
	}
}

void json_validator::validate_array(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "array", name);

	// maxItems
	const auto &maxItems = schema.find("maxItems");
	if (maxItems != schema.end())
		if (instance.size() > maxItems.value().get<size_t>())
			throw std::out_of_range(name + " has too many items.");

	// minItems
	const auto &minItems = schema.find("minItems");
	if (minItems != schema.end())
		if (instance.size() < minItems.value().get<size_t>())
			throw std::out_of_range(name + " has too few items.");

	// uniqueItems
	const auto &uniqueItems = schema.find("uniqueItems");
	if (uniqueItems != schema.end())
		if (uniqueItems.value().get<bool>() == true) {
			std::set<json> array_to_set;
			for (auto v : instance) {
				auto ret = array_to_set.insert(v);
				if (ret.second == false)
					throw std::out_of_range(name + " should have only unique items.");
			}
		}

	// items and additionalItems
	// default to empty schemas
	auto items_iter = schema.find("items");
	json items = {};
	if (items_iter != schema.end())
		items = items_iter.value();

	auto additionalItems_iter = schema.find("additionalItems");
	json additionalItems = {};
	if (additionalItems_iter != schema.end())
		additionalItems = additionalItems_iter.value();

	size_t i = 0;
	bool validation_done = false;

	for (auto &value : instance) {
		std::string sub_name = name + "[" + std::to_string(i) + "]";

		switch (items.type()) {

		case json::value_t::array:

			if (i < items.size())
				validate(value, items[i], sub_name);
			else {
				switch (additionalItems.type()) { // items is an array
					                                // we need to take into consideration additionalItems
				case json::value_t::object:
					validate(value, additionalItems, sub_name);
					break;

				case json::value_t::boolean:
					if (additionalItems.get<bool>() == false)
						throw std::out_of_range("additional values in array are not allowed for " + sub_name);
					else
						validation_done = true;
					break;

				default:
					break;
				}
			}

			break;

		case json::value_t::object: // items is a schema
			validate(value, items, sub_name);
			break;

		default:
			break;
		}
		if (validation_done)
			break;

		i++;
	}
}

void json_validator::validate_object(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "object", name);

	json properties = {};
	if (schema.find("properties") != schema.end())
		properties = schema["properties"];

#if 0
	// check for default values of properties
	// and insert them into this object, if they don't exists
	// works only for object properties for the moment
	if (default_value_insertion)
		for (auto it = properties.begin(); it != properties.end(); ++it) {

			const auto &default_value = it.value().find("default");
			if (default_value == it.value().end())
				continue; /* no default value -> continue */

			if (instance.find(it.key()) != instance.end())
				continue; /* value is present */

			/* create element from default value */
			instance[it.key()] = default_value.value();
		}
#endif
	// maxProperties
	const auto &maxProperties = schema.find("maxProperties");
	if (maxProperties != schema.end())
		if (instance.size() > maxProperties.value().get<size_t>())
			throw std::out_of_range(name + " has too many properties.");

	// minProperties
	const auto &minProperties = schema.find("minProperties");
	if (minProperties != schema.end())
		if (instance.size() < minProperties.value().get<size_t>())
			throw std::out_of_range(name + " has too few properties.");

	// additionalProperties
	enum {
		True,
		False,
		Object
	} additionalProperties = True;

	const auto &additionalPropertiesVal = schema.find("additionalProperties");
	if (additionalPropertiesVal != schema.end()) {
		if (additionalPropertiesVal.value().type() == json::value_t::boolean)
			additionalProperties = additionalPropertiesVal.value().get<bool>() == true ? True : False;
		else
			additionalProperties = Object;
	}

	// patternProperties
	json patternProperties = {};
	if (schema.find("patternProperties") != schema.end())
		patternProperties = schema["patternProperties"];

	// check all elements in object
	for (auto child = instance.begin(); child != instance.end(); ++child) {
		std::string child_name = name + "." + child.key();

		bool property_or_patternProperties_has_validated = false;
		// is this a property which is described in the schema
		const auto &object_prop = properties.find(child.key());
		if (object_prop != properties.end()) {
			// validate the element with its schema
			validate(child.value(), object_prop.value(), child_name);
			property_or_patternProperties_has_validated = true;
		}

		for (auto pp = patternProperties.begin();
		     pp != patternProperties.end(); ++pp) {
#ifndef NO_STD_REGEX
			REGEX_NAMESPACE::regex re(pp.key(), REGEX_NAMESPACE::regex::ECMAScript);

			if (REGEX_NAMESPACE::regex_search(child.key(), re)) {
				validate(child.value(), pp.value(), child_name);
				property_or_patternProperties_has_validated = true;
			}
#else
			// accept everything in case of a patternProperty
			property_or_patternProperties_has_validated = true;
			break;
#endif
		}

		if (property_or_patternProperties_has_validated)
			continue;

		switch (additionalProperties) {
		case True:
			break;

		case Object:
			validate(child.value(), additionalPropertiesVal.value(), child_name);
			break;

		case False:
			throw std::invalid_argument("unknown property '" + child.key() + "' in object '" + name + "'");
			break;
		};
	}

	// required
	const auto &required = schema.find("required");
	if (required != schema.end())
		for (const auto &element : required.value()) {
			if (instance.find(element) == instance.end()) {
				throw std::invalid_argument("required element '" + element.get<std::string>() +
				                            "' not found in object '" + name + "'");
			}
		}

	// dependencies
	const auto &dependencies = schema.find("dependencies");
	if (dependencies == schema.end())
		return;

	for (auto dep = dependencies.value().cbegin();
	     dep != dependencies.value().cend();
	     ++dep) {

		// property not present in this instance - next
		if (instance.find(dep.key()) == instance.end())
			continue;

		std::string sub_name = name + ".dependency-of-" + dep.key();

		switch (dep.value().type()) {

		case json::value_t::object:
			validate(instance, dep.value(), sub_name);
			break;

		case json::value_t::array:
			for (const auto &prop : dep.value())
				if (instance.find(prop) == instance.end())
					throw std::invalid_argument("failed dependency for " + sub_name + ". Need property " + prop.get<std::string>());
			break;

		default:
			break;
		}
	}
}

static std::size_t utf8_length(const std::string &s)
{
	size_t len = 0;
	for (const unsigned char &c : s)
		if ((c & 0xc0) != 0x80)
			len++;
	return len;
}

void json_validator::validate_string(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "string", name);

	// minLength
	auto attr = schema.find("minLength");
	if (attr != schema.end())
		if (utf8_length(instance) < attr.value().get<size_t>()) {
			std::ostringstream s;
			s << "'" << name << "' of value '" << instance << "' is too short as per minLength ("
			  << attr.value() << ")";
			throw std::out_of_range(s.str());
		}

	// maxLength
	attr = schema.find("maxLength");
	if (attr != schema.end())
		if (utf8_length(instance) > attr.value().get<size_t>()) {
			std::ostringstream s;
			s << "'" << name << "' of value '" << instance << "' is too long as per maxLength ("
			  << attr.value() << ")";
			throw std::out_of_range(s.str());
		}

#ifndef NO_STD_REGEX
	// pattern
	attr = schema.find("pattern");
	if (attr != schema.end()) {
		REGEX_NAMESPACE::regex re(attr.value().get<std::string>(), REGEX_NAMESPACE::regex::ECMAScript);
		if (!REGEX_NAMESPACE::regex_search(instance.get<std::string>(), re))
			throw std::invalid_argument(instance.get<std::string>() + " does not match regex pattern: " + attr.value().get<std::string>() + " for " + name);
	}
#endif

	// format
	attr = schema.find("format");
	if (attr != schema.end()) {
		if (format_check_ == nullptr)
			throw std::logic_error("A format checker was not provided but a format-attribute for this string is present. " +
			                       name + " cannot be validated for " + attr.value().get<std::string>());
		format_check_(attr.value(), instance);
	}
}



} // json_schema_draft4


void local_json_pointer::from_string(const std::string &r)
{
	str_ = "#";

	if (r.size() == 0)
		return;

	if (r[0] != '#')
		throw std::invalid_argument("not a valid JSON pointer - missing # at the beginning");

	if (r.size() == 1)
		return;

	std::size_t pos = 1;

	do {
		std::size_t next = r.find('/', pos + 1);
		str_.append(r.substr(pos, next - pos));
		pos = next;
	} while (pos != std::string::npos);
}

void json_uri::from_string(const std::string &uri)
{
	// if it is an urn take it as it is - maybe there is more to be done
	if (uri.find("urn:") == 0) {
		urn_ = uri;
		return;
	}

	std::string pointer = "#"; // default pointer is the root

	// first split the URI into URL and JSON-pointer
	auto pointer_separator = uri.find('#');
	if (pointer_separator != std::string::npos) // and extract the JSON-pointer-string if found
		pointer = uri.substr(pointer_separator);

	// the rest is an URL
	std::string url = uri.substr(0, pointer_separator);
	if (url.size()) { // if an URL is part of the URI

		std::size_t pos = 0;
		auto proto = url.find("://", pos);
		if (proto != std::string::npos) { // extract the protocol
			proto_ = url.substr(pos, proto - pos);
			pos = 3 + proto; // 3 == "://"

			auto hostname = url.find("/", pos);
			if (hostname != std::string::npos) { // and the hostname (no proto without hostname)
				hostname_ = url.substr(pos, hostname - pos);
				pos = hostname;
			}
		}

		// the rest is the path
		auto path = url.substr(pos);
		if (path[0] == '/') // if it starts with a / it is root-path
			path_ = path;
		else // otherwise it is a subfolder
			path_.append(path);

		pointer_ = local_json_pointer("");
	}

	if (pointer.size() > 0)
		pointer_ = pointer;
}

const std::string json_uri::url() const
{
	std::stringstream s;

	if (proto_.size() > 0)
		s << proto_ << "://";

	s << hostname_
	  << path_;

	return s.str();
}

std::string json_uri::to_string() const
{
	std::stringstream s;

	s << urn_
	  << url()
	  << pointer_.to_string();

	return s.str();
}

std::ostream &operator<<(std::ostream &os, const json_uri &u)
{
	return os << u.to_string();
}

std::string json_uri::unescape(const std::string &src)
{
	std::string l = src;
	std::size_t pos = src.size() - 1;

	do {
		pos = l.rfind('~', pos);

		if (pos == std::string::npos)
			break;

		if (pos < l.size() - 1) {
			switch (l[pos + 1]) {
			case '0':
				l.replace(pos, 2, "~");
				break;

			case '1':
				l.replace(pos, 2, "/");
				break;

			default:
				break;
			}
		}

		if (pos == 0)
			break;
		pos--;
	} while (pos != std::string::npos);

	// TODO - percent handling

	return l;
}

std::string json_uri::escape(const std::string &src)
{
	std::vector<std::pair<std::string, std::string>> chars = {
	    {"~", "~0"},
	    {"/", "~1"},
	    {"%", "%25"}};

	std::string l = src;

	for (const auto &c : chars) {
		std::size_t pos = 0;
		do {
			pos = l.find(c.first, pos);
			if (pos == std::string::npos)
				break;
			l.replace(pos, 1, c.second);
			pos += c.second.size();
		} while (1);
	}

	return l;
}
} // nlohmann


#endif /* NLOHMANN_JSON_SCHEMA_HPP__ */
