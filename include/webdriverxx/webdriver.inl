#include "conversions.h"
#include "detail/error_handling.h"

namespace webdriverxx {

inline
BasicWebDriver::BasicWebDriver(const std::string& url)
	: server_root_(&http_connection_, url) {}

inline
picojson::object BasicWebDriver::GetStatus() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const picojson::value value = server_root_.Get("status").get("value");
	WEBDRIVERXX_CHECK(value.is<picojson::object>(), "Value is not an object");
	return value.get<picojson::object>();
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
SessionsInformation BasicWebDriver::GetSessions() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	return FromJsonArray<SessionInformation>(
		server_root_.Get("sessions").get("value")
		);
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
BasicWebDriver::Session BasicWebDriver::CreateSession(
	const Capabilities& required,
	const Capabilities& desired
	) const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const picojson::value& response = server_root_.Post("session",
		JsonObject()
			.With("requiredCapabilities", detail::CapabilitiesAccess::GetJsonObject(required))
			.With("desiredCapabilities", detail::CapabilitiesAccess::GetJsonObject(desired))
			.Build());

	WEBDRIVERXX_CHECK(response.get("sessionId").is<std::string>(), "Session ID is not a string");
	WEBDRIVERXX_CHECK(response.get("value").is<picojson::object>(), "Capabilities is not an object");
	
	const std::string& sessionId = response.get("sessionId").to_str();
	
	return Session(
		server_root_
			.GetSubResource<detail::Resource>("session")
			.GetSubResource(sessionId),
		detail::CapabilitiesAccess::Construct(response.get("value").get<picojson::object>())
		);
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
WebDriver::WebDriver(
	const std::string& url,
	const Capabilities& required,
	const Capabilities& desired
	)
	: BasicWebDriver(url)
	, session_(CreateSession(required, desired))
	, resource_(session_.resource)
	, session_deleter_(resource_) {}

inline
Capabilities WebDriver::GetCapabilities() const {
	return session_.capabilities;
}

inline
std::string WebDriver::GetSource() const {
	return resource_.GetString("source");
}

inline
std::string WebDriver::GetTitle() const {
	return resource_.GetString("title");
}

inline
std::string WebDriver::GetUrl() const {
	return resource_.GetString("url");
}

inline
Window WebDriver::GetCurrentWindow() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	return MakeWindow(resource_.GetString("window_handle"));
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
const WebDriver& WebDriver::CloseCurrentWindow() const {
	resource_.Delete("window");
	return *this;
}

inline
const WebDriver& WebDriver::Navigate(const std::string& url) const {
	resource_.Post("url", "url", url);
	return *this;
}

inline
const WebDriver& WebDriver::Forward() const {
	resource_.Post("forward");
	return *this;
}

inline
const WebDriver& WebDriver::Back() const {
	resource_.Post("back");
	return *this;
}

inline
const WebDriver& WebDriver::Refresh() const {
	resource_.Post("refresh");
	return *this;
}

inline
const WebDriver& WebDriver::Execute(const std::string& script, const JsArgs& args) const {
	InternalEval(script, args);
	return *this;
}

template<typename T>
inline
T WebDriver::Eval(const std::string& script, const JsArgs& args) const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	return FromJson<T>(InternalEval(script, args));
	WEBDRIVERXX_FUNCTION_CONTEXT_END_EX(detail::Fmt()
		<< "script: " << script
		)
}

inline
Element WebDriver::EvalElement(const std::string& script, const JsArgs& args) const {
	return MakeElement(Eval<detail::ElementRef>(script, args).ref);
}

inline
const WebDriver& WebDriver::SetFocusToWindow(const std::string& name_or_handle) const {
	resource_.Post("window", "name", name_or_handle);
	return *this;
}

inline
std::vector<Window> WebDriver::GetWindows() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const std::vector<std::string> handles =
		FromJsonArray<std::string>(
			resource_.Get("window_handles")
			);
	std::vector<Window> result;
	for (std::vector<std::string>::const_iterator it = handles.begin();
		it != handles.end(); ++it)
		result.push_back(MakeWindow(*it));
	return result;
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
Element WebDriver::FindElement(const By& by) const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	return FindElement(by, session_.resource);
	WEBDRIVERXX_FUNCTION_CONTEXT_END_EX(detail::Fmt()
		<< "strategy: " << by.GetStrategy()
		<< ", value: " << by.GetValue()
		)
}

inline
std::vector<Element> WebDriver::FindElements(const By& by) const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	return FindElements(by, session_.resource);
	WEBDRIVERXX_FUNCTION_CONTEXT_END_EX(detail::Fmt()
		<< "strategy: " << by.GetStrategy()
		<< ", value: " << by.GetValue()
		)
}

inline
const WebDriver& WebDriver::SendKeys(const char* keys) const {
	GetKeyboard().SendKeys(keys);
	return *this;
}

inline
const WebDriver& WebDriver::SendKeys(const std::string& keys) const {
	GetKeyboard().SendKeys(keys);
	return *this;
}

inline
const WebDriver& WebDriver::SendKeys(const Shortcut& shortcut) const {
	GetKeyboard().SendKeys(shortcut);
	return *this;
}

inline
Element WebDriver::FindElement(
	const By& by,
	const detail::Resource& context
	) const {
	return MakeElement(FromJson<detail::ElementRef>(
		context.Post("element",
			JsonObject()
				.With("using", by.GetStrategy())
				.With("value", by.GetValue())
				.Build()
			)).ref);
}

inline
std::vector<Element> WebDriver::FindElements(
	const By& by,
	const detail::Resource& context
	) const {
	const std::vector<detail::ElementRef> ids =
		FromJsonArray<detail::ElementRef>(
			context.Post("elements", JsonObject()
				.With("using", by.GetStrategy())
				.With("value", by.GetValue())
				.Build()
			));
	std::vector<Element> result;
	for (std::vector<detail::ElementRef>::const_iterator it = ids.begin();
		it != ids.end(); ++it)
		result.push_back(MakeElement(it->ref));
	return result;
}

inline
Window WebDriver::MakeWindow(const std::string& handle) const {
	return Window(handle,
		resource_.GetSubResource("window").GetSubResource(handle)
		);
}	

inline
Element WebDriver::MakeElement(const std::string& id) const {
	return Element(
		id,
		resource_.GetSubResource("element").GetSubResource(id),
		this
		);
}

inline
detail::Keyboard WebDriver::GetKeyboard() const
{
	return detail::Keyboard(resource_, "keys");
}

inline
picojson::value WebDriver::InternalEval(const std::string& script, const JsArgs& args) const {
	return resource_.Post("execute", 
		JsonObject()
			.With("script", script)
			.With("args", args.args_)
			.Build()
		);
}

} // namespace webdriverxx
