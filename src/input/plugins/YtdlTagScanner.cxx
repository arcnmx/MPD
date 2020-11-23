#include "config.h"
#include "YtdlTagScanner.hxx"
#include "lib/ytdl/TagHandler.hxx"
#include "lib/ytdl/Parser.hxx"
#include "tag/Tag.hxx"


YtdlTagScanner::YtdlTagScanner(const Ytdl::YtdlInit &_init, const std::string &_uri, RemoteTagHandler &_handler)
	:init(_init), uri(_uri), handler(_handler) { }

YtdlTagScanner::~YtdlTagScanner() { }

void
YtdlTagScanner::Start()
{
	assert(context == nullptr);
	try {
		context = Ytdl::InvokeContext::Invoke(init, uri.c_str(), Ytdl::PlaylistMode::SINGLE, *this);
	} catch (...) {
		handler.OnRemoteTagError(std::current_exception());
	}
}

void YtdlTagScanner::OnComplete([[maybe_unused]] Ytdl::YtdlMonitor* monitor) {
	handler.OnRemoteTag(context->GetMetadata().GetTagBuilder().Commit());
}

void YtdlTagScanner::OnError([[maybe_unused]] Ytdl::YtdlMonitor* monitor, std::exception_ptr e) {
	handler.OnRemoteTagError(e);
}
