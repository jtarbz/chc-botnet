const net = require('net');

var server = net.createServer( (socket) => {
    socket.setEncoding('utf8');
    console.log(`Client at ${socket.remoteAddress}`);
    socket.write('AAAHHH');

    socket.on('data', (res) => {
        console.log(res);
    })

    socket.on('end', () => {
        console.log('aaah');
    });
});

server.listen(8081, () => {
    console.log('bound');
})