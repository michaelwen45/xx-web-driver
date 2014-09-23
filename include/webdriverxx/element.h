#ifndef WEBDRIVERXX_ELEMENT_H
#define WEBDRIVERXX_ELEMENT_H

#include "by.h"
#include "types.h"
#include "detail/resource.h"
#include "detail/conversions.h"
#include <string>
#include <vector>

namespace webdriverxx {
namespace detail {

class Finder;

} // namespace detail

class Element { // copyable
public:
	Element(
		const std::string& id,
		const detail::Resource& resource,
		const detail::Finder* finder
		);

	bool operator != (const Element& other) const;
	bool operator == (const Element& other) const;
	Element FindElement(const By& by) const;
	std::vector<Element> FindElements(const By& by) const;
	void Click() const;
	void Submit() const;
	std::string GetText() const;
	std::string GetTagName() const;
	void Clear() const;
	bool IsSelected() const;
	bool IsEnabled() const;
	bool Equals(const Element& other) const;
	std::string GetAttribute(const std::string& name) const;
	bool IsDisplayed() const;
	Point GetLocation() const;
	Point GetLocationInView() const;
	Size GetSize() const;
	std::string GetCssProperty(const std::string& name) const;

private:
	std::string id_;
	detail::Resource resource_;
	const detail::Finder* finder_;
};

} // namespace webdriverxx

#include "element.inl"

#endif
