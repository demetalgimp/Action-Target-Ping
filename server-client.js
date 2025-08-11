
server.on("connection",
	(socket) => {
		socket.on("data",
			(data) => {
				console.log(String.fromCharCode(data));
			}
		);
	}
);

#---Filter service
const net = require("net");

client = net.createConnection({ host: "127.0.0.1", port: 8080 });
client.on("data",
	(data) => {
		console.log(data);
	}
);
client.write("request");

#================================ Client ================================
const WebSocket = require("ws");

let socket = new WebSocket("wss://127.0.0.1/article/websocket/demo/hello");

socket.onopen = function(e) {
  alert("[open] Connection established");
  alert("Sending to server");
  socket.send("My name is John");
};

socket.onmessage = function(event) {
  alert(`[message] Data received from server: ${event.data}`);
};

socket.onclose = function(event) {
  if (event.wasClean) {
    alert(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    // e.g. server process killed or network down
    // event.code is usually 1006 in this case
    alert('[close] Connection died');
  }
};

socket.onerror = function(error) {
  alert(`[error]`);
};
