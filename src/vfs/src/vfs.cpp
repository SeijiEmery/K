#include "vfs.hpp"

class StdFunctionFSRequest : public FSRequest {
    std::function<void(const FileData&)> onLoadCallback;
    std::function<void(const FileException&)> onErrorCallback;

    StdFunctionFSRequest (
        const std::string& path, 
        const decltype(onLoadCallback) onLoadCallback,
        const decltype(onErrorCallback) onErrorCallback,
    ) : FSRequest(path), onLoadCallback(onLoadCallback), onErrorCallback(onErrorCallback) {}
};

class FSInstance::Impl {
    std::vector<Path>                               rootPaths;
    std::unordered_map<std::string,std::string>     pathVars;
    std::vector<std::weak_ptr<FSRequest>>           listeners;

    moodycamel::ConcurrentQueue<FSOp>               opQueue;
    FSRunner*                                       runner = nullptr;    
    std::mutex                                      mutex;
};

void FSRequest::reload (std::shared_ptr<FSRequest>& req) {
    req.status = FSRequest::Status::NOT_YET_LOADED;
    // send reload msg or reload immed here...
}
void FSRequest::wait (std::shared_ptr<FSRequest>& req) {
    if (req.status != FSRequest::Status::NOT_YET_LOADED)
        return;
    // force load and/or wait...
}

std::shared_ptr<FSRequest> loadAsync (
    const std::string& path,
    std::function<void(const FileData&)>      onLoad,
    std::function<void(const FileException&)> onError
) {
    auto req = std::ptr_cast<FSRequest>(std::make_shared<StdFunctionFSRequest>(path, onLoad, onError));
    std::weak_ptr<FSRequest> wreq (req);
    impl->opQueue.emplace(AddFileListener(wreq)); 
    // if (std::lock lock (impl->mutex); true) {
    //     impl->listeners.emplace_back(req);
    // }
    return std::move(req);
}

std::shared_ptr<FSRequest> loadAsync (
    std::shared_ptr<FSRequest> req
) {
    std::weak_ptr<FSRequest> wreq (req);
    impl->opQueue.emplace(AddFileListener(wreq)); 
    // if (std::lock lock (impl->mutex), true) {
    //     impl->listeners.emplace_back(req);
    // }
    return std::move(req);
}


















