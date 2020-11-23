#ifndef MPD_LIB_YTDL_PARSER_HXX
#define MPD_LIB_YTDL_PARSER_HXX

#include "event/SocketEvent.hxx"
#include "lib/yajl/Handle.hxx"
#include "Handler.hxx"
#include <memory>

namespace Ytdl {

class MetadataHandler;
class ParserContext;
class YtdlMonitor;
class YtdlInit;

enum class PlaylistMode {
	SINGLE,
	FLAT,
};

class Parser {
	std::unique_ptr<ParserContext> context;

public:
	Parser(MetadataHandler &handler) noexcept;
	~Parser() noexcept;

	std::unique_ptr<Yajl::Handle> CreateHandle() noexcept;
};

class YtdlHandler {
public:
	virtual void OnComplete(YtdlMonitor* monitor) = 0;
	virtual void OnError(YtdlMonitor* monitor, std::exception_ptr e) = 0;
};

class YtdlProcess {
	Yajl::Handle &handle;
	FileDescriptor fd;
	int pid;

public:
	YtdlProcess(Yajl::Handle &_handle, int _fd, int _pid) noexcept
		:handle(_handle), fd(_fd), pid(_pid) {}

	~YtdlProcess();

	FileDescriptor const& GetDescriptor() const noexcept {
		return fd;
	}

	static std::unique_ptr<YtdlProcess> Invoke(const YtdlInit &init, Yajl::Handle &handle, const char *url, PlaylistMode mode);
	bool Process();
	void Complete();
	void Close();
};

class YtdlMonitor {
	SocketEvent socket_event;
	YtdlHandler &handler;
	std::unique_ptr<YtdlProcess> process;

public:
	YtdlMonitor(YtdlHandler &_handler, std::unique_ptr<YtdlProcess> && _process, EventLoop &_loop) noexcept
		:socket_event(_loop, BIND_THIS_METHOD(OnSocketReady), SocketDescriptor::FromFileDescriptor(_process->GetDescriptor())),
		 handler(_handler), process(std::move(_process)) {}

	auto &GetEventLoop() const noexcept {
		return socket_event.GetEventLoop();
	}

	bool Schedule(unsigned flags) noexcept {
		return socket_event.Schedule(flags);
	}

	bool IsDefined() const noexcept {
		return socket_event.IsDefined();
	}

	SocketDescriptor ReleaseSocket() noexcept {
		return socket_event.ReleaseSocket();
	}


private:
	void OnSocketReady(unsigned flags) noexcept;
};

void BlockingInvoke(const YtdlInit &init, Yajl::Handle &handle, const char *url, PlaylistMode mode);

std::unique_ptr<YtdlMonitor>
Invoke(const YtdlInit &init, Yajl::Handle &handle, const char *url, PlaylistMode mode, YtdlHandler &handler);

} // namespace Ytdl

#endif
