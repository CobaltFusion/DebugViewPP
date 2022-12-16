//
//  value.h
//  docopt
//
//  Created by Jared Grubb on 2013-10-14.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#ifndef docopt__value_h_
#define docopt__value_h_

#include <string>
#include <vector>
#include <functional> // std::hash
#include <iosfwd>
#include <stdexcept>

namespace docopt {

	enum class Kind {
		Empty,
		Bool,
		Long,
		String,
		StringList
	};

	/// A generic type to hold the various types that can be produced by docopt.
	///
	/// This type can be one of: {bool, long, string, vector<string>}, or empty.
	struct value {
		/// An empty value
		value() {}

		value(std::string);
		value(std::vector<std::string>);
		
		explicit value(bool);
		explicit value(long);
		explicit value(int v) : value(static_cast<long>(v)) {}

		~value();
		value(value const&);
		value(value&&) noexcept;
		value& operator=(value const&);
		value& operator=(value&&) noexcept;

		Kind kind() const { return kind_; }
		
		// Test if this object has any contents at all
		explicit operator bool() const { return kind_ != Kind::Empty; }
		
		// Test the type contained by this value object
		bool isBool()       const { return kind_==Kind::Bool; }
		bool isString()     const { return kind_==Kind::String; }
		bool isLong()       const { return kind_==Kind::Long; }
		bool isStringList() const { return kind_==Kind::StringList; }

		// Throws std::invalid_argument if the type does not match
		bool asBool() const;
		long asLong() const;
		std::string const& asString() const;
		std::vector<std::string> const& asStringList() const;

		size_t hash() const noexcept;
		
		friend bool operator==(value const&, value const&);
		friend bool operator!=(value const&, value const&);

	private:
		union Variant {
			Variant() {}
			~Variant() {  /* do nothing; will be destroyed by ~value */ }
			
			bool boolValue;
			long longValue;
			std::string strValue;
			std::vector<std::string> strList;
		};
		
		static const char* kindAsString(Kind kind) {
			switch (kind) {
				case Kind::Empty: return "empty";
				case Kind::Bool: return "bool";
				case Kind::Long: return "long";
				case Kind::String: return "string";
				case Kind::StringList: return "string-list";
			}
			return "unknown";
		}

		void throwIfNotKind(Kind expected) const {
			if (kind_ == expected)
				return;

			std::string error = "Illegal cast to ";
			error += kindAsString(expected);
			error += "; type is actually ";
			error += kindAsString(kind_);
			throw std::runtime_error(std::move(error));
		}

		Kind kind_ = Kind::Empty;
		Variant variant_ {};
	};

	/// Write out the contents to the ostream
	DOCOPT_API std::ostream& operator<<(std::ostream&, value const&);
}

namespace std {
	template <>
	struct hash<docopt::value> {
		size_t operator()(docopt::value const& val) const noexcept {
			return val.hash();
		}
	};
}

namespace docopt {
	inline
	value::value(bool v)
	: kind_(Kind::Bool)
	{
		variant_.boolValue = v;
	}

	inline
	value::value(long v)
	: kind_(Kind::Long)
	{
		variant_.longValue = v;
	}

	inline
	value::value(std::string v)
	: kind_(Kind::String)
	{
		new (&variant_.strValue) std::string(std::move(v));
	}

	inline
	value::value(std::vector<std::string> v)
	: kind_(Kind::StringList)
	{
		new (&variant_.strList) std::vector<std::string>(std::move(v));
	}

	inline
	value::value(value const& other)
	: kind_(other.kind_)
	{
		switch (kind_) {
			case Kind::String:
				new (&variant_.strValue) std::string(other.variant_.strValue);
				break;

			case Kind::StringList:
				new (&variant_.strList) std::vector<std::string>(other.variant_.strList);
				break;

			case Kind::Bool:
				variant_.boolValue = other.variant_.boolValue;
				break;

			case Kind::Long:
				variant_.longValue = other.variant_.longValue;
				break;

			case Kind::Empty:
			default:
				break;
		}
	}

	inline
	value::value(value&& other) noexcept
	: kind_(other.kind_)
	{
		switch (kind_) {
			case Kind::String:
				new (&variant_.strValue) std::string(std::move(other.variant_.strValue));
				break;

			case Kind::StringList:
				new (&variant_.strList) std::vector<std::string>(std::move(other.variant_.strList));
				break;

			case Kind::Bool:
				variant_.boolValue = other.variant_.boolValue;
				break;

			case Kind::Long:
				variant_.longValue = other.variant_.longValue;
				break;

			case Kind::Empty:
			default:
				break;
		}
	}

	inline
	value::~value()
	{
		switch (kind_) {
			case Kind::String:
				variant_.strValue.~basic_string();
				break;

			case Kind::StringList:
				variant_.strList.~vector();
				break;

			case Kind::Empty:
			case Kind::Bool:
			case Kind::Long:
			default:
				// trivial dtor
				break;
		}
	}

	inline
	value& value::operator=(value const& other) {
		// make a copy and move from it; way easier.
		return *this = value{other};
	}

	inline
	value& value::operator=(value&& other) noexcept {
		// move of all the types involved is noexcept, so we dont have to worry about 
		// these two statements throwing, which gives us a consistency guarantee.
		this->~value();
		new (this) value(std::move(other));

		return *this;
	}

	template <class T>
	void hash_combine(std::size_t& seed, const T& v);

	inline
	size_t value::hash() const noexcept
	{
		switch (kind_) {
			case Kind::String:
				return std::hash<std::string>()(variant_.strValue);

			case Kind::StringList: {
				size_t seed = std::hash<size_t>()(variant_.strList.size());
				for(auto const& str : variant_.strList) {
					hash_combine(seed, str);
				}
				return seed;
			}

			case Kind::Bool:
				return std::hash<bool>()(variant_.boolValue);

			case Kind::Long:
				return std::hash<long>()(variant_.longValue);

			case Kind::Empty:
			default:
				return std::hash<void*>()(nullptr);
		}
	}

	inline
	bool value::asBool() const
	{
		throwIfNotKind(Kind::Bool);
		return variant_.boolValue;
	}

	inline
	long value::asLong() const
	{
		// Attempt to convert a string to a long
		if (kind_ == Kind::String) {
			const std::string& str = variant_.strValue;
			std::size_t pos;
			const long ret = stol(str, &pos); // Throws if it can't convert
			if (pos != str.length()) {
				// The string ended in non-digits.
				throw std::runtime_error( str + " contains non-numeric characters.");
			}
			return ret;
		}
		throwIfNotKind(Kind::Long);
		return variant_.longValue;
	}

	inline
	std::string const& value::asString() const
	{
		throwIfNotKind(Kind::String);
		return variant_.strValue;
	}

	inline
	std::vector<std::string> const& value::asStringList() const
	{
		throwIfNotKind(Kind::StringList);
		return variant_.strList;
	}

	inline
	bool operator==(value const& v1, value const& v2)
	{
		if (v1.kind_ != v2.kind_)
			return false;
		
		switch (v1.kind_) {
			case Kind::String:
				return v1.variant_.strValue==v2.variant_.strValue;

			case Kind::StringList:
				return v1.variant_.strList==v2.variant_.strList;

			case Kind::Bool:
				return v1.variant_.boolValue==v2.variant_.boolValue;

			case Kind::Long:
				return v1.variant_.longValue==v2.variant_.longValue;

			case Kind::Empty:
			default:
				return true;
		}
	}

	inline
	bool operator!=(value const& v1, value const& v2)
	{
		return !(v1 == v2);
	}
}

#endif /* defined(docopt__value_h_) */
