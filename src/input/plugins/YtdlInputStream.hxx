#ifndef MPD_INPUT_YTDL_STREAM_HXX
#define MPD_INPUT_YTDL_STREAM_HXX

#include "../ProxyInputStream.hxx"
#include "lib/ytdl/Invoke.hxx"

class Tag;
class InputStreamHandler;

class YtdlInputStream : public ProxyInputStream, public Ytdl::YtdlHandler {
	static const int MAX_RETRY = 1;

	std::unique_ptr<Ytdl::InvokeContext> context;
	std::unique_ptr<Tag> tag;
	std::exception_ptr pending_exception;
	EventLoop &event_loop;

	int retry_counter = 0;

	void InvokeYtdl();

public:
	YtdlInputStream(const char *_uri, Mutex &_mutex, EventLoop &event_loop) noexcept;

	virtual ~YtdlInputStream() noexcept;
	virtual void Check();
	virtual const char *GetURI() const noexcept;

	virtual std::unique_ptr<Tag> ReadTag() noexcept;

	gcc_nonnull_all
	virtual size_t Read(std::unique_lock<Mutex> &lock, void *ptr, size_t size);

	void OnComplete(Ytdl::YtdlMonitor* monitor);
	void OnError(Ytdl::YtdlMonitor* monitor, std::exception_ptr e);

	void OnInputStreamReady() noexcept override;
};

#endif
