//
//  docopt_util.h
//  docopt
//
//  Created by Jared Grubb on 2013-11-04.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#ifndef docopt_docopt_util_h
#define docopt_docopt_util_h

#if DOCTOPT_USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace std {
    using boost::regex;
    using boost::sregex_token_iterator;
}
#else
#include <regex>
#endif

#if 0
#pragma mark -
#pragma mark General utility
#endif

namespace {
	bool starts_with(std::string const& str, std::string const& prefix)
	{
		if (str.length() < prefix.length())
			return false;
		return std::equal(prefix.begin(), prefix.end(),
				  str.begin());
	}

	std::string trim(std::string&& str,
			 const std::string& whitespace = " \t\n")
	{
		const auto strEnd = str.find_last_not_of(whitespace);
		if (strEnd==std::string::npos)
			return {}; // no content
		str.erase(strEnd+1);

		const auto strBegin = str.find_first_not_of(whitespace);
		str.erase(0, strBegin);

		return std::move(str);
	}

	std::vector<std::string> split(std::string const& str, size_t pos = 0)
	{
		const char* const anySpace = " \t\r\n\v\f";

		std::vector<std::string> ret;
		while (pos != std::string::npos) {
			auto start = str.find_first_not_of(anySpace, pos);
			if (start == std::string::npos) break;

			auto end = str.find_first_of(anySpace, start);
			auto size = end==std::string::npos ? end : end-start;
			ret.emplace_back(str.substr(start, size));

			pos = end;
		}

		return ret;
	}

	std::tuple<std::string, std::string, std::string> partition(std::string str, std::string const& point)
	{
		std::tuple<std::string, std::string, std::string> ret;

		auto i = str.find(point);

		if (i == std::string::npos) {
			// no match: string goes in 0th spot only
		} else {
			std::get<2>(ret) = str.substr(i + point.size());
			std::get<1>(ret) = point;
			str.resize(i);
		}
		std::get<0>(ret) = std::move(str);

		return ret;
	}

	template <typename I>
	std::string join(I iter, I end, std::string const& delim) {
		if (iter==end)
			return {};

		std::string ret = *iter;
		for(++iter; iter!=end; ++iter) {
			ret.append(delim);
			ret.append(*iter);
		}
		return ret;
	}

	std::vector<std::string> regex_split(std::string const& text, std::regex const& re)
	{
		std::vector<std::string> ret;
		for (auto it = std::sregex_token_iterator(text.begin(), text.end(), re, -1);
			it != std::sregex_token_iterator();
			++it) {
			ret.emplace_back(*it);
		}
		return ret;
	}
}

namespace docopt {
	template <class T>
	inline void hash_combine(std::size_t& seed, T const& v)
	{
		// stolen from boost::hash_combine
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
}

#endif
