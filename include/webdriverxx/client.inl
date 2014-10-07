#include "conversions.h"
#include "detail/shared.h"
#include "detail/error_handling.h"
#include "detail/types.h"

namespace webdriverxx {

inline
Client::Client(const std::string& url)
	: resource_(new detail::RootResource(
		url,
		detail::Shared<detail::IHttpClient>(new detail::HttpConnection)
		))
{}

inline
picojson::object Client::GetStatus() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const picojson::value value = resource_->Get("status").get("value");
	WEBDRIVERXX_CHECK(value.is<picojson::object>(), "Value is not an object");
	return value.get<picojson::object>();
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
std::vector<Session> Client::GetSessions() const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const std::vector<detail::SessionRef> sessions =
		FromJsonArray<detail::SessionRef>(
			resource_->Get("sessions").get("value")
			);
	std::vector<Session> result;
	for (std::vector<detail::SessionRef>::const_iterator it = sessions.begin();
		it != sessions.end(); ++it)
		result.push_back(MakeSession(it->id, detail::Resource::IsObserver));
	return result;
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
Session Client::CreateSession(
	const Capabilities& desired,
	const Capabilities& required
	) const {
	WEBDRIVERXX_FUNCTION_CONTEXT_BEGIN()
	const picojson::value& response = resource_->Post("session",
		JsonObject()
			.With("desiredCapabilities", detail::CapabilitiesAccess::GetJsonObject(desired))
			.With("requiredCapabilities", detail::CapabilitiesAccess::GetJsonObject(required))
			.Build()
			);

	WEBDRIVERXX_CHECK(response.get("sessionId").is<std::string>(), "Session ID is not a string");
	WEBDRIVERXX_CHECK(response.get("value").is<picojson::object>(), "Capabilities is not an object");
	
	const std::string sessionId = response.get("sessionId").to_str();
	
	return MakeSession(sessionId, detail::Resource::IsOwner);
	WEBDRIVERXX_FUNCTION_CONTEXT_END()
}

inline
Session Client::MakeSession(
	const std::string& id,
	detail::Resource::Ownership mode
	) const {
	return Session(detail::MakeSubResource(resource_, "session", id, mode));
}

} // namespace webdriverxx
