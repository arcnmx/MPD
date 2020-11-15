#include "config.h"
#include "Parser.hxx"
#include "Invoke.hxx"
#include "system/Error.hxx"
#include "util/RuntimeError.hxx"
#include "util/ScopeExit.hxx"
#include "event/Loop.hxx"
#include "event/Call.hxx"
#include "Instance.hxx"
#include "Main.hxx"

#include <cinttypes>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>

namespace Ytdl {

std::unique_ptr<YtdlProcess>
YtdlProcess::Invoke(Yajl::Handle &handle, const char *url, PlaylistMode mode)
{
	FileDescriptor read, write;
	if (!FileDescriptor::CreatePipe(read, write)) {
		throw MakeErrno("Failed to create pipe");
	}

	AtScopeExit(&read, &write) {
		if (read.IsDefined()) {
			read.Close();
		}
		write.Close();
	};

	// block all signals while forking child
	int res;
	sigset_t signals_new, signals_old;
	sigfillset(&signals_new);
	if ((res = pthread_sigmask(SIG_SETMASK, &signals_new, &signals_old))) {
		throw MakeErrno(res, "Failed to block signals");
	}

	int pid = vfork();

	if (!pid) {
		// restore all signal handlers to default
		struct sigaction act;
		act.sa_handler = SIG_DFL;
		act.sa_flags = 0;
		sigemptyset(&act.sa_mask);
		for (int i = 0; i < NSIG; i++) {
			sigaction(i, &act, nullptr);
		}

		if (pthread_sigmask(SIG_SETMASK, &signals_old, nullptr)) {
			_exit(EXIT_FAILURE);
		}

		if (!write.Duplicate(FileDescriptor(STDOUT_FILENO))) {
			_exit(EXIT_FAILURE);
		}

		const char *playlist_flag;
		switch (mode) {
			case PlaylistMode::SINGLE:
				playlist_flag = "--no-playlist";
				break;
			case PlaylistMode::FLAT:
				playlist_flag = "--yes-playlist";
				break;
		}

		if (execlp("youtube-dl", "youtube-dl",
			"-Jf", "bestaudio/best", "--flat-playlist", playlist_flag, url, nullptr) < 0)
		{
			_exit(EXIT_FAILURE);
		}
	}

	// restore blocked signals
	if ((res = pthread_sigmask(SIG_SETMASK, &signals_old, nullptr))) {
		throw MakeErrno(res, "Failed to unblock signals");
	}

	if (pid < 0) {
		throw MakeErrno("Failed to fork()");
	}

	return std::make_unique<YtdlProcess>(handle, read.Steal(), pid);
}

YtdlProcess::~YtdlProcess()
{
	Close();

	if (pid != -1) {
		waitpid(pid, nullptr, 0);
	}
}

bool
YtdlProcess::Process()
{
	uint8_t buffer[0x80];
	int res;
	do {
		res = fd.Read(buffer, sizeof(buffer));
		if (res < 0) {
			if (errno == EWOULDBLOCK) {
				return true;
			} else if (errno != EINTR && errno != EAGAIN) {
				throw MakeErrno("failed to read from youtube-dl pipe");
			}
		} else if (res > 0) {
			handle.Parse(buffer, res);
		}
	} while (res != 0);

	return false;
}

void
YtdlProcess::Complete()
{
	if (!fd.Close()) {
		throw MakeErrno("failed to close youtube-dl pipe");
	}

	handle.CompleteParse();

	int res;
	if (waitpid(pid, &res, 0) < 0) {
		throw MakeErrno("failed to wait on youtube-dl process");
	}

	pid = -1;

	if (res) {
		throw FormatRuntimeError("youtube-dl exited with code %d", res);
	}
}

void
YtdlProcess::Close()
{
	if (fd.IsDefined()) {
		fd.Close();
	}
}

void
YtdlMonitor::OnSocketReady([[maybe_unused]] unsigned flags) noexcept
{
	try {
		if (!process->Process()) {
			ReleaseSocket();
			process->Complete();
			handler.OnComplete(this);
		}
	} catch (...) {
		if (IsDefined()) {
			ReleaseSocket();
		}
		process->Close();
		handler.OnError(this, std::current_exception());
	}
}

std::unique_ptr<YtdlMonitor>
Invoke(Yajl::Handle &handle, const char *url, PlaylistMode mode, EventLoop &loop, YtdlHandler &handler)
{
	auto process = YtdlProcess::Invoke(handle, url, mode);

	std::unique_ptr<YtdlMonitor> monitor = std::make_unique<YtdlMonitor>(handler, std::move(process), loop);

	return monitor;
}

void
BlockingInvoke(Yajl::Handle &handle, const char *url, PlaylistMode mode)
{
	std::unique_ptr<YtdlProcess> process;
	BlockingCall(global_instance->event_loop, [&] {
		process = YtdlProcess::Invoke(handle, url, mode);
	});

	while (process->Process()) {}
	process->Complete();
}

std::unique_ptr<InvokeContext>
InvokeContext::Invoke(const char* uri, PlaylistMode mode, EventLoop &event_loop, YtdlHandler &handler) {
	auto metadata = std::make_unique<TagHandler>();
	auto parser = std::make_unique<Parser>(*metadata);
	auto handle = parser->CreateHandle();
	std::unique_ptr<YtdlMonitor> monitor;
	BlockingCall(event_loop, [&] {
		BlockingCall(global_instance->event_loop, [&] {
			monitor = Ytdl::Invoke(*handle, uri, mode, event_loop, handler);
		});
		monitor->Schedule(SocketEvent::READ);
	});
	return std::make_unique<InvokeContext>(
		std::move(metadata),
		std::move(parser),
		std::move(handle),
		std::move(monitor)
	);
}

InvokeContext::~InvokeContext() {
	BlockingCall(monitor->GetEventLoop(), [&] {
		if (monitor->IsDefined()) {
			monitor->ReleaseSocket();
		}
	});
}

} // namespace Ytdl
