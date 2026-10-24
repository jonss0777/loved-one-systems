#include <drogon/drogon.h>
#include <drogon/WebSocketController.h>
#include <drogon/orm/DbClient.h>
#include <gst/gst.h>
#include <iostream>
#include <json/json.h>
#include <unordered_set>
#include <mutex>

// Store active WebSocket connections globally
std::unordered_set<drogon::WebSocketConnectionPtr> g_clients;
std::mutex g_clientsMutex;

class FirmwareWebSocketServer : public drogon::WebSocketController<FirmwareWebSocketServer> {
public:
    // Called when the ESP32 establishes a new connection
    void handleNewConnection(const drogon::HttpRequestPtr& req, 
                             const drogon::WebSocketConnectionPtr& wsConnPtr) override {
        std::cout << "[WS Server] ESP32 client connected from: " 
                  << wsConnPtr->peerAddr().toIp() << std::endl;
        
        // Track the connection
        {
            std::lock_guard<std::mutex> lock(g_clientsMutex);
            g_clients.insert(wsConnPtr);
        }
        
        // Send initial config command upon connection 
        Json::Value command;
        command["set_sensor"] = 1; 
        Json::FastWriter writer;
        wsConnPtr->send(writer.write(command));
        std::cout << "[WS Server] Sent initial config command: set_sensor = 1" << std::endl;
    }

    // Called when the ESP32 disconnects
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& wsConnPtr) override {
        std::cout << "[WS Server] ESP32 client disconnected." << std::endl;
    
        // Remove the connection from tracking
        {
            std::lock_guard<std::mutex> lock(g_clientsMutex);
            g_clients.erase(wsConnPtr);
        }
    }

    // Called when the ESP32 sends text payloads (JSON containing heartbeats or sensor values)
    void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr, 
                          std::string&& message, 
                          const drogon::WebSocketMessageType& type) override {
        
        if (type == drogon::WebSocketMessageType::Text) {
            std::cout << "[WS Server] Received raw text payload: " << message << std::endl;

            // Check if the message looks like JSON (starts with '{')
            if (!message.empty() && message.front() == '{') {
                Json::Value root;
                Json::CharReaderBuilder builder;
                std::string errs;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

                if (reader->parse(message.c_str(), message.c_str() + message.size(), &root, &errs)) {
                    
                    // 1. Handle Heartbeat Message from ESP32
                    if (root.isMember("type") && root["type"].asString() == "heartbeat") {
                        std::string clientIp = wsConnPtr->peerAddr().toIp();
                        std::cout << "[WS Server] Heartbeat received from IP: " << clientIp << std::endl;

                        // Log heartbeat to PostgreSQL asynchronously
                        auto dbClient = drogon::app().getDbClient();
                        if (dbClient) {
                            dbClient->execSqlAsync(
                                "INSERT INTO device_logs (ip_address, event_type, created_at) VALUES ($1, $2, NOW())",
                                [](const drogon::orm::Result &result) {
                                    // Successfully logged
                                },
                                [](const drogon::orm::DrogonDbException &e) {
                                    std::cerr << "[DB Error] Failed to log heartbeat: " << e.base().what() << std::endl;
                                },
                                clientIp,
                                "heartbeat"
                            );
                        } else {
                            std::cerr << "[DB Error] Database client not configured or unavailable." << std::endl;
                        }
                    }
                    // 2. Handle Sensor Value payloads
                    else if (root.isMember("sensor_type") && root.isMember("value")) {
                        int sensorType = root["sensor_type"].asInt();
                        int value = root["value"].asInt();

                        std::cout << ">>> Extracted Event -> Sensor Type: " << sensorType 
                                  << " | Value: " << value << std::endl;

                        switch (sensorType) {
                            case 1:
                                std::cout << "Handling Temperature Sensor value: " << value << std::endl;
                                break;
                            case 2:
                                std::cout << "Handling CO2 Sensor value: " << value << std::endl;
                                break;
                            case 3:
                                std::cout << "Handling Ultrasonic Sensor value: " << value << std::endl;
                                break;
                            default:
                                std::cout << "Unknown sensor type received." << std::endl;
                                break;
                        }
                    } else {
                        std::cout << "Received unknown JSON format from client." << std::endl;
                    }
                } else {
                    std::cerr << "Failed to parse incoming JSON payload: " << errs << std::endl;
                }
            } else {
                // Handle plain text messages
                std::cout << "[WS Server] Handling plain text message: " << message << std::endl;
            }
        }
    }

    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/");
    WS_PATH_LIST_END
};

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    // HTTP Endpoint: /hello
    drogon::app().registerHandler(
        "/hello",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("Hello World");
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
        },
        {drogon::Get}
    );

    // HTTP Endpoint: /set_mode?type=X (Broadcasts command to all connected ESP32s)
    drogon::app().registerHandler(
        "/set_mode",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            
            std::string typeStr = req->getParameter("type");
            if (typeStr.empty()) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setBody("Missing 'type' query parameter. Use /set_mode?type=2");
                resp->setStatusCode(drogon::k400BadRequest);
                callback(resp);
                return;
            }

            int type = std::stoi(typeStr);
            std::cout << "[HTTP] /set_mode triggered with type: " << type << std::endl;

            // Build JSON command package
            Json::Value command;
            command["set_sensor"] = type;
            Json::FastWriter writer;
            std::string payload = writer.write(command);

            int sentCount = 0;
            {
                std::lock_guard<std::mutex> lock(g_clientsMutex);
                std::cout << "[HTTP] Active WebSocket clients connected: " << g_clients.size() << std::endl;
                for (const auto& client : g_clients) {
                    if (client && client->connected()) {
                        client->send(payload);
                        sentCount++;
                    }
                }
            }

            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody("Command broadcasted to " + std::to_string(sentCount) + " client(s). Type: " + std::to_string(type));
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
        },
        {drogon::Get}
    );

    // Optional: Load configuration file (which includes PostgreSQL connection setup)
    drogon::app().loadConfigFile("../config.json");

    // drogon::app().addListener("0.0.0.0", 8080);
    // drogon::app().addListener("0.0.0.0", 81);
    
    std::cout << "Drogon server running..." << std::endl;
    drogon::app().run();
    return 0;
}