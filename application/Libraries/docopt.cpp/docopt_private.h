//
//  docopt_private.h
//  docopt
//
//  Created by Jared Grubb on 2013-11-04.
//  Copyright (c) 2013 Jared Grubb. All rights reserved.
//

#ifndef docopt_docopt_private_h
#define docopt_docopt_private_h

#include <vector>
#include <memory>
#include <unordered_set>
#include <assert.h>

// Workaround GCC 4.8 not having std::regex
#if DOCTOPT_USE_BOOST_REGEX
#include <boost/regex.hpp>
namespace std {
	using boost::regex;
   	using boost::sregex_iterator;
   	using boost::smatch;
   	using boost::regex_search;
   	namespace regex_constants {
		using boost::regex_constants::match_not_null;
   	}
}
#else
#include <regex>
#endif

#include "docopt_value.h"

namespace docopt {

	class Pattern;
	class LeafPattern;

	using PatternList = std::vector<std::shared_ptr<Pattern>>;

	// Utility to use Pattern types in std hash-containers
	struct PatternHasher {
		template <typename P>
		size_t operator()(std::shared_ptr<P> const& pattern) const {
			return pattern->hash();
		}
		template <typename P>
		size_t operator()(P const* pattern) const {
			return pattern->hash();
		}
		template <typename P>
		size_t operator()(P const& pattern) const {
			return pattern.hash();
		}
	};

	// Utility to use 'hash' as the equality operator as well in std containers
	struct PatternPointerEquality {
		template <typename P1, typename P2>
		bool operator()(std::shared_ptr<P1> const& p1, std::shared_ptr<P2> const& p2) const {
			return p1->hash()==p2->hash();
		}
		template <typename P1, typename P2>
		bool operator()(P1 const* p1, P2 const* p2) const {
			return p1->hash()==p2->hash();
		}
	};

	// A hash-set that uniques by hash value
	using UniquePatternSet = std::unordered_set<std::shared_ptr<Pattern>, PatternHasher, PatternPointerEquality>;


	class Pattern {
	public:
		// flatten out children, stopping descent when the given filter returns 'true'
		virtual std::vector<Pattern*> flat(bool (*filter)(Pattern const*)) = 0;

		// flatten out all children into a list of LeafPattern objects
		virtual void collect_leaves(std::vector<LeafPattern*>&) = 0;

		// flatten out all children into a list of LeafPattern objects
		std::vector<LeafPattern*> leaves();

		// Attempt to find something in 'left' that matches this pattern's spec, and if so, move it to 'collected'
		virtual bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const = 0;

		virtual std::string const& name() const = 0;

		virtual bool hasValue() const { return false; }

		virtual size_t hash() const = 0;

		virtual ~Pattern() = default;
	};

	class LeafPattern
	: public Pattern {
	public:
		LeafPattern(std::string name, value v = {})
		: fName(std::move(name)),
		  fValue(std::move(v))
		{}

		virtual std::vector<Pattern*> flat(bool (*filter)(Pattern const*)) override {
			if (filter(this)) {
				return { this };
			}
			return {};
		}

		virtual void collect_leaves(std::vector<LeafPattern*>& lst) override final {
			lst.push_back(this);
		}

		virtual bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;

		virtual bool hasValue() const override { return static_cast<bool>(fValue); }

		value const& getValue() const { return fValue; }
		void setValue(value&& v) { fValue = std::move(v); }

		virtual std::string const& name() const override { return fName; }

		virtual size_t hash() const override {
			size_t seed = typeid(*this).hash_code();
			hash_combine(seed, fName);
			hash_combine(seed, fValue);
			return seed;
		}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const&) const = 0;

	private:
		std::string fName;
		value fValue;
	};

	class BranchPattern
	: public Pattern {
	public:
		BranchPattern(PatternList children = {})
		: fChildren(std::move(children))
		{}

		Pattern& fix() {
			UniquePatternSet patterns;
			fix_identities(patterns);
			fix_repeating_arguments();
			return *this;
		}

		virtual std::string const& name() const override {
			throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
		}

		virtual value const& getValue() const {
			throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
		}

		virtual std::vector<Pattern*> flat(bool (*filter)(Pattern const*)) override {
			if (filter(this)) {
				return {this};
			}

			std::vector<Pattern*> ret;
			for(auto& child : fChildren) {
				auto sublist = child->flat(filter);
				ret.insert(ret.end(), sublist.begin(), sublist.end());
			}
			return ret;
		}

		virtual void collect_leaves(std::vector<LeafPattern*>& lst) override final {
			for(auto& child : fChildren) {
				child->collect_leaves(lst);
			}
		}

		void setChildren(PatternList children) {
			fChildren = std::move(children);
		}

		PatternList const& children() const { return fChildren; }

		virtual void fix_identities(UniquePatternSet& patterns) {
			for(auto& child : fChildren) {
				// this will fix up all its children, if needed
				if (auto bp = dynamic_cast<BranchPattern*>(child.get())) {
					bp->fix_identities(patterns);
				}

				// then we try to add it to the list
				auto inserted = patterns.insert(child);
				if (!inserted.second) {
					// already there? then reuse the existing shared_ptr for that thing
					child = *inserted.first;
				}
			}
		}

		virtual size_t hash() const override {
			size_t seed = typeid(*this).hash_code();
			hash_combine(seed, fChildren.size());
			for(auto const& child : fChildren) {
				hash_combine(seed, child->hash());
			}
			return seed;
		}
	private:
		void fix_repeating_arguments();

	protected:
		PatternList fChildren;
	};

	class Argument
	: public LeafPattern {
	public:
		using LeafPattern::LeafPattern;

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	class Command : public Argument {
	public:
		Command(std::string name, value v = value{false})
		: Argument(std::move(name), std::move(v))
		{}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
	};

	class Option final
	: public LeafPattern
	{
	public:
		static Option parse(std::string const& option_description);

		Option(std::string shortOption,
		       std::string longOption,
		       int argcount = 0,
		       value v = value{false})
		: LeafPattern(longOption.empty() ? shortOption : longOption,
			      std::move(v)),
		  fShortOption(std::move(shortOption)),
		  fLongOption(std::move(longOption)),
		  fArgcount(argcount)
		{
			// From Python:
			//   self.value = None if value is False and argcount else value
			if (argcount && v.isBool() && !v.asBool()) {
				setValue(value{});
			}
		}

		Option(Option const&) = default;
		Option(Option&&) = default;
		Option& operator=(Option const&) = default;
		Option& operator=(Option&&) = default;

		using LeafPattern::setValue;

		std::string const& longOption() const { return fLongOption; }
		std::string const& shortOption() const { return fShortOption; }
		int argCount() const { return fArgcount; }

		virtual size_t hash() const override {
			size_t seed = LeafPattern::hash();
			hash_combine(seed, fShortOption);
			hash_combine(seed, fLongOption);
			hash_combine(seed, fArgcount);
			return seed;
		}

	protected:
		virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;

	private:
		std::string fShortOption;
		std::string fLongOption;
		int fArgcount;
	};

	class Required : public BranchPattern {
	public:
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	class Optional : public BranchPattern {
	public:
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override {
			for(auto const& pattern : fChildren) {
				pattern->match(left, collected);
			}
			return true;
		}
	};

	class OptionsShortcut : public Optional {
		using Optional::Optional;
	};

	class OneOrMore : public BranchPattern {
	public:
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

	class Either : public BranchPattern {
	public:
		using BranchPattern::BranchPattern;

		bool match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const override;
	};

#if 0
#pragma mark -
#pragma mark inline implementations
#endif

	inline std::vector<LeafPattern*> Pattern::leaves()
	{
		std::vector<LeafPattern*> ret;
		collect_leaves(ret);
		return ret;
	}

	static inline std::vector<PatternList> transform(PatternList pattern)
	{
		std::vector<PatternList> result;

		std::vector<PatternList> groups;
		groups.emplace_back(std::move(pattern));

		while(!groups.empty()) {
			// pop off the first element
			auto children = std::move(groups[0]);
			groups.erase(groups.begin());

			// find the first branch node in the list
			auto child_iter = std::find_if(children.begin(), children.end(), [](std::shared_ptr<Pattern> const& p) {
				return dynamic_cast<BranchPattern const*>(p.get());
			});

			// no branch nodes left : expansion is complete for this grouping
			if (child_iter == children.end()) {
				result.emplace_back(std::move(children));
				continue;
			}

			// pop the child from the list
			auto child = std::move(*child_iter);
			children.erase(child_iter);

			// expand the branch in the appropriate way
			if (Either* either = dynamic_cast<Either*>(child.get())) {
				// "[e] + children" for each child 'e' in Either
				for(auto const& eitherChild : either->children()) {
					PatternList group = { eitherChild };
					group.insert(group.end(), children.begin(), children.end());

					groups.emplace_back(std::move(group));
				}
			} else if (OneOrMore* oneOrMore = dynamic_cast<OneOrMore*>(child.get())) {
				// child.children * 2 + children
				auto const& subchildren = oneOrMore->children();
				PatternList group = subchildren;
				group.insert(group.end(), subchildren.begin(), subchildren.end());
				group.insert(group.end(), children.begin(), children.end());

				groups.emplace_back(std::move(group));
			} else { // Required, Optional, OptionsShortcut
				BranchPattern* branch = dynamic_cast<BranchPattern*>(child.get());

				// child.children + children
				PatternList group = branch->children();
				group.insert(group.end(), children.begin(), children.end());

				groups.emplace_back(std::move(group));
			}
		}

		return result;
	}

	inline void BranchPattern::fix_repeating_arguments()
	{
		std::vector<PatternList> either = transform(children());
		for(auto const& group : either) {
			// use multiset to help identify duplicate entries
			std::unordered_multiset<std::shared_ptr<Pattern>, PatternHasher> group_set {group.begin(), group.end()};
			for(auto const& e : group_set) {
				if (group_set.count(e) == 1)
					continue;

				LeafPattern* leaf = dynamic_cast<LeafPattern*>(e.get());
				if (!leaf) continue;

				bool ensureList = false;
				bool ensureInt = false;

				if (dynamic_cast<Command*>(leaf)) {
					ensureInt = true;
				} else if (dynamic_cast<Argument*>(leaf)) {
					ensureList = true;
				} else if (Option* o = dynamic_cast<Option*>(leaf)) {
					if (o->argCount()) {
						ensureList = true;
					} else {
						ensureInt = true;
					}
				}

				if (ensureList) {
					std::vector<std::string> newValue;
					if (leaf->getValue().isString()) {
						newValue = split(leaf->getValue().asString());
					}
					if (!leaf->getValue().isStringList()) {
						leaf->setValue(value{newValue});
					}
				} else if (ensureInt) {
					leaf->setValue(value{0});
				}
			}
		}
	}

	inline bool LeafPattern::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
		auto match = single_match(left);
		if (!match.second) {
			return false;
		}

		left.erase(left.begin()+static_cast<std::ptrdiff_t>(match.first));

		auto same_name = std::find_if(collected.begin(), collected.end(), [&](std::shared_ptr<LeafPattern> const& p) {
			return p->name()==name();
		});
		if (getValue().isLong()) {
			long val = 1;
			if (same_name == collected.end()) {
				collected.push_back(match.second);
				match.second->setValue(value{val});
			} else if ((**same_name).getValue().isLong()) {
				val += (**same_name).getValue().asLong();
				(**same_name).setValue(value{val});
			} else {
				(**same_name).setValue(value{val});
			}
		} else if (getValue().isStringList()) {
			std::vector<std::string> val;
			if (match.second->getValue().isString()) {
				val.push_back(match.second->getValue().asString());
			} else if (match.second->getValue().isStringList()) {
				val = match.second->getValue().asStringList();
			} else {
				/// cant be!?
			}

			if (same_name == collected.end()) {
				collected.push_back(match.second);
				match.second->setValue(value{val});
			} else if ((**same_name).getValue().isStringList()) {
				std::vector<std::string> const& list = (**same_name).getValue().asStringList();
				val.insert(val.begin(), list.begin(), list.end());
				(**same_name).setValue(value{val});
			} else {
				(**same_name).setValue(value{val});
			}
		} else {
			collected.push_back(match.second);
		}
		return true;
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Argument::single_match(PatternList const& left) const
	{
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret {};

		for(size_t i = 0, size = left.size(); i < size; ++i)
		{
			auto arg = dynamic_cast<Argument const*>(left[i].get());
			if (arg) {
				ret.first = i;
				ret.second = std::make_shared<Argument>(name(), arg->getValue());
				break;
			}
		}

		return ret;
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Command::single_match(PatternList const& left) const
	{
		std::pair<size_t, std::shared_ptr<LeafPattern>> ret {};

		for(size_t i = 0, size = left.size(); i < size; ++i)
		{
			auto arg = dynamic_cast<Argument const*>(left[i].get());
			if (arg) {
				if (name() == arg->getValue()) {
					ret.first = i;
					ret.second = std::make_shared<Command>(name(), value{true});
				}
				break;
			}
		}

		return ret;
	}

	inline Option Option::parse(std::string const& option_description)
	{
		std::string shortOption, longOption;
		int argcount = 0;
		value val { false };

		auto double_space = option_description.find("  ");
		auto options_end = option_description.end();
		if (double_space != std::string::npos) {
			options_end = option_description.begin() + static_cast<std::ptrdiff_t>(double_space);
		}

		static const std::regex pattern {"(-{1,2})?(.*?)([,= ]|$)"};
		for(std::sregex_iterator i {option_description.begin(), options_end, pattern, std::regex_constants::match_not_null},
			   e{};
			i != e;
			++i)
		{
			std::smatch const& match = *i;
			if (match[1].matched) { // [1] is optional.
				if (match[1].length()==1) {
						shortOption = "-" + match[2].str();
				} else {
						longOption =  "--" + match[2].str();
				}
			} else if (match[2].length() > 0) { // [2] always matches.
				std::string m = match[2];
				argcount = 1;
			} else {
				// delimeter
			}

			if (match[3].length() == 0) { // [3] always matches.
				// Hit end of string. For some reason 'match_not_null' will let us match empty
				// at the end, and then we'll spin in an infinite loop. So, if we hit an empty
				// match, we know we must be at the end.
				break;
			}
		}

		if (argcount) {
			std::smatch match;
			if (std::regex_search(options_end, option_description.end(),
						  match,
						  std::regex{"\\[default: (.*)\\]", std::regex::icase}))
			{
				val = match[1].str();
			}
		}

		return {std::move(shortOption),
			std::move(longOption),
			argcount,
			std::move(val)};
	}

	inline std::pair<size_t, std::shared_ptr<LeafPattern>> Option::single_match(PatternList const& left) const
	{
		auto thematch = find_if(left.begin(), left.end(), [this](std::shared_ptr<Pattern> const& a) {
			auto leaf = std::dynamic_pointer_cast<LeafPattern>(a);
			return leaf && this->name() == leaf->name();
		});
		if (thematch == left.end()) {
			return {};
		}
		return { std::distance(left.begin(), thematch), std::dynamic_pointer_cast<LeafPattern>(*thematch) };
	}

	inline bool Required::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const {
		auto l = left;
		auto c = collected;
		for(auto const& pattern : fChildren) {
			bool ret = pattern->match(l, c);
			if (!ret) {
				// leave (left, collected) untouched
				return false;
			}
		}

		left = std::move(l);
		collected = std::move(c);
		return true;
	}

	inline bool OneOrMore::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
		assert(fChildren.size() == 1);

		auto l = left;
		auto c = collected;

		bool matched = true;
		size_t times = 0;

		decltype(l) l_;
		bool firstLoop = true;

		while (matched) {
			// could it be that something didn't match but changed l or c?
			matched = fChildren[0]->match(l, c);

			if (matched)
				++times;

			if (firstLoop) {
				firstLoop = false;
			} else if (l == l_) {
				break;
			}

			l_ = l;
		}

		if (times == 0) {
			return false;
		}

		left = std::move(l);
		collected = std::move(c);
		return true;
	}

	inline bool Either::match(PatternList& left, std::vector<std::shared_ptr<LeafPattern>>& collected) const
	{
		using Outcome = std::pair<PatternList, std::vector<std::shared_ptr<LeafPattern>>>;

		std::vector<Outcome> outcomes;

		for(auto const& pattern : fChildren) {
			// need a copy so we apply the same one for every iteration
			auto l = left;
			auto c = collected;
			bool matched = pattern->match(l, c);
			if (matched) {
				outcomes.emplace_back(std::move(l), std::move(c));
			}
		}

		auto min = std::min_element(outcomes.begin(), outcomes.end(), [](Outcome const& o1, Outcome const& o2) {
			return o1.first.size() < o2.first.size();
		});

		if (min == outcomes.end()) {
			// (left, collected) unchanged
			return false;
		}

		std::tie(left, collected) = std::move(*min);
		return true;
	}

}

#endif
