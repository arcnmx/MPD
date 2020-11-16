#include "YtdlInputStream.hxx"
#include "CurlInputPlugin.hxx"
#include "tag/Tag.hxx"

YtdlInputStream::YtdlInputStream(const char *_uri, Mutex &_mutex, EventLoop &event_loop) noexcept
	:ProxyInputStream(_uri, _mutex)
{
	try {
		context = Ytdl::InvokeContext::Invoke(_uri, Ytdl::PlaylistMode::SINGLE, event_loop, *this);
	} catch (...) {
		pending_exception = std::current_exception();
	}
}

YtdlInputStream::~YtdlInputStream() noexcept {
}

const char *YtdlInputStream::GetURI() const noexcept {
	if (input != nullptr) {
		return input->GetURI();
	} else {
		return InputStream::GetURI();
	}
}

void YtdlInputStream::Check() {
	if (pending_exception != nullptr) {
		std::exception_ptr ex = pending_exception;
		pending_exception = nullptr;
		std::rethrow_exception(ex);
	}

	if (input != nullptr) {
		input->Check();
	}
}

std::unique_ptr<Tag> YtdlInputStream::ReadTag() noexcept {
	if (tag != nullptr) {
		return std::move(tag);
	} else if (input != nullptr) {
		return input->ReadTag();
	} else {
		return nullptr;
	}
}

size_t YtdlInputStream::Read(std::unique_lock<Mutex> &lock, void *ptr, size_t sz) {
	if (input != nullptr) {
		context = nullptr;
	}
	return ProxyInputStream::Read(lock, ptr, sz);
}

void YtdlInputStream::OnComplete([[maybe_unused]] Ytdl::YtdlMonitor* monitor) {
	const std::lock_guard<Mutex> protect(mutex);
	try {
		// Don't attempt to play playlist url
		if (context->GetMetadata().GetType() == "playlist") {
			throw std::runtime_error("attempting to play playlist url");
		}

		tag = context->GetMetadata().GetTagBuilder().CommitNew();
		SetInput(OpenCurlInputStream(context->GetMetadata().GetURL().c_str(),
			context->GetMetadata().GetHeaders(), mutex));
	} catch (...) {
		pending_exception = std::current_exception();
		SetReady(); // Notify the handler so it doesn't stuck waiting
	}
}

void YtdlInputStream::OnError([[maybe_unused]] Ytdl::YtdlMonitor* monitor, std::exception_ptr e) {
	const std::lock_guard<Mutex> protect(mutex);
	pending_exception = e;
	SetReady();
}
