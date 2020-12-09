#include "config.h"
#include "Init.hxx"
#include "config/Block.hxx"
#include "util/StringView.hxx"
#include "util/StringCompare.hxx"
#include "util/IterableSplitString.hxx"

const Domain ytdl_domain("youtube-dl");

static const char* DEFAULT_EXECUTABLE = "youtube-dl";
static const char* DEFAULT_FORMAT = "bestaudio/best";
static const char* DEFAULT_WHITELIST =
	"youtu.be "
	"music.youtube.com "
	"www.youtube.com";

namespace Ytdl {

static std::weak_ptr<YtdlInit> singleton;

YtdlInit::YtdlInit(): event_loop(nullptr), executable(DEFAULT_EXECUTABLE), format(DEFAULT_FORMAT) { }

std::shared_ptr<YtdlInit>
YtdlInit::Init() {
	auto ptr = singleton.lock();
	if (!ptr) {
		ptr = std::make_shared<YtdlInit>();
		singleton = ptr;
	}

	return ptr;
}

const char *
YtdlInit::UriSupported(const char *uri) const
{
	const char* p;

	if ((p = StringAfterPrefix(uri, "ytdl://"))) {
		return p;
	} else if (WhitelistMatch(uri)) {
		return uri;
	} else {
		return nullptr;
	}
}

bool
YtdlInit::WhitelistMatch(const char *uri) const
{
	const char* p;
	if (!(p = StringAfterPrefix(uri, "http://")) &&
		!(p = StringAfterPrefix(uri, "https://"))) {
		return false;
	}

	StringView domain(p);
	for (const auto &whitelist : domain_whitelist) {
		if (domain.StartsWith(whitelist.c_str())) {
			return true;
		}
	}

	return false;
}

void
YtdlInit::InitInput(const ConfigBlock &block, EventLoop &_event_loop)
{
	const char* domains = block.GetBlockValue("domain_whitelist", DEFAULT_WHITELIST);

	for (const auto domain : IterableSplitString(domains, ' ')) {
		if (!domain.empty()) {
			domain_whitelist.emplace_front(domain.ToString());
		}
	}

	executable = block.GetBlockValue("executable", DEFAULT_EXECUTABLE);
	format = block.GetBlockValue("format", DEFAULT_FORMAT);
	config_file = block.GetBlockValue("config_file", "");

	event_loop = &_event_loop;
}

void
YtdlInit::InitPlaylist([[maybe_unused]] const ConfigBlock &block)
{
	// no playlist-specific config settings yet
}

} // namespace Ytdl
