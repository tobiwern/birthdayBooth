const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Photo Viewer</title>
</head>
<body>
  <h1>ESP32-CAM Photo Viewer</h1>
  <button onclick="capturePhoto()">Capture Photo</button>
  <div id="photoContainer"></div>

  <script>
    var socket;

    // Function to connect to the WebSocket server
    function connectWebSocket() {
      socket = new WebSocket('ws://'+location.hostname+':81/');
      socket.binaryType = 'arraybuffer';

      // When the WebSocket connection is established
      socket.onopen = function(event) {
        console.log('Connected to WebSocket');
      };

      // When data is received from the WebSocket server
      socket.onmessage = function(event) {
        console.log("Message from WebSocket connection received.");                     
        const contentType = event.data.constructor.name;
        console.log('Received data type: ' + event.data.constructor.name);

        if (contentType === 'Blob' || contentType === 'ArrayBuffer') {
          console.log("Receiving image...");
          var imgData = new Uint8Array(event.data);
          var blob = new Blob([imgData], { type: 'image/jpeg' });
          var imgUrl = URL.createObjectURL(blob);
          var img = document.createElement('img');
          img.src = imgUrl;
          document.getElementById('photoContainer').replaceChildren(img);
        }
        else{
          console.log('WebSocket message: ' + event.data);
          console.log("Capture in progress...");          
        }
      };

      // When the WebSocket connection is closed
      socket.onclose = function(event) {
        console.log('WebSocket connection closed');
      };
    }

    // Function to capture a photo from the ESP32-CAM
    function capturePhoto() {
      console.log('Capturing photo...');
      if (socket.readyState === WebSocket.OPEN) {
        socket.send('C');
      }
      else{
        console.log('WebSocket not connected.')
      }
    }

    // Connect to the WebSocket server on page load
    window.addEventListener('load', connectWebSocket);
  </script>
</body>
</html>
)rawliteral";