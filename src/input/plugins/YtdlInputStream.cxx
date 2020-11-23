#include "YtdlInputStream.hxx"
#include "CurlInputPlugin.hxx"
#include "tag/Tag.hxx"

YtdlInputStream::YtdlInputStream(const char *_uri, Mutex &_mutex, const Ytdl::YtdlInit &_init) noexcept
	:ProxyInputStream(_uri, _mutex), init(_init)
{
	InvokeYtdl();
}

YtdlInputStream::~YtdlInputStream() noexcept {
}

void YtdlInputStream::InvokeYtdl() {
	try {
		context = Ytdl::InvokeContext::Invoke(init, InputStream::GetURI(), Ytdl::PlaylistMode::SINGLE, *this);
	} catch (...) {
		pending_exception = std::current_exception();
	}
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

void YtdlInputStream::OnInputStreamReady() noexcept {
	// This can be triggered when CurlInputStream fails. When that happens, try
	// to invoke youtube-dl again and get a new url.
	if (input != nullptr) {
		try {
			input->Check();
			retry_counter = 0;
		} catch (...) {
			if (retry_counter < MAX_RETRY) {
				input = nullptr;
				InvokeYtdl();
				retry_counter++;
				return;
			} else {
				pending_exception = std::current_exception();
			}
		}
	}

	ProxyInputStream::OnInputStreamReady();
}
