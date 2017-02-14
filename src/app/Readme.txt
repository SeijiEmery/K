
The k::app library will provide core application infrastructure built on top of GLFW and a threading
library, and will provide a high-level interface used by application Clients.

– Application behavior gets implemented by Clients.
– The execution of which is managed by the k::app implementation.

– Clients will be multithreaded (ie. each can run on its individual thread), and will be given the
    illusion of direct-state access by the public interfaces of 'app/include/*.hxx'
– Specifically, we use per-client objects with get/set/onChange properties as facades.
– These facades (implemented in 'app/src/*.cxx') internally send all state changes to command
    buffers, which get processed by the main application.
– Note that the threading and event systems are thus tightly intertwined, and the true application
    implementation (in app/src) is completely hidden from the user.

See app/include/app_window_manager.hxx for a sample interface, or app/include/app.hxx for an
    example of how AppClients are created / will work.

Structuring the application like this will have multiple benefits:
    – Hiding threading + GLFW implementation details will simplify user code significantly, 
        and reduce compile times.
    – Having a high-level threading sytem, and thread-aware event system (for the application layer)
        will be extremely useful.
    – We can host multiple programs (Clients) inside of one application shell.
    – This leaves the door open to implement hotloadable clients down the road.

Note: this is an improved and iterated version of the architecture I was originally trying 
to implement in GLSandbox / GSB / SB. I now see the importance of *completely* wrapping the 
client layer and NOT allowing Clients direct access to any shared application state. This does,
of course, necessitate wrapping GLFW (and everything else) with a layer of facades and commands
for state access and mutation.

In case this isn't obvious, K AppClient = GLSandbox module = GSB UIComponent (bad name), and
I'll be pulling major inspiration from GSB's 'engine' implementation to implement k::app's 
threading model.

I'll probably change terminology as / if this gets further developed.
