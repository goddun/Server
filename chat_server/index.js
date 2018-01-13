var app = require("express")();
var http = require("http").Server(app);
var io = require("socket.io")(http)

http.listen(port = 7711, function() {
    console.log("Server Listening at port %d", port);
});

app.get("/", function(req, res) {
    res.send("<h1>Chat Server</h1>")
});

var numUsers = 0;

io.on("connection", function(socket)
{
    var addedUser = false;

    // client emits "new message"
    socket.on("new message", function(data)
    {
        socket.broadcast.emit("new message", {
            username: socket.username,
            message: data
        });
    });

    // client emits "add user"
    socket.on("add user", function(username)
    {
        if(addedUser)
            return;

        socket.username = username;
        ++numUsers;
        addedUser = true;

        socket.emit("login", {
            numUsers: numUsers
        });

        socket.broadcast.emit("user joined", {
            username: socket.username,
            numUsers: numUsers
        });

    });

    // client emits "typing"
    socket.on("typing", function()
    {
        socket.broadcast.emit("typing", {
            username: socket.username
        });
    });

    // client emits "stop typing"
    socket.on("stop typing", function()
    {
        socket.broadcast.emit("stop typing", {
            username: socket.username
        });
    });

    // when user disconnects...
    socket.on("disconnect", function()
    {
        if(addedUser)
        {
            --numUsers;

            socket.broadcast.emit("user left", {
                username: socket.username,
                numUsers: numUsers
            });
        }
    });

});

