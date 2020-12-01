#ifndef MPD_LIB_YTDL_INIT_HXX
#define MPD_LIB_YTDL_INIT_HXX

#include "input/InputStream.hxx"
#include "util/Domain.hxx"
#include <string>
#include <forward_list>

class EventLoop;
class ConfigBlock;

extern const class Domain ytdl_domain;

namespace Ytdl {

class YtdlInit {
	EventLoop* event_loop;
	std::forward_list<std::string> domain_whitelist;

public:
	YtdlInit();

	static std::shared_ptr<YtdlInit> Init();

	const char *UriSupported(const char *uri) const;
	bool WhitelistMatch(const char *uri) const;

	void InitPlaylist(const ConfigBlock &block);
	void InitInput(const ConfigBlock &block, EventLoop &_event_loop);

	EventLoop &GetEventLoop() const { return *event_loop; }
};

} // namespace Ytdl

#endif
