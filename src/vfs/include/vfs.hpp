
#pragma once

//
// Data structures
//

// POD FileData container (raw bytes)
struct FileData {
    std::string          path;
    std::vector<uint8_t> data;

    FileData () {}
    FileData (const decltype(path) path, decltype(data)& data)
        : path(path), data(std::move(data)) {}
    FileData (const FileData&) = delete;
    ~FileData () {}
};

// Base File Exception
struct FileException : public std::runtime_error {
    std::string path;
    Reason reason = Reason::NONE;

    enum class Reason {
        NONE = 0,
        INTERNAL_ERROR,
        FILE_DOES_NOT_EXIST,
        AMBIGUOUS_FILE_NAMES,
    };

    FileException (const std::string path, Reason reason, const char* what) 
        : path(path), reason(reason), std::runtime_error(what) {}
};

class FSInstance;
struct FSRequest {
    const std::string      path;        // path to asset (readonly)
    std::atomic<LoadFlags> loadFlags;   // current flags, set by user (write)
    std::atomic<Status>    status;      // current status, set by file loader (read)

    enum class LoadFlags {
        DISABLE_LOADING = 1 >> 0,   // If set, disables low callback (does NOT queue missed events).
        ALLOW_RELOAD = 1 >> 1,      // If set, disallows
        ALL_MATCHING = 1 >> 2,      // If set, can interpret "*.obj" as 'call callback on all .obj files'
    };                              // (otherwise results in an error if path may refer to multiple files)

    enum class Status {
        NOT_YET_LOADED = 0,         // file has not yet been loaded
        LOADED_ONCE,                // file has been loaded once successfully
        RELOADED,                   // last reload successful; loaded > 1 times
        ERROR_FIRST_LOAD,           // initial file load failed; has NOT loaded successfully
        ERROR_RELOAD                // last reload failed; loaded 1+ times successfully
    };

    // User callback for onLoad / onError (this class may be overridden + passed to FSInstance)
    virtual void onLoad  (const FileData&) = 0;
    virtual void onError (const FileException&) = 0;

    // No default ctor (b/c const path)
    FSRequest (const std::string& path, LoadFlags loadFlags = LoadFlags::ALLOW_RELOAD) :
        path(path), loadFlags(loadFlags), status(Status::NOT_YET_LOADED) {}
    
    // Disabled copy (store in std::shared_ptr instead).
    FSRequest (const FSRequest&) = delete;
    FSRequest& operator= (const FSRequest&) = delete;
    
    virtual ~FSRequest () {}
};

// FS Instance; threadsafe methods.
class FSInstance {

    // Add / remove root asset paths.
    void addRootPath    (const std::string& path);
    void removeRootPath (const std::string& path);

    // Get / set path variable. Null variable == "".
    void        setVar (const std::string& var, const std::string& value);
    std::string getVar (const std::string& var);

    std::string resolvePath (const std::string& path);
    std::vector<std::string> resolveAllPaths (const std::string& path);


    std::shared_ptr<FSRequest> loadAsync (
        const std::string& path,
        std::function<void(const FileData&)>      onLoad,
        std::function<void(const FileException&)> onError
    );
    std::shared_ptr<FSRequest>& loadAsync (
        std::shared_ptr<FSRequest> request
    );

    // Throws FileException if file does not exist
    std::unique_ptr<FileData> loadImmediate (
        const std::string& path
    );

    // Force reload of a previous request (note: run iff ALLOW_RELOAD set).
    void reload (std::shared_ptr<FSRequest>& req);

    // Wait for completion of initial OR last reload() call (block until then).
    void wait   (std::shared_ptr<FSRequest>& req);


    FSInstance  (const std::string& configPath);
    ~FSInstance ();

    FSInstance (const FSInstance&) = delete;
    FSInstance& operator= (const FSInstance&) = delete;
private:
    class Impl;
    Impl* impl;
};

// Runs a FSInstance.
class FSRunner {
    std::shared_ptr<FSInstance> instance;

    FSRunner (std::shared_ptr<FSInstance> instance)
        : instance(std::move(instance)) {}

    void run ();
    bool isRunning ();
    void kill ();
};








