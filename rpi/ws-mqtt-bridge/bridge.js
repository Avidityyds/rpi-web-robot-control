// WebSocket + MQTT Bridge
const WebSocket = require('ws');
const mqtt = require('mqtt');

const WS_PORT = 8081;                     // 瀏覽器連線的 WebSocket Port
const MQTT_URL = 'mqtt://localhost:1883'; // 本地 Mosquitto Broker

// 建立 MQTT Client
const mqttClient = mqtt.connect(MQTT_URL);
mqttClient.on('connect', () => {
  console.log('Connected to local MQTT broker');
});

// 建立 WebSocket Server
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`WebSocket server started on ws://0.0.0.0:${WS_PORT}`);

// WebSocket 連線處理
wss.on('connection', ws => {
  console.log('WebSocket client connected');

  // 接收前端訊息 → 發佈到 MQTT
  ws.on('message', msg => {
    try {
      const data = JSON.parse(msg);
      if (data.type === 'pub') {
        mqttClient.publish(data.topic, data.payload);
        console.log(`WS→MQTT ${data.topic}: ${data.payload}`);
      } else if (data.type === 'sub') {
        mqttClient.subscribe(data.topic);
        console.log(`MQTT subscribed: ${data.topic}`);
      }
    } catch (err) {
      console.error('WS message parse error', err);
    }
  });

  // MQTT 訊息 → 傳回所有 WebSocket Client
  mqttClient.on('message', (topic, payload) => {
    ws.send(JSON.stringify({
      type: 'msg',
      topic,
      payload: payload.toString()
    }));
  });
});